
#pragma once

#include "windows.h"

HANDLE open_port (wchar_t* port);
int check_port_handle (HANDLE h_port);
int close_port (HANDLE h_port);
int serial_write (HANDLE h_port, char* data);
int configure_port (HANDLE h_port);
int get_port_state (HANDLE h_port);