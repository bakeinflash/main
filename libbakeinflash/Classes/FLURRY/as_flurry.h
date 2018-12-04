
#ifndef AS_FLURRY_H
#define AS_FLURRY_H

//#import <GAI.h>
#include "bakeinflash/bakeinflash_root.h"

namespace bakeinflash
{

	void	as_flurry_ctor(const fn_call& fn);

	struct as_flurry : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_USER_PLUGIN + 1 };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_flurry(player* player);

	};
}

#endif
