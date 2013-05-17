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

#include "fitsio.h"

extern float over_exposed_ratio_par;

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
    cout << "Enter arguments\n\n"
            "\t1) Wavelength\n"
            "\t2) Over exposed pixels percentage\n";
    return -1;
  }

  string wavelength = argv[1];
  string time_str = "";

  over_exposed_ratio_par = atof (argv[2]) / 100.0;

  get_camera_list ();

  get_system_time (time_str);

  ostringstream logfilename;
  logfilename << "log-auto-exposure-" << wavelength << "-" << time_str << ".txt";
  cout << "Log file is " << logfilename.str () << endl;
  ofstream ofile (logfilename.str ().c_str ());

  HIDS h_cam = 0;
  try {

    h_cam = setup_camera (&h_cam);
    if (h_cam == -1) {
      return -1;
    }

    if (set_display_mode (h_cam, IS_SET_DM_MONO) == -1) {
      return -1;
    }

    //UINT color_mode = IS_CM_MONO8;    
    UINT color_mode = IS_CM_MONO12;  
    if (set_color_mode (h_cam, color_mode) == -1) {
      cout << "Unable to se color mode ..\n";
      ofile << "Unable to se color mode ..\n";
      return -1;
    }

    cout << endl;

    double fps = 0.;
    if (get_fps (h_cam, &fps) != -1) {
      cout << "New Frame rate = " << fps << endl;
      ofile << "New Frame rate = " << fps << endl;
    }

    int gamma;
    if (get_gamma (h_cam, &gamma) == -1) {	  
      return -1;
    }
    cout << "New gamma = " << gamma << endl;
    ofile << "New gamma = " << gamma << endl;

    int width = 0; 
    int height = 0;
    get_AOI (h_cam, width, height);

    int bits_pp = 12; // bits per pixel

    cout << "Bits per pixel = " << bits_pp << endl;
    ofile << "Bits per pixel = " << bits_pp << endl;

    INT offset = 0;
    is_Blacklevel (h_cam, IS_BLACKLEVEL_CMD_GET_OFFSET_DEFAULT, (void*)&offset, sizeof (offset));
    cout << "Black level default offset = " << offset << endl;
    is_Blacklevel (h_cam, IS_BLACKLEVEL_CMD_GET_OFFSET, (void*)&offset, sizeof (offset));
    cout << "Black level  offset = " << offset << endl;

    char* image = NULL;
    int mem_id = 0;

    if (setup_image_memory (h_cam, width, height, bits_pp, &image, &mem_id) == -1) {
      return -1;
    }

    if (is_CaptureVideo (h_cam, IS_WAIT) != IS_SUCCESS) {
      cout << "Failed to start live mode ..\n";
      ofile << "Failed to start live mode ..\n";
      throw std::exception ();
    }


    // SETTING AUTO EXPOSURE
    double val = 1.;
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL) != IS_SUCCESS) {
      cout << "Unable to set auto shutter ..\n";
      ofile << "Unable to set auto shutter ..\n";
      throw std::exception ();
    }

    // SETTING AUTO FRAME RATE 
    val = 1.;
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_FRAMERATE, &val, NULL) != IS_SUCCESS) {
      cout << "Unable to set auto shutter ..\n";
      ofile << "Unable to set auto shutter ..\n";
      throw std::exception ();
    }

    val = 0.;
     //DISABLING AUTO GAIN
    if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_GAIN, &val, NULL) != IS_SUCCESS) {
      cout << "Unable to set auto gain ..\n";
      ofile << "Unable to set auto gain ..\n";
      throw std::exception ();
    }


#ifdef WIN32
    sleep (5000);
#else
    sleep (5);
#endif


    find_best_exposure (h_cam, image, width, height, ofile, atoi (wavelength.c_str ()), time_str);

    if (capture_image (h_cam) == -1) {
      cout << "Capture Image failed ..\n";
      ofile << "Capture Image failed ..\n";
      throw std::exception ();
    }
    else {
      cout << "Image Captured ..\n";
      ofile << "Image Captured ..\n";
    }

    ostringstream image_name;
    image_name << "test-auto-exposure-" << wavelength << "-" << time_str << ".png";
    cout << "Image name is " << image_name.str () << endl;
    wstring image_name_w = L"";
    string tmp = image_name.str ();
    image_name_w.assign (tmp.begin (), tmp.end ());
    wstring image_type = L"png";

    if (save_image (h_cam, image_name_w, image_type) == -1) {
      cout << "Couldn't save image " << image_name.str () << endl;
      ofile << "Couldn't save image " << image_name.str () << endl;
      return -1;
    }
 
    double exposure = 0.0;

    get_current_exposure (h_cam, exposure);

    fitsfile *fptr;
    long fpixel = 1;		
    long naxis = 2;
    long naxes[2] = {width, height};
    const int size = width * height;
    //unsigned short * pixel = new unsigned short[size];
    int fitsstatus = 0;		

    image_name.str ("");
    image_name << "test-auto-exposure-" << wavelength << "-" << time_str << ".fit";
    string fits_filename = image_name.str ();
    cout << "Creating file " << fits_filename << " ..\n";

    if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
      cout << "Unable to create file ..\n";		
      print_fits_error (fitsstatus);
    }

    cout << "Creating image ..\n";

    if (fits_create_img (fptr, USHORT_IMG, naxis, naxes, &fitsstatus)) {
      print_fits_error (fitsstatus);
    }

    cout << "Writing image ..\n";
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

    cout << "\n\nClosing the camera ..\n";
    ofile << "\n\nClosing the camera ..\n";
    free_image_memory (h_cam, image, mem_id);

    if (is_StopLiveVideo (h_cam, IS_FORCE_VIDEO_STOP) != IS_SUCCESS) {
      cout << "Unable to stop live video ..\n";
      ofile << "Unable to stop live video ..\n";
      throw std::exception ();
    }
  }
  catch (std::exception& e) {
    exit_camera (h_cam);		
    ofile.close ();
    return -1;
  }

  ofile.close ();

  exit_camera (h_cam);		

  return 0;
}
