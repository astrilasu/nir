#pragma once

#include <string>
#include <fstream>
using namespace std;

#include <ueye.h>

#define MAX_INTENSITY 3700
#define MAX_EXPOSURE 1999

class CameraWrapper
{

  public:

    static void getCameraList ();

    CameraWrapper ();
    ~CameraWrapper ();

    void setupCamera ();
    void exitCamera ();

    void findAOI (int& width, int& height);
    int getHeight () { return height; }
    int getWidth () { return width; }

    void getSensorInfo (SENSORINFO* sensor_info);

    void setupImageMemory ();
    char* getImageMemory () { return image; }

    void captureImage ();
    void saveImage (wstring image_name, wstring type);
    void freeImageMemory ();

    void setDisplayMode (UINT mode);

    void setExposure (double e); // in milli seconds
    void getCurrentExposure (double& curr_exp);

    void getPixelClockInfo ();

    void setColorMode (UINT color_mode);

    void setFps (double fps, double& new_fps);
    void getFps (double& fps);

    void setGamma (int gamma);
    void getGamma (int& gamma);


    void setBitsPerPixel (int bits_pp) { this->bits_pp = bits_pp; }
    void setOverExposedRatio (float over_exposed_ratio) { over_exposed_ratio_par = over_exposed_ratio; }

    void setMasterGain (int gain);
    void setRedGain (int gain);
    void setGreenGain (int gain);
    void setBlueGain (int gain);


    void setParameter (double val, UINT option, string message);

    void findBestExposure (int wavelength, string time_str);

    void inquireImageMemory (int mem_id);

    

  protected:

    HIDS h_cam;
    int width;
    int height;

    char* image;
    int mem_id;
    int bits_pp;

    float over_exposed_ratio_par;

};
