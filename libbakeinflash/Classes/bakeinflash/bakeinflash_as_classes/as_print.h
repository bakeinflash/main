// as_cef.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2015

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// CEF embedd browser implementation


#ifndef bakeinflash_AS_PRINT_H
#define bakeinflash_AS_PRINT_H

#if TU_CONFIG_LINK_TO_LIBHPDF == 1

#include "base/tu_types.h"

namespace bakeinflash
{
		int create_pdf(Uint8* buf, int w, int h, const char* pdf_file);
		int print_image(Uint8* buf, int w, int h, const char* pdf_file);
		int print_text(const tu_string& s, const tu_string& dev);

}	// namespace bakeinflash

#endif
#endif
