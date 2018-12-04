// as_transform.h	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_AS_TRANSFORM_H
#define BAKEINFLASH_AS_TRANSFORM_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_as_classes/as_color_transform.h"

namespace bakeinflash
{

	void	as_global_transform_ctor(const fn_call& fn);

	struct as_transform : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_TRANSFORM };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_transform(character* movie_clip);

		 virtual bool	set_member(const tu_string& name, const as_value& val);
		 virtual bool	get_member(const tu_string& name, as_value* val);

		smart_ptr<as_color_transform> m_color_transform;
		smart_ptr<character> m_movie;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_TRANSFORM_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
