#include <iostream>
#include <exception>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <memory>
#include <cstring>
#include <fstream>
using namespace std;

#include "camera.h"
#include "logger.h"
#include "fitsio.h"
#include "systemtime.h"

Logger logger;

void print_fits_error(int status)
{
	if(status) {
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}

int main (int argc, char *argv[])
{
  if (argc < 3) {
    logger << "Enter arguments\n\n"
            "\t1) Wavelength\n"
            "\t2) Over exposed pixels percentage\n";
    return -1;
  }

  logger.open ("log.txt");
  int wavelength = atoi (argv[1]);
  string time_str = "";

  float over_exposed_ratio_par = atof (argv[2]) / 100.0;

  CameraWrapper* camera = NULL;
  try
  {
    camera = new CameraWrapper ();
    camera->setOverExposedRatio (over_exposed_ratio_par);

    logger << "<<Starting>> wavelength " << wavelength << endl;
    logger << "<<wavelength>> = " << wavelength << endl;

    // Giving some time for the filter wheel to initialize. 
    sleep (8);

    utils::getSystemTime (time_str);

    // At this point, auto exposure is enabled using the camera APIs.
    // auto exposure is disabled in the below function call, to fine tune exposure using our own algorithm
    camera->findBestExposure (wavelength, time_str);


    // At this point, auto exposure remains disabled.
    // Image is captured using the exposure obtained from our own algorithm.
    camera->captureImage ();

    ostringstream image_name;
    image_name << "test-auto-exposure-" << wavelength << "-" << time_str << ".png";
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

    fitsfile *fptr = NULL;
    long fpixel = 1;		
    long naxis = 2;
    long naxes[2] = {width, height};
    const int size = width * height;
    int fitsstatus = 0;		
    int status = 0;
    image_name.str ("");
    image_name << "test-auto-exposure-" << wavelength << "-" << time_str << ".fit";
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
    if(fits_update_key(fptr, TFLOAT, "EXPOSURE", &fexposure, (char*)"Exposure in milli seconds", &fitsstatus)) {
      print_fits_error(fitsstatus);
    }

    if(fits_close_file(fptr, &fitsstatus)) {
      print_fits_error(fitsstatus);	
    }

    logger << "<<Completed>> wavelength " << wavelength << endl;
  }
  catch (std::exception& e) {
    logger << "Exception : " << e.what () << endl;
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
