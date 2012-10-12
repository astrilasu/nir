#ifndef SERIAL_H
#define SERIAL_H

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>


extern "C" {

int open_port (char* port, int baud_rate);
void close_port (int fd);
int write_to_port (int fd, char* buffer);

}

#endif
