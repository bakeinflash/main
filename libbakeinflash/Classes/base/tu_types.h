// tu_types.h	-- Ignacio CastaÒo, Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Minimal typedefs.  Follows SDL conventions; falls back on SDL.h if
// platform isn't obvious.


#ifndef TU_TYPES_H
#define TU_TYPES_H


#include "base/tu_config.h"
#include <stdio.h>

#if TU_USE_SDL == 1
	#include <SDL2/SDL.h>
#endif

#if defined(__i386__) || defined(_WIN32) || defined(__GNUC__)

	// On known little-endian platforms, define this stuff.
	#define _TU_LITTLE_ENDIAN_	1
	
	#ifdef TU_USE_SDL
	#	ifndef _SDL_stdinc_h
		typedef unsigned char	Uint8;
		typedef signed char	Sint8;
		typedef unsigned short	Uint16;
		typedef short	Sint16;
		typedef unsigned int	Uint32;
		typedef int	Sint32;
	#	endif
	#else
	#	ifndef Uint8
		typedef unsigned char	Uint8;
		typedef signed char	Sint8;
		typedef unsigned short	Uint16;
		typedef short	Sint16;
		typedef unsigned int	Uint32;
		typedef int	Sint32;
	#	endif
	#endif

	#ifndef _MSC_VER
		// Probably gcc or something compatible
	//	typedef unsigned long long	Uint64;
	//	typedef long long Sint64;
	#else	// _MSC_VER
		typedef unsigned __int64	Uint64;
		typedef __int64	Sint64;
	#endif	// _MSC_VER

#else	// not __I386__ and not _WIN32

	// On unknown platforms, rely on SDL
	#include <SDL.h>
	
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		#define _TU_LITTLE_ENDIAN_ 1
	#else
		#undef _TU_LITTLE_ENDIAN_
	#endif // SDL_BYTEORDER == SDL_LIL_ENDIAN

#endif	// not __I366__ and not _WIN32

#if defined(__MACH__) || defined(ANDROID) 
  typedef unsigned long long	 Uint64;
  typedef int64_t	Sint64;
#endif

typedef Uint8 uint8;
typedef Sint8 sint8;
typedef Sint8 int8;
typedef Uint16 uint16;
typedef Sint16 sint16;
typedef Sint16 int16;
typedef Uint32 uint32;
typedef Sint32 sint32;
typedef Sint32 int32;
typedef Uint64 uint64;
typedef Sint64 sint64;
typedef Sint64 int64;
typedef uint64 UINT64_C;

typedef float coord_component;

// A function to run some validation checks.
#ifdef _WIN32
__declspec(dllexport) bool	tu_types_validate();
#else // _WIN32
bool	tu_types_validate();
#endif

#if defined(ANDROID) || defined(__GNUC__)
	#define nullptr NULL
#endif

#endif // TU_TYPES_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
