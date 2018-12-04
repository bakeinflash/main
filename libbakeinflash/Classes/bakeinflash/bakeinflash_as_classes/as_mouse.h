// as_mouse	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// 'Mouse' action script class implementation

#ifndef BAKEINFLASH_AS_MOUSE_H
#define BAKEINFLASH_AS_MOUSE_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_listener.h"

namespace bakeinflash
{
	struct as_mouse : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_MOUSE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_mouse();
	};

}

#endif //bakeinflash_AS_MOUSE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
