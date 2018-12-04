// as_iprinter.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// system utilities

#ifndef BAKEINFLASH_AS_iprinter_H
#define BAKEINFLASH_AS_iprinter_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{
    
    void	as_global_iprinter_ctor(const fn_call& fn);

	struct as_iprinter : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IPRINTER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_iprinter();
		virtual ~as_iprinter();
        
        smart_ptr<as_array> m_printJob;
		
	};

}	// namespace bakeinflash

#endif
