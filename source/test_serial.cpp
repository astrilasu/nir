#include <iostream>
#include <cstdlib>
using namespace std;

#include "serial.h"


int main (int argc, char *argv[])
{
  if (argc != 3) {
    cout << "\nEnter the following arguments..\n"
            " (1) Port \n"
            " (2) Baud rate \n\n";
    cout << "Sample use::  ./test_serial /dev/ttyUSB0 9600\n\n";
    return -1;
  }
  char* port = argv[1];
  int baud_rate = atoi (argv[2]);

  int fd = serial_setup (port, baud_rate);
  if (fd == -1) {
    cout << "Cannot open port " << port << endl;
    return -1;
  }

  char* command = "i2";
  int status = serial_send (fd, command, 2);
  cout << "Write status = " << status << endl;
  
  usleep (1000 * 50); // sleep for 50 milliseconds

  //int serial_recv(int fd, void *response, int length);

  char response[5] = "";
  status = serial_recv (fd, response, 2);
  response[2] = '\0';
  cout << "response = " << response << "\n";

  return 0;
}
