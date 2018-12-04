// as_class.h	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script 3 Class object

#ifndef bakeinflash_AS_CLASS_H
#define bakeinflash_AS_CLASS_H

#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_abc.h"

namespace bakeinflash
{
	class as_class : public as_object
	{
	public:
		as_class()
		{
		}

		void set_class(as_object* target, class_info * info);

	private:

		smart_ptr<class_info> m_class;
		
	};

}


#endif
