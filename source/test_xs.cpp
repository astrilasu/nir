#include <iostream>
#include <stdexcept>
using namespace std;

#include <ueye.h>


void findAOI (HIDS& h_cam, int& width, int& height)
{
  IS_RECT rect_aoi;

  int n_ret = is_AOI (h_cam, IS_AOI_IMAGE_GET_AOI, (void*)&rect_aoi, sizeof (rect_aoi));
  if (n_ret == -1) {
    throw std::runtime_error ("Unable to get AOI ..");
  }

  width = rect_aoi.s32Width;
  height = rect_aoi.s32Height;
}



int main (int argc, char *argv[])
{
  HIDS h_cam = 0;
  int n_ret = is_InitCamera (&h_cam, NULL);

  if (n_ret != IS_SUCCESS) {
    cout  << "InitCamera failed.. Uploading new firmware..\n";
    //Check if GigE uEye SE needs a new starter firmware
    if (n_ret == IS_STARTER_FW_UPLOAD_NEEDED) { 
      //Upload new starter firmware during initialization
      h_cam =  h_cam | IS_ALLOW_STARTER_FW_UPLOAD;
      n_ret = is_InitCamera (&h_cam, NULL);
    }
  }
  else {
    cout << "Camera " << h_cam << " initialized..\n";
  }

  if (n_ret == -1) {
    cout << "Failed to initialize camera again ..\n";
    throw std::runtime_error ("Failed to initialize camera ..\n");
  }
  else {
    cout << "Camera handle = " << h_cam << endl;
  }

  int width = 0, height = 0;

  findAOI (h_cam, width, height);

  cout << "width = " << width << " , height = " << height << endl;

  int bits_pp = 24;
  char* image = NULL;
  int mem_id = 0;

  n_ret = is_AllocImageMem (h_cam, width, height, bits_pp, &image, &mem_id);
  if (n_ret == IS_SUCCESS) {
    cout << "Image Memory allocation success ..\n";
  }
  else {
    cout << "Image Memory allocation failed ..\n";
    throw std::runtime_error ("Failed to allocate memory");
  }

  if ( (n_ret = is_SetImageMem (h_cam, image, mem_id)) == IS_SUCCESS) {
    cout << "SetImageMem () SUCCESS ..\n";
  }
  else {
    cout << "SetImageMem () FAILED ..\n";
    throw std::runtime_error ("SetImageMem () FAILED ..");
  }


  if (is_CaptureVideo (h_cam, IS_WAIT) != IS_SUCCESS) {
    cout << "Failed to start live mode ..\n";
    throw std::runtime_error ("Failed to start live mode");
  }
  else {
    cout << " Started live mode ..\n";
  }

  n_ret = is_FreezeVideo (h_cam, 2000);
  if (n_ret == -1) {
    cout << "FreezeVideo () FAILED ..\n";
    throw std::runtime_error ("Capture image failed..");
  }
  cout << "Image captured ..\n";

  IMAGE_FILE_PARAMS file_par;
  file_par.pwchFileName = L"testimage.jpg";
  file_par.pnImageID = NULL;
  file_par.ppcImageMem = NULL;
  file_par.nQuality = 0;
  file_par.nFileType = IS_IMG_JPG;

  n_ret = is_ImageFile (h_cam, IS_IMAGE_FILE_CMD_SAVE, (void*)&file_par, sizeof (file_par));

  if (n_ret == -1) {
    cout << "Save image failed ..\n";
    throw std::runtime_error ("Save image failed ..\n");
  }


  n_ret = is_ExitCamera (h_cam);
  if (n_ret == IS_NO_SUCCESS) {
    cout << "ExitCamera () for Camera " << h_cam << " failed..\n";
    throw std::runtime_error ("Exit camera failed...\n");
  }
  cout << "Exit camera succeeded ..\n";


  return 0;
}
