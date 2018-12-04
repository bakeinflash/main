// Dummy header; should get included first into tu-testbed headers.
// This is for manual project-specific configuration.

//
// Some optional general configuration.
//

// WIN32, ANDROID, __APPLE__
//#if defined(__APPLE__) && defined(TARGET_OS_MAC)
//#define iOS 1
//#endif

#ifndef TU_CONFIG_LINK_TO_THREAD
#define TU_CONFIG_LINK_TO_THREAD 2		// pthread
#endif


#ifndef TU_CONFIG_LINK_TO_SSL
	#define TU_CONFIG_LINK_TO_SSL 1
#endif

#ifndef TU_USE_OGLES
#	define TU_USE_OGLES 1
#endif


#ifndef TU_USE_OPENAL
#	define TU_USE_OPENAL 1
#endif
//#define TU_USE_SDL_SOUND

#ifndef TU_CONFIG_LINK_TO_SQLITE
  #define TU_CONFIG_LINK_TO_SQLITE 1
#endif


//TU_CONFIG_LINK_TO_LIBPNG=0

//TU_CONFIG_LINK_TO_LIB3DS=0
#ifndef TU_CONFIG_LINK_TO_MYSQL
  #define TU_CONFIG_LINK_TO_MYSQL 1
#endif

#ifndef TU_CONFIG_LINK_TO_FREETYPE
#define TU_CONFIG_LINK_TO_FREETYPE 1
#endif

#ifndef TU_CONFIG_LINK_TO_FFMPEG
	#define TU_CONFIG_LINK_TO_FFMPEG 1
#endif

// tu_error_exit() is for fatal errors; it should not return!
// You can #define it to something else in compatibility_include.h; e.g. you could
// throw an exception, halt, whatever.
#ifndef tu_error_exit
#include <stdlib.h>	// for exit()
#define tu_error_exit(error_code, error_message) { printf(error_message); exit(error_code); }
#endif


// define TU_CONFIG_LINK_TO_JPEGLIB to 0 to exclude jpeg code from
// your build.  Be aware of what you're doing -- it may break
// features!
#ifndef TU_CONFIG_LINK_TO_JPEGLIB
#define TU_CONFIG_LINK_TO_JPEGLIB 1
#endif

// define TU_CONFIG_LINK_TO_ZLIB to 0 to exclude zlib code from your
// build.  Be aware of what you're doing -- it may break features that
// you need!
#ifndef TU_CONFIG_LINK_TO_ZLIB
#define TU_CONFIG_LINK_TO_ZLIB 1
#endif

// define TU_CONFIG_LINK_TO_LIBPNG to 0 to exclude libpng code from
// your build.  Be aware of what you're doing -- it may break
// features!
//#ifndef TU_CONFIG_LINK_TO_LIBPNG
//#define TU_CONFIG_LINK_TO_LIBPNG 0
//#endif

// define TU_CONFIG_LINK_TO_LIB3DS to 1 to include 3DS file support in
// bakeinflash, depending on the lib3ds library
//#ifndef TU_CONFIG_LINK_TO_LIB3DS
//#define TU_CONFIG_LINK_TO_LIB3DS 0
//#endif

// define TU_CONFIG_LINK_TO_FFMPEG to 1 to include MP3 & video support in
// bakeinflash, depending on the libavcode, libavutil & libavformat libraries
//#ifndef TU_CONFIG_LINK_TO_FFMPEG
//#define TU_CONFIG_LINK_TO_FFMPEG 0
//#endif

// define TU_CONFIG_LINK_TO_FREETYPE to 1 to include dynamic font support in
// bakeinflash, depending on the freetype library
#ifndef WINPHONE
#ifndef TU_CONFIG_LINK_TO_FREETYPE
#define TU_CONFIG_LINK_TO_FREETYPE 1
#endif
#endif

// define TU_CONFIG_LINK_TO_THREAD to 0 to switch in bakeinflash to single thread mode
// define TU_CONFIG_LINK_TO_THREAD to 1 to include SDL thread & mutex support in bakeinflash
// define TU_CONFIG_LINK_TO_THREAD to 2 to include ... (TODO: pthread support)
//#ifndef TU_CONFIG_LINK_TO_THREAD
//#	define TU_CONFIG_LINK_TO_THREAD 2
//#endif

#if TU_CONFIG_LINK_TO_THREAD == 0 && TU_CONFIG_LINK_TO_FFMPEG == 1
#error video & MP3 requires multi thread support
#endif

// define TU_CONFIG_LINK_STATIC to 1 to link bakeinflash statically 
//#ifndef TU_CONFIG_LINK_STATIC
//#	define TU_CONFIG_LINK_STATIC 0
//#endif // TU_CONFIG_LINK_STATIC

// define TU_ENABLE_NETWORK to 1 to enable network supprt
#ifndef TU_ENABLE_NETWORK
#	define TU_ENABLE_NETWORK 1
#endif // TU_ENABLE_NETWORK

/*
#ifndef exported_module
#	ifdef _WIN32
#		if TU_CONFIG_LINK_STATIC == 0
#			define  __declspec(dllexport)
#		else
#			define 
#		endif // TU_CONFIG_LINK_STATIC == 0
#	else
#		define 
#	endif // _WIN32
#endif // 
*/

// define TU_CONFIG_VERBOSE to 1 to allow verbose debugging
#ifndef TU_CONFIG_VERBOSE
#	define TU_CONFIG_VERBOSE 0
#endif

// define TU_USE_FLASH_COMPATIBLE_HITTEST to 0 to allow verbose debugging
#ifndef TU_USE_FLASH_COMPATIBLE_HITTEST
#	define TU_USE_FLASH_COMPATIBLE_HITTEST 1
#endif

// define TU_USE_SDL to 1 to use SDL sound/render handler
#ifndef TU_USE_SDL
#	define TU_USE_SDL 1
#endif


