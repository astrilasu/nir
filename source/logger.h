#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
using namespace std;

class Logger
{
  protected:

  public:

    Logger ();

    Logger (const char* filename);

    Logger (string& filename);

    ~Logger ();

    void open (string filename);


    // this is the type of std::cout
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;

    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);

    // define an operator<< to take in std::endl
    Logger& operator << (StandardEndLine manip);

    
    Logger& operator << (const char* str);
    Logger& operator << (wchar_t* str);
    Logger& operator << (string str);
    Logger& operator << (char c);
    Logger& operator << (int n);
    Logger& operator << (unsigned int n);
    Logger& operator << (double d);


  private:

    ofstream ofile;
    string filename;
};
