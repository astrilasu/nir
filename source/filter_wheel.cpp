#include "filter_wheel.h"
#include <iostream>
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
  sprintf (command, "g%d\0", id);

  int status = serial_send (fd_, command, 2);
  cout << "Command = " << command << " :: Status = " << status << endl;
  
  assert (status != -1 && "               Writing to filter wheel port failed...");
}

void FilterWheel::gotoFilterById (int id)
{
  cout << "Filter id = " << (int)id << endl;
  assert (id != -1 && "                 Filter provided doesn't exist.. Please check..");

  char command[3];
  sprintf (command, "g%d\0", id);

  int status = serial_send (fd_, command, 2);
  cout << "Command = " << command << " :: Status = " << status << endl;
  
  assert (status != -1 && "               Writing to filter wheel port failed...");
}
