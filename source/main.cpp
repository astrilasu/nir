#include <iostream>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <fstream>
#include <iterator>
#include <exception>
using namespace std;

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "camera.h"
#include "logger.h"
#include "config_parser.hpp"
#include "filterwheel.h"
#include "filterwheel_manager.h"

#include "fitsio.h"

Logger logger;


void print_fits_error (int status)
{
	if (status) {
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}



int main (int argc, char *argv[])
{
  if (argc < 3) {
    cout << "Enter arguments ..\n"
      "\t1) Config file \n"
      "\t2) directory to store output files\n"
      "\t3) over exposed pixels percentage\n";
    return -1;
  }

  float over_exposed_ratio_par = atof (argv[3]) / 100.;

  string dir = argv[2];
  if (opendir (argv[2])) {
    cout << "Directory already exists .. Please enter a new directory name ..\n";
    return -1;
  }
  mkdir (argv[2], 0777);

  string time_str = "";
  get_system_time (time_str);
  ostringstream logfilename;
  logfilename << dir << "/log-auto-exposure-" << time_str << ".txt";
  logger.open (logfilename.str ());

  logger << "Log file is " << logfilename.str () << endl;

  logger << "Over exposed percentage = " << over_exposed_ratio_par << endl;


  CameraWrapper* camera = NULL;
  try
  {
    int status = 0;
    fitsfile* fptr = NULL;	

    /**** XML PARSER setup ****/

    ConfigParser* parser = NULL;
    string inputfile = argv[1];

    parser = new ConfigParserDOM ();
    logger << "Input file is " << inputfile << "\n";
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      logger << "Error Message : " << parser->getErrorMessage () << "\n";
      return -1;
    }


    /**** FilterWheel setup ****/

    FilterWheel* f1 = new FilterWheel ();

    logger << "Setting up filter wheel 1..\n";

    f1->setId (1);
    f1->setFilterWheelMap (parser->getFilterWheelById (0));
#ifndef WIN32
    f1->setPort (parser->get_fw_port (0));
#else
    string str = parser->get_fw_port (0);
    wstring wstr;
    wstr.assign (str.begin (), str.end ());
    f1->setPort ((wchar_t*) wstr.c_str ());
#endif

    f1->setBaudRate (parser->get_fw_baud (0));
    f1->openPort ();


    logger << "Setting up filter wheel 2..\n";

    FilterWheel* f2 = new FilterWheel ();  
    f2->setId (2);
    f2->setFilterWheelMap (parser->getFilterWheelById (1));
#ifndef WIN32
    f2->setPort (parser->get_fw_port (1));
#else
    str = parser->get_fw_port (1);  
    wstr.assign (str.begin (), str.end ());
    f2->setPort ((wchar_t*) wstr.c_str ());
#endif
    //
    f2->setBaudRate (parser->get_fw_baud (0));
    f2->openPort ();
    //  return 0;  

    FilterWheel* fw[2];
    fw[0] = f1;
    fw[1] = f2;  

    FilterWheelManager fw_man (fw);


    camera = new CameraWrapper ();
    camera->setOverExposedRatio (over_exposed_ratio_par);
    //sleep (3);
    //camera->captureImage ();
    //wstring name = L"test_oo.png";
    //wstring type = L"png";
    //camera->saveImage (name, type);



    /**** IMAGE CAPTURE LOOP ****/

    string wavelengths_str = "999 450 532 671 750 850 920 1050";
    istringstream istr (wavelengths_str);
    vector <int> wavelengths;
    copy (istream_iterator <int> (istr), istream_iterator <int> (), back_inserter (wavelengths));

    for (int i = 0; i < wavelengths.size (); i++) {
      int wavelength = wavelengths[i];
      logger << "<<Starting>> wavelength " << wavelength << endl;
      logger << "<<wavelength>> = " << wavelength << endl;

      fw_man.gotoFilter (wavelength);

      // Giving some time for the filter wheel to initialize. 
      sleep (8);

      get_system_time (time_str);

      // At this point, auto exposure is enabled using the camera APIs.
      // auto exposure is disabled in the below function call, to fine tune exposure using our own algorithm
      camera->findBestExposure (wavelength, time_str);


      // At this point, auto exposure remains disabled.
      // Image is captured using the exposure obtained from our own algorithm.
      camera->captureImage ();


      // Enabling auto exposure again for the next filter, after obtaining exposure using our own algorithm
      double val = 1.0;
      camera->setParameter (val, IS_SET_ENABLE_AUTO_SHUTTER, "AUTO SHUTTER");

      val = 1.;
      camera->setParameter (val, IS_SET_ENABLE_AUTO_FRAMERATE, "AUTO FRAME RATE");

      ostringstream image_name;
      image_name << dir << "/test-auto-exposure-" << wavelength << "-" << time_str << ".png";
      logger << "<<wavelength>> = "  << wavelength<< "\tImage name is " << image_name.str () << endl;
      wstring image_name_w = L"";
      string tmp = image_name.str ();
      image_name_w.assign (tmp.begin (), tmp.end ());
      wstring image_type = L"png";

      camera->saveImage (image_name_w, image_type);

      double exposure = 0.0;
      camera->getCurrentExposure (exposure);

      int width = camera->getWidth ();
      int height = camera->getHeight ();

      fitsfile *fptr;
      long fpixel = 1;		
      long naxis = 2;
      long naxes[2] = {width, height};
      const int size = width * height;
      int fitsstatus = 0;		

      image_name.str ("");
      image_name << dir << "/test-auto-exposure-" << wavelength << "-" << time_str << ".fit";
      string fits_filename = image_name.str ();
      logger << "<<wavelength>> = "  << wavelength<< "\tCreating file " << fits_filename << endl;

      if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
        logger << "<<wavelength>> = "  << wavelength<< "\tUnable to create file ..\n";		
        print_fits_error (fitsstatus);
      }

      logger << "Creating image ..\n";

      if (fits_create_img (fptr, USHORT_IMG, naxis, naxes, &fitsstatus)) {
        print_fits_error (fitsstatus);
      }

      logger << "Writing image ..\n";
      unsigned short* pixels = new unsigned short [width*height];
      unsigned int im_size = width*height*2;
      char* image = camera->getImageMemory ();

      memcpy (pixels, image, height*width*2);

      if (fits_write_img (fptr, TUSHORT, fpixel, im_size, pixels, &fitsstatus)) {
        print_fits_error(fitsstatus);
      }

      float fexposure = exposure;
      if(fits_update_key(fptr, TFLOAT, "EXPOSURE", &fexposure, "Exposure in milli seconds", &fitsstatus)) {
        print_fits_error(fitsstatus);
      }

      if(fits_close_file(fptr, &fitsstatus)) {
        print_fits_error(fitsstatus);	
      }

      logger << "<<Completed>> wavelength " << wavelength << endl;
    }
  }
  catch (std::exception& e) {
    cout << "Exception : " << e.what () << endl;
    if (camera) {
      delete camera;
    }
    return -1;
  }

  if (camera) {
    delete camera;
  }

  return 0;
}
