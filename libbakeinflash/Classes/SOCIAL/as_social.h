#ifndef ASGAI_H
#define ASGAI_H

#import <GAI.h>
#include "bakeinflash/bakeinflash_root.h"

namespace bakeinflash
{

	void	as_ga_ctor(const fn_call& fn);

	struct as_ga : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_USER_PLUGIN + 1 };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_ga(player* player);

	};
}

#endif
