// bakeinflash_string.h	-- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain. Do whatever
// you want with it.

// Implementation for ActionScript String object.


#ifndef BAKEINFLASH_STRING_H
#define BAKEINFLASH_STRING_H

#include "bakeinflash/bakeinflash_action.h"

namespace bakeinflash 
{

	// Constructor for creating ActionScript String object.
	void as_global_string_ctor(const fn_call& fn);
	as_object * get_global_string_ctor();

	void string_char_code_at(const fn_call& fn);
	void string_concat(const fn_call& fn);
	void string_from_char_code(const fn_call& fn);
	void string_index_of(const fn_call& fn);
	void string_last_index_of(const fn_call& fn);
	void string_slice(const fn_call& fn);
	void string_split(const fn_call& fn);
	void string_substr(const fn_call& fn);
	void string_substring(const fn_call& fn);
	void string_to_lowercase(const fn_call& fn);
	void string_to_uppercase(const fn_call& fn) ;
	void string_char_at(const fn_call& fn);
	void string_to_string(const fn_call& fn);
	void string_length(const fn_call& fn);
}


#endif // bakeinflash_STRING_H
