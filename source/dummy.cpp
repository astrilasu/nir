#include "logger.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

extern Logger logger;

void func ()
{
  mkdir ("mydir", 0777);
  logger.open ("mydir/test_logger.txt");
  logger << "hello world.. "<< endl;
}
