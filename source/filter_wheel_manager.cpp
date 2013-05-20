#include "filter_wheel_manager.h"
#include "logger.h"

#include <iostream>
#include <map>
#include <cassert>
#include <stdexcept>
using namespace std;

extern Logger logger;

void FilterWheelManager::verifyPosition (int pos1, int pos2)
{
  int cur_pos = fw_[0]->getCurrentPosition ();
  logger << "FIRST  FILTER WHEEL,\tcurrent position = " << cur_pos <<  "\t";
  logger << "expected position = " << pos1 <<  "\n";
  if (cur_pos != pos1) {
    throw std::runtime_error ("Filter wheel not in the right position ..\n");
  }

  cur_pos = fw_[1]->getCurrentPosition ();
  logger << "SECOND FILTER WHEEL,\tcurrent position = " << cur_pos <<  "\t";
  logger << "expected position = " << pos2 <<  "\n";
  if (cur_pos != pos2) {
    throw std::runtime_error ("Filter wheel not in the right position ..\n");
  }
}

void FilterWheelManager::gotoFilter (int filter)
{
  cout << "Going to filter " << filter << endl << endl;

  if (filter == 999) {
    fw_[0]->gotoFilter (999);
    fw_[1]->gotoFilter (999);
    sleep (6);
    verifyPosition (1, 1);
    return;
  }


  char id1 = -1;
  map <int, int>& filters1 = fw_[0]->getFilterWheelMap ();
  cout << "Size of filters1 = " << filters1.size () << endl;
  map <int, int>::iterator iter = filters1.begin ();
  while (iter != filters1.end ()) {
    if (iter->second == filter) {
      id1 = iter->first;
      break;
    }
    iter++;
  }

  char id2 = -1;
  map <int, int>& filters2 = fw_[1]->getFilterWheelMap ();
  cout << "Size of filters2 = " << filters2.size () << endl;
  iter = filters2.begin ();
  while (iter != filters2.end ()) {
    if (iter->second == filter) {
      id2 = iter->first;
      break;
    }
    iter++;
  }

  assert ( (id1 != -1 && id2 == -1)  || (id1 == -1 && id2 != -1) );

  if (id1 == -1) {
    fw_[0]->gotoFilter (999);
    fw_[1]->gotoFilterById (id2);
    sleep (6);
    verifyPosition (1, id2);
  }
  else {
    fw_[0]->gotoFilterById (id1);
    fw_[1]->gotoFilter (999);
    sleep (6);
    verifyPosition (id1, 1);
  }
}
