#pragma once

#include "filterwheel.h"

class FilterWheelManager
{
  protected:

  public:

    FilterWheelManager ()
    {
    }
    
    FilterWheelManager (FilterWheel *fw[2])
    {
      fw_[0] = fw[0];
      fw_[1] = fw[1];
    }

    ~FilterWheelManager ()
    {
    }

    void gotoFilter (int filter);
    void verifyPosition (int pos1, int pos2);

  private:

    FilterWheel *fw_[2];
};
