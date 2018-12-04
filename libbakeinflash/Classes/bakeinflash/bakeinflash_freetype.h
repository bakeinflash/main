// bakeinflash_freetype.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Truetype font rasterizer based on freetype library

#ifndef BAKEINFLASH_FREETYPE_H
#define BAKEINFLASH_FREETYPE_H

#include "base/tu_config.h"

#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_shape.h"
#include "bakeinflash/bakeinflash_canvas.h"

#if TU_CONFIG_LINK_TO_FREETYPE == 1

#include <ft2build.h>
#include FT_OUTLINE_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#define FT_CONST 

namespace bakeinflash
{

	// helper
	struct glyph_entity
	{
		glyph_entity() :
				m_image(NULL),
				m_width(0),
				m_height(0),
				m_top(0),
				m_left(0),
				m_advance(0)
		{
		}

		~glyph_entity()
		{
			free(m_image);
		}

		Uint8* m_image;
		int m_width;
		int m_height;
		int m_top;
		int m_left;
		int m_advance;
	};

	struct face_entity : public ref_counted
	{
		FT_Face m_face;
		hash<Uint64, glyph_entity*> m_ge;	// <code, glyph_entity>

		face_entity(FT_Face face, const FT_Byte* file_buf) :
			m_face(face)
		{
			assert(face);
		}

		~face_entity()
		{
			FT_Done_Face(m_face);
			for (hash<Uint64, glyph_entity*>::iterator it = m_ge.begin(); it != m_ge.end(); ++it)
			{
				delete it->second;
			}
		}
	};

	struct glyph_freetype_provider : public glyph_provider
	{
		glyph_freetype_provider();
		~glyph_freetype_provider();

		virtual image::alpha* render_string(const tu_string& str, int alignment, const tu_string& fontname, 
			bool is_bold, bool is_italic, int fontsize, const array<int>& xleading, const array<int>& yleading, int vert_advance,
			int w, int h, bool multiline, int cursor, float xscale, float yscale);

	private:

		glyph_entity* load_char_image(face_entity* fe, Uint16 code, int fontsize, float xscale);
		glyph_entity* get_glyph_entity(face_entity* fe, Uint16 code, int fontsize, float xscale);
		void draw_cursor(int fontsize, int pen_x, int pen_y, image::alpha* im);
		void draw_line(image::alpha* im, face_entity* fe, const array<Uint32>& ch, 
			int i1, int i2, int* pen_x, int pen_y, int alignment, int boxw, int fontsize, int cursor, float xscale);

		face_entity* get_face_entity(const tu_string& fontname,	bool is_bold, bool is_italic);

		// callbacks
		static int move_to_callback(FT_CONST FT_Vector* vec, void* ptr);
		static int line_to_callback(FT_CONST FT_Vector* vec, void* ptr);
		static int conic_to_callback(FT_CONST FT_Vector* ctrl, FT_CONST FT_Vector* vec, 
			void* ptr);
		static int cubic_to_callback(FT_CONST FT_Vector* ctrl1, FT_CONST FT_Vector* ctrl2,
			FT_CONST FT_Vector* vec, void* ptr);

		float m_scale;
		string_hash<smart_ptr<face_entity> > m_face_entity;
	};

}

#endif		// TU_CONFIG_LINK_TO_FREETYPE
#endif	// bakeinflash_FREETYPE_H
