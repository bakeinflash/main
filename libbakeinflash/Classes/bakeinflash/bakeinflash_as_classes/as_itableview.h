// as_imap.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef bakeinflash_AS_ITABLEVIEW_H
#define bakeinflash_AS_ITABLEVIEW_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_types.h"	// for rgba
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

#define CELLSIZE 46

namespace bakeinflash
{
	struct as_itableview : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_ITABLE_VIEW };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		as_itableview(int x, int y, int w, int h);
		~as_itableview();
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& val);
		
		void* m_tableview;
		smart_ptr<as_array> m_items;
		bool m_edit;
		tu_string m_edit_style;
		rgba m_text_color;
		rgba m_bk_color;
		bool m_selection_enabled;
		tu_string m_delete_title;
		
	};
	
	void	as_global_itableview_ctor(const fn_call& fn);
	
}	// namespace bakeinflash

#endif
