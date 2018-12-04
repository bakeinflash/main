// as_event.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2014

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.
//
// AS3, Event class
//

#ifndef BAKEINFLASH_AS_EVENT_H
#define BAKEINFLASH_AS_EVENT_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"

namespace bakeinflash
{

	// constructor of an Event object
	void	as_global_event_ctor(const fn_call& fn);

	struct as_event : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_EVENT };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_event(const tu_string& type, as_object* target);
		virtual ~as_event();
	};


	// this is "_global.Event" object
	struct as_global_event : public as_c_function
	{
		as_global_event();
	};

	// this is "_global.MouseEvent" object
	struct as_global_mouseevent : public as_c_function
	{
		as_global_mouseevent();
	};

	// this is "_global.DataEvent" object
	struct as_global_dataevent : public as_c_function
	{
		as_global_dataevent();
	};

	// this is "_global.IOErrorEvent" object
	struct as_global_ioerrorevent : public as_c_function
	{
		as_global_ioerrorevent();
	};

	// this is "_global.SecurityErrorEvent" object
	struct as_global_securityerrorevent : public as_c_function
	{
		as_global_securityerrorevent();
	};


}	// end namespace bakeinflash


#endif // bakeinflash_AS_event_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
