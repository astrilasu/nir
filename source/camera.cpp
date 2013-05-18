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

CameraWrapper::CameraWrapper ()
{
  h_cam = 0;
  width = height = -1;
  mem_id = -1;
  bits_pp = 12;
  image = NULL;
  over_exposed_ratio_par = 1.0;


  setupCamera ();

}


CameraWrapper::~CameraWrapper ()
{
  if (is_StopLiveVideo (h_cam, IS_FORCE_VIDEO_STOP) != IS_SUCCESS) {
    logger << "Unable to stop live video ..\n";
    throw std::exception ();
  }

  freeImageMemory ();
  exitCamera ();
}

void CameraWrapper::getCameraList ()
{
  int n_cams = 0;
  int n_ret = is_GetNumberOfCameras (&n_cams);
  if (n_ret == -1) {
    logger << "Failed to get the number of cameras ..\n";
    throw std::runtime_error ("Failed to get the number of cameras");
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
}

void CameraWrapper::setupCamera ()
{
  int n_ret = is_InitCamera (&h_cam, NULL);

  if (n_ret != IS_SUCCESS) {
    logger  << "InitCamera failed.. Uploading new firmware..\n";
    //Check if GigE uEye SE needs a new starter firmware
    if (n_ret == IS_STARTER_FW_UPLOAD_NEEDED) { 
      //Upload new starter firmware during initialization
      h_cam =  h_cam | IS_ALLOW_STARTER_FW_UPLOAD;
      n_ret = is_InitCamera (&h_cam, NULL);
    }
  }
  else {
    logger << "Camera " << h_cam << " initialized..\n";
  }

  if (n_ret == -1) {
    logger << "Failed to initialize camera again ..\n";
    throw std::runtime_error ("Failed to initialize camera ..\n");
  }
  else {
    logger << "Camera handle = " << h_cam << endl;
  }

  setDisplayMode (IS_SET_DM_MONO);
  setColorMode (IS_CM_MONO12);

  findAOI (this->width, this->height);
  setupImageMemory ();


  if (is_CaptureVideo (h_cam, IS_WAIT) != IS_SUCCESS) {
    logger << "Failed to start live mode ..\n";
    throw std::exception ();
  }
  else {
    logger << " Started live mode ..\n";
  }

  double val = 1.0;
  setParameter (val, IS_SET_ENABLE_AUTO_SHUTTER, "AUTO SHUTTER");

  val = 1.0;
  setParameter (val, IS_SET_ENABLE_AUTO_FRAMERATE, "AUTO FRAME RATE");

  val = 0.0;
  setParameter (val, IS_SET_ENABLE_AUTO_GAIN, "AUTO GAIN");

  setMasterGain (0);
}

void CameraWrapper::exitCamera ()
{
  int n_ret = is_ExitCamera (h_cam);
  if (n_ret == IS_NO_SUCCESS) {
    logger << "ExitCamera () for Camera " << h_cam << " failed..\n";
    throw std::runtime_error ("Exit camera failed...\n");
  }
  logger << "Exit camera succeeded ..\n";
}

void CameraWrapper::setParameter (double val, UINT option, string message)
{
  if (is_SetAutoParameter (h_cam, option, &val, NULL) != IS_SUCCESS) {
    logger << "Unable to set " << message << endl;
    throw std::exception ();
  }
  else {
    logger << "Setting " << message << " to " << val << endl;
  }
}

void CameraWrapper::findAOI (int& width, int& height)
{
  IS_RECT rect_aoi;

  int n_ret = is_AOI (h_cam, IS_AOI_IMAGE_GET_AOI, (void*)&rect_aoi, sizeof (rect_aoi));
  if (n_ret == -1) {
    throw std::runtime_error ("Unable to get AOI ..");
  }


  logger << "--- AOI params ---\n";
  logger << "x = " << rect_aoi.s32X << endl;
  logger << "y = " << rect_aoi.s32Y << endl;
  logger << "width = " << (width = rect_aoi.s32Width) << endl;
  logger << "height = " << (height = rect_aoi.s32Height) << endl;
  logger << "-------------------\n";


  this->width = width;
  this->height = height;
}

void CameraWrapper::getSensorInfo (SENSORINFO* sensor_info)
{
  int n_ret = is_GetSensorInfo (h_cam, sensor_info);

  if (n_ret != IS_SUCCESS) {
    std::runtime_error ("Unable to get sensor info ..");
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

}

void CameraWrapper::setupImageMemory ()
{
  int n_ret = is_AllocImageMem (h_cam, width, height, bits_pp, &image, &mem_id);
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
    throw std::runtime_error ("Image Memory allocation FAILED ..");
  }
  if ( (n_ret = is_SetImageMem (h_cam, image, mem_id)) == IS_SUCCESS) {
    logger << "SetImageMem () SUCCESS ..\n";
  }
  else {
    logger << "SetImageMem () FAILED ..\n";
    throw std::runtime_error ("SetImageMem () FAILED ..");
  }
}

void CameraWrapper::freeImageMemory ()
{
  int n_ret = 0;
  if ( (n_ret = is_FreeImageMem (h_cam, image, mem_id)) == -1) {
    logger << "Free Image memroy failed ..\n";
    std::runtime_error ("Free Image Memory failed ..");
  }	
  logger << "Free Image memory succeeded..\n";
}

void CameraWrapper::inquireImageMemory (int mem_id)
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

}

void CameraWrapper::captureImage ()
{
  //int n_ret = is_FreezeVideo (h_cam, IS_WAIT);
  int n_ret = is_FreezeVideo (h_cam, 2000);
  if (n_ret == -1) {
    logger << "FreezeVideo () FAILED ..\n";
    throw std::runtime_error ("Capture image failed..");
  }
  logger << "Image captured ..\n";
}

void CameraWrapper::saveImage ( wstring image_name, wstring type)
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
    throw std::runtime_error ("Save image failed ..\n");
  }
}

void CameraWrapper::setDisplayMode (UINT mode)
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
      throw std::runtime_error ("Set display mode failed");
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
}

void CameraWrapper::findBestExposure (int wavelength, string time_str) 
{
  // Giving sometime for the sensor to adjust exposure -- required for the camera's auto exposure API
  sleep (5);

  double exposure = 0.0;
  getCurrentExposure (exposure);
  logger << "<<wavelength>> = " << wavelength << "\tInitial exposure from auto exposure = " << exposure << endl;

  captureImage ();

  unsigned int size = width * height;
  unsigned short* pixels = new unsigned short [width * height];
  memcpy (pixels, image, height*width*2);

  double val = 0.;
  int count = 0;
  unsigned int iter = 1;

  getCurrentExposure (exposure);
  float right_exposure = exposure;

  bool over_exposure_adjust = false;

  unsigned short maxpixval = 0;
  float over_exposed_ratio  = 0.0;


  // disable auto exposure ..
  val = 0.;
  setParameter (val, IS_SET_ENABLE_AUTO_SHUTTER, "AUTO SHUTTER");
  logger << "<<wavelength>> = " << wavelength << "\tDisabled auto exposure after getting initial estimate from APIs\n";

  // disabling auto frame rate
  val = 0.;
  setParameter (val, IS_SET_ENABLE_AUTO_FRAMERATE, "AUTO FRAME RATE");
  logger << "<<wavelength>> = " << wavelength << "\tDisabled auto framerate after getting initial estimate from APIs\n";

  double fps = 0.5;
  double newfps = 0.0;

  // setting a frame rate of 0.5 to ensure that we can set maximum exposure (2 sec)
  setFps (fps, newfps);
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

    if (over_exposed_ratio > 0.5) {
      over_exposed_ratio = 0.5;
    }

    if (over_exposed_ratio < over_exposed_ratio_par) {
      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;
      break;
    }

    getCurrentExposure (exposure);

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

    setExposure (exposure);
    over_exposure_adjust = true;

    // giving some time for the new exposure to take effect. Is this needed? CHECK..
    sleep (3);

    captureImage ();
    logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";

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
      getCurrentExposure (exposure);

      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;

      if (over_exposed_ratio < 0.001) {
        right_exposure = 1.35 * exposure;
      }
      else {
        right_exposure = 1.05 * exposure; // increase the exposure by 5%
      }

      logger << "<<wavelength>> = "  << wavelength<< "\tUnder exposure logic <<<<<<< Setting exposure to " 
        << right_exposure << " at iteration " << iter << " >>>>>>>\n\n\n";

      setExposure (right_exposure);

      // giving some time for the new exposure to take effect. Is this needed? CHECK..
      sleep (3);

      exposure = 0.0;
      getCurrentExposure (exposure);
      logger << "Exposure after updating = " << exposure << endl;

      if (right_exposure > MAX_EXPOSURE) {
        right_exposure = MAX_EXPOSURE;
        logger << "Maximum exposure set .. Breaking ..\n";
        break;
      }

      captureImage ();
      logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";

      memcpy (pixels, image, height*width*2);

      iter++;
      continue;
    }
    else {
      logger << "<<wavelength>> = "  << wavelength<< "\tOver exposure ratio = " << over_exposed_ratio << endl;
      break;
    }
  }


  // REDUCE OVER EXPOSURE AGAIN TO ENSURE THAT THE UNDER EXPOSRE LOGIC 
  // DOESN'T OVER SHOOT THE USER SPECIFIED PERCENTAGE

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

    getCurrentExposure (exposure);

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

    setExposure (exposure);

    // giving some time for the new exposure to take effect. Is this needed? CHECK..
    sleep (3);

    captureImage ();
    logger << "<<wavelength>> = "  << wavelength<< "\tImage Captured ..\n";

    memcpy (pixels, image, height*width*2);
    iter++;
  }

  logger << "<<wavelength>> = " << wavelength << "\t**** Found the right exposure for wavelength " << wavelength
    <<" .. expsoure = " << right_exposure << ", maxpixval count = " << count << ", over exposure ratio = " << over_exposed_ratio <<  " ... Breaking ..****\n";
}

void CameraWrapper::getCurrentExposure (double& curr_exp)
{
  int n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  if (n_ret != IS_SUCCESS) {
    throw std::runtime_error ("Get current exposure failed");
  }
}

void CameraWrapper::setExposure (double e)
{
  //double new_exp = e * 0.001;
  double new_exp = e;
  logger << "Setting exposure to " << new_exp << " ms\n";
  int n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_SET_EXPOSURE, 
      (void*)&new_exp, sizeof (new_exp));

  if (n_ret != IS_SUCCESS) {
    throw std::runtime_error ("Unable to set exposure");
  }

  double curr_exp = 0.0;
  n_ret = is_Exposure (h_cam, IS_EXPOSURE_CMD_GET_EXPOSURE, 
      (void*)&curr_exp, sizeof (curr_exp));
  if (n_ret != IS_SUCCESS) {
    throw std::runtime_error ("Unable to get exposure");
  }
  logger << "Current exposure = " << curr_exp << " ms " << endl;
}

void CameraWrapper::getPixelClockInfo ()
{
  logger << "--------- PIXEL CLOCK -------------\n";
  int n_ret = 0;
  int pc_freq = 0;
  n_ret = is_PixelClock (h_cam, IS_PIXELCLOCK_CMD_GET, (void*)&pc_freq, sizeof (pc_freq));

  if (n_ret != IS_SUCCESS) {
    throw std::runtime_error ("Unable to get pixel clock info");
  }

  logger << "Pixel clock frequency = " << pc_freq << endl;
  logger << "------------------------------------\n\n";

}

void CameraWrapper::setMasterGain (int gain)
{
  logger << "Setting Master gain to " << gain << endl;
  int n_ret = is_SetHardwareGain (h_cam, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Master gain failed ..\n";
    throw std::runtime_error ("Unable to set master gain");
  }

  logger << "Master gain = " << is_SetHardwareGain (h_cam, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
}

void CameraWrapper::setRedGain (int gain)
{
  logger << "Setting red gain to " << gain << endl;
  int n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Red gain failed ..\n";
    throw std::runtime_error ("Unable to set red gain");
  }

  logger << "Red gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_GET_RED_GAIN, 
      IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) << endl;
}

void CameraWrapper::setGreenGain (int gain)
{
  logger << "Setting green gain to " << gain << endl;
  int n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain, IS_IGNORE_PARAMETER);
  if (n_ret == -1) {
    logger << "Setting Green gain failed ..\n";
    throw std::runtime_error ("Unable to set green gain");
  }

  logger << "Green gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER) << endl;
}

void CameraWrapper::setBlueGain (int gain)
{
  logger << "Setting blue gain to " << gain << endl;
  int n_ret = is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, gain);
  if (n_ret == -1) {
    logger << "Setting Blue gain failed ..\n";
    throw std::runtime_error ("Unable to set blue gain");
  }

  logger << "Blue gain = " << is_SetHardwareGain (h_cam, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, 
      IS_IGNORE_PARAMETER, IS_GET_BLUE_GAIN) << endl;
}

void CameraWrapper::setColorMode (UINT color_mode)
{
  int n_ret = is_SetColorMode (h_cam, color_mode);

  if (n_ret == IS_SUCCESS) {
    logger << "SetColorMode () successful ..\n";
  }
  else {
    logger << "SetColorMode () FAILED ..\n";
    throw std::runtime_error ("Unable to set color mode");
  }
}

void CameraWrapper::setGamma (int gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, gamma);

	if (n_ret != IS_SUCCESS) {
		logger << "Unable to set gamma to " << gamma << " ..\n";
    throw std::runtime_error ("Unable to set gamma");
	}	
}

void CameraWrapper::getGamma (int& gamma)
{
	int n_ret = 0;

	n_ret = is_SetGamma (h_cam, IS_GET_GAMMA);

	if (n_ret == IS_NO_SUCCESS) {
		logger << "Unable to get gamma ..\n";
    throw std::runtime_error ("Unable to get gamma");
	}
	gamma = n_ret;
}

void CameraWrapper::getFps (double& fps)
{
  double nfps = 0;
  int n_ret = is_GetFramesPerSecond (h_cam, &nfps);
  if (n_ret != IS_SUCCESS) {
    logger << "Unable to get fps\n";
    throw std::runtime_error ("Unable to get fps");
  }
  fps = nfps;
}

void CameraWrapper::setFps (double fps, double& new_fps)
{
  double nfps = 0.0;
	int n_ret = is_SetFrameRate (h_cam, fps, &nfps);
  if (n_ret != IS_SUCCESS) {
    logger << "Unable to set fps\n";
    throw std::runtime_error ("Unable to set fps");
  }
  new_fps = nfps;
}
