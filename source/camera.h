#pragma once

#include <string>
#include <fstream>
using namespace std;

#include <ueye.h>


#define MAX_INTENSITY 3700
#define MAX_EXPOSURE 1999

void get_system_time (string& time_str);

int get_camera_list ();
int setup_camera (HIDS* h_cam);
int exit_camera (HIDS h_cam);
int get_AOI (HIDS h_cam, int& width, int& height);
int get_sensor_info (HIDS h_cam, SENSORINFO* sensor_info);
int setup_image_memory (HIDS h_cam, int width, int height, int bits_pp, char** image, int* mem_id);
int print_flash_parameters (HIDS h_cam);
int capture_image (HIDS h_cam);
int save_image (HIDS h_cam, wstring image_name, wstring type);
int set_display_mode (HIDS h_cam, UINT mode);
int get_exposure_info (HIDS h_cam);
int set_exposure (UINT h_cam, double e); // in milli seconds
int get_pixel_clock_info (HIDS h_cam);
void print_gain_parameters (HIDS h_cam);
void color_mode_error_msg (UINT n_ret);
int set_color_mode (HIDS h_cam, UINT color_mode);
int set_fps (HIDS h_cam, double fps, double* new_fps);
int get_fps (HIDS h_cam, double* fps);
int set_gamma (HIDS h_cam, int gamma);
int get_gamma (HIDS h_cam, int* gamma);
int free_image_memory (HIDS h_cam, char* image, int mem_id);
int inquire_image_memory (HIDS h_cam, char* image, int mem_id);
UINT get_current_exposure (HIDS h_cam, double& curr_exp);
void find_best_exposure (HIDS h_cam, char* image, int width, int height, 
                          int wavelength, string time_str);

int set_master_gain (HIDS h_cam, int gain);
int set_red_gain (HIDS h_cam, int gain);
int set_green_gain (HIDS h_cam, int gain);
int set_blue_gain (HIDS h_cam, int gain);
