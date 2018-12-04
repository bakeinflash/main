// as_imap.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef bakeinflash_AS_IWEBVIEW_H
#define bakeinflash_AS_IWEBVIEW_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_types.h"	// for rgba
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

#if ANDROID == 1
	#include <jni.h>
#endif

namespace bakeinflash
{
	struct as_iwebview : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IWEB_VIEW };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		as_iwebview(int x, int y, int w, int h);
		~as_iwebview();
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& val);
    void setFullScreen();
		
#ifdef ANDROID
		jobject m_iwebview;
#else
		void* m_iwebview;
#endif
	
	};
	
	void	as_global_iwebview_ctor(const fn_call& fn);
	
}	// namespace bakeinflash

#endif
