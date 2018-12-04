// bakeinflash_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

#ifndef BAKEINFLASH_AS_FILTERS_PLUGIN
#define BAKEINFLASH_AS_FILTERS_PLUGIN

#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"
#include "bakeinflash/bakeinflash_render_handler_ogles.h"
#include "opencv2/opencv.hpp"

namespace bakeinflash
{

	void	as_bevelFilter_ctor(const fn_call& fn);
	void	as_embossFilter_ctor(const fn_call& fn);


	struct as_embossFilter_character_def : public character_def
	{
		virtual void get_bound(rect* bound) { };
	};

	struct as_embossFilter : public character
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_USER_PLUGIN + 2 };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_embossFilter(sprite_instance* parent, const tu_string& file);
		~as_embossFilter() {}

	//	virtual bool	get_member(const tu_string& name, as_value* val);
	//	virtual bool	set_member(const tu_string& name, const as_value& new_val);
		virtual void	display();
		virtual void	advance(float delta_time) {};
		virtual character_def* get_character_def() { return m_def; }

		bitmap_info* apply();

		smart_ptr<character_def> m_def;
		weak_ptr<sprite_instance> m_parent;
		rect m_bound;

		cv::Mat mask;
		cv::Mat img;

	};

}	// end namespace bakeinflash


#endif
