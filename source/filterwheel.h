#pragma once

#include <cstdio>
#include <cassert>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
using namespace std;

#include <serial.h>

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
      //setId (1);
#ifndef WIN32
	  setPort ("/dev/ttyUSB0");
#else
      //setPort (L"COM3");
#endif
	  //cout << "In constructor ..\n";
      //setBaudRate (9600);
      //openPort ();
    }

#ifndef WIN32
    FilterWheel (int id, string port, int baud_rate)
#else
	FilterWheel (int id, wchar_t* port, int baud_rate)
#endif	
    {
      setId (id);
      setPort (port);
      setBaudRate (baud_rate);
      openPort ();
    }

    void openPort ()
    {
#ifndef WIN32
		fd_ = serial_setup ((char *) port_.c_str (), baud_rate_);
		assert (fd_ != -1);
#else		
		h_port_ = open_port (port_);

		assert (check_port_handle (h_port_) != -1);
		assert (get_port_state (h_port_) != -1);
		assert (configure_port (h_port_) != -1);	

		//if (check_port_handle (h_port_) != -1) { return; }
		//if (get_port_state (h_port_) != -1) { return; }
		//if (configure_port (h_port_) != -1) { return; }

#endif
    }

    ~FilterWheel ()
    {
    }

#ifndef WIN32
    inline string getPort () const { return port_; }
#else
	inline wchar_t* getPort () const { return port_; }
#endif
    inline int getBaudRate () const { return baud_rate_; }

#ifndef WIN32
    inline int getFd () const { return fd_; }
#endif

    inline void setId (int id) { id_ = id; }
#ifndef WIN32
    inline void setPort (string port) { port_ = port; }
#else
	inline void setPort (wchar_t* port) { port_ = port; }
#endif
    inline void setBaudRate (int baud_rate) { baud_rate_ = baud_rate; }
    inline void setFilterWheelMap (map <int, int>& filters) 
    { 
      this->filters_ = filters;
    }

    inline map <int, int>& getFilterWheelMap () { return filters_; }

    void gotoFilter (int filter);
    void gotoFilterById (int id);

  private:

#ifndef WIN32
	string port_;
#else
    wchar_t* port_;
#endif
    int baud_rate_;
    int id_;
    map <int, int> filters_;

#ifndef WIN32
	int fd_;
#else
	HANDLE h_port_;
#endif
};
