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
#include <pthread.h>

#include "camera.h"
#include "logger.h"
#include "config_parser.hpp"
#include "filterwheel.h"
#include "filter_wheel_manager.h"
#include "systemtime.h"

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

struct ThreadParams
{
  FilterWheelManager* fwman;
  int wavelength;
};

void* workerThread (void* p)
{
  ThreadParams* param = (ThreadParams*) p;
  FilterWheelManager* fw = param->fwman;
  fw->gotoFilter (param->wavelength);
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
  utils::getSystemTime (time_str);
  ostringstream logfilename;
  logfilename << dir << "/log-auto-exposure-" << time_str << ".txt";
  logger.open (logfilename.str ());

  logger << "Log file is " << logfilename.str () << endl;

  logger << "Over exposed percentage = " << over_exposed_ratio_par << endl;


  ConfigParser* parser = NULL;
  FilterWheel* fw1[2] = {NULL, NULL};
  FilterWheel* fw2[2] = {NULL, NULL};
  CameraWrapper* camera = NULL;
  try
  {
    int status = 0;
    fitsfile* fptr = NULL;	

    /**** XML PARSER setup ****/

    string inputfile = argv[1];

    parser = new ConfigParserDOM ();
    logger << "Input file is " << inputfile << "\n";
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      logger << "Error Message : " << parser->getErrorMessage () << "\n";
      return -1;
    }

    //return 0;

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

    fw1[0] = f1;
    fw1[1] = f2;  


    FilterWheel* f3 = new FilterWheel ();

    logger << "Setting up filter wheel 3..\n";

    f3->setId (3);
    f3->setFilterWheelMap (parser->getFilterWheelById (2));
#ifndef WIN32
    f3->setPort (parser->get_fw_port (2));
#else
    string str = parser->get_fw_port (2);
    wstring wstr;
    wstr.assign (str.begin (), str.end ());
    f3->setPort ((wchar_t*) wstr.c_str ());
#endif

    f3->setBaudRate (parser->get_fw_baud (2));
    f3->openPort ();


    logger << "Setting up filter wheel 4..\n";

    FilterWheel* f4 = new FilterWheel ();  
    f4->setId (4);
    f4->setFilterWheelMap (parser->getFilterWheelById (3));
#ifndef WIN32
    f4->setPort (parser->get_fw_port (3));
#else
    str = parser->get_fw_port (3);  
    wstr.assign (str.begin (), str.end ());
    f4->setPort ((wchar_t*) wstr.c_str ());
#endif
    //
    f4->setBaudRate (parser->get_fw_baud (0));
    f4->openPort ();
    //  return 0;  


    fw2[0] = f3;
    fw2[1] = f4;  


    FilterWheelManager fw_man1 (fw1);
    FilterWheelManager fw_man2 (fw2);


    //camera = new CameraWrapper ();
    //camera->setOverExposedRatio (over_exposed_ratio_par);


    /**** IMAGE CAPTURE LOOP ****/

    string wavelengths_str = "999 450 532 671 750 850 920 1050";
    istringstream istr (wavelengths_str);
    vector <int> wavelengths;
    copy (istream_iterator <int> (istr), istream_iterator <int> (), back_inserter (wavelengths));

    for (int i = 0; i < wavelengths.size (); i++) {
      int wavelength = wavelengths[i];
      logger << "<<Starting>> wavelength " << wavelength << endl;
      logger << "<<wavelength>> = " << wavelength << endl;

      pthread_t p1, p2;

      ThreadParams tp1, tp2;
      tp1.fwman = &fw_man1;
      tp2.fwman = &fw_man2;

      tp1.wavelength  = tp2.wavelength = wavelength;

      int ret = pthread_create (&p1, NULL, workerThread, &tp1);
      ret = pthread_create (&p2, NULL, workerThread, &tp2);

      pthread_join (p1, NULL);
      pthread_join (p2, NULL);

      // Giving some time for the filter wheel to initialize. 
      sleep (8);

     utils::getSystemTime (time_str);

      // At this point, auto exposure is enabled using the camera APIs.
      // auto exposure is disabled in the below function call, to fine tune exposure using our own algorithm
      //camera->findBestExposure (wavelength, time_str);


      // At this point, auto exposure remains disabled.
      // Image is captured using the exposure obtained from our own algorithm.
      //camera->captureImage ();


      // Enabling auto exposure again for the next filter, after obtaining exposure using our own algorithm
      double val = 1.0;
      //camera->setParameter (val, IS_SET_ENABLE_AUTO_SHUTTER, "AUTO SHUTTER");

      val = 1.;
      //camera->setParameter (val, IS_SET_ENABLE_AUTO_FRAMERATE, "AUTO FRAME RATE");

      ostringstream image_name;
      image_name << dir << "/test-auto-exposure-" << wavelength << "-" << time_str << ".png";
      logger << "<<wavelength>> = "  << wavelength<< "\tImage name is " << image_name.str () << endl;
      wstring image_name_w = L"";
      string tmp = image_name.str ();
      image_name_w.assign (tmp.begin (), tmp.end ());
      wstring image_type = L"png";

      //camera->saveImage (image_name_w, image_type);

      //double exposure = 0.0;
      //camera->getCurrentExposure (exposure);

      //int width = camera->getWidth ();
      //int height = camera->getHeight ();

      //fitsfile *fptr = NULL;
      //long fpixel = 1;		
      //long naxis = 2;
      //long naxes[2] = {width, height};
      //const int size = width * height;
      //int fitsstatus = 0;		

      //image_name.str ("");
      //image_name << dir << "/test-auto-exposure-" << wavelength << "-" << time_str << ".fit";
      //string fits_filename = image_name.str ();
      //logger << "<<wavelength>> = "  << wavelength<< "\tCreating file " << fits_filename << endl;

      //if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
      //  logger << "<<wavelength>> = "  << wavelength<< "\tUnable to create file ..\n";		
      //  print_fits_error (fitsstatus);
      //}

      //logger << "Creating image ..\n";

      //if (fits_create_img (fptr, USHORT_IMG, naxis, naxes, &fitsstatus)) {
      //  print_fits_error (fitsstatus);
      //}

      //logger << "Writing image ..\n";
      //unsigned short* pixels = new unsigned short [width*height];
      //unsigned int im_size = width*height*2;
      //char* image = camera->getImageMemory ();

      //memcpy (pixels, image, height*width*2);

      //if (fits_write_img (fptr, TUSHORT, fpixel, im_size, pixels, &fitsstatus)) {
      //  print_fits_error(fitsstatus);
      //}

      //float fexposure = exposure;
      //if(fits_update_key(fptr, TFLOAT, "EXPOSURE", &fexposure, (char*)"Exposure in milli seconds", &fitsstatus)) {
      //  print_fits_error(fitsstatus);
      //}

      //if(fits_close_file(fptr, &fitsstatus)) {
      //  print_fits_error(fitsstatus);	
      //}

      //logger << "<<Completed>> wavelength " << wavelength << endl;
    }
  }
  catch (std::exception& e) {
    cout << "Exception : " << e.what () << endl;

    if (parser) {
      delete parser;
    }

    if (fw1[0]) {
      delete fw1[0];
    }
    if (fw1[1]) {
      delete fw1[1];
    }
    if (fw2[0]) {
      delete fw2[0];
    }
    if (fw2[1]) {
      delete fw2[1];
    }
    if (camera) {
      delete camera;
    }
    return -1;
  }

  if (camera) {
    if (parser) {
      delete parser;
    }
    if (fw1[0]) {
      delete fw1[0];
    }
    if (fw1[1]) {
      delete fw1[1];
    }
    if (fw2[0]) {
      delete fw2[0];
    }
    if (fw2[1]) {
      delete fw2[1];
    }
    delete camera;
  }

  return 0;
}
