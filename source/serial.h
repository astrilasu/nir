#ifndef SERIAL_H
#define SERIAL_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>


/** @name Maximum number of serial ports to check for available data. */
#ifndef SERIAL_MAX_PORTS
#define SERIAL_MAX_PORTS 10
#endif /* SERIAL_MAX_PORTS */

/** @name Timeout in [s] to check for available serial ports. */
#ifndef SERIAL_TIMEOUT_SECS
#define SERIAL_TIMEOUT_SECS 0
#endif /* SERIAL_TIMEOUT_SECS */

/** @name Timeout in [us] to check for available serial ports. */
#ifndef SERIAL_TIMEOUT_USECS
#define SERIAL_TIMEOUT_USECS 1 //20000
#endif /* SERIAL_TIMEOUT_USECS */

/** @name The maximum amount of serial data that can be stored. */
#ifndef SERIAL_MAX_DATA
#define SERIAL_MAX_DATA 65535
#endif /* SERIAL_MAX_DATA */

#ifndef SERIAL_EXTRA_DELAY_LENGTH
#define SERIAL_EXTRA_DELAY_LENGTH 40000
#endif /* SERIAL_EXTRA_DELAY_LENGTH */


/******************************
 *
 * Data types
 *
 *****************************/



/******************************
 *
 * Function prototypes
 *
 *****************************/

//! Opens a serial port. If SERIAL_DEBUG is defined then open system call
//! errors are printed to screen.
//! Returns a file descriptor for later use.
//! \param port_name String containing the name of the physical port.
//! \param baud Pointer to the desired baud rate.
//! \return The new file descriptor when successful, else -1.
int serial_setup(char *port_name, int baud);

//! Writes commands to serial port. If SERIAL_DEBUG is defined then write
//! system call errors are printed to screen.
//! Returns number of bytes sent.
//! \param fd A file descriptor for the serial port.
//! \param command Pointer to the message to send.
//! \param length Length of command.
//! \return Number of bytes written to serial port if successful, else -1.
int serial_send(int fd, void *command, int length);

//! Get data from the serial port. If SERIAL_DEBUG is defined then read system
//! call errors are printed to screen.
//! Returns number of bytes received.
//! \param fd File descriptor of serial port.
//! \param response Pointer to a buffer to hold response data.
//! \param length Expected length of message.
//! \return Number of bytes received when successful, else -1.
int serial_recv(int fd, void *response, int length);

//! Checks to see how many bytes are available to read on the serial stack. If
//! SERIAL_DEBUG is defined then the ioctl system call error is printed to the
//! screen.
//! Returns the number of bytes available.
//! \param fd A file descriptor for the port.
//! \return The number of bytes available in the serial buffer.
int serial_bytes_available(int fd);

#endif /* SERIAL_H */
