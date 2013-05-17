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

extern float over_exposed_ratio_par;
extern Logger logger;

void print_fits_error(int status)
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
    logger << "Enter arguments ..\n"
            "\t1) Config file \n"
            "\t2) directory to store output files\n"
            "\t3) over exposed pixels percentage\n";
		return -1;
	}

  over_exposed_ratio_par = atof (argv[3]) / 100.;

  string dir = argv[2];
  if (opendir (argv[2])) {
    logger << "Directory already exists .. Please enter a new directory name ..\n";
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

	int status = 0;
	fitsfile* fptr = NULL;	

	/**** XML PARSER setup ****/

  ConfigParser* parser = NULL;
  string inputfile = argv[1];

  try {
    parser = new ConfigParserDOM ();
    logger << "Input file is " << inputfile << "\n";
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      logger << "Error Message : " << parser->getErrorMessage () << "\n";
      return -1;
    }
  }
  catch (MyException& e) {
    logger << e.m_error_message << "\n";
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


  /**** Camera setup ****/

  get_camera_list ();

  HIDS h_cam = 0;
  try {

    h_cam = setup_camera (&h_cam);
    if (h_cam == -1) {
      return -1;
    }

    if (set_display_mode (h_cam, IS_SET_DM_MONO) == -1) {
      return -1;
    }

    UINT color_mode = IS_CM_MONO12;    
    if (set_color_mode (h_cam, color_mode) == -1) {
      logger << "Unable to se color mode ..\n";
      return -1;
    }

    double fps = 0.;
    if (get_fps (h_cam, &fps) != -1) {
      logger << "New Frame rate = " << fps << "\n";
    }

    int width = 0; 
    int height = 0;
    get_AOI (h_cam, width, height);

    logger << "width = " << width << " height = " << height << "\n";

    int bits_pp = 12; // bits per pixel

    logger << "Bits per pixel = " << bits_pp << "\n";

    char* image = NULL;
    int mem_id = 0;

    if (setup_image_memory (h_cam, width, height, bits_pp, &image, &mem_id) == -1) {
      return -1;
    }

    if (is_CaptureVideo (h_cam, IS_WAIT) != IS_SUCCESS) {
      logger << "Failed to start live mode ..\n";
      throw std::exception ();
    }


    // SETTING AUTO EXPOSURE
    double val = 1.;
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL) != IS_SUCCESS) {
      logger << "Unable to set auto shutter ..\n";
      throw std::exception ();
    }

    // SETTING AUTO FRAME RATE
    val = 1.;
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_FRAMERATE, &val, NULL) != IS_SUCCESS) {
      logger << "Unable to set auto shutter ..\n";
      throw std::exception ();
    }

    val = 0.;
    // DISABLING AUTO GAIN
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_GAIN, &val, NULL) != IS_SUCCESS) {
      logger << "Unable to set auto white balance ..\n";
      throw std::exception ();
    }


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
      find_best_exposure (h_cam, image, width, height, wavelength, time_str);


      // At this point, auto exposure remains diabled.
      // Image is captured using the exposure obtained from our own algorithm.
      if (capture_image (h_cam) == -1) {
        logger << "<<wavelength>> = "  << wavelength << "\tCapture Image failed ..\n";
        throw std::exception ();
      }
      else {
        logger << "<<wavelength>> = "  << wavelength << "\tImage Captured ..\n";
      }


      // Enabling auto exposure again for the next filter, after obtaining exposure using our own algorithm
      val = 1.0;
      if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL) != IS_SUCCESS) {
        logger << "<<wavelength>> = " << wavelength << "\tUnable to set auto shutter ..\n";
        throw std::exception ();
      }
      else {
        logger << "<<wavelength>> = " << wavelength << "\tEnabling auto shutter to initialize auto exposure for the next filter..\n";
      }

      val = 1.;
      if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_FRAMERATE, &val, NULL) != IS_SUCCESS) {
        logger << "Unable to set auto frame rate ..\n";
        throw std::exception ();
      }

      ostringstream image_name;
      image_name << dir << "/test-auto-exposure-" << wavelength << "-" << time_str << ".png";
      logger << "<<wavelength>> = "  << wavelength<< "\tImage name is " << image_name.str () << endl;
      wstring image_name_w = L"";
      string tmp = image_name.str ();
      image_name_w.assign (tmp.begin (), tmp.end ());
      wstring image_type = L"png";

      if (save_image (h_cam, image_name_w, image_type) == -1) {
        logger << "<<wavelength>> = "  << wavelength<< "\tCouldn't save image " << image_name.str () << endl;
        return -1;
      }

      double exposure = 0.0;
      get_current_exposure (h_cam, exposure);

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

    logger << "\n\nClosing the camera ..\n";

    free_image_memory (h_cam, image, mem_id);

    if (is_StopLiveVideo (h_cam, IS_FORCE_VIDEO_STOP) != IS_SUCCESS) {
      logger << "Unable to stop live video ..\n";
      throw std::exception ();
    }
  } // end of try
  catch (std::exception& e) {
    exit_camera (h_cam);		
    return -1;
  }

  exit_camera (h_cam);		

  return 0;
}
