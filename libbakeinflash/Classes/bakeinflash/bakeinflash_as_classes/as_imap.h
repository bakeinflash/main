// as_imap.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef bakeinflash_AS_IMAP_H
#define bakeinflash_AS_IMAP_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_as_classes/as_array.h"


namespace bakeinflash
{
	
	struct as_imap : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IMAP };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		as_imap(int x, int y, int w, int h, as_array* latlon);
		~as_imap();
		
		void* m_map;
		void* m_location_manager;
		
	//	virtual void	advance(float delta_time);		
		void notifySelect(const char* title);
		void notifyDeselect(const char* title);
		
	};
	
	void	as_global_imap_ctor(const fn_call& fn);
	
}	// namespace bakeinflash

#endif
