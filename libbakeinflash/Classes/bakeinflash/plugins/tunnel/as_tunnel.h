// bakeinflash_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

#ifndef BAKEINFLASH_AS_TUNNEL_PLUGIN
#define BAKEINFLASH_AS_TUNNEL_PLUGIN

#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"
#include "bakeinflash/bakeinflash_render_handler_ogles.h"


#if ANDROID == 1
  #define GL_BGRA                           0x80E1
  #include <GLES/gl.h>
#elif TU_USE_SDL
	#include <SDL2/SDL.h>  // for cursor handling & the scanning for extensions.
	#include <SDL2/SDL_opengl.h>	// for opengl const
//	#include <GL/GLU.h>	// for opengl const
#else
	#include <OpenGLES/ES1/glext.h>
#endif

namespace bakeinflash
{

	void	as_tunnel_ctor(const fn_call& fn);

	
struct color {
    double r,g,b;
    color(double _rgb) {
        r = g = b = _rgb;
    }
    color(double _r, double _g, double _b) :
        r(_r), g(_g), b(_b)
    { }
};

	struct as_tunnel_character_def : public character_def
	{
		virtual void get_bound(rect* bound) { };
	};

	struct as_tunnel : public character
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_USER_PLUGIN + 100};
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
	
		struct uv
		{
			GLfloat u;
			GLfloat v;
		};

		struct xyz
		{
			GLfloat x;
			GLfloat y;
			GLfloat z;
		};

		as_tunnel(sprite_instance* mc);
		virtual ~as_tunnel();

		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& new_val);
		virtual void	display();
		virtual void	advance(float delta_time) {};
		virtual character_def* get_character_def() { return m_def; }

		void save_ogl();
		void restore_ogl();
		void makeTorus(int sides, int cs_sides, float radius, float cs_radius);
		void read_params();

//	private :
		smart_ptr<as_bitmapdata> m_bitmapdata;
		smart_ptr<character_def> m_def;

		array<xyz> m_vertices;
		array<uv> m_texels;
		array<GLushort> m_indices;
		GLuint m_texture;
		GLuint m_texture1;

		float m_speed;
		float m_diameter;
		rgba m_colour;
		rgba m_hue;
		shader m_colour_shader;
		shader m_sharpen_shader;
		float m_sharp;
		float m_smooth;
	};

}	// end namespace bakeinflash

#endif
