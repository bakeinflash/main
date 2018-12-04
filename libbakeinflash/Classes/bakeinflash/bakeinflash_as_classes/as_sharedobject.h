// as_sharedobject.h	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script SharedObject implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_SHAREOBJECT_H
#define BAKEINFLASH_AS_SHAREOBJECT_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	class as_sharedobject : public as_object
	{
		static string_hash<smart_ptr<as_object> > m_local;

	public:

		as_sharedobject();

		bool	get_member(const tu_string& name, as_value* val);

		static smart_ptr<as_object> get_local( const tu_string & name);
	};

}

#endif //bakeinflash_AS_SHAREOBJECT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
