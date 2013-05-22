#include "filterwheel.h"
#include <iostream>
#include <stdexcept>
#include <cstdio>
using namespace std;

void FilterWheel::gotoFilter (int filter)
{
  char id = -1;

  map <int, int>::iterator iter = filters_.begin ();
  while (iter != filters_.end ()) {
    if (iter->second == filter) {
      id = iter->first;
      break;
    }
    iter++;
  }

  cout << "Filter id = " << (int)id << endl;
  assert (id != -1 && "                 Filter provided doesn't exist.. Please check..");

  char command[3];
#ifndef WIN32
  snprintf (command, 3, "g%d\0", id);
#else
  sprintf_s (command, 3, "g%d\0", id);
#endif

#ifndef WIN32
  int status = serial_send (fd_, command, 2);
#else
  int status = serial_write (h_port_, command);
#endif
  cout << "Command = " << command << " :: Status = " << status << endl;
  
  assert (status != -1 && "               Writing to filter wheel port failed...");
}

void FilterWheel::gotoFilterById (int id)
{
  cout << "Filter id = " << (int)id << endl;
  assert (id != -1 && "                 Filter provided doesn't exist.. Please check..");

  char command[3];
#ifndef WIN32
  snprintf (command, 3, "g%d\0", id);
#else
  sprintf_s (command, 3, "g%d\0", id);
#endif

#ifndef WIN32
  int status = serial_send (fd_, command, 2);
#else
  int status = serial_write (h_port_, command);
#endif

  cout << "Command = " << command << " :: Status = " << status << endl;
  
  assert (status != -1 && "               Writing to filter wheel port failed...");
}

int FilterWheel::getCurrentPosition ()
{
  char response[5] = "";
  int k = 0;
  for (k = 0; k < 15; k++) {
    char* command = (char*)"i2";
    int status = serial_send (fd_, command, 2);
    cout << "Write status = " << status << endl;

    usleep (1000 * 50); // sleep for 50 milliseconds

    strcpy (response, "");
    status = serial_recv (fd_, response, 2);
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

  if (k == 15) {
    throw std::runtime_error ("Get Current Position of Filter wheel failed..\n");
  }

  response[2] = '\0';
  cout << "response = " << response << "\n";
  return ((int)response[1]-48);
}
