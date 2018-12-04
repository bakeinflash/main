// bakeinflash_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

#ifndef bakeinflash_3DS_DEF_H
#define bakeinflash_3DS_DEF_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "bakeinflash/bakeinflash_render_handler_ogles.h"
#include "bakeinflash/bakeinflash_impl.h"

extern "C" 
{
	#include <lib3ds/types.h>
}

namespace bakeinflash
{
	struct x3ds_texture
	{
		GLuint tex_id;
		GLfloat scale_x, scale_y;
	};

	struct x3ds_definition : public character_def
	{
		Lib3dsFile* m_file;
		int m_lightList;
		rect	m_rect;

		// textures required for 3D model
		string_hash< x3ds_texture * > m_texture;
		string_hash< x3ds_texture * > m_bump;

		x3ds_definition(const tu_string& url);
		~x3ds_definition();

		virtual character* create_character_instance(character* parent, int id);
		virtual void get_bound(rect* bound)
		{
			*bound = m_rect;
		}

		void set_bound(const rect& bound)
		{
			m_rect = bound;
		}

		void load_texture(const char* infile);
		void load_bump(const char* infile);

		void get_bounding_center(Lib3dsVector bmin, Lib3dsVector bmax, Lib3dsVector center, float* size);
		void ensure_camera();
		void ensure_lights();
		void ensure_nodes();
		bool is_loaded() const { return m_file != NULL; }
		void clear_obj(Lib3dsNode* nodes);

	};

}	// end namespace bakeinflash

#else

#endif


#endif
