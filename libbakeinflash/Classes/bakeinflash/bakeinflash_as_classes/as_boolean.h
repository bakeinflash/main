// as_boolean.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef BAKEINFLASH_AS_BOOLEAN_H
#define BAKEINFLASH_AS_BOOLEAN_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	void	as_global_boolean_ctor(const fn_call& fn);
	void	as_boolean_to_string(const fn_call& fn);
	void	as_boolean_valueof(const fn_call& fn);

}	// end namespace bakeinflash


#endif // bakeinflash_AS_BOOLEAN_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
