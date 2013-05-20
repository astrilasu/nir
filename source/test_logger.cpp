#include <iostream>
using namespace std;


#include "logger.h"

extern void func ();
Logger logger;

int main (int argc, char *argv[])
{
  func ();
  return 0;
}
