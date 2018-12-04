// as_color.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Selection class lets you set and control the text field in which 
// the insertion point is located (that is, the field that has focus).
// Selection-span indexes are zero-based
// (for example, the first position is 0, the second position is 1, and so on). 


#ifndef BAKEINFLASH_AS_SELECTION_H
#define BAKEINFLASH_AS_SELECTION_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	// Create built-in Selection object.
	as_object* selection_init();

	struct as_selection : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_SELECTION };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_selection();
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_SELECTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
