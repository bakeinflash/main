// as_imap.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef bakeinflash_AS_as_igamecenter_H
#define bakeinflash_AS_as_igamecenter_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{
	struct as_igamecenter : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IGAMECENTER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		as_igamecenter();
		~as_igamecenter();
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& val);
		
		//void* m_as_igamecenterview;
		tu_string m_leader_board;
		smart_ptr<character> m_top_image;
	
	};
	
	void	as_global_igamecenter_ctor(const fn_call& fn);
	
}	// namespace bakeinflash

#endif
