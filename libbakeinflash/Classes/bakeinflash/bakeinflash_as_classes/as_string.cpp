// as_string.cpp	  -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation of ActionScript String class.

#include "bakeinflash/bakeinflash_as_classes/as_string.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "base/utf8.h"
#include "bakeinflash/bakeinflash_function.h"

namespace bakeinflash
{

	void string_char_code_at(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		double val = get_nan();
		if (obj && fn.nargs > 0)
		{
			int	index = fn.arg(0).to_int();
			Uint16 ch = obj->utf8_char_code_at(index);
			if (ch != 0xFFFF)	// invalid ?
			{
				val = ch;
			}
		}
		fn.result->set_double(val);
	}
  
	void string_concat(const fn_call& fn)
	{
		const tu_string& str = fn.this_value.to_tu_string();

		tu_string result(str);
		for (int i = 0; i < fn.nargs; i++)
		{
			result += fn.arg(i).to_string();
		}

		fn.result->set_tu_string(result);
	}
  
	void string_from_char_code(const fn_call& fn)
	{
		// Takes a variable number of args.  Each arg
		// is a numeric character code.  Construct the
		// string from the character codes.
		tu_string result;
		for (int i = 0; i < fn.nargs; i++)
		{
			uint32 c = (uint32) fn.arg(i).to_number();
			result.append_wide_char(c);
		}

		fn.result->set_tu_string(result);
	}

	void string_index_of(const fn_call& fn)
	{
		const tu_string& sstr = fn.this_value.to_tu_string();

		if (fn.nargs < 1)
		{
			fn.result->set_double(-1);
		}
		else
		{
			int	start_index = 0;
			if (fn.nargs > 1)
			{
				start_index = fn.arg(1).to_int();
			}
			const char*	str = sstr.c_str();
			const char*	p = strstr(
				str + start_index,	// FIXME: not UTF-8 correct!
				fn.arg(0).to_string());
			if (p == NULL)
			{
				fn.result->set_double(-1);
				return;
			}
			fn.result->set_double(tu_string::utf8_char_count(str, (int) (p - str)));
		}
	}


	void string_last_index_of(const fn_call& fn)
	{
		const tu_string& sstr = fn.this_value.to_tu_string();

		if (fn.nargs < 1)
		{
			fn.result->set_double(-1);
		} else {
			int	start_index = 0;
			if (fn.nargs > 1)
			{
				start_index = fn.arg(1).to_int();
			}
			const char* str = sstr.c_str();
			const char* last_hit = NULL;
			const char* haystack = str;
			for (;;) {
				const char*	p = strstr(haystack, fn.arg(0).to_string());
				if (p == NULL || (start_index !=0 && p > str + start_index ) )	// FIXME: not UTF-8 correct!
				{
					break;
				}
				last_hit = p;
				haystack = p + 1;
			}
			if (last_hit == NULL) {
				fn.result->set_double(-1);
			} else {
				fn.result->set_double(tu_string::utf8_char_count(str, (int) (last_hit - str)));
			}
		}
	}
	
	void string_slice(const fn_call& fn)
	{
		const tu_string& this_str = fn.this_value.to_tu_string();

		int len = this_str.utf8_length();
		int start = 0;
		if (fn.nargs >= 1) 
		{
			start = fn.arg(0).to_int();
			if (start < 0)
			{
				start = len + start;
			}
		}
		int end = len;
		if (fn.nargs >= 2)
		{
			end = fn.arg(1).to_int();
			if (end < 0)
			{
				end = len + end;
			}
		}

		start = iclamp(start, 0, len);
		end = iclamp(end, start, len);

		fn.result->set_tu_string(this_str.utf8_substring(start, end));
	}

	void string_split(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		if (obj && fn.nargs > 0)
		{
			const tu_string& delimiter = fn.arg(0).to_tu_string();

			array<tu_string> a;
			obj->to_tu_string().split(delimiter, &a);

			as_array* res = new as_array();
			for (int i = 0; i < a.size(); i++)
			{
				res->push(a[i].c_str());
			}

			fn.result->set_as_object(res);
		}
	}

	// public substr(start:Number, length:Number) : String
	void string_substr(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		if (obj && fn.nargs > 0)
		{
			fn.result->set_tu_string(obj->utf8_substr(fn.arg(0).to_int(), fn.nargs > 1 ? fn.arg(1).to_int() : -1));
		}
	}

	// start:Number — An integer that indicates the position of the first character of my_str used to create the substring. 
	// Valid values for start are 0 through String.length - 1. If start is a negative value, 0 is used.
	// end:Number — An integer that is 1+ the index of the last character in my_str to be extracted. 
	// Valid values for end are 1 through String.length. The character indexed by the end parameter is not included in the extracted string.
	// If this parameter is omitted, String.length is used. If this parameter is a negative value, 0 is used. 
	void string_substring(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		if (obj && fn.nargs > 0)
		{
			int	size = obj->utf8_length();

			int	start = fn.arg(0).to_int();
			if (start < 0) start = 0;
			if (start > size - 1) return;

			int	end = size;
			if (fn.nargs > 1)
			{
				end = fn.arg(1).to_int();
			}
			if (end < 0) end = 0;
			if (end > size) return;

			assert(end >= start);

			fn.result->set_tu_string(obj->utf8_substr(fn.arg(0).to_int(), end - start));
		}
	}

	void string_to_lowercase(const fn_call& fn)
	{
		const tu_string& this_str = fn.this_value.to_tu_string();
		fn.result->set_tu_string(this_str.utf8_to_lower());
	}
	
	void string_to_uppercase(const fn_call& fn) 
	{
		const tu_string& this_str = fn.this_value.to_tu_string();
		fn.result->set_tu_string(this_str.utf8_to_upper());
	}

	void string_char_at(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		if (obj && fn.nargs > 0)
		{
			int	index = fn.arg(0).to_int();

			// Two bytes.
			Uint16 ch = obj->utf8_char_code_at(index);
			fn.result->set_tu_string(tu_string(ch));
		}
	}
	
	void string_to_string(const fn_call& fn)
	{
		const tu_string& str = fn.this_value.to_tu_string();
		fn.result->set_tu_string(str);
	}

	void string_length(const fn_call& fn)
	{
		as_string* obj = cast_to<as_string>(fn.this_value.to_object_interface());
		if (obj)
		{
			fn.result->set_int(obj->utf8_length());
		}
	}

	void as_global_string_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.result->set_string(fn.arg(0).to_string());
		}	
		else
		{
			fn.result->set_string("");
		}
	}

	as_object * get_global_string_ctor()
	{
		as_object * string = new as_c_function(as_global_string_ctor);

		string->builtin_member( "fromCharCode", string_from_char_code );
		//string->builtin_member( "charCodeAt", string_char_at );

		return string;
	}

} // namespace bakeinflash
