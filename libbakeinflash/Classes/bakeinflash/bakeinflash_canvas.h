// bakeinflash_canvas.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Drawing API implementation


#ifndef BAKEINFLASH_CANVAS_H
#define BAKEINFLASH_CANVAS_H

#include "bakeinflash_shape.h"

namespace bakeinflash
{

	struct canvas : public shape_character_def
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CANVAS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return shape_character_def::is(class_id);
		}

		float m_current_x;
		float m_current_y;
		int m_current_fill;
		int m_current_line;
		int m_current_path;

		canvas();
		~canvas();

		void begin_fill(const rgba& color);
		void end_fill();
		void close_path();

		void move_to(float x, float y);
		void line_to(float x, float y);
		void curve_to(float cx, float cy, float ax, float ay);

		void add_path(bool new_path);
		void set_line_style(Uint16 width, const rgba& color, const tu_string& caps_style);

		virtual void get_bound(rect* bound);
		virtual void	tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const;
	};


}	// end namespace bakeinflash

#endif // bakeinflash_CANVAS_H

