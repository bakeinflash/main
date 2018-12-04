// utility.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#ifndef UTILITY_H
#define UTILITY_H

#include "base/tu_config.h"
#include <assert.h>
#include "base/tu_math.h"
#include "base/tu_types.h"
#include "base/tu_swap.h"
#include <ctype.h>
#include <new>


#ifdef _WIN32

#define __PRETTY_FUNCTION__ __FUNCDNAME__
#define snprintf _snprintf
#define strncasecmp _strnicmp
#define isnan _isnan
#define F_OK 0	// for access
#define strdup _strdup
#define valloc(x) _aligned_malloc(x, 8)

//#ifdef NULL
//#undef NULL
//#define NULL nullptr
//#endif

// It will help users to send messages about bugs
//#undef assert
//#define assert(x)		\
//if (!(x))		\
//{		\
//	printf("assert(%s): %s\n%s(%d)\n",  #x, __PRETTY_FUNCTION__, __FILE__, __LINE__);		\
//	exit(0);		\
//}

	#ifndef NDEBUG

	// On windows, replace ANSI assert with our own, for a less annoying
	// debugging experience.
	//int	tu_testbed_assert_break(const char* filename, int linenum, const char* expression);
//	#undef assert
//	#define assert(x)	if (!(x)) { __asm { int 3 } }	// tu_testbed_assert_break(__FILE__, __LINE__, #x))

	#endif // not NDEBUG
#endif // _WIN32


// Compile-time assert.  Thanks to Jon Jagger
// (http://www.jaggersoft.com) for this trick.
#define compiler_assert(x)	// switch(0){case 0: case x:;}

#ifdef ANDROID
	#include <android/log.h>
	#define  alog(...)  __android_log_print(ANDROID_LOG_INFO, "bakeinflash",__VA_ARGS__)
#endif

#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI

//
// some misc handy math functions
//

#if defined(_MSC_VER) && _MSC_VER <= 1700		// VS 2005
const float LN_2 = 0.693147180559945f;
inline float	log2(float f) { return logf(f) / LN_2; }
inline float	fmax(float a, float b) { if (a < b) return b; else return a; }
inline float	fmin(float a, float b) { if (a < b) return a; else return b; }
#endif

inline int64	i64abs(int64 i) { if (i < 0) return -i; else return i; }
inline int	iabs(int i) { if (i < 0) return -i; else return i; }
inline int	imax(int a, int b) { if (a < b) return b; else return a; }
inline int	imin(int a, int b) { if (a < b) return a; else return b; }
inline int rol(int a, int n) { return (a << n) | (a >> (32 - n)); } 
inline int ror(int a, int n) { return (a >> n) | (a << (32 - n)); }

inline int isaturate(int a) { return imin(255, imax(0, a)); }
inline float fsaturate(float a) { return (float) fmin(255.0f, fmax(0.0f, a)); }

inline int	iclamp(int i, int min, int max) {
	assert( min <= max );
	return imax(min, imin(i, max));
}

inline float	fclamp(float f, float xmin, float xmax) {
	assert( xmin <= xmax );
	return  (float) fmax(xmin, fmin(f, xmax));
}

inline float flerp(float a, float b, float f) { return (b - a) * f + a; }
inline float fround(float a) { return  (float) floor(a + 0.5f); }

#ifdef ANDROID
	const float LN_2 = 0.693147180559945f;
	inline float	log2(float f) { return logf(f) / LN_2; }
	inline float	fmax(float a, float b) { if (a < b) return b; else return a; }
	inline float	fmin(float a, float b) { if (a < b) return a; else return b; }
#endif

inline int	fchop( float f ) { return (int) f; }	// replace w/ inline asm if desired
inline int	frnd(float f) { return fchop(f + 0.5f); }	// replace with inline asm if desired

inline int p2(int n) { int k = 1; while (k < n) k <<= 1; return k; }

// Handy macro to quiet compiler warnings about unused parameters/variables.
#define UNUSED(x) (void) (x)


// Compile-time constant size of array.
#define TU_ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))


inline Uint32	bernstein_hash(const void* data_in, int size, Uint32 seed = 5381)
// Computes a hash of the given data buffer.
// Hash function suggested by http://www.cs.yorku.ca/~oz/hash.html
// Due to Dan Bernstein.  Allegedly very good on strings.
//
// One problem with this hash function is that e.g. if you take a
// bunch of 32-bit ints and hash them, their hash values will be
// concentrated toward zero, instead of randomly distributed in
// [0,2^32-1], because of shifting up only 5 bits per byte.
{
	const unsigned char*	data = (const unsigned char*) data_in;
	Uint32	h = seed;
	while (size > 0) {
		size--;
		h = ((h << 5) + h) ^ (unsigned) data[size];
	}

	return h;
}


inline Uint32	sdbm_hash(const void* data_in, int size, Uint32 seed = 5381)
// Alternative: "sdbm" hash function, suggested at same web page
// above, http::/www.cs.yorku.ca/~oz/hash.html
//
// This is somewhat slower, but it works way better than the above
// hash function for hashing large numbers of 32-bit ints.
{
	const unsigned char*	data = (const unsigned char*) data_in;
	Uint32	h = seed;
	while (size > 0) {
		size--;
		h = (h << 16) + (h << 6) - h + (unsigned) data[size];
	}

	return h;
}


inline Uint32	bernstein_hash_case_insensitive(const void* data_in, int size, Uint32 seed = 5381)
// Computes a hash of the given data buffer; does tolower() on each
// byte.  Hash function suggested by
// http://www.cs.yorku.ca/~oz/hash.html Due to Dan Bernstein.
// Allegedly very good on strings.
{
	const unsigned char*	data = (const unsigned char*) data_in;
	Uint32	h = seed;
	while (size > 0) {
		size--;
		h = ((h << 5) + h) ^ (unsigned) tolower(data[size]);
	}

	// Alternative: "sdbm" hash function, suggested at same web page above.
	// h = 0;
	// for bytes { h = (h << 16) + (h << 6) - hash + *p; }

	return h;
}


// return NaN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4723)  // The divide by zero is intentional.
#endif   // _MSC_VER
inline double get_nan() { double zero = 0.0; return zero / zero; }
#ifdef _MSC_VER
#pragma warning(pop)
#endif   // _MSC_VER


// check existense of a file
bool exist(const char* path);

#endif // UTILITY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
