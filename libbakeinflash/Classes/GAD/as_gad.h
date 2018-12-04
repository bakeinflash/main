
#ifndef AS_GAD_H
#define AS_GAD_H

//#import <GAI.h>
#include "bakeinflash/bakeinflash_root.h"

namespace bakeinflash
{

	void	as_gad_ctor(const fn_call& fn);

	struct as_gad : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_USER_PLUGIN + 2 };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_gad(player* player);

	};
}

#endif
