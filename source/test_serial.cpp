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

  int fd = open_port (port, baud_rate);
  if (fd == -1) {
    close_port (fd);
  }

  char* command = "F1";
  write_to_port (fd, command);

  return 0;
}

