#include <iostream>
#include <sstream>
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


  char commands[5][3] = { "g1", "g2", "g3", "g4", "g5"};
  for (int i=0; i<5;i++) {
    int status = serial_send (fd, commands[i], 2);
    cout << "Command = " << commands[i] << " Write status = " << status << endl;

    sleep (6);


    char response[5] = "";

    for (int k=0; k<15; k++) {
      char* command = (char*)"i2";
      //sleep (3);
      status = serial_send (fd, command, 2);
      cout << "Write status = " << status << endl;

      usleep (1000 * 50); // sleep for 50 milliseconds

      //int serial_recv(int fd, void *response, int length);

      strcpy (response, "");
      status = serial_recv (fd, response, 2);
      response[2] = '\0';
      cout << "response = *" << response << "*\n";

      if (response[0] != 'P' && 
          (response[1] != '1' || response[1] != '2' || response[1] != '3'
           || response[1] != '4' || response[1] != '5')) {
        cout << "k = " << k << " No response.. Trying again ..\n";
      }
      else {
        cout << "k = " << k << " ***Found response .. Breaking ****..\n";
        break;
      }
    }
    cout << "-------------------------------------------------\n\n";
  }
  return 0;
}
