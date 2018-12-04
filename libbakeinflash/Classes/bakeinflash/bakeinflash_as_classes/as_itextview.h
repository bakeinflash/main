// as_imap.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef bakeinflash_as_itextview_H
#define bakeinflash_as_itextview_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_types.h"	// for rgba
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

namespace bakeinflash
{
	struct as_itextview : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_ITEXT_VIEW };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		as_itextview(character* ch);
		~as_itextview();
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& val);
    virtual void  advance(float delta_time);
		void destroy();
		
		void* m_tableview;
		bool m_edit;
		tu_string m_edit_style;
		rgba m_text_color;
		rgba m_bk_color;
    smart_ptr<character> m_parent;
		
	};
	
	void	as_global_itextview_ctor(const fn_call& fn);
	
}	// namespace bakeinflash

#endif
