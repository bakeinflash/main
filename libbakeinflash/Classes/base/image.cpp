// image.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Handy image utilities for RGB surfaces.

#include "base/image.h"

#include "base/container.h"
#include "base/utility.h"
#include "base/jpeg.h"
#include "base/tu_file.h"
#include <stdlib.h>
#include <string.h>

namespace image
{
	//
	// image_base
	//
	image_base::image_base(Uint8* data, int width, int height, int pitch, id_image type)
		:
		m_type(type),
		m_data(data),
		m_width(width),
		m_height(height),
		m_pitch(pitch)
	{
	}

	image_base::~image_base()
	{
		if (m_data) 
		{
			delete [] m_data;
			m_data = 0;
		}
	}

	Uint8*	scanline(image_base* surf, int y)
	{
		assert(surf);
		assert(y >= 0 && y < surf->m_height);
		return ((Uint8*) surf->m_data) + surf->m_pitch * y;
	}


	const Uint8*	scanline(const image_base* surf, int y)
	{
		assert(surf);
		assert(y >= 0 && y < surf->m_height);
		return ((const Uint8*) surf->m_data) + surf->m_pitch * y;
	}


	//
	// rgb
	//

	rgb::rgb(int width, int height)
		:
		image_base(
			0,
			width,
			height,
			(width * 3 + 3) & ~3, RGB)	// round pitch up to nearest 4-byte boundary
	{
		assert(width > 0);
		assert(height > 0);
		assert(m_pitch >= m_width * 3);
		assert((m_pitch & 3) == 0);

		m_data = new Uint8[m_pitch * m_height];
	}

	rgb::~rgb()
	{
//		if (m_data) {
//			dlfree(m_data);
//			delete [] m_data;
//			m_data = 0;
//		}
	}


	rgb*	create_rgb(int width, int height)
	// Create an system-memory rgb surface.  The data order is
	// packed 24-bit, RGBRGB..., regardless of the endian-ness of
	// the CPU.
	{
		rgb*	s = new rgb(width, height);

		return s;
	}


	//
	// rgba
	//


	rgba::rgba(int width, int height)	:
		image_base(0, width, height, width * 4, RGBA)
	{
		assert(width > 0);
		assert(height > 0);
		assert(m_pitch >= m_width * 4);
		assert((m_pitch & 3) == 0);

		m_data = new Uint8[m_pitch * m_height];
	}

	rgba::~rgba()
	{
//		if (m_data) {
//			dlfree(m_data);
//			delete [] m_data;
//			m_data = 0;
//		}
	}


	rgba*	create_rgba(int width, int height)
	// Create an system-memory rgb surface.  The data order is
	// packed 32-bit, RGBARGBA..., regardless of the endian-ness
	// of the CPU.
	{
		rgba*	s = new rgba(width, height);

		return s;
	}


	void	rgba::set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
	// Set the pixel at the given position.
	{
		assert(x >= 0 && x < m_width);
		assert(y >= 0 && y < m_height);

		Uint8*	data = scanline(this, y) + 4 * x;

		data[0] = r;
		data[1] = g;
		data[2] = b;
		data[3] = a;
	}


	//
	// alpha
	//


	alpha*	create_alpha(int width, int height)
	// Create an system-memory 8-bit alpha surface.
	{
		alpha*	s = new alpha(width, height);

		return s;
	}


	alpha::alpha(int width, int height)
		:
		image_base(0, width, height, width, ALPHA)
	{
		assert(width > 0);
		assert(height > 0);

		m_data = new Uint8[m_pitch * m_height];
	}


	alpha::~alpha()
	{
//		if (m_data) {
//			dlfree(m_data);
//			delete [] m_data;
//			m_data = 0;
//		}
	}


	void	alpha::set_pixel(int x, int y, Uint8 a)
	// Set the pixel at the given position.
	{
		assert(x >= 0 && x < m_width);
		assert(y >= 0 && y < m_height);

		Uint8*	data = scanline(this, y) + x;

		data[0] = a;
	}


	bool	alpha::operator==(const alpha& a) const
	// Bitwise content comparison.
	{
		if (m_width != a.m_width
		    || m_height != a.m_height)
		{
			return false;
		}

		for (int j = 0, n = m_height; j < n; j++)
		{
			if (memcmp(scanline(this, j), scanline(&a, j), m_width))
			{
				// Mismatch.
				return false;
			}
		}

		// Images are identical.
		return true;
	}

	void alpha::print()
	{
#ifndef NDEBUG
		FILE* fi = fopen("alpha.txt", "w");
		for (int y = 0; y < m_height; y++)
		{
			tu_string s;
			for (int x = 0; x < m_pitch; x++)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "%02X", m_data[y*m_pitch + x]);
				s += m_data[y*m_pitch + x] == 0 ? ".." : buf;
			}
			fprintf(fi, "%s\n", s.c_str());
		}
		fclose(fi);
#endif
	}

	unsigned int	alpha::compute_hash() const
	// Compute a hash code based on image contents.  Can be useful
	// for comparing images.
	{
		unsigned int	h = (unsigned int) bernstein_hash(&m_width, sizeof(m_width));
		h = (unsigned int) bernstein_hash(&m_height, sizeof(m_height), h);

		for (int i = 0, n = m_height; i < n; i++)
		{
			h = (unsigned int) bernstein_hash(scanline(this, i), m_width, h);
		}

		return h;
	}


	//
	// utility
	//


	void	write_jpeg(tu_file* out, rgb* image, int quality)
	// Write the given image to the given out stream, in jpeg format.
	{
		jpeg::output*	j_out = jpeg::output::create(out, image->m_width, image->m_height, quality);

		for (int y = 0; y < image->m_height; y++) {
			j_out->write_scanline(scanline(image, y));
		}

		delete j_out;
	}


	rgb*	read_jpeg(const char* filename)
	// Create and read a new image from the given filename, if possible.
	{
		tu_file	in(filename, "rb");	// file automatically closes when 'in' goes out of scope.
		if (! in.get_error())
		{
			rgb*	im = read_jpeg(&in);
			return im;
		}
		else
		{
			return NULL;
		}
	}


	rgb*	read_jpeg(tu_file* in)
	// Create and read a new image from the stream.
	{
		jpeg::input*	j_in = jpeg::input::create(in);
		if (j_in == NULL) return NULL;
		
		rgb*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++)
		{
			j_in->read_scanline(scanline(im, y));
		}

		delete j_in;

		return im;
	}


	rgb*	read_swf_jpeg2(tu_file* in)
	// Create and read a new image from the stream.  Image is in
	// SWF JPEG2-style format (the encoding tables come first in a
	// separate "stream" -- otherwise it's just normal JPEG).  The
	// IJG documentation describes this as "abbreviated" format.
	{
		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2_header_only(in);
		if (j_in == NULL) return NULL;
		
		rgb*	im = read_swf_jpeg2_with_tables(j_in);

		delete j_in;

		return im;
	}


	rgb*	read_swf_jpeg2_with_tables(jpeg::input* j_in)
	// Create and read a new image, using a input object that
	// already has tables loaded.
	{
		assert(j_in);

		j_in->start_image();

		rgb*	im = image::create_rgb(j_in->get_width(), j_in->get_height());

		for (int y = 0; y < j_in->get_height(); y++) {
			j_in->read_scanline(scanline(im, y));
		}

		j_in->finish_image();

		return im;
	}


	rgba*	read_swf_jpeg3(tu_file* in)
	// For reading SWF JPEG3-style image data, like ordinary JPEG, 
	// but stores the data in rgba format.
	{
		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2_header_only(in);
		if (j_in == NULL) return NULL;
		
		j_in->start_image();

		rgba*	im = image::create_rgba(j_in->get_width(), j_in->get_height());

		Uint8*	line = new Uint8[3*j_in->get_width()];

		for (int y = 0; y < j_in->get_height(); y++) 
		{
			j_in->read_scanline(line);

			Uint8*	dst = scanline(im, y);
			Uint8* src = line;
			for (int x = 0; x < j_in->get_width(); x++) 
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;
			}
		}

		delete [] line;

		j_in->finish_image();
		delete j_in;

		return im;
	}


	void	write_tga(tu_file* out, rgba* im)
	// Write a 32-bit Targa format bitmap.  Dead simple, no compression.
	{
		out->write_byte(0);
		out->write_byte(0);
		out->write_byte(2);	/* uncompressed RGB */
		out->write_le16(0);
		out->write_le16(0);
		out->write_byte(0);
		out->write_le16(0);	/* X origin */
		out->write_le16(0);	/* y origin */
		out->write_le16(im->m_width);
		out->write_le16(im->m_height);
		out->write_byte(32);	/* 32 bit bitmap */
		out->write_byte(0);

		for (int y = 0; y < im->m_height; y++)
		{
			uint8*	p = scanline(im, y);
			for (int x = 0; x < im->m_width; x++)
			{
				out->write_byte(p[x * 4]);
				out->write_byte(p[x * 4 + 1]);
				out->write_byte(p[x * 4 + 2]);
				out->write_byte(p[x * 4 + 3]);
			}
		}
	}

};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
