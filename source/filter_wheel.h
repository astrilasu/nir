#pragma once

#include <cstdio>
#include <cassert>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
using namespace std;

#include "serial.h"

enum Filter
{
  FILTER_NONE,
  FILTER_450,
  FILTER_532,
  FILTER_600,
  FILTER_670
};

class FilterWheel
{
  public:

    FilterWheel ()
    {
      setId (1);
      setPort ("/dev/ttyUSB0");
      setBaudRate (9600);
      openPort ();
    }

    FilterWheel (int id, string port, int baud_rate)
    {
      setId (id);
      setPort (port);
      setBaudRate (baud_rate);
      openPort ();
    }

    void openPort ()
    {
      fd_ = serial_setup ((char *) port_.c_str (), baud_rate_);
      assert (fd_ != -1);
    }

    ~FilterWheel ()
    {
    }

    inline string getPort () const { return port_; }
    inline int getBaudRate () const { return baud_rate_; }
    inline int getFd () const { return fd_; }

    inline void setId (int id) { id_ = id; }
    inline void setPort (string port) { port_ = port; }
    inline void setBaudRate (int baud_rate) { baud_rate_ = baud_rate; }
    inline void setFilterWheelMap (map <int, int>& filters) 
    { 
      this->filters_ = filters;
    }

    inline map <int, int>& getFilterWheelMap () { return filters_; }

    void gotoFilter (int filter);
    void gotoFilterById (int id);

  private:

    string port_;
    int baud_rate_;
    int fd_;
    int id_;
    map <int, int> filters_;
};
