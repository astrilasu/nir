#include <iostream>
using namespace std;

#include "config_parser.hpp"
#include "filter_wheel.h"
#include "filter_wheel_manager.h"

int main (int argc, char *argv[])
{
  ConfigParser* parser = NULL;
  string inputfile = "../source/config-new.xml";

  try {
    parser = new ConfigParserDOM ();
    parser->setInputfile (inputfile);
    if (parser->parse () == false) {
      cout << "Error Message : " << parser->getErrorMessage () << endl;
      return -1;
    }
  }
  catch (MyException& e) {
    cout << e.m_error_message << endl;
    return -1;
  }


  FilterWheel fw[2];

  cout << "Setting up filter wheel 1..\n";
  fw[0].setId (1);
  fw[0].setFilterWheelMap (parser->getFilterWheelById (0));
  fw[0].setPort (parser->get_fw_port (0));
  fw[0].setBaudRate (parser->get_fw_baud (0));
  fw[0].openPort ();

  cout << "Setting up filter wheel 2..\n";
  fw[1].setId (2);
  fw[1].setFilterWheelMap (parser->getFilterWheelById (1));
  fw[1].setPort (parser->get_fw_port (1));
  fw[1].setBaudRate (parser->get_fw_baud (1));
  fw[1].openPort ();


  FilterWheelManager fw_man (fw);
  fw_man.gotoFilter (450);

  //fw[0].gotoFilter (671);
  //fw[1].gotoFilter (800);

  return 0;
}
