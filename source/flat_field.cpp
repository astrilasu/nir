#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>
#include <sstream>
#include <fstream>
#include <exception>
using namespace std;

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "systemtime.h"
#include "camera.h"
#include "fitsio.h"
#include "logger.h"


Logger logger;

void print_fits_error(int status)
{
	if(status){
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}

int main (int argc, char *argv[])
{	
  if (argc <2) {
    cout << "Enter arguments ..\n"
      "1) directory for storing files\n";
    return -1;
  }

  string dir = argv[1];
  if (opendir (dir.c_str ())) {
    cout << "Directory already exists .. Please enter a new directory name ..\n";
    return -1;
  }
  mkdir (dir.c_str (), 0777);


  string time_str = "";
  utils::getSystemTime (time_str);

	int status = 0;
	fitsfile* fptr = NULL;	

  CameraWrapper::getCameraList ();


  CameraWrapper* camera = NULL;
  try {

    camera = new CameraWrapper ();

    // SETTING MANUAL EXPOSURE
    double val = 0.;
    camera->setParameter (val, IS_SET_ENABLE_AUTO_SHUTTER, "AUTO SHUTTER");
   
    // SETTING MANUAL FRAME RATE
    val = 0.;
    camera->setParameter (val, IS_SET_ENABLE_AUTO_FRAMERATE, "AUTO FRAMERATE");

    // Setting frame rate to 0.5 to get the maximum exposure of 2 sec
    double fps = 0.5;
    double newfps = 0.0;
    camera->setFps (fps, newfps);

    val = 0.;
    // SETTING AUTO GAIN
    camera->setParameter (val, IS_SET_ENABLE_AUTO_GAIN, "AUTO GAIN");

#ifndef WIN32
    sleep (5);
#else
    sleep (5000);
#endif

    /**** IMAGE CAPTURE logic ****/


    //string exposures_str = "0.01 1 10 20 40 80 120 240 350 450 600 700 800 900 1000 1100 1200 1300 1400 1500 1600 1700 1800 1900 1999";
    //string exposures_str = "0.01 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100 105 110 115 120 125 130 135 140";
    //450 nm, 532, 671, 920
    //string exposures_str = "0.01 1 2 3 4 5 6 7 8 9 10 10.3 10.6 10.9 11.2 11.5 11.8 12.1 12.4 12.7 13.0 13.3 13.6 13.9 14.2 14.5 14.8 15.1 15.4 15.7 16.0 16.3 16.6 16.9 17.2 17.5 18.0 18.5 19.0, 19.5 20.0";
    // 1050
    //string exposures_str = "0.01 1 2  4  6  8  10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72";
    //string exposures_str = "62 64 66 68 70 72 74 76 78 80 82 84 86 88 90 92 94 96 98 100 102 104 106 108 110 112 114 116 118 120";
    //string exposures_str = "122 124 126 128 130 132 134 136 138 140 142 144 146 148 150 152 154 156 158 160 162 164 166 168 170 172 174 176 178 180";
    //string exposures_str = "180 190 200 210 220 230 240 250 260 ";
    //750, 850
    //string exposures_str = "0.01 1 1.2 1.4 1.6 1.8 2.0 2.2 2.5 2.8 3.1 3.4 3.7 4.0 4.3 4.6 4.9 5.2";
    // dark frame
    //string exposures_str ="1 10 25 50 100 150 200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950 1000 1050 1100 1150 1200 1250 1300 1350 1400 1450 1500 1550 1600 1650 1700 1750 1800 1850 1900 1950 2000";
    string exposures_str ="1 10 25 50 100 150 200 250";
    istringstream istr (exposures_str);
    vector <float> exposures;
    float exp_val = 0.;
    while (istr) {
      istr >> exp_val;
      exposures.push_back (exp_val);
    }
    exposures.resize (exposures.size () -1);
    for (int i=0; i<exposures.size (); i++) {
      cout << exposures[i] << " ";
    }
    cout << endl;
    
    for (int i = 0; i < exposures.size () ; i++) {

      double exposure = exposures[i];

      utils::getSystemTime (time_str);

      camera->setExposure (exposure);

#ifndef WIN32
      sleep (5);
#else
      ::Sleep(5000);
#endif

      cout << "Capturing image for exposure " << exposure << endl;
      camera->captureImage ();

      exposure = 0.0;
      camera->getCurrentExposure (exposure);

      ostringstream image_name;
      image_name << dir << "/flat-field-" << exposure << "-" << time_str << ".png";
      wstring image_name_w = L"";
      string tmp = image_name.str ();
      image_name_w.assign (tmp.begin (), tmp.end ());
      wstring image_type = L"png";

      camera->saveImage (image_name_w, image_type);


      int width = 0, height = 0;
      camera->findAOI (width, height);
      fitsfile *fptr;
      long fpixel = 1;		
      long naxis = 2;
      long naxes[2] = {width, height};
      const int size = width * height;
      //unsigned short * pixel = new unsigned short[size];
      int fitsstatus = 0;		

      image_name.str ("");
      image_name << dir << "/flat-field-" << exposure << "-" << time_str << ".fit";
      string fits_filename = image_name.str ();
      cout << "\tCreating file " << fits_filename << " ..\n";

      if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
        cout << "\tUnable to create file ..\n";		
        print_fits_error (fitsstatus);
      }

      cout << "Creating image ..\n";

      if (fits_create_img (fptr, USHORT_IMG, naxis, naxes, &fitsstatus)) {
        print_fits_error (fitsstatus);
      }

      
      cout << "Writing image ..\n";
      unsigned short* pixels = new unsigned short [width*height];
      unsigned int im_size = width*height*2;

      char* image = camera->getImageMemory ();
      memcpy (pixels, image, height*width*2);

      if (fits_write_img (fptr, TUSHORT, fpixel, im_size, pixels, &fitsstatus)) {
        print_fits_error(fitsstatus);
      }

      float fexposure = exposure;
      if(fits_update_key(fptr, TFLOAT, "EXPOSURE", &fexposure, (char*)"Exposure in milli seconds", &fitsstatus)) {
        print_fits_error(fitsstatus);
      }

      if(fits_close_file(fptr, &fitsstatus)) {
        print_fits_error(fitsstatus);	
      }
    }
  } // end of try
  catch (std::exception& e) {
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
