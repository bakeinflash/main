// bakeinflash_log.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Helpers for logging messages & errors.


#ifndef BAKEINFLASH_LOG_H
#define BAKEINFLASH_LOG_H

#include "bakeinflash/bakeinflash.h"

namespace bakeinflash
{
	// Printf-style interfaces.

#ifdef __GNUC__
	// use the following to catch errors: (only with gcc)
	#ifdef ANDROID
		void	myprintf(const char* fmt, ...);
	#else
//		void	log(const char* fmt, ...) __attribute__((format (printf, 1, 2)));
		void	myprintf(const char* fmt, ...) __attribute__((format (printf, 1, 2)));
		void	myprintf(const char* fmt, ...) __attribute__((format (printf, 1, 2)));
	#endif
#else	// not __GNUC__
	 void	myprintf(const char* fmt, ...);
#endif	// not __GNUC__

	void dump(const char* p, int size);
}


#endif // bakeinflash_LOG_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
