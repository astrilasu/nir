#pragma once

#include <map>
using namespace std;

#include "filterwheel.h"

class FilterWheelManager
{
  protected:

  public:
    FilterWheelManager ()
    {
    }
    FilterWheelManager (FilterWheel* fw[2])
    {
      this->fw_[0] = fw[0];
	  this->fw_[1] = fw[1];
    }
    ~FilterWheelManager ()
    {
    }
    void gotoFilter (int filter)
    {
      char id1 = -1;
      map <int, int>& filters1 = fw_[0]->getFilterWheelMap ();
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
      }
      else {
        fw_[0]->gotoFilterById (id1);
        fw_[1]->gotoFilter (999);
      }
    }

  private:
    FilterWheel* fw_[2];
};
