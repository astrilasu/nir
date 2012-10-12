#include "serial.h"
#include <stdio.h>
#include <memory.h>

int get_baud (int baud_rate)
{
  int baudr = 0;
  switch (baud_rate) {
    case      50 : baudr = B50;
                   break;
    case      75 : baudr = B75;
                   break;
    case     110 : baudr = B110;
                   break;
    case     134 : baudr = B134;
                   break;
    case     150 : baudr = B150;
                   break;
    case     200 : baudr = B200;
                   break;
    case     300 : baudr = B300;
                   break;
    case     600 : baudr = B600;
                   break;
    case    1200 : baudr = B1200;
                   break;
    case    1800 : baudr = B1800;
                   break;
    case    2400 : baudr = B2400;
                   break;
    case    4800 : baudr = B4800;
                   break;
    case    9600 : baudr = B9600;
                   break;
    case   19200 : baudr = B19200;
                   break;
    case   38400 : baudr = B38400;
                   break;
    case   57600 : baudr = B57600;
                   break;
    case  115200 : baudr = B115200;
                   break;
    case  230400 : baudr = B230400;
                   break;
    case  460800 : baudr = B460800;
                   break;
    case  500000 : baudr = B500000;
                   break;
    case  576000 : baudr = B576000;
                   break;
    case  921600 : baudr = B921600;
                   break;
    case 1000000 : baudr = B1000000;
                   break;
    default      : fprintf (stderr, "Invalid baud rate %d \n", baud_rate);
                   return -1;
  }
  return baudr;
}
int open_port (char* port, int baud_rate)
{
  int status = 0;
  int fd = 0;

  baud_rate = get_baud (baud_rate);
  if (baud_rate == -1) {
    return -1;
  }

  fd = open (port, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    fprintf (stderr, "Unable to open serial port - %s ..\n", port);
    return fd;
  }
  else {
    fprintf (stderr, "Opened serial port - %s ..\n", port);
  }

  struct termios new_port_settings;
  struct termios old_port_settings;

  status = tcgetattr (fd, &old_port_settings);
  if (status == -1) {
    close_port (fd);
    fprintf(stderr, "Unable to read port settings for %s.\n", port);
    return status;
  }

  memset (&new_port_settings, 0, sizeof (new_port_settings));  // clear the new struct 

  new_port_settings.c_cflag = baud_rate |   // specified baud rate 
                              CS8 |         //  Number of bits per byte
                              CREAD;        //  Enable reciever, otherwise no characters will be recieved

  new_port_settings.c_iflag = IGNPAR;       // Ignore bits with parity error
  new_port_settings.c_oflag = 0;            // Output attributes
  new_port_settings.c_lflag = 0;            // Controls various functions like echo, signal, flush etc
  new_port_settings.c_cc[VMIN] = 0;         // block untill n bytes are received 
  new_port_settings.c_cc[VTIME] = 0;        // block untill a timer expires (n * 100 mSec.) 

  status = tcsetattr (fd, TCSANOW, &new_port_settings);

  //fcntl (fd, F_SETFL, 0);

  if (status == -1) {
    close_port (fd);
    fprintf (stderr, "Unable to adjust port settings for %s.\n", port);
    return status;
  }
  return fd;
}

int write_to_port (int fd, char* buffer)
{
  int bytes = write (fd, buffer, strlen (buffer));
  if (bytes < 0) {
    fprintf (stderr, "Writing %d bytes of buffer failed.\n", strlen (buffer));
  }
  return bytes;
}

void close_port (int fd)
{
  close (fd);
}
