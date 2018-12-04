// -- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_AS_BITMAPDATA_H
#define BAKEINFLASH_AS_BITMAPDATA_H

#include "base/tu_config.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"

namespace bakeinflash
{

	void	as_global_rectangle_ctor(const fn_call& fn);
	void	as_global_bitmapdata_ctor(const fn_call& fn);

	struct as_rectangle : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_RECTANGLE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_rectangle(const rect& rect):
			m_rect(rect)
		{
		}

		//virtual ~as_rectangle();

		rect m_rect;
	};


	struct as_bitmapdata : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_BITMAPDATA };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_bitmapdata(int w, int h, bool transparent, Uint32 fillColor);
		virtual ~as_bitmapdata();

		void draw(sprite_instance* mc);
		int count_pixels(const rgba min_pixel, const rgba& max_pixel);
		void fill_color(as_rectangle* rect, Uint32 fillColor);
		Uint8* get_data() const;
		int get_width() const;
		int get_height() const;

		image::rgba* m_image;
	};


}	// end namespace bakeinflash

#endif // bakeinflash_AS_XML_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
