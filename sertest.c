/*******************************************************/
/* Program to test serial port throughput              */
/*                                                     */
/* Copyright Tealdrones 2016                           */
/*                                                     */
/* Author: Dennis Millard                              */
/*         dennis@tealdrones.com                       */
/*                                                     */
/*******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <fcntl.h> // Required for open()
#include <unistd.h> // Required for read(), write(), close()
#include <termios.h> // Required for tcgetattr(), cfsetispeed(), cfsetospeed(), tcflush()
#include <errno.h> // Require for errno
#include <unistd.h> // Required for error codes, among other things
#include <poll.h> // Required for poll()
#include <sys/types.h>
#include <sys/stat.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define DEVLENGTH 128
#define BAUDLENGTH 32

#define RX 1
#define TX 2

static const char *optString = "vd:b:rts";

typedef struct
{
	int verbose;			/* enable verbose output */
	char devpath[DEVLENGTH];	/* -d serial device */
	int baudrate;			/* baudrate to use */
	int sermode;			/* serial mode RX or TX */
	int fd;				/* file descriptor */
        int single;                     /* single character mode */
} Settings_t;

/*************************************************************/
/* parse_args() - Parse command line arguments and set       */
/* values as necessary.                                      */
/* parameters: argc = argument count, argv = argument values */
/*************************************************************/
void parse_args(int argc, char *argv[], Settings_t *settings)
{
	int opt = 0;

	/* set default device path */
	strncpy(settings->devpath, "/dev/ttyUSB0", DEVLENGTH);
	
	/* set default baud rate */
	settings->baudrate = 9600;

        /* set single character mode to FALSE */
        settings->single = 0;

	opt = getopt( argc, argv, optString );
	while( opt != -1 ) {
		switch( opt ) {
			case 'v':
				// turn on verbose mode
				settings->verbose = 1;
				break;
			case 'd':
				// sets the device path
				strncpy(settings->devpath, optarg, DEVLENGTH);
				break;
			case 'b':
				// sets the baud rate
				settings->baudrate = atoi(optarg);
				break;
			case 'r':
				// sets to serial receive mode
				settings->sermode = RX;
				break;
			case 't':
				// sets to serial transmit mode
				settings->sermode = TX;
				break;
                        case 's':
                                settings->single = 1;
                                break;
			case 'h':
			case '?':
				printf("sertest version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
				printf("usage: ./%s [-v] [-d device] [-b baud] -t|-r\n", argv[0]);
				printf("  -v enables verbose mode\n");
				printf("  -d <devicename> sets the serial device\n");
				printf("  -b <baud> sets the baud rate\n");
				printf("  -t sets mode to transmit\n");
				printf("  -r sets mode to receive\n");
                                printf("  -s turns on single character mode\n");
				exit( EXIT_FAILURE );
				break;
			default:
				/* You won't actually get here. */
				break;
		}
		
		opt = getopt( argc, argv, optString );
	}

	if (settings->verbose) {
		printf("Arguments:\n");
		printf("  -v: %d\n", settings->verbose);
		printf("  -d: %s\n", settings->devpath);
		printf("  -b: %d\n", settings->baudrate);
		printf("  -r/t: %d\n", settings->sermode);
                printf("  -s: %d\n", settings->single);
	}
}

/*************************************************************/
/* openport() - opens the selected serial port and gets a    */
/* file descriptor                                           */
/*************************************************************/
int openport(Settings_t *settings)
{
    settings->fd = open(settings->devpath, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (settings->fd <= 0)
    {
        char errorString[0xFF];
        strerror_r(errno, errorString, sizeof(errorString));
        fprintf(stderr, "Open device failed: Unable to open device file %s. Error: %s\n", settings->devpath, errorString);
        return -1;
    }
    else
    {
	    if (settings->verbose)
	    {
		    fprintf(stderr, "Got file descriptor: %d\n", settings->fd);
	    }
    }

    // Reset the serial device file descriptor for non-blocking read / write
    fcntl(settings->fd, F_SETFL, 0);

    return 0;
}

/*************************************************************/
/* configureport() - configures the selected serial port     */
/*************************************************************/
int configureport(Settings_t *settings)
{
    // Modify the settings on the serial device (baud rate, 8n1, receiver enabled, ignore modem status, no flow control) and apply them
    struct termios deviceOptions;
    memset (&deviceOptions, 0, sizeof deviceOptions);
    tcgetattr(settings->fd, &deviceOptions);

    /* Set the baud rates... */
    speed_t MyBaud;
    switch (settings->baudrate)
    {
	case 9600:
		MyBaud = B9600;
		break;
	case 57600:
		MyBaud = B57600;
		break;
	case 115200:
		MyBaud = B115200;
		break;
	case 230400:
		MyBaud = B230400;
		break;
	case 460800:
		MyBaud = B460800;
		break;
	case 921600:
		MyBaud = B921600;
		break;
	case 1000000:
		MyBaud = B1000000;
		break;
	case 2000000:
		MyBaud = B2000000;
		break;
	case 3000000:
		MyBaud = B3000000;
		break;
	case 4000000:
		MyBaud = B4000000;
		break;
	default:
		MyBaud = B9600;
    }

    //cfsetispeed(&deviceOptions, settings->baudrate);
    //cfsetospeed(&deviceOptions, settings->baudrate);
    cfsetispeed(&deviceOptions, MyBaud);
    cfsetospeed(&deviceOptions, MyBaud);

    // Set character length
    deviceOptions.c_cflag &= ~CSIZE;       // Mask the character size bits
    deviceOptions.c_cflag |= CS8;

    // No parity
    deviceOptions.c_cflag &= ~PARENB;
    deviceOptions.c_cflag |= PARODD;

    // Stick parity flag
    deviceOptions.c_cflag |= CMSPAR;

    // ignores parity errors and passes bytes regardless
    deviceOptions.c_iflag |= (IGNPAR);

    //causes parity errors to be 'marked' in the input stream using special characters.
    //If IGNPAR is enabled, a NUL character (0x00) is sent to your program before every character with a parity error.
    //Otherwise, a DEL (0x7F) and NUL character is sent along with the bad character.
    deviceOptions.c_iflag |= (PARMRK);

    //enable checking and stripping of the parity bit
    //NOTE: PARMRK and INPCK are exclusive of each other
    //options.c_iflag |= (INPCK);
    //options.c_iflag |= (ISTRIP);

    // One stop bit
    deviceOptions.c_cflag &= ~CSTOPB;

    /* No hardware flow control */
    deviceOptions.c_cflag &= ~CRTSCTS;

    // Raw input, no echo
    deviceOptions.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | IEXTEN | ISIG);
    deviceOptions.c_iflag &=  ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY | INPCK );

    // Raw output
    deviceOptions.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
                ONOCR | OFILL | OLCUC | OPOST);

    // Wait timeout for each character
    deviceOptions.c_cc[VMIN] = 1;
    deviceOptions.c_cc[VTIME] = 0;

    /* Enable the receiver and set local mode... */
    deviceOptions.c_cflag |= (CLOCAL | CREAD);

    tcflush(settings->fd, TCIFLUSH);
    
    if (tcsetattr(settings->fd, TCSANOW, &deviceOptions) != 0)
    {
        close(settings->fd);
        char errorString[0xFF];
        strerror_r(errno, errorString, sizeof(errorString));
        fprintf(stderr, "Create failed: Unable to set options on device. Error: %s\n", errorString);
        return -1;
    }
    
    if (settings->verbose)
    {
    	printf("Configured serial device file descriptor for 8n1 and no flow control: %s\n", settings->devpath);
    }
    
    return 0;
}

/*************************************************************/
/* readBytes() - reads bytes from the selected serial port   */
/*************************************************************/
ssize_t readBytes(int deviceFileDescriptor, void* destination, ssize_t size)
{
    ssize_t bytesRead = 0;
    while (bytesRead < (ssize_t)size)
    {
        ssize_t result = read(deviceFileDescriptor, destination + bytesRead, size - bytesRead);
        
        if (result <= 0 && errno != EWOULDBLOCK)
        {
            char errorString[0xFF];
            strerror_r(errno, errorString, sizeof(errorString));
            fprintf(stderr, "Failed to read from device descriptor. Error: %s\n", errorString);
            return result;
        }
        
        if (result <= 0 && errno != EAGAIN)
        {
            char errorString[0xFF];
            strerror_r(errno, errorString, sizeof(errorString));
            fprintf(stderr, "Error: %s\n", errorString);
            return result;
        }
        
        bytesRead += result;
    }
    
    return bytesRead;
}

/*************************************************************/
/* writeBytes() - writes bytes to the selected serial port   */
/*************************************************************/
ssize_t writeBytes(int deviceFileDescriptor, const void* source, ssize_t size)
{
    ssize_t bytesWritten = 0;
    
    while (bytesWritten < size)
    {
        ssize_t result = write(deviceFileDescriptor, source + bytesWritten, size - bytesWritten);
        
        if (result <= 0 && errno != EWOULDBLOCK)
        {
            char errorString[0xFF];
            strerror_r(errno, errorString, sizeof(errorString));
            fprintf(stderr, "Failed to write to serial device descriptor. Error: %s\n", errorString);
            return result;
        }
        
        bytesWritten += result;
    }

    return bytesWritten;
}

/*************************************************************/
/* Main function                                             */
/*************************************************************/
int main(int argc, char *argv[])
{
	Settings_t MySettings;
	long rxcount = 0;
	long txcount = 0;

	/* parse the command line arguments */
	parse_args(argc, argv, &MySettings);

	/* open the port */
	if (openport(&MySettings) != 0)
	{
		return -1;
	}

	/* ---- RX mode ---- */
	if (MySettings.sermode == RX)
	{
		// Reset the serial device file descriptor for blocking read / write
		fcntl(MySettings.fd, F_SETFL, 0);

		if (configureport(&MySettings) != 0)
		{
			return -1;
		}

		// Begin reading
		char testchar;
                char expected_char;
                if (MySettings.single == 0) {
		    expected_char = 'A';
                } else {
                    expected_char = 'U';
                }

		fprintf(stderr, "Waiting for data...\n");

		while (MySettings.fd > 0)
		{
			// Wait until there is data to read or a timeout happens
			struct pollfd pollOptions;
			pollOptions.fd = MySettings.fd;
			pollOptions.events = POLLIN;
			pollOptions.revents = 0;
			poll(&pollOptions, 1, 200);
			ssize_t result = readBytes(MySettings.fd, &testchar, 1);

			if (result != 1)
			{
				if (errno != EWOULDBLOCK)
				{
					char errorString[0xFF];
					strerror_r(errno, errorString, sizeof(errorString));
					fprintf(stderr, "Error reading data (%s). Exiting...\n", errorString);
					break;
				}
			}
			
			rxcount++;
			
			if (testchar != expected_char)
			{
				fprintf(stderr, "Error - unexpected value: %c(0x%02x), should be: %c(0x%02x)\n", 
                                testchar, testchar, expected_char, expected_char);
			}
			
                        /* increment the character if not in single mode */
                        if (MySettings.single == 0) {
			    if (++expected_char > 'Z')
			    {
				// wrap around
				expected_char = 'A';
			    }
                        }

			// print status
			if (rxcount % 1000 == 0)
			{
				fprintf(stderr, "rx: %ld\n", rxcount);
			}
		}
	}

	/* ---- TX mode ---- */
	if (MySettings.sermode == TX)
	{
		// Reset the serial device file descriptor for non-blocking read / write
		fcntl(MySettings.fd, F_SETFL, O_NONBLOCK);

		if (configureport(&MySettings) != 0)
		{
			return -1;
		}

		// Begin writing
		char testchar;
                if (MySettings.single == 0) {
                  testchar = 'A';
                } else {
                  testchar = 'U';
                }

		while (MySettings.fd > 0)
		{
			// Wait until port is writeable
			struct pollfd pollOptions;
			pollOptions.fd = MySettings.fd;
			pollOptions.events = POLLOUT;
			pollOptions.revents = 0;
			poll(&pollOptions, 1, 200);

			// write the data
			ssize_t result = writeBytes(MySettings.fd, &testchar, 1);
			
			if (result != 1) {
				if (errno != EWOULDBLOCK) {
					char errorString[0xFF];
					strerror_r(errno, errorString, sizeof(errorString));
					fprintf(stderr, "Error writing data (%s). Exiting...\n", errorString);
					break;
				}
			}
			
			txcount++;
			
                        /* increment the character if not in single mode */
                        if (MySettings.single == 0) {
			    if (++testchar > 'Z') {
				// wrap around
				testchar = 'A';
			    }
			    //usleep(50);

			    // print status
                        }

			if (txcount % 1000 == 0)
			{
				fprintf(stderr, "tx: %ld\n", txcount);
			}
		}
	}

	fprintf(stderr, "ERROR- you must select a mode with -r or -t\n");
	return 0;
}

