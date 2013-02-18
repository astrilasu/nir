#include "filterwheel.h"
#include <iostream>
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
