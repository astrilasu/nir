#include <iostream>
#include <cstdio>
using namespace std;

#include "camera.h"

int main (int argc, char* argv[])
{
  get_camera_list ();

  HIDS h_cam = setup_camera ();
  if (h_cam == -1) {
    return -1;
  }

  if (set_display_mode (h_cam, IS_SET_DM_MONO) == -1) {
	  return -1;
  }

  UINT color_mode = IS_CM_MONO8;  
  //UINT color_mode = IS_CM_MONO16;  

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


  double fps = 1;  
  double new_fps = 0.5;
  //if (set_fps (h_cam, fps, &new_fps) != -1) {
  //  cout << "Frame rate = " << new_fps << endl;
  //}

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

    double val = 1.;
	is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL);


	  val = 0.;
  is_SetAutoParameter (h_cam, IS_GET_ENABLE_AUTO_SHUTTER, &val, NULL);
  cout << "Auto shutter = " << val << endl;

  //is_SetAutoParameter (h_cam, IS_GET_ENABLE_AUTO_SENSOR_SHUTTER, &val, NULL);
  //cout << "Auto sensor shutter = " << val << endl;

  is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_FRAMERATE, &val, NULL);

  is_SetAutoParameter (h_cam, IS_GET_ENABLE_AUTO_FRAMERATE, &val, NULL);
  cout << "Auto frame rate = " << val << endl;

 /* if (set_exposure (h_cam, 50000) == -1) {
	  return -1;
  }*/
  if (get_exposure_info (h_cam) == -1) {
    return -1;
  }


  char* image = NULL;
  int mem_id = 0;

  if (setup_image_memory (h_cam, width, height, bits_pp, &image, &mem_id) == -1) {
    return -1;
  }

  if (capture_image (h_cam) == -1) {
    cout << "Capture Image failed ..\n";
    return -1;
  }
  else {
    cout << "Image Captured ..\n";
  }

  wstring image_type = L"bmp";
  wstring image_name = wstring(L"test-sri") + wstring (L".") + image_type;
  wcout << "Saving file " << image_name << endl;



  if (save_image (h_cam, image_name, image_type) == -1) {
	wcout << "Couldn't save image " << image_name << endl;
    return -1;
  }

  free_image_memory (h_cam, image, mem_id);

  exit_camera (h_cam);		
  return 0;
}
