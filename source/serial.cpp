#include "serial.h"

/*------------------------------------------------------------------------------
 * int serial_recv()
 * Get data from the serial port.
 *----------------------------------------------------------------------------*/

int serial_recv(int fd, void *response, int length)
{
    // Declare variables.
    int status = -1;
    int port_count = 0;

    struct timeval timeout;
    fd_set serial_fds;

    timeout.tv_sec = SERIAL_TIMEOUT_SECS;
    timeout.tv_usec = SERIAL_TIMEOUT_USECS;

    FD_ZERO(&serial_fds);
    FD_SET(fd, &serial_fds);

    port_count = select(SERIAL_MAX_PORTS, &serial_fds, NULL, NULL, &timeout);

    if ((port_count == 0) || (!FD_ISSET(fd, &serial_fds)))
    {
        status = -2;
    }

    status = read(fd, response, length);

    if (status == -1)
    {
        perror("read");
    }

    sleep (1);
    tcflush (fd, TCIOFLUSH);

    return status;
} // end serial_recv()


/*------------------------------------------------------------------------------
 * int serial_send()
 * Writes commands to serial port.
 *----------------------------------------------------------------------------*/

int serial_send(int fd, void *command, int length)
{
    // Declare variables.
    int status = 0;
    status = write(fd, command, length);
    
    
    // ********* Note to AM to revisit the following line. -AM
    tcdrain(fd);

    if (status < 0)
    {
        perror("write");
    }

    return status;
} // end serial_send()


/*------------------------------------------------------------------------------
 * int serial_setup()
 * Opens a serial port.
 *----------------------------------------------------------------------------*/

int serial_setup(char *port_name, int baud)
{
    // Declare variables.
    int fd = -1;
    int status = -1;
    struct termios options;
   
    // Open port.
    fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 1)
    {
        perror("open");
        return fd;
    }

	// Get the current options for the port.
	tcgetattr(fd, &options);

	// Set the baud rate.
    switch (baud)
    {
    case 1200:
        cfsetispeed(&options, B1200);
        cfsetospeed(&options, B1200);
        break;

    case 2400:
        cfsetispeed(&options, B2400);
        cfsetospeed(&options, B2400);
        break;

    case 4800:
        cfsetispeed(&options, B4800);
        cfsetospeed(&options, B4800);
        break;

    case 9600:
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        break;

    case 19200:
        cfsetispeed(&options, B19200);
        cfsetospeed(&options, B19200);
        break;

    case 38400:
        cfsetispeed(&options, B38400);
        cfsetospeed(&options, B38400);
        break;

    case 57600:
        cfsetispeed(&options, B57600);
        cfsetospeed(&options, B57600);
        break;

    case 115200:
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        break;

    case 230400:
        cfsetispeed(&options, B230400);
        cfsetospeed(&options, B230400);
        break;

    default: //Bad baud rate passed.
        close(fd);
        return -2;
    }

	// Set the number of data bits.
	options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

	// Set the number of stop bits to 1.
	options.c_cflag &= ~CSTOPB;
	
	// Set parity to none.
	options.c_cflag &=~PARENB;
	
	// Set for non-canonical (raw processing, no echo, etc.).
   	options.c_iflag = IGNPAR; // ignore parity check
   	options.c_oflag = 0; // raw output
	options.c_lflag = 0; // raw input

	// Time-Outs -- won't work with NDELAY option in the call to open.
	options.c_cc[VMIN]  = 0;  // Block reading until RX x characers. If x = 0, it is non-blocking.
	options.c_cc[VTIME] = 10; // Inter-Character Timer -- i.e. timeout= x*.1 s.

	// Set local mode and enable the receiver.
	options.c_cflag |= (CLOCAL | CREAD);

	// Purge serial port buffers.
    tcflush(fd, TCIFLUSH);

	// Set the new options for the port.
	status = tcsetattr(fd, TCSANOW, &options);
	if (status != 0)
	{
        fprintf(stderr, "%s(). Failed to set up options.\n", __FUNCTION__);
    	return status;
    }

    return fd;
} // end serial_setup()


/*------------------------------------------------------------------------------
 * int serial_bytes_available()
 * Checks to see how many bytes are available to read on the serial stack.
 *----------------------------------------------------------------------------*/

int serial_bytes_available(int fd)
{
    // Declare variables.
    int bytes_available;
    int status = 0;

    status = ioctl(fd, FIONREAD, &bytes_available);
    if (status == -1)
    {
        perror("ioctl");
    }

    return bytes_available;
} // end serial_bytes_available()
