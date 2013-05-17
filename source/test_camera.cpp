#include <iostream>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>
using namespace std;

#include "camera.h"

#include "fitsio.h"

void print_fits_error(int status)
{
	if(status){
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}

void reset_exposure_defaults (HIDS hcam)
{
    double val = 0.;
    double expose = 0.;
    is_SetAutoParameter (hcam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL);
    is_Exposure (hcam, IS_EXPOSURE_CMD_GET_EXPOSURE_DEFAULT, (void*)&expose, sizeof (expose));
    cout << "Default exposure time = " << expose << endl;
    is_Exposure (hcam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expose,  sizeof (expose));
}

void save_image_as_text (HIDS hcam, char* image, int width, int height, string filename, int bits_pp)
{
  ofstream ofile (filename.c_str ());
  unsigned int size = height * width;
  if (bits_pp > 8) {
    size *= 2;
  }
  for (unsigned int i = 0; i < size; i++) {
    ofile << (int)image[i] << " ";
  }
  ofile << endl;
  ofile.close ();
}

void save_image_as_ascii (HIDS hcam, char* image, int width, int height)
{
  cout << "----- Save image as ascii -----\n\n";
  cout << "width = " << width << " height = " << height << endl;

  int pitch = 0;
  if (is_GetImageMemPitch (hcam, &pitch) != IS_SUCCESS) {
    cout << "Unable to determine the image pitch ..\n";
    return;
  }

  cout << "width = " << width << " image pitch = " << pitch << endl;

  int inc = pitch / width;

  cout << "inc = " << inc << endl;

  ofstream ofile ("image-ascii.txt");
  for (unsigned int i = 0; i < height; i++) {
    for (unsigned int j = 0; j < pitch; j+=2) {
      char* ptr = image;
      ptr += ((i*pitch) + j);
      int N = 0;

      char buffer[10];
      fill_n (buffer, 10, '\0');

      int k = 0;
      for (k = 0; k < inc; k++) {
        buffer[k] = ptr[k];
      }

      //N = ((((int)(buffer[1]-48)) << 8) | ((int)(buffer[0]-48)));

      //ofile << N << "(" << (int)buffer[1] << "," << (int)buffer[0] <<")\t";

      ofile << (int)buffer[1] << "\t" << (int)buffer[0] <<"\t";

    }
    ofile << endl;
  }
  ofile.close ();
}

void get_trigger_info (HIDS hcam)
{
  cout << "\n----------- TRIGGER INFO -------------\n";
  UINT val = 0;
  UINT ret = 0;

  ret = is_Trigger (hcam, IS_TRIGGER_CMD_GET_BURST_SIZE_SUPPORTED, (void*)&val, sizeof (val));

  if (val == 1) {
    cout << "Burst size supported ..\n";
    RANGE_OF_VALUES_U32 burst_size;
    ret = is_Trigger (hcam, IS_TRIGGER_CMD_GET_BURST_SIZE_RANGE, (void*)&burst_size, sizeof (burst_size));

    if (ret == IS_SUCCESS) {
      UINT max, min;
      min = burst_size.u32Minimum;
      max = burst_size.u32Maximum;
      cout << "Burst size min = " << min << " max = " << max << endl;
    }

    ret = is_Trigger (hcam, IS_TRIGGER_CMD_GET_BURST_SIZE, (void*)&val, sizeof (val));
    cout << "Current burst size = " << val << endl;
  }
  else {
    cout << "Burst size not supported ..\n";
  }
  cout << "\n------------------------\n";
}

int main (int argc, char* argv[])
{
  if (argc < 3) {
    cout << "Enter arguments\n(1) 8 / 12 / 16 (bits) \n(2) bmp / png / jpg\n\n";
    return 0;
  }
  HIDS h_cam = 0;
  try {
    h_cam = setup_camera (&h_cam);
    if (h_cam == -1) {
      return -1;
    }

    char* image = NULL;
    int mem_id = 0;
    int mode = atoi (argv[1]);

    UINT color_mode = IS_CM_MONO12;
    int bits_pp = 12; // bits per pixel
    cout << "Bits per pixel = " << bits_pp << endl;

    int ret = is_SetColorMode (h_cam, color_mode);
    if(ret == IS_SUCCESS) std::cout << "Color Mode Set\n";


    int width = 0; 
    int height = 0;
    get_AOI (h_cam, width, height);

    if (setup_image_memory (h_cam, width, height, bits_pp, &image, &mem_id) == -1) {
      return -1;
    }

    //get_pixel_clock_info (h_cam);

    ret = 0;
    if ( (ret = is_CaptureVideo (h_cam, IS_WAIT)) != IS_SUCCESS) {
      cout << "Failed to start live mode ..\n";
      throw std::exception ();
    }


    double frame_rate = 0.;
    is_SetFrameRate (h_cam, IS_GET_FRAMERATE, &frame_rate);
    cout << "Frame rate = " << frame_rate << endl;


    if (capture_image (h_cam) == -1) {
      cout << "Capture Image failed ..\n";
      return -1;
    }
    else {
      cout << "Image Captured ..\n";
    }

    string image_t = argv[2];
    wstring image_type = L"";
    image_type.assign (image_t.begin (), image_t.end ());

    wostringstream image_name_w;
    image_name_w << L"ids-data/ids-test-" << mode << L"-bit." << image_type;
    wstring image_name = image_name_w.str ();
    wcout << "Saving file " << image_name << endl;


    if (save_image (h_cam, image_name, image_type) != 0) {
      wcout << "Couldn't save image " << image_name << endl;
      exit_camera (h_cam);
      return -1;
    }
    else {
      wcout << "Saved image " << image_name << endl;
    }

    ostringstream im_name;
    im_name << "ids-data/ids-test-" << mode << "-bit-" << argv[2] << ".txt";

    fitsfile* fptr = NULL;	
    long fpixel = 1;		
    long naxis = 2;
    long naxes[2] = {width, height};
    const int size = width * height;
    //unsigned short * pixel = new unsigned short[size];
    int fitsstatus = 0;		

    string fits_filename = "test.fit";
    cout << "Creating file " << fits_filename << "..\n";

    if (fits_create_file (&fptr, fits_filename.c_str (), &fitsstatus)) {
      cout << "Unable to create file ..\n";		
      print_fits_error (fitsstatus);
    }

    cout << "Creating image ..\n";

    if (fits_create_img (fptr, USHORT_IMG, naxis, naxes, &fitsstatus)) {
      print_fits_error (fitsstatus);
    }


    int pitch = 2056;
    int inc = 2;
    int z = 0;

    unsigned short* sbuffer = new unsigned short[height*width];
    
    memcpy (sbuffer, image, height*width*2);

    cout << "sbuffer ..";
    cout << sbuffer[0] << " " << sbuffer[1] << endl;


    unsigned int im_size = width*height;
    if (fits_write_img(fptr, TUSHORT, fpixel, im_size, sbuffer, &fitsstatus)) {
      print_fits_error(fitsstatus);
    }


    if (fits_close_file (fptr, &fitsstatus)) {
      print_fits_error (fitsstatus);
    }


    free_image_memory (h_cam, image, mem_id);


    if (is_StopLiveVideo (h_cam, IS_FORCE_VIDEO_STOP) != IS_SUCCESS) {
      cout << "Unable to stop live video ..\n";
      throw std::exception ();
    }
  } // end of try
  catch (std::exception &e) {
    exit_camera (h_cam);		
    return 0;
  }
  
  exit_camera (h_cam);		

  return 0;
}
