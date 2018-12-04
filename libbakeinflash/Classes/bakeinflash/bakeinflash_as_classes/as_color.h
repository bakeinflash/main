// as_color.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Color class lets you set the RGB color value and color transform
// of movie clips and retrieve those values once they have been set. 


#ifndef BAKEINFLASH_AS_COLOR_H
#define BAKEINFLASH_AS_COLOR_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"

namespace bakeinflash
{

	void	as_global_color_ctor(const fn_call& fn);

	struct as_color : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_COLOR };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_color(character* target);

		weak_ptr<character> m_target;
		cxform	m_cxform;	// original cxform
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_COLOR_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
