#include <iostream>
#include <cstdio>
#include <ctime>
#include <sstream>
using namespace std;


#include "camera.h"
#include "config_parser.hpp"
#include "filterwheel.h"
#include "filterwheel_manager.h"

#include "fitsio.h"


void print_fits_error(int status)
{
	if(status){
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}

void get_system_time (string& time_str)
{
  time_t rawtime;
  struct tm* timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (buffer, 80, "%Y-%m-%d--%H-%M-%S", timeinfo);
  time_str = buffer;
}

int main (int argc, char *argv[])
{	
	/*cout << "Size of unsigned short = " << sizeof (unsigned short) << endl;
	cout << "Size of unsigned int = " << sizeof (unsigned int) << endl;
	return 0;*/

	if (argc < 2) {
		cout << "Enter the config file as the command line argument ..\n";
		return -1;
	}

	int status = 0;
	fitsfile* fptr = NULL;	

	/**** XML PARSER setup ****/

  ConfigParser* parser = NULL;
  string inputfile = argv[1];

  try {
    parser = new ConfigParserDOM ();
	cout << "Input file is " << inputfile << endl;
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      cout << "Error Message : " << parser->getErrorMessage () << endl;
      return -1;
    }
  }
  catch (MyException& e) {
    cout << e.m_error_message << endl;
    return -1;
  }
 

  /**** FilterWheel setup ****/

  FilterWheel* f1 = new FilterWheel ();

  cout << "Setting up filter wheel 1..\n";
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


  cout << "Setting up filter wheel 2..\n";

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

  HIDS h_cam = setup_camera ();
  if (h_cam == -1) {
    return -1;
  }

  if (set_display_mode (h_cam, IS_SET_DM_MONO) == -1) {
	  return -1;
  }

  UINT color_mode = IS_CM_MONO8;    
  //UINT color_mode = IS_CM_MONO12;  
  if (set_color_mode (h_cam, color_mode) == -1) {
	  cout << "Unable to se color mode ..\n";
    return -1;
  }

  //print_flash_parameters (h_cam);		

  cout << endl;

  //print_gain_parameters (h_cam);

  //if (get_pixel_clock_info (h_cam) == -1) {
  //  return -1;
  //}


  double fps = 0.5;  
  double new_fps = 0.5;
  if (set_fps (h_cam, fps, &new_fps) != -1) {
    cout << "Frame rate = " << new_fps << endl;
  }

  if (get_fps (h_cam, &fps) != -1) {
    cout << "New Frame rate = " << fps << endl;
  }

  /*if (set_gamma (h_cam, 100) == -1) {	  
	  return -1;
  }*/

  int gamma;
  if (get_gamma (h_cam, &gamma) == -1) {	  
	  return -1;
  }
  cout << "New gamma = " << gamma << endl;

  if (get_exposure_info (h_cam) == -1) {
    return -1;
  }


  int width = 0; 
  int height = 0;
  get_AOI (h_cam, width, height);

  int bits_pp = 0; // bits per pixel
  switch (color_mode) {

    case IS_CM_MONO8:
      bits_pp = 8;
      break;

	case IS_CM_MONO12:
      bits_pp = 12;
      break;

    case IS_CM_MONO16:
      bits_pp = 16;
      break;

  }

  cout << "Bits per pixel = " << bits_pp << endl;

  char* image = NULL;
  int mem_id = 0;

  if (setup_image_memory (h_cam, width, height, bits_pp, &image, &mem_id) == -1) {
    return -1;
  }


  /**** IMAGE CAPTURE logic ****/


  //for (int i = 0; i < 2 /*2*/; i++) { // Loop through 2 filter wheels here.
	 // map <int, int>& filters = parser->getFilterWheelById (i);

	 // int k = 0;
	 // map <int, int>::iterator itr = filters.begin ();
	 // while (itr != filters.end ()) {
		//  int wavelength = itr->second;

		//  if (wavelength == 999) {
		//	  itr++;
		//	  continue;
		//  }


  
		
	  int wavelengths[7] = {450, 532, 671, 750, 850, 920, 1050};	  

	for (int i = 0; i < 7; i++) {
		  int wavelength = wavelengths[i];
		  int k = 0;
		  int exposure = parser->getExposureForFilter (wavelength);
		  cout << "wavelength = " << wavelength << " exposure = " << exposure << endl;

		  fw_man.gotoFilter (wavelength);

		  if (k == 0) {
#ifndef WIN32
		  sleep (8);
#else
		  ::Sleep(8000);
#endif
		  k++;
		  }
		  else {
#ifndef WIN32
		  sleep (6);
#else
		  ::Sleep(6000);
#endif
		  }
		  cout << "Setting exposure = " << exposure << endl;
		  if (set_exposure (h_cam, exposure) == -1) {
			  break;
		  }

		if (capture_image (h_cam) == -1) {
			cout << "Capture Image failed ..\n";
			break;
		}
		else {
			cout << "Image Captured ..\n";
		}

		
#ifndef WIN32
#else
		string time = "";
		get_system_time (time);
#endif
		
		ostringstream ostr;
		ostr << "wavelength-" << wavelength << "-exposure-" << exposure << "-" << time << ".bmp";
		wstring image_name;
		string tmp = ostr.str ();
		image_name.assign (tmp.begin (), tmp.end ());
		cout << "Saving image "<< ostr.str () << endl;
		if (save_image (h_cam, image_name.c_str (), L"jpg") == -1) {
			cout << "Save image " << ostr.str () << " failed ..\n";
			break;
		}

		inquire_image_memory (h_cam, image, mem_id);

		ostr.str ("");
		ostr << "wavelength-" << wavelength << "-exposure-" << exposure << "-" << time << ".fit";
		cout << "Test print filename = " << ostr.str () << endl;
		string fits_filename = ostr.str ();
		fitsfile *fptr;
		long fpixel =1;		
		long naxis = 2;
		long naxes[2] = {width, height};
		const int size = width * height;
		//unsigned short * pixel = new unsigned short[size];
		int fitsstatus = 0;		

		cout << "Creating file " << fits_filename << "..\n";
			
		if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
			cout << "Unable to create file ..\n";		
			print_fits_error (fitsstatus);
		}
		
		cout << "Creating image ..\n";

		if (fits_create_img (fptr, bits_pp, naxis, naxes, &fitsstatus)) {
			print_fits_error (fitsstatus);
		}

		cout << "Writing image ..\n";
		//unsigned short* pixel = new unsigned short [width*height];
		//
		unsigned int im_size = width*height;
		//fill_n (pixel, im_size, 0);

		//for (int i=0; i<im_size; i++) {
		//	pixel[i] = (unsigned short) image[i];
		//	if (pixel[i] > 255) {
		//		cout << "Pixel size overflow " << pixel[i] << " " << image[i] << " at pixel " << i << endl;
		//		pixel[i] = 0;
		//		//return 0;
		//	}

		//}
		if (fits_write_img(fptr, TBYTE, fpixel, im_size, image, &fitsstatus)) {
			print_fits_error(fitsstatus);
		}
		
		long lexposure = exposure;
		if(fits_update_key(fptr, TLONG, "EXPOSURE", &lexposure, "Exposure in microseconds", &status)) {
			print_fits_error(status);
		}

		if(fits_close_file(fptr, &status)) {
			print_fits_error(status);	
		}

		//delete[] pixel;
		//return 0;
		 //itr++;
	  }
  

  //fw_man.gotoFilter (532);






  cout << "\n\nClosing the camera ..\n";
  free_image_memory (h_cam, image, mem_id);
  exit_camera (h_cam);		

  return 0;
}




//for (int i = 0; i < 2 /*2*/; i++) { // Loop through 2 filter wheels here.
//	  map <int, int>& filters = parser->getFilterWheelById (i);
//
//	  int k = 0;
//	  map <int, int>::iterator itr = filters.begin ();
//	  while (itr != filters.end ()) {
//		  int wavelength = itr->second;
//
//		  if (wavelength == 999) {
//			  itr++;
//			  continue;
//		  }
//		  int exposure = parser->getExposureForFilter (wavelength);
//		  cout << "wavelength = " << wavelength << " exposure = " << exposure << endl;
//
//		  fw_man.gotoFilter (wavelength);
//
//		  if (k == 0) {
//#ifndef WIN32
//		  sleep (8);
//#else
//		  ::Sleep(8000);
//#endif
//		  k++;
//		  }
//		  else {
//#ifndef WIN32
//		  sleep (6);
//#else
//		  ::Sleep(6000);
//#endif
//		  }
//		  cout << "Setting exposure = " << exposure << endl;
//		  if (set_exposure (h_cam, exposure) == -1) {
//			  break;
//		  }
//
//		if (capture_image (h_cam) == -1) {
//			cout << "Capture Image failed ..\n";
//			break;
//		}
//		else {
//			cout << "Image Captured ..\n";
//		}
//
//		
//#ifndef WIN32
//#else
//		string time = "";
//		get_system_time (time);
//#endif
//		
//		ostringstream ostr;
//		ostr << "wavelength-" << wavelength << "-exposure-" << exposure << "-" << time << ".bmp";
//		wstring image_name;
//		string tmp = ostr.str ();
//		image_name.assign (tmp.begin (), tmp.end ());
//		cout << "Saving image "<< ostr.str () << endl;
//		if (save_image (h_cam, image_name.c_str ()) == -1) {
//			cout << "Save image " << ostr.str () << " failed ..\n";
//			break;
//		}
//
//		inquire_image_memory (h_cam, image, mem_id);
//
//		ostr.str ("");
//		ostr << "wavelength-" << wavelength << "-exposure-" << exposure << "-" << time << ".fit";
//		cout << "Test print filename = " << ostr.str () << endl;
//		string fits_filename = ostr.str ();
//		fitsfile *fptr;
//		long fpixel =1;		
//		long naxis = 2;
//		long naxes[2] = {width, height};
//		const int size = width * height;
//		//unsigned short * pixel = new unsigned short[size];
//		int fitsstatus = 0;		
//
//		cout << "Creating file " << fits_filename << "..\n";
//			
//		if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
//			cout << "Unable to create file ..\n";		
//			print_fits_error (fitsstatus);
//		}
//		
//		cout << "Creating image ..\n";
//
//		if (fits_create_img (fptr, bits_pp, naxis, naxes, &fitsstatus)) {
//			print_fits_error (fitsstatus);
//		}
//
//		cout << "Writing image ..\n";
//		//unsigned short* pixel = new unsigned short [width*height];
//		//
//		unsigned int im_size = width*height;
//		//fill_n (pixel, im_size, 0);
//
//		//for (int i=0; i<im_size; i++) {
//		//	pixel[i] = (unsigned short) image[i];
//		//	if (pixel[i] > 255) {
//		//		cout << "Pixel size overflow " << pixel[i] << " " << image[i] << " at pixel " << i << endl;
//		//		pixel[i] = 0;
//		//		//return 0;
//		//	}
//
//		//}
//		if (fits_write_img(fptr, TBYTE, fpixel, im_size, image, &fitsstatus)) {
//			print_fits_error(fitsstatus);
//		}
//		
//		long lexposure = exposure;
//		if(fits_update_key(fptr, TLONG, "EXPOSURE", &lexposure, "Exposure in microseconds", &status)) {
//			print_fits_error(status);
//		}
//
//		if(fits_close_file(fptr, &status)) {
//			print_fits_error(status);	
//		}
//
//		//delete[] pixel;
//		//return 0;
//		 itr++;
//	  }
//  }
//
//  //fw_man.gotoFilter (532);
//
//
//
//
//
//
//  cout << "\n\nClosing the camera ..\n";
//  free_image_memory (h_cam, image, mem_id);
//  exit_camera (h_cam);		
//
//  return 0;
//}
