// com.cpp. hardware port communication module
// Do whatever you want with it.

#include "tu_serial.h"

namespace bakeinflash
{

	serial::serial() :
		m_fd(INVALID_HANDLE_VALUE)
	{
	}

	bool serial::open(
		const tu_string& port,
		Uint32 flags,
		Uint32 rate,
		int bits)
	{
		close();

#ifdef WIN32
		int bPortReady; 
		DCB dcb;
		COMMTIMEOUTS CommTimeouts;
		//char s[20];

		//_snprintf(s, 10, "%s\0", port.c_str());
		//_snprintf(s, 10, "\\\\.\\%s\0", port.c_str());
		m_fd = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE,
			0,						// exclusive access
			NULL,					// no security
			OPEN_EXISTING,
			0,						// no overlapped I/O
			NULL);					// null template 

		if (m_fd != INVALID_HANDLE_VALUE)
		{
			bPortReady = SetupComm((void*) m_fd, 128, 128); // set buffer sizes
			bPortReady = GetCommState((void*) m_fd, &dcb);
			dcb.BaudRate = rate;
			dcb.ByteSize = bits;
			dcb.Parity = flags;			// EVENPARITY, NOPARITY etc.
			dcb.StopBits = ONESTOPBIT;
			dcb.fAbortOnError = FALSE;      
			bPortReady = SetCommState((void*) m_fd, &dcb);

			bPortReady = GetCommTimeouts ((void*) m_fd, &CommTimeouts);
			CommTimeouts.ReadIntervalTimeout = 0;			// 50;
			CommTimeouts.ReadTotalTimeoutConstant = 1;		//50;
			CommTimeouts.ReadTotalTimeoutMultiplier = 0;	//10;
			CommTimeouts.WriteTotalTimeoutConstant = 50;
			CommTimeouts.WriteTotalTimeoutMultiplier = 10;
			bPortReady = SetCommTimeouts ((void*) m_fd, &CommTimeouts);
		}

#else

		m_fd = ::open(port, O_RDWR | O_NOCTTY | O_SYNC);
		if (m_fd >= 0)
		{
			struct termios newtio;
			memset(&newtio, 0, sizeof(newtio));

			newtio.c_cflag = flags;
			newtio.c_iflag = 0;
			newtio.c_oflag = 0;
			newtio.c_lflag = 0;
			newtio.c_cc[VMIN] = 0;		// no input queue
			newtio.c_cc[VTIME] = 0;		//wait_input ? 1 : 0; // timeout
			tcflush(m_fd, TCIOFLUSH);
			tcsetattr(m_fd, TCSANOW, &newtio);
		}

#endif

		bool rc = is_open();
		if (rc == false)
		{
			printf("could not open serial port on %s\n", port.c_str());
		}
		else
		{
			printf("using serial port on %s\n", port.c_str());
		}

		return rc;
	}

	serial::~serial()
	{
		close();
	}

	void serial::close()
	{
		if (m_fd != INVALID_HANDLE_VALUE)
		{
#ifdef WIN32
			CloseHandle((void*) m_fd);
#else
			tcflush(m_fd, TCIOFLUSH);
			::close(m_fd);
#endif
			m_fd = INVALID_HANDLE_VALUE;
		}
	}

	int serial::read(Uint8* buf, int len)
	{
		if (m_fd != INVALID_HANDLE_VALUE)
		{
#ifdef WIN32
			DWORD n;
			ReadFile((void*) m_fd, buf, len, &n, NULL);
			return n;
#else
			return ::read(m_fd, buf, len);
#endif
		}
		return 0;
	}

	int serial::write(Uint8* buf, int len)
	{
		if (m_fd != INVALID_HANDLE_VALUE)
		{
#ifdef WIN32
			DWORD n = 0;	//zeroing??
			WriteFile((void*) m_fd, buf, len, &n, NULL);
			return n;
#else
			return ::write(m_fd, buf, len);  
			//			tcflush(m_fd, TCIOFLUSH);
#endif
		}
		return 0;
	}
}
