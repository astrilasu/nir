#include "camera.h"

#include <iostream>
#include <cstdio>
using namespace std;


int get_camera_list ()
{
  int n_cams = 0;
  UINT n_ret = is_GetNumberOfCameras (&n_cams);
  if (n_ret == -1) {
    cout << "Failed to get the number of cameras ..\n";
    return -1;
  }

  cout << "Number of cameras = " << n_cams << endl;

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

int setup_camera ()
{
  HIDS h_cam = 0;
  INT n_ret = is_InitCamera (&h_cam, NULL);

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
    return n_ret;
  }

  SENSORINFO sensor_info;
  if (get_sensor_info (h_cam, &sensor_info) == -1) {
    return -1;
  }

  if (set_master_gain (h_cam, 1) == -1) {
    return -1;
  }
  if (set_red_gain (h_cam, 125) == -1) {
    return -1;
  }
  if (set_green_gain (h_cam, 125) == -1) {
    return -1;
  }
  if (set_blue_gain (h_cam, 125) == -1) {
    return -1;
  }

  //UINT color_mode = IS_CM_MONO12;
  UINT color_mode = IS_CM_MONO8;
  if (set_color_mode (h_cam, color_mode) == -1) {
    return -1;
  }

  return h_cam;
}

int exit_camera (HIDS h_cam)
{
  INT n_ret = is_ExitCamera (h_cam);
  if (n_ret == IS_NO_SUCCESS) {
    cout << "ExitCamera () for Camera " << h_cam << " failed..\n";
  }
  return n_ret;
}

int get_AOI (HIDS h_cam, int& width, int& height)
{
  IS_RECT rect_aoi;

  int n_ret = is_AOI (h_cam, IS_AOI_IMAGE_GET_AOI, (void*)&rect_aoi, sizeof (rect_aoi));
  cout << "--- AOI params ---\n";
  cout << "x = " << rect_aoi.s32X << endl;
  cout << "y = " << rect_aoi.s32Y << endl;
  cout << "width = " << (width = rect_aoi.s32Width) << endl;
  cout << "height = " << (height = rect_aoi.s32Height) << endl;
  cout << "-------------------\n";

  return n_ret;
}

int get_sensor_info (HIDS h_cam, SENSORINFO* sensor_info)
{
  INT n_ret = is_GetSensorInfo (h_cam, sensor_info);

  if (n_ret == -1) {
    cout << "GetSensorInfo FAILED ..\n";
    return n_ret;
  }

  cout << "\n--------- SENSOR INFO ----------\n";
  cout << "Sensor name = " << sensor_info->strSensorName << endl;
  char color_mode = sensor_info->nColorMode;

  switch (color_mode) {

    case IS_COLORMODE_BAYER:
      cout << "Bayer mode..\n";
      break;
    case IS_COLORMODE_MONOCHROME:
      cout << "Monochrome mode ..\n";
      break;
    case  IS_COLORMODE_CBYCRY:
      cout << "CBYCRY mode ..\n";
      break;
  }

  cout << "Max width = " << sensor_info->nMaxWidth << endl;
  cout << "Max height = " << sensor_info->nMaxHeight << endl;
  cout << "Pixel size = " << sensor_info->wPixelSize << endl;
  cout << "Global shutter = " << sensor_info->bGlobShutter << endl;
  cout << "Master Gain = " << sensor_info->bMasterGain << endl;
  cout << "Red Channel gain " << sensor_info->bRGain << endl;
  cout << "Green Channel gain " << sensor_info->bGGain << endl;
  cout << "Blue Channel gain " << sensor_info->bBGain << endl;

  cout << "--------------------------------\n\n";

  return n_ret;
}

int setup_image_memory (HIDS h_cam, int width, int height, int bits_pp, char** image, int* mem_id)
{
  int n_ret = is_AllocImageMem (h_cam, width, height, bits_pp, image, mem_id);
  if (n_ret == IS_SUCCESS) {
    cout << "Image Memory allocation success ..\n";
  }
  else {
    cout << "Image Memory allocation FAILED ..\n";

    switch (n_ret) {
      case IS_CANT_ADD_TO_SEQUENCE:
        cout << "Image memory is already included in the sequence and can't be added again..\n";
        break;
      case IS_INVALID_CAMERA_HANDLE:
        cout << "Invalid camera handle..\n";
        break;
      case IS_INVALID_PARAMETER:
        cout << "Invalid parameter passed .. width = " << width << " height = " 
          << height << " bits per pixel = " << bits_pp << endl;
        break;
      case IS_OUT_OF_MEMORY:
        cout << "Out of memory ..\n";
        break;
      case IS_SEQUENCE_BUF_ALREADY_LOCKED:
        cout << "Memory could not be locked. Pointer to the buffer is invalid ..\n";
        break;
    }		
    return n_ret;
  }
  if (n_ret = is_SetImageMem (h_cam, *image, *mem_id) == IS_SUCCESS) {
    cout << "SetImageMem () SUCCESS ..\n";
  }
  else {
    cout << "SetImageMem () FAILED ..\n";
  }
  return n_ret;
}

int print_flash_parameters (HIDS h_cam)
{
  IO_FLASH_PARAMS flash_par;
  int n_ret = is_IO (h_cam, IS_IO_CMD_FLASH_GET_PARAMS, (void*)&flash_par, sizeof (flash_par));
  if (n_ret == IS_SUCCESS) {
    cout << "Flash delay = " << flash_par.s32Delay << endl;
    cout << "Flash duration = " << flash_par.u32Duration << endl;
  }
  else {
    cout << "Getting Flash paramenters failed ..\n";
    return -1;
  }
  return n_ret;
}

int free_image_memory (HIDS h_cam, char* image, int mem_id)
{
  UINT n_ret = 0;
  if ( (n_ret = is_FreeImageMem (h_cam, image, mem_id)) == -1) {
    cout << "Free Image memroy failed ..\n";
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
	
	cout << "\n---------- Image Memory Status -----------\n";
	cout << "width = " << nX << endl;
	cout << "height = " << nY << endl;
	cout << "bits = " << nBits << endl;
	cout << "Pitch = " << nPitch << endl;

	cout << "\n------------------------------------------\n";

	return status;

}
int capture_image (HIDS h_cam)
{
  int n_ret = is_SetExternalTrigger (h_cam, IS_SET_TRIGGER_SOFTWARE);
  if (n_ret == -1) {
    cout << "SetExternalTrigger () FAILED ..\n";
    return n_ret;
  }

  UINT mode = IO_FLASH_MODE_TRIGGER_HI_ACTIVE;
  n_ret = is_IO (h_cam, IS_IO_CMD_FLASH_SET_MODE, (void*)&mode, sizeof (mode));
  if (n_ret == -1) {
    cout << "IO () FAILED ..\n";
    return n_ret;
  }

  n_ret = is_FreezeVideo (h_cam, IS_WAIT);
  if (n_ret == -1) {
    cout << "FreezeVideo () FAILED ..\n";
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
//		assert ("Unsupported file format used for saving image");
	}

  int n_ret = is_ImageFile (h_cam, IS_IMAGE_FILE_CMD_SAVE, (void*)&file_par, sizeof (file_par));

  if (n_ret == -1) {
    cout << "Save image failed ..\n";
    return -1;
  }

  cout << "Save image return value = " << n_ret << endl;
  return n_ret;
}

int set_display_mode (HIDS h_cam, UINT mode)
{
  int display_mode = is_SetDisplayMode (h_cam, mode);
  display_mode = is_SetDisplayMode (h_cam, IS_GET_DISPLAY_MODE);
  cout << "Display mode = " << display_mode << endl;
  switch (display_mode)
  {
    case IS_SUCCESS:
      cout << "Get Display Mode Success..\n";
      break;
    case IS_NO_SUCCESS:
      cout << "Set display mode failed..\n";
      break;
    case IS_GET_DISPLAY_MODE:
      cout << "Get display mode..\n";
      break;
    case IS_SET_DM_DIB:
      cout << "DM DIB mode..\n";
      break;
    case IS_SET_DM_MONO:
      cout << "DM mono mode..\n";
      break;
  }
  return display_mode;
}

int get_exposure_info (HIDS h_cam)
{
  cout << "----------- EXPOSURE --------------\n";
  double range[3];
  int n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_FINE_INCREMENT_RANGE, 
      (void*) range, sizeof (range));

  if (n_ret == IS_SUCCESS) {
    cout << "Expsoure range :: min = " << range[0] << " ms, max = "
      << range[1] << " ms, increment = " << range[2] << " ms" << endl;
  }
  else {
    cout << "Query exposure times failed ..\n";
    return -1;
  }

  UINT n_caps = 0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_CAPS, (void*)&n_caps, sizeof (n_caps));

  if (n_ret == IS_SUCCESS) {
    if (n_caps & IS_EXPOSURE_CAP_LONG_EXPOSURE) {
      cout << "Long exposure supported ..\n";
      n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_LONG_EXPOSURE_RANGE, 
          (void*) range, sizeof (range));

      if (n_ret == IS_SUCCESS) {
        cout << "Long expsoure range :: min = " << range[0] << " ms, max = "
          << range[1] << " ms, increment = " << range[2] << " ms" << endl;
      }
      else {
        cout << "Query long exposure times failed ..\n";
        return -1;
      }
    }
    else {
      cout << "Long exposure NOT supported ..\n";
    }
  }

  double curr_exp = 0.0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  cout << "Current exposure = " << curr_exp << " ms " << endl;


  cout << "---------------------------------\n";

  return n_ret;
}

int set_exposure (UINT h_cam, double e)
{
  double new_exp = e * 0.001;
  cout << "Setting exposure to " << new_exp << " ms\n";
  UINT n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_SET_EXPOSURE, 
      (void*)&new_exp, sizeof (new_exp));

  double curr_exp = 0.0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  cout << "Current exposure = " << curr_exp << " ms " << endl;
  return n_ret;
}

int get_pixel_clock_info (HIDS h_cam)
{
  cout << "--------- PIXEL CLOCK -------------\n";
  UINT n_pix_clocks = 0;
  INT n_ret = is_PixelClock(h_cam, IS_PIXELCLOCK_CMD_GET_NUMBER,
      (void*)&n_pix_clocks,
      sizeof(n_pix_clocks));

  cout << "Number of pixel clocks = " << n_pix_clocks << endl;

  UINT default_pix_clock = 0;
  n_ret = is_PixelClock(h_cam, IS_PIXELCLOCK_CMD_GET_DEFAULT,
      (void*)&default_pix_clock,
      sizeof(default_pix_clock));

  cout << "Default pixel clock = " << default_pix_clock << endl;

  UINT current_pix_clock = 0;
  n_ret = is_PixelClock(h_cam, IS_PIXELCLOCK_CMD_GET_DEFAULT,
      (void*)&current_pix_clock,
      sizeof(current_pix_clock));

  cout << "Current pixel clock = " << current_pix_clock << endl;

  cout << "------------------------------------\n\n";
  return n_ret;
}

int set_master_gain (HIDS h_cam, int gain)
{
  cout << "Setting Master gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    cout << "Setting Master gain failed ..\n";
    return n_ret;
  }

  cout << "Master gain = " << is_SetHardwareGain (h_cam, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
  return n_ret;
}

int set_red_gain (HIDS h_cam, int gain)
{
  cout << "Setting red gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    cout << "Setting Red gain failed ..\n";
    return n_ret;
  }

  cout << "Red gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_GET_RED_GAIN, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
  return n_ret;
}

int set_green_gain (HIDS h_cam, int gain)
{
  cout << "Setting green gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    cout << "Setting Green gain failed ..\n";
    return n_ret;
  }

  cout << "Green gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER) << endl;

  return n_ret;
}

int set_blue_gain (HIDS h_cam, int gain)
{
  cout << "Setting blue gain to " << gain << endl;
  UINT n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain);
  if (n_ret == -1) {
    cout << "Setting Blue gain failed ..\n";
    return n_ret;
  }

  cout << "Blue gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_GET_BLUE_GAIN) << endl;
  return n_ret;
}

int set_color_mode (HIDS h_cam, UINT color_mode)
{
  UINT n_ret = is_SetColorMode (h_cam, color_mode);

  if (n_ret == IS_SUCCESS) {
    cout << "SetColorMode () successful ..\n";
  }
  else {
    color_mode_error_msg (n_ret);
    cout << "SetColorMode () FAILED ..\n";
  }
  return n_ret;
}

int set_gamma (HIDS h_cam, int gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, gamma);

	if (n_ret != IS_SUCCESS) {
		cout << "Unable to set gamma to " << gamma << " ..\n";
		return -1;
	}	
}

int get_gamma (HIDS h_cam, int* gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, IS_GET_GAMMA);

	if (n_ret == IS_NO_SUCCESS) {
		cout << "Unable to get gamma ..\n";
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
  cout << "------------ GAIN ------------\n";
  cout << "Master gain = " << is_SetHardwareGain (h_cam, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;

  cout << "Red gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_GET_RED_GAIN, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;

  cout << "Green gain = " << is_SetHardwareGain (h_cam,  IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER) << endl;

  cout << "Blue gain = " << is_SetHardwareGain (h_cam,  IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_GET_BLUE_GAIN) << endl;
  cout << "-----------------------------\n\n";
}

void color_mode_error_msg (UINT n_ret)
{
  switch (n_ret)
  {
    case IS_CANT_COMMUNICATE_WITH_DRIVER:
      cout << "Cannot communicate with driver ..\n";
      break;
    case IS_CANT_OPEN_DEVICE:
      cout << "Can't open device ..\n";
      break;
    case IS_CAPTURE_RUNNING:
      cout << "A capture operation is still in progress ..\n";
      break;
    case IS_INVALID_CAMERA_TYPE:
      cout << "Invalid camera type ..\n";
      break;
    case IS_INVALID_COLOR_FORMAT:
      cout << "Invalid color format ..\n";
      break;
    case IS_INVALID_CAMERA_HANDLE:
      cout << "Invalid camera handle ..\n";
      break;
    case IS_INVALID_MODE:
      cout << "Invalid mode ..\n";
      break;
    case IS_INVALID_PARAMETER:
      cout << "Invalid parameter ..\n";
      break;
    case IS_IO_REQUEST_FAILED:
      cout << "IO request failed ..\n";
      break;
    case IS_NO_IR_FILTER:
      cout << "No IR filter available ..\n";
      break;
    case IS_NO_SUCCESS:
      cout << "No success ..  error code = " << n_ret << endl;
      break;
    case IS_NOT_CALIBRATED:
      cout << "Not calibrated ..\n";
      break;
    case IS_NOT_SUPPORTED:
      cout << "This color mode is not supported in this camera model ..\n";
      break;
    case IS_NULL_POINTER:
      cout << "Invalid array ..\n";
      break;
    case IS_OUT_OF_MEMORY:
      cout << "Memory can't be allocated ..\n";
      break;
    case IS_TIMED_OUT:
      cout << "Timeout ..\n";
      break;
  }
}
