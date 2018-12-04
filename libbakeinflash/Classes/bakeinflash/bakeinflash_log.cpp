// bakeinflash_log.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Helpers for logging messages & errors.


#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash.h"

#include <stdio.h>
#include <stdarg.h>

#if WINPHONE == 1
	#include "winphone.h"
#endif

namespace bakeinflash
{

	bool find(const char* s, const char* pattern)
	{
		int n = strlen(pattern);
		int k = strlen(s);
		for (int i = 0; i < k - n; i++)
		{
			if (strncmp(s + i, pattern, n) == 0)
			{
				return true;
			}
		}
		return false;
	}

	void standard_logger(const bool error, const char* msg)
	{
		myprintf("%s", msg);
	}

	// Function pointer to log callback.
	static void (*s_log_callback)(bool error, const char* message) = standard_logger;

	void	register_log_callback(void (*callback)(bool error, const char* message))
	// The host app can use this to install a function to receive log
	// & error messages from bakeinflash.
	//
	// Pass in NULL to inhibit logging of messages & errors.
	{
		s_log_callback = callback;
	}


#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif // _WIN32

	void	myprintf(const char* fmt, ...)
	// Printf-style informational log.
	{
		// Workspace for vsnprintf formatting.
		char*	buffer = NULL;
		int n = 0;
		for (int size = 4096; size <= 4096 * 32 && n <= 0; size *= 2)
		{
			free(buffer);
			buffer = (char*) malloc(size);

			va_list ap;				
			va_start(ap, fmt);			
			n = vsnprintf(buffer, size, fmt, ap);	
			va_end(ap);
		}

#if ANDROID == 1
		__android_log_print(ANDROID_LOG_FATAL, "bakeinflash", "%s",	buffer);
#elif WINPHONE == 1
		int len = strlen(buffer);
		wchar_t* buf =  (wchar_t*) malloc(len * 2 + 2);
		mbstowcs(buf, buffer, len + 1);
		OutputDebugString(buf);
		free(buf);
#else
		printf("%s", buffer);
#endif

		//if (find(s_buffer, "<!DOC"))
		//{
		//	int k=1;
		//}

		free(buffer);
	}

	void dump(const char* p, int size)
	{
		myprintf("dump of %d bytes:\n", size);
		const Uint8* ptr = (const Uint8*) p;
		for (int i = 0; i < size; i++)
		{
			myprintf("%02X ", *ptr);
			ptr++;
		}
		myprintf("\n");
	}

}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

