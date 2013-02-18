// serialPortComm.cpp : Defines the entry point for the console application.
//

#include "serial.h"

#include <iostream>
using namespace std;

static DCB dcb;


HANDLE open_port (wchar_t* port)
{
	wcout << "Opening port *" << port << "*\n";
	HANDLE h_port = CreateFile ((wchar_t*)port, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	return h_port;
}

int check_port_handle (HANDLE h_port)
{
	if (h_port == INVALID_HANDLE_VALUE) {
		cout << "Invalid handle..\n";

		if (GetLastError () == ERROR_FILE_NOT_FOUND) {
			cout << "File not found ..\n";
			return -1;
		}
	}
	return 0;
}
int close_port (HANDLE h_port)
{
	return CloseHandle (h_port);
}

int serial_write (HANDLE h_port, char* data)
{	
	cout << "Writing command " << data << " to the port ..\n";
	DWORD bytes_written = 0;
	int ret = WriteFile (h_port, data, 2, &bytes_written, NULL);	
	return ret;
}

int get_port_state (HANDLE h_port)
{
	int ret (0);
	if (!(ret = GetCommState (h_port, &dcb))) {
		cout << "Couldn't read comm port state ..\n";		
		cout << "Error code = " << ret << endl;
		return -1;
	}
	else {
		cout << "Comm Port state read ..\n";
	}
	return ret;
}
int configure_port (HANDLE h_port)
{	
	dcb.BaudRate = 9600;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState (h_port, &dcb)) {
		cout << "Unable to configure comm port ..\n";
		return -1;
	}
	else {
		cout << "Configured COMM port ..\n";
	}

	return 0;
}