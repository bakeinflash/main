// bakeinflash_freetype.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// TrueType font rasterizer based on freetype library
// Also serve as fontlib

//   """
//    Portions of this software are copyright ©1996-2002,2006 The FreeType Project (www.freetype.org).  All rights reserved.
//   """
#include "base/tu_config.h"

#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_freetype.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_canvas.h"
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/utf8.h"

#ifdef _WIN32
	#include <Windows.h>
#else
#endif

bool get_memoryfile(const char* url, Uint8** ptr, int* length);
void get_fontface(const tu_string& font_name, bool is_bold, bool is_italic, tu_string* key);

#ifndef iOS
bool get_memoryfile(const char* url, Uint8** ptr, int* length)
{
	return false;
}

void get_fontface(const tu_string& fontname, bool is_bold, bool is_italic, tu_string* key)
{
	if (is_bold && is_italic)
	{
		*key = fontname + " Bold Italic";
	}
	else
	if (is_bold)
	{
		*key = fontname + " Bold";
	}
	else
	if (is_italic)
	{
		*key = fontname + " Italic";
	}
	else
	{
		*key = fontname;
	}
}
#endif

namespace bakeinflash
{

#if TU_CONFIG_LINK_TO_FREETYPE == 1

	static FT_Library	m_lib;

	// 
	//	glyph provider implementation
	//

	glyph_freetype_provider::glyph_freetype_provider() :
		m_scale(0)
	{
	}

	glyph_freetype_provider::~glyph_freetype_provider()
	{
		m_face_entity.clear();

		int error = FT_Done_FreeType(m_lib);
		if (error)
		{
			myprintf("FreeType provider: can't close FreeType!  error = %d\n", error);
		}
		m_lib = NULL;
	}

	face_entity* glyph_freetype_provider::get_face_entity(const tu_string& fontname, bool is_bold, bool is_italic)
	{
		// form hash key
		tu_string key;
		get_fontface(fontname, is_bold, is_italic, &key);

		// first try to find from hash
		smart_ptr<face_entity> fe;
		if (m_face_entity.get(key, &fe))
		{
			return fe.get();
		}

		FT_Face face = NULL;
		const FT_Byte* file_base = NULL;
		FT_Long face_index = 0;	// The index of the face within the font. The first face has index 0.

		
		// first try memory file
		int file_size = 0;
		Uint8* file_ptr = NULL;
    tu_string url = key + ".ttf";
		if (get_memoryfile(url.c_str(), &file_ptr, &file_size))
		{
			file_base = (FT_Byte*) file_ptr;
			FT_New_Memory_Face(m_lib, file_base, file_size, face_index, &face);
			//myprintf("m_lib=%p face=%p file=%p len=%d\n", m_lib, face, file_base, file_size);		
		}
		else
		{
			url = get_startdir() + key + ".ttf";
			FT_Error er = FT_New_Face(m_lib, url.c_str(), 0, &face);
			if (er)
			{
				myprintf("could not open '%s'\n", url.c_str());
				url = get_startdir() + "default.ttf";
				myprintf("using '%s'\n", url.c_str());

				er = FT_New_Face(m_lib, url.c_str(), 0, &face);
				if (er)
				{
					myprintf("could not open '%s'\n", url.c_str());
				}
			}
		}

		if (face)
		{
			if (is_bold)
			{
				face->style_flags |= FT_STYLE_FLAG_BOLD;
			}

			if (is_italic)
			{
				face->style_flags |= FT_STYLE_FLAG_ITALIC;
			}
			fe = new face_entity(face, file_base);
			m_face_entity.add(key, fe);
			return fe.get();
		}
		//myprintf("some error opening font '%s'\n", font_filename.c_str());
		return NULL;
	}

	glyph_provider*	create_glyph_provider_freetype()
	{
		int	error = FT_Init_FreeType(&m_lib);
		if (error != 0)
		{
			myprintf("FreeType provider: can't init FreeType!  error = %d\n", error);
			return NULL;
		}
		return new glyph_freetype_provider();
	}


	glyph_entity* glyph_freetype_provider::load_char_image(face_entity* fe, Uint16 code, int fontsize, float xscale)
	{
		// form hash key
		Uint64 xs = (Uint32) (xscale * 0x10000);
		Uint64 key = (xs << 32) | (fontsize << 16) | code;

		// try to find the stored image of the character
		glyph_entity* ge = NULL;
		FT_GlyphSlot  slot = fe->m_face->glyph;
		if (fe->m_ge.get(key, &ge) == false)
		{

			FT_Matrix transform = {	(FT_Fixed) (xscale * 0x10000), 0, 0, 0x10000 };
			FT_Set_Transform(fe->m_face, &transform, NULL);

			if (FT_Load_Char(fe->m_face, code, FT_LOAD_RENDER) == 0)
			{
				ge = new glyph_entity();
				ge->m_width = fe->m_face->glyph->bitmap.width;
				ge->m_height = fe->m_face->glyph->bitmap.rows;
				ge->m_image = (Uint8*) malloc(ge->m_width * ge->m_height);
				ge->m_left = slot->bitmap_left;
				ge->m_top = slot->bitmap_top;
				ge->m_advance = slot->advance.x >> 6;

				memcpy(ge->m_image, fe->m_face->glyph->bitmap.buffer, ge->m_width * ge->m_height);

				// keep image of the character
				fe->m_ge.add(key, ge);
			}
		}
		return ge;
	}

	glyph_entity* glyph_freetype_provider::get_glyph_entity(face_entity* fe, Uint16 code, int fontsize, float xscale)
	{
		// form hash key
		Uint64 xs = (Uint32) (xscale * 0x10000);
		Uint64 key = (xs << 32) | (fontsize << 16) | code;

		glyph_entity* ge = NULL;
		fe->m_ge.get(key, &ge);
		return ge;
	}

	void glyph_freetype_provider::draw_line(image::alpha* im, face_entity* fe, const array<Uint32>& ch, 
		int i1, int i2, int* pen_x, int pen_y, int alignment, int boxw, int fontsize, int cursor, float xscale)
	{
		int line_width = 0;
		for (int i = i1; i < i2; i++)
		{
			glyph_entity* ge = get_glyph_entity(fe, ch[i], fontsize, xscale);
			line_width += ge->m_advance;
		}

		// align
		if (alignment > 0)		// not left
		{
			if (alignment == 1)	// right
			{
				*pen_x = boxw - line_width - 4;		// hack, чуть справа отступ
			}
			else
			if (alignment == 2)	// center
			{
				*pen_x = (boxw - line_width) / 2;
			}
		}

		// draw line
		for (int i = i1; i < i2; i++)
		{
			glyph_entity* ge = get_glyph_entity(fe, ch[i], fontsize, xscale);
			int h = ge->m_height;
			int w = ge->m_width;

			// now, draw to our target surface
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					int dy = pen_y + y - ge->m_top;
					int dx = *pen_x + x + ge->m_left;
					if (dy >= 0 && dy < im->m_height && dx >=0 && dx < im->m_pitch)
					{
						int k = dy * im->m_pitch + dx;
						im->m_data[k] = ge->m_image[y * w + x];
					}
				}
			}

			// курсор после символа i
			if (cursor == i)
			{
				draw_cursor(fontsize, *pen_x, pen_y, im);
			}

			// increment pen position 
			*pen_x += ge->m_advance;
		}

		// курсор после послднего символа
		if (cursor == i2)
		{
			draw_cursor(fontsize, *pen_x, pen_y, im);
		}
	}

	void glyph_freetype_provider::draw_cursor(int fontsize, int pen_x, int pen_y, image::alpha* im)
	{
		for (int y = 0; y < fontsize; y++)
		{
			int dy = pen_y + y - (int) (fontsize * 0.9f);
			int dx = pen_x; // - 1;
			if (dy >= 0 && dy < im->m_height && dx >=0 && dx < im->m_pitch)
			{
				int k = dy * im->m_pitch + dx;
				im->m_data[k] = 255;
			}
		}
	}

	image::alpha* glyph_freetype_provider::render_string(const tu_string& str, int alignment, const tu_string& fontname, 
		bool is_bold, bool is_italic, int fontsize, const array<int>& xleading, const array<int>& yleading, int vertAdvance, 
		int boxw, int boxh, bool multiline, int cursor, float xscale, float yscale)
	{
		face_entity* fe = get_face_entity(fontname, is_bold, is_italic);
		if (fe == NULL)
		{
			return NULL;
		}

		FT_Set_Pixel_Sizes(fe->m_face, 0, fontsize);

		// dynamic text ?
		if (yleading.size() == 0)
		{
			// для дина текста передается ток доп смещение между строчками
			vertAdvance += fe->m_face->size->metrics.height >> 6;
		}

		// load char images
		array<Uint32> ch;
		Uint32	code = 1;
		const char*	p = str.c_str();
		const char*	end = p + str.size();
		while (code && p < end)
		{
			code = utf8::decode_next_unicode_character(&p);
			ch.push_back(code);
			if (code != 0 && code != 10 && code != 13)
			{
				load_char_image(fe, code, fontsize, xscale);
			}
		}

		int p2w = p2(boxw);
		int p2h = p2(boxh);
		image::alpha* im = new image::alpha(p2w, p2h);
		memset(im->m_data, 0, im->m_pitch * im->m_height);

		int rowindex = 0;
		int pen_x0 = xleading[rowindex];
		int pen_x = pen_x0;
//		int pen_y_dyna = (fe->m_face->size->metrics.ascender - fe->m_face->size->metrics.descender) >> 6;
		int pen_y_dyna = (fe->m_face->size->metrics.height + fe->m_face->size->metrics.descender) >> 6;
		int pen_y = yleading.size() > rowindex ? yleading[rowindex] : pen_y_dyna;
		pen_y += (int) (2 * yscale);

		// draw

		int i;
		int j;
		int i1 = 0;		// begin of line
		FT_GlyphSlot  slot = fe->m_face->glyph;
		int lw = 0;		// line width
		for (i = 0; i < ch.size(); i++)
		{
			code = ch[i];
			if (code == 0 || code == 10 || code == 13)
			{
				draw_line(im, fe, ch, i1, i, &pen_x, pen_y, alignment, boxw, fontsize, cursor, xscale);
				i1 = i + 1;
				i = i1 - 1;		// потомц что прибавится в конце цикла
				lw = 0;

				rowindex++;
				if (rowindex < xleading.size())
				{
					pen_x0 = xleading[rowindex];
				}

				if (multiline)
				{
					pen_x = pen_x0;
					if (rowindex < xleading.size())
					{
						pen_y = yleading.size() > rowindex ? yleading[rowindex] : fe->m_face->size->metrics.ascender >> 6;
					}
					else
					{
						pen_y += vertAdvance;
					}
				}
				continue;
			}

			// только для ствт полей перевод кареток
			glyph_entity* ge = get_glyph_entity(fe, code, fontsize, xscale);
			if (ge)
			{
				lw += ge->m_advance;
				if (lw <= (boxw - pen_x0))
				{
					continue;
				}

				// назад до пробела
				for (j = i; j >= i1 && ch[j] != 32; j--)
				{
					glyph_entity* je = get_glyph_entity(fe, ch[j], fontsize, xscale);
					lw -= je->m_advance;
				}

				if (j >= i1)
				{
					// есть пробел
					// j указывает на пробел вывести вклюяач пробел
					draw_line(im, fe, ch, i1, j + 1, &pen_x, pen_y, alignment, boxw, fontsize, cursor, xscale);
					i1 = j + 1;
				}
				else
				{
					// пробела нет.. всю строку выводить от i1 до i
					if (i1 == i)
					{
						// нет места даже для 1 символа.. 1 все таки вывести
						i++;
					}
					draw_line(im, fe, ch, i1, i, &pen_x, pen_y, alignment, boxw, fontsize, cursor, xscale);
					i1 = i;
				}

				i = i1 - 1;		// потомц что прибавится в конце цикла
				lw = 0;
				if (multiline)
				{
					pen_x = pen_x0;
					pen_y += vertAdvance;
				}

			}
		}

		// draw last line
		draw_line(im, fe, ch, i1, ch.size(), &pen_x, pen_y, alignment, boxw, fontsize, cursor, xscale);
		return im;
	}


#endif

}	// end of namespace bakeinflash
