#include <iostream>
using namespace std;

#include "logger.h"
#include "config_parser.hpp"
#include "filterwheel.h"
#include "filter_wheel_manager.h"

Logger logger;

int main (int argc, char *argv[])
{
  if (argc < 3){
    cout << "Enter two arguments 1) config file 2) go to filter {450, 532, 671, 750, 850, 920, 1050} ..\n";
    return -1;
  }

  ConfigParser* parser = NULL;
  string inputfile = argv[1];

  try {
    parser = new ConfigParserDOM ();
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      cout << "Error Message : " << parser->getErrorMessage () << endl;
      return -1;
    }
  }
  catch (std::exception& e) {
    cout << e.what () << endl;
    return -1;
  }

  try {

    FilterWheel* f1 = new FilterWheel ();

    cout << "Setting up filter wheel 1..\n";
    f1->setId (1);
    f1->setFilterWheelMap (parser->getFilterWheelById (0));
#ifndef WIN32
    f1->setPort (parser->get_fw_port (0));
#else
    string str = parser->get_fw_port (0);
    wstring wstr;
    wstr.assign (str.begin (), str.end ());
    f1->setPort ((wchar_t*) wstr.c_str ());
#endif

    f1->setBaudRate (parser->get_fw_baud (0));
    f1->openPort ();

    cout << "Setting up filter wheel 2..\n";

    FilterWheel* f2 = new FilterWheel ();  
    f2->setId (2);
    f2->setFilterWheelMap (parser->getFilterWheelById (1));
    cout << "getFilterWheelById (1) = " << parser->getFilterWheelById (1).size () << endl;
#ifndef WIN32
    f2->setPort (parser->get_fw_port (1));
#else
    str = parser->get_fw_port (1);  
    wstr.assign (str.begin (), str.end ());
    f2->setPort ((wchar_t*) wstr.c_str ());
#endif
    //
    f2->setBaudRate (parser->get_fw_baud (0));
    f2->openPort ();

    FilterWheel* fw[2];
    fw[0] = f1;
    fw[1] = f2;  

    cout << "**** " << fw[0]->getFilterWheelMap ().size () << " " << fw[1]->getFilterWheelMap ().size () << endl;

    FilterWheelManager fw_man (fw);
    int filter = atoi (argv[2]);
    cout << "Input filter = " << filter << endl;
    fw_man.gotoFilter (filter);

  }
  catch (std::exception& e){
    cout << "Exception ..\n";
    cout << e.what () << endl;
  }

  //fw[0].gotoFilter (671);
  //fw[1].gotoFilter (800);

  return 0;
}
