// com.h. hardware port communication module
// Do whatever you want with it.
#ifndef COM_H
#define COM_H

#ifdef WIN32
#include <windows.h>
#define DEFAULT_SERIAL_SETTINGS NOPARITY//EVENPARITY
#else
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#define DWORD int
#define HANDLE int
#define INVALID_HANDLE_VALUE -1
#define DEFAULT_SERIAL_SETTINGS B9600 | CS8 | CLOCAL | CREAD | PARENB
#endif

#include "base/container.h"
#include "bakeinflash/bakeinflash.h"

namespace bakeinflash
{

	struct serial : public ref_counted   
	{
		serial();
		virtual ~serial();

		bool open(const tu_string& port,
			Uint32 flags, // = DEFAULT_SERIAL_SETTINGS,
			Uint32 rate, // = 9600,
			int bits);

		void close();

		int read(Uint8* buf, int len);
		int write(Uint8* buf, int len);

		inline bool is_open() const
		{
#ifdef WIN32
			return m_fd != INVALID_HANDLE_VALUE;
#else
			return m_fd >= 0 ? true : false;
#endif
		}

	private:
		HANDLE m_fd;
	};

};

#endif
