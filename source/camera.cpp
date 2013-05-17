#include "camera.h"
#include "logger.h"

#include <iostream>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdexcept>
using namespace std;

#include <cstdlib>
#include <sys/timeb.h>

float over_exposed_ratio_par = 0.;
Logger logger;

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

int get_camera_list ()
{
  int n_cams = 0;
  UINT n_ret = is_GetNumberOfCameras (&n_cams);
  if (n_ret == -1) {
    logger << "Failed to get the number of cameras ..\n";
    return -1;
  }

  logger << "Number of cameras = " << n_cams << endl;

  if( n_cams >= 1 ) {
    // Create new list with suitable size
    UEYE_CAMERA_LIST* pucl;
    pucl = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + n_cams * sizeof (UEYE_CAMERA_INFO)];
    pucl->dwCount = n_cams;

    //Retrieve camera info
    if (is_GetCameraList(pucl) == IS_SUCCESS) {
      int iCamera;
      for (iCamera = 0; iCamera < (int)pucl->dwCount; iCamera++) {
        //Test output of camera info on the screen
        printf("Camera %i Id: %d\n", iCamera, pucl->uci[iCamera].dwCameraID);
      }
    }	
    delete [] pucl;
  }
  return n_ret;
}

int setup_camera (HIDS* h_cam)
{
  INT n_ret = is_InitCamera (h_cam, NULL);

  if (n_ret != IS_SUCCESS) {
    logger  << "InitCamera failed.. Uploading new firmware..\n";
    //Check if GigE uEye SE needs a new starter firmware
    if (n_ret == IS_STARTER_FW_UPLOAD_NEEDED) { 
      //Upload new starter firmware during initialization
      *h_cam =  (*h_cam) | IS_ALLOW_STARTER_FW_UPLOAD;
      n_ret = is_InitCamera (h_cam, NULL);
    }
  }
  else {
    logger << "Camera " << *h_cam << " initialized..\n";
  }

  if (n_ret == -1) {
    logger << "Failed to initialize camera again ..\n";
    return n_ret;
  }

  if (set_master_gain (*h_cam, 0) == -1) {
    return -1;
  }

  return *h_cam;
}

int exit_camera (HIDS h_cam)
{
  INT n_ret = is_ExitCamera (h_cam);
  if (n_ret == IS_NO_SUCCESS) {
    logger << "ExitCamera () for Camera " << h_cam << " failed..\n";
  }
  return n_ret;
}

int get_AOI (HIDS h_cam, int& width, int& height)
{
  IS_RECT rect_aoi;

  int n_ret = is_AOI (h_cam, IS_AOI_IMAGE_GET_AOI, (void*)&rect_aoi, sizeof (rect_aoi));
  logger << "--- AOI params ---\n";
  logger << "x = " << rect_aoi.s32X << endl;
  logger << "y = " << rect_aoi.s32Y << endl;
  logger << "width = " << (width = rect_aoi.s32Width) << endl;
  logger << "height = " << (height = rect_aoi.s32Height) << endl;
  logger << "-------------------\n";

  return n_ret;
}

int get_sensor_info (HIDS h_cam, SENSORINFO* sensor_info)
{
  INT n_ret = is_GetSensorInfo (h_cam, sensor_info);

  if (n_ret == -1) {
    logger << "GetSensorInfo FAILED ..\n";
    return n_ret;
  }

  logger << "\n--------- SENSOR INFO ----------\n";
  logger << "Sensor name = " << sensor_info->strSensorName << endl;
  char color_mode = sensor_info->nColorMode;

  switch (color_mode) {

    case IS_COLORMODE_BAYER:
      logger << "Bayer mode..\n";
      break;
    case IS_COLORMODE_MONOCHROME:
      logger << "Monochrome mode ..\n";
      break;
    case  IS_COLORMODE_CBYCRY:
      logger << "CBYCRY mode ..\n";
      break;
  }

  logger << "Max width = " << sensor_info->nMaxWidth << endl;
  logger << "Max height = " << sensor_info->nMaxHeight << endl;
  logger << "Pixel size = " << sensor_info->wPixelSize << endl;
  logger << "Global shutter = " << sensor_info->bGlobShutter << endl;
  logger << "Master Gain = " << sensor_info->bMasterGain << endl;
  logger << "Red Channel gain " << sensor_info->bRGain << endl;
  logger << "Green Channel gain " << sensor_info->bGGain << endl;
  logger << "Blue Channel gain " << sensor_info->bBGain << endl;

  logger << "--------------------------------\n\n";

  return n_ret;
}

int setup_image_memory (HIDS h_cam, int width, int height, int bits_pp, char** image, int* mem_id)
{
  int n_ret = is_AllocImageMem (h_cam, width, height, bits_pp, image, mem_id);
  if (n_ret == IS_SUCCESS) {
    logger << "Image Memory allocation success ..\n";
  }
  else {
    logger << "Image Memory allocation FAILED ..\n";

    switch (n_ret) {
      case IS_CANT_ADD_TO_SEQUENCE:
        logger << "Image memory is already included in the sequence and can't be added again..\n";
        break;
      case IS_INVALID_CAMERA_HANDLE:
        logger << "Invalid camera handle..\n";
        break;
      case IS_INVALID_PARAMETER:
        logger << "Invalid parameter passed .. width = " << width << " height = " 
          << height << " bits per pixel = " << bits_pp << endl;
        break;
      case IS_OUT_OF_MEMORY:
        logger << "Out of memory ..\n";
        break;
      case IS_SEQUENCE_BUF_ALREADY_LOCKED:
        logger << "Memory could not be locked. Pointer to the buffer is invalid ..\n";
        break;
    }		
    return n_ret;
  }
  if (n_ret = is_SetImageMem (h_cam, *image, *mem_id) == IS_SUCCESS) {
    logger << "SetImageMem () SUCCESS ..\n";
  }
  else {
    logger << "SetImageMem () FAILED ..\n";
  }
  return n_ret;
}

int print_flash_parameters (HIDS h_cam)
{
  IO_FLASH_PARAMS flash_par;
  int n_ret = is_IO (h_cam, IS_IO_CMD_FLASH_GET_PARAMS, (void*)&flash_par, sizeof (flash_par));
  if (n_ret == IS_SUCCESS) {
    logger << "Flash delay = " << flash_par.s32Delay << endl;
    logger << "Flash duration = " << flash_par.u32Duration << endl;
  }
  else {
    logger << "Getting Flash paramenters failed ..\n";
    return -1;
  }
  return n_ret;
}

int free_image_memory (HIDS h_cam, char* image, int mem_id)
{
  UINT n_ret = 0;
  if ( (n_ret = is_FreeImageMem (h_cam, image, mem_id)) == -1) {
    logger << "Free Image memroy failed ..\n";
  }	
  return n_ret;
}

int inquire_image_memory (HIDS h_cam, char* image, int mem_id)
{
	int nID = 0;
	int nX;
	int nY;
	int nBits;
	int nPitch;

	int status = is_InquireImageMem (h_cam, image, mem_id, &nX, &nY, &nBits, &nPitch);
	
	logger << "\n---------- Image Memory Status -----------\n";
	logger << "width = " << nX << endl;
	logger << "height = " << nY << endl;
	logger << "bits = " << nBits << endl;
	logger << "Pitch = " << nPitch << endl;

	logger << "\n------------------------------------------\n";

	return status;

}
int capture_image (HIDS h_cam)
{
  //int n_ret = is_FreezeVideo (h_cam, IS_WAIT);
  int n_ret = is_FreezeVideo (h_cam, 2000);
  //int n_ret = is_FreezeVideo (h_cam, IS_DONT_WAIT);
  if (n_ret == -1) {
    logger << "FreezeVideo () FAILED ..\n";
    return n_ret;
  }
  return n_ret;
}

int save_image (HIDS h_cam,  wstring image_name, wstring type)
{
  IMAGE_FILE_PARAMS file_par;

  file_par.pwchFileName = (wchar_t*)image_name.c_str ();
  file_par.pnImageID = NULL;
  file_par.ppcImageMem = NULL;
  file_par.nQuality = 0;

  if (type.compare(L"bmp") == 0) {
    file_par.nFileType = IS_IMG_BMP;
  }
  else if (type.compare(L"png") == 0) {
	  file_par.nFileType = IS_IMG_PNG;
  }
  else if (type.compare(L"jpg") == 0) {
	  file_par.nFileType = IS_IMG_JPG;
  }
  else if (type.compare(L"tif") == 0) {
	  file_par.nFileType = IS_IMG_TIF;
  }
	else if (type.compare(L"raw") == 0) {
	  file_par.nFileType = IS_IMG_RAW;
  }
	else {
		throw std::runtime_error ("Unsupported file format used for saving image");
	}

  int n_ret = is_ImageFile (h_cam, IS_IMAGE_FILE_CMD_SAVE, (void*)&file_par, sizeof (file_par));

  if (n_ret == -1) {
    logger << "Save image failed ..\n";
    return -1;
  }

  logger << "Save image return value = " << n_ret << " for file " << file_par.pwchFileName << endl;
  return n_ret;
}

int set_display_mode (HIDS h_cam, UINT mode)
{
  int display_mode = is_SetDisplayMode (h_cam, mode);
  display_mode = is_SetDisplayMode (h_cam, IS_GET_DISPLAY_MODE);
  logger << "Display mode = " << display_mode << endl;
  switch (display_mode)
  {
    case IS_SUCCESS:
      logger << "Get Display Mode Success..\n";
      break;
    case IS_NO_SUCCESS:
      logger << "Set display mode failed..\n";
      break;
    case IS_GET_DISPLAY_MODE:
      logger << "Get display mode..\n";
      break;
    case IS_SET_DM_DIB:
      logger << "DM DIB mode..\n";
      break;
    case IS_SET_DM_MONO:
      logger << "DM mono mode..\n";
      break;
  }
  return display_mode;
}

int get_exposure_info (HIDS h_cam)
{
  logger << "----------- EXPOSURE --------------\n";
  double range[3];
  int n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_FINE_INCREMENT_RANGE, 
      (void*) range, sizeof (range));

  if (n_ret == IS_SUCCESS) {
    logger << "Expsoure range :: min = " << range[0] << " ms, max = "
      << range[1] << " ms, increment = " << range[2] << " ms" << endl;
  }
  else {
    logger << "Query exposure times failed ..\n";
    return -1;
  }

  UINT n_caps = 0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_CAPS, (void*)&n_caps, sizeof (n_caps));

  if (n_ret == IS_SUCCESS) {
    if (n_caps & IS_EXPOSURE_CAP_LONG_EXPOSURE) {
      logger << "Long exposure supported ..\n";
      n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_LONG_EXPOSURE_RANGE, 
          (void*) range, sizeof (range));

      if (n_ret == IS_SUCCESS) {
        logger << "Long expsoure range :: min = " << range[0] << " ms, max = "
          << range[1] << " ms, increment = " << range[2] << " ms" << endl;
      }
      else {
        logger << "Query long exposure times failed ..\n";
        return -1;
      }
    }
    else {
      logger << "Long exposure NOT supported ..\n";
    }
  }

  double curr_exp = 0.0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  logger << "******** Current exposure = " << curr_exp << " ms ********" << endl;


  logger << "---------------------------------\n";

  return n_ret;
}

void find_best_exposure (HIDS h_cam, char* image, int width, int height, 
                         int wavelength, string time_str) 
{
  // Giving sometime for the sensor to adjust exposure -- required for the camera's auto exposure API
  sleep (5);

  double exposure = 0.0;
  get_current_exposure (h_cam, exposure);
  logger << "<<wavelength>> = " << wavelength << "\tInitial exposure from auto exposure = " << exposure << endl;

  if (capture_image (h_cam) == -1) {
    logger << "<<wavelength>> = "  << wavelength<< "\tCapture Image failed ..\n";
  }
  else {
    logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";
  }

  unsigned int size = width * height;
  unsigned short* pixels = new unsigned short [width * height];
  memcpy (pixels, image, height*width*2);

  double val = 0.;
  int count = 0;
  unsigned int iter = 1;

  get_current_exposure (h_cam, exposure);
  float right_exposure = exposure;

  bool over_exposure_adjust = false;

  unsigned short maxpixval = 0;
  float over_exposed_ratio  = 0.0;


  // disable auto exposure ..
  val = 0.;
  if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_SHUTTER, &val, NULL) != IS_SUCCESS) {
    logger << "<<wavelength>> = "  << wavelength<< "\tUnable to set auto shutter ..\n";
    throw std::exception ();
  }
  else {
    logger << "<<wavelength>> = " << wavelength
      << "\tDisabled auto exposure after getting initial estimate from APIs\n";
  }

  // disabling auto frame rate
  val = 0.;
  if (is_SetAutoParameter (h_cam, IS_SET_ENABLE_AUTO_FRAMERATE, &val, NULL) != IS_SUCCESS) {
    logger << "Unable to set auto shutter ..\n";
    throw std::exception ();
  }

  double fps = 0.5;
  double newfps = 0.0;

  // setting a frame rate of 0.5 to ensure that we can set maximum exposure (2 sec)
  is_SetFrameRate (h_cam, fps, &newfps);
  logger << "New frame rate = " << newfps << endl;


  // REDUCE OVER EXPSOURE IN THIS LOOP
  while (true) {

    count = 0;
    maxpixval = 0;

    for (unsigned int i = 0; i < size; i++) {
      unsigned short pixval = (unsigned short) pixels[i];
      if (pixval > maxpixval) {
        maxpixval = pixval;
      }
      
      if (pixval > (MAX_INTENSITY-1)) {
        count++;
      }
    }

    over_exposed_ratio = count / (float) size;

    if (over_exposed_ratio < over_exposed_ratio_par) {
      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;
      break;
    }

    get_current_exposure (h_cam, exposure);

    // disabling camera's auto exposure at this point to fine tune using our own logic
    logger << "<<wavelength>> = "  << wavelength<< "\tCurrent exposure = " << exposure << endl;
    logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;

    if (over_exposed_ratio < 0.1) { // reduce the exposure by 10% for faster convergence
      exposure = 0.9 * exposure;
    }
    else {
      exposure = (1.0 - over_exposed_ratio) * exposure;
    }

    right_exposure = exposure;

    logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure logic <<<<<<< Setting exposure to " 
      << exposure << " at iteration " << iter << " >>>>>>>\n\n\n";

    set_exposure (h_cam, exposure);
    over_exposure_adjust = true;

    // giving some time for the new exposure to take effect. Is this needed? CHECK..
    sleep (3);

    if (capture_image (h_cam) == -1) {
      logger << "<<wavelength>> = "  << wavelength<< "\tCapture Image failed ..\n";
      throw std::exception ();
    }
    else {
      logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";
    }

    memcpy (pixels, image, height*width*2);
    iter++;
  }

  float under_exposed_ratio = 0.;

  // REDUCE UNDER EXPOSURE IN THIS LOOP
  while (true) {

    // if the image has been adjusted for over exposure already, need not run this loop .. break..
    if (over_exposure_adjust) {
      break;
    }

    count = 0;
    maxpixval = 0;
    unsigned short pixval = 0;
    for (unsigned int i = 0; i < size; i++) {
      pixval = (unsigned short) pixels[i];
      if (pixval > maxpixval) {
        maxpixval = pixval;
      }
      if (pixval > (MAX_INTENSITY-1)) {
        count++;
      }
    }

    over_exposed_ratio = count / (float) size;

    if (over_exposed_ratio < over_exposed_ratio_par) {

      exposure = 0.;
      get_current_exposure (h_cam, exposure);

      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;

      if (over_exposed_ratio < 0.001) {
        right_exposure = 1.35 * exposure;
      }
      else {
        right_exposure = 1.05 * exposure; // increase the exposure by 5%
      }

      logger << "<<wavelength>> = "  << wavelength<< "\tUnder exposure logic <<<<<<< Setting exposure to " 
        << right_exposure << " at iteration " << iter << " >>>>>>>\n\n\n";

      set_exposure (h_cam, right_exposure);

      // giving some time for the new exposure to take effect. Is this needed? CHECK..
      sleep (3);

      exposure = 0.0;
      get_current_exposure (h_cam, exposure);
      logger << "Exposure after updating = " << exposure << endl;

      if (right_exposure > MAX_EXPOSURE) {
        right_exposure = MAX_EXPOSURE;
        logger << "Maximum exposure set .. Breaking ..\n";
        break;
      }

      if (capture_image (h_cam) == -1) {
        logger << "<<wavelength>> = "  << wavelength<< "\tCapture Image failed ..\n";
        throw std::exception ();
      }
      else {
        logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";
      }

      memcpy (pixels, image, height*width*2);

      iter++;
      continue;
    }
    else {
      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;
      break;
    }
  }


  // REDUCE OVER EXPOSURE AGAIN TO ENSURE THAT THE UNDER EXPOSRE LOGIC DOESN'T OVER SHOOT THE USER SPECIFIED PERCENTAGE

  while (true) {

    count = 0;
    maxpixval = 0;

    for (unsigned int i = 0; i < size; i++) {
      unsigned short pixval = (unsigned short) pixels[i];
      if (pixval > maxpixval) {
        maxpixval = pixval;
      }
      
      if (pixval > (MAX_INTENSITY-1)) {
        count++;
      }
    }

    over_exposed_ratio = count / (float) size;

    if (over_exposed_ratio < over_exposed_ratio_par) {
      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;
      break;
    }

    get_current_exposure (h_cam, exposure);

    logger << "<<wavelength>> = "  << wavelength<< "\tCurrent exposure = " << exposure << endl;
    logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;

    if (over_exposed_ratio < 0.1) { // reduce the exposure by 10% for faster convergence
      exposure = 0.9 * exposure;
    }
    else {
      exposure = (1.0 - over_exposed_ratio) * exposure;
    }
    right_exposure = exposure;

    logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure logic <<<<<<< Setting exposure to " 
      << exposure << " at iteration " << iter << " >>>>>>>\n\n\n";

    set_exposure (h_cam, exposure);

    // giving some time for the new exposure to take effect. Is this needed? CHECK..
    sleep (3);

    if (capture_image (h_cam) == -1) {
      logger << "<<wavelength>> = "  << wavelength<< "\tCapture Image failed ..\n";
      throw std::exception ();
    }
    else {
      logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";
    }

    memcpy (pixels, image, height*width*2);
    iter++;
  }

  logger << "<<wavelength>> = " << wavelength << "\t**** Found the right exposure for wavelength " << wavelength
    <<" .. expsoure = " << right_exposure << ", maxpixval count = " << count << ", over exposure ratio = " << over_exposed_ratio <<  " ... Breaking ..****\n";
}

UINT get_current_exposure (HIDS h_cam, double& curr_exp)
{
  UINT n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  return n_ret;
}

int set_exposure (UINT h_cam, double e)
{
  //double new_exp = e * 0.001;
  double new_exp = e;
  logger << "Setting exposure to " << new_exp << " ms\n";
  UINT n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_SET_EXPOSURE, 
      (void*)&new_exp, sizeof (new_exp));

  double curr_exp = 0.0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  logger << "Current exposure = " << curr_exp << " ms " << endl;
  return n_ret;
}

int get_pixel_clock_info (HIDS h_cam)
{
  logger << "--------- PIXEL CLOCK -------------\n";
  INT n_ret = 0;
  UINT pc_freq = 0;
  n_ret = is_PixelClock (h_cam, IS_PIXELCLOCK_CMD_GET, (void*)&pc_freq, sizeof (pc_freq));

  logger << "Pixel clock frequency = " << pc_freq << endl;

  logger << "------------------------------------\n\n";
  return n_ret;
}

int set_master_gain (HIDS h_cam, int gain)
{
  logger << "Setting Master gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Master gain failed ..\n";
    return n_ret;
  }

  logger << "Master gain = " << is_SetHardwareGain (h_cam, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
  return n_ret;
}

int set_red_gain (HIDS h_cam, int gain)
{
  logger << "Setting red gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Red gain failed ..\n";
    return n_ret;
  }

  logger << "Red gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_GET_RED_GAIN, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
  return n_ret;
}

int set_green_gain (HIDS h_cam, int gain)
{
  logger << "Setting green gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Green gain failed ..\n";
    return n_ret;
  }

  logger << "Green gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER) << endl;

  return n_ret;
}

int set_blue_gain (HIDS h_cam, int gain)
{
  logger << "Setting blue gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain);
  if (n_ret == -1) {
    logger << "Setting Blue gain failed ..\n";
    return n_ret;
  }

  logger << "Blue gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_GET_BLUE_GAIN) << endl;
  return n_ret;
}

int set_color_mode (HIDS h_cam, UINT color_mode)
{
  UINT n_ret = is_SetColorMode (h_cam, color_mode);

  if (n_ret == IS_SUCCESS) {
    logger << "SetColorMode () successful ..\n";
  }
  else {
    color_mode_error_msg (n_ret);
    logger << "SetColorMode () FAILED ..\n";
  }
  return n_ret;
}

int set_gamma (HIDS h_cam, int gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, gamma);

	if (n_ret != IS_SUCCESS) {
		logger << "Unable to set gamma to " << gamma << " ..\n";
		return -1;
	}	
}

int get_gamma (HIDS h_cam, int* gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, IS_GET_GAMMA);

	if (n_ret == IS_NO_SUCCESS) {
		logger << "Unable to get gamma ..\n";
		return -1;
	}
	*gamma = n_ret;

}

int get_fps (HIDS h_cam, double* fps)
{
  return is_GetFramesPerSecond (h_cam, fps);
}

int set_fps (HIDS h_cam, double fps, double* new_fps)
{
	return is_SetFrameRate (h_cam, fps, new_fps);
}

void print_gain_parameters (HIDS h_cam)
{
  logger << "------------ GAIN ------------\n";
  logger << "Master gain = " << is_SetHardwareGain (h_cam, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;

  logger << "Red gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_GET_RED_GAIN, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;

  logger << "Green gain = " << is_SetHardwareGain (h_cam,  IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER) << endl;

  logger << "Blue gain = " << is_SetHardwareGain (h_cam,  IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_GET_BLUE_GAIN) << endl;
  logger << "-----------------------------\n\n";
}

void color_mode_error_msg (UINT n_ret)
{
  switch (n_ret)
  {
    case IS_CANT_COMMUNICATE_WITH_DRIVER:
      logger << "Cannot communicate with driver ..\n";
      break;
    case IS_CANT_OPEN_DEVICE:
      logger << "Can't open device ..\n";
      break;
    case IS_CAPTURE_RUNNING:
      logger << "A capture operation is still in progress ..\n";
      break;
    case IS_INVALID_CAMERA_TYPE:
      logger << "Invalid camera type ..\n";
      break;
    case IS_INVALID_COLOR_FORMAT:
      logger << "Invalid color format ..\n";
      break;
    case IS_INVALID_CAMERA_HANDLE:
      logger << "Invalid camera handle ..\n";
      break;
    case IS_INVALID_MODE:
      logger << "Invalid mode ..\n";
      break;
    case IS_INVALID_PARAMETER:
      logger << "Invalid parameter ..\n";
      break;
    case IS_IO_REQUEST_FAILED:
      logger << "IO request failed ..\n";
      break;
    case IS_NO_IR_FILTER:
      logger << "No IR filter available ..\n";
      break;
    case IS_NO_SUCCESS:
      logger << "No success ..  error code = " << n_ret << endl;
      break;
    case IS_NOT_CALIBRATED:
      logger << "Not calibrated ..\n";
      break;
    case IS_NOT_SUPPORTED:
      logger << "This color mode is not supported in this camera model ..\n";
      break;
    case IS_NULL_POINTER:
      logger << "Invalid array ..\n";
      break;
    case IS_OUT_OF_MEMORY:
      logger << "Memory can't be allocated ..\n";
      break;
    case IS_TIMED_OUT:
      logger << "Timeout ..\n";
      break;
  }
}
