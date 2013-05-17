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
  serial_send (fd_, (void*)"i2", 2);
  usleep (1000);
  char response[5] = "";
  int status = serial_recv (fd_, response, 2);
  cout << "<< Response = " << response << " >>\n";
  if (status == -1) {
    throw std::runtime_error ("Serial recv failed..");
  }
  response[2] = '\0';
  cout << "response = " << response << "\n";
  return ((int)response[1]-48);
}
