// bakeinflash_object.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#ifndef BAKEINFLASH_STRING_OBJECT_H
#define BAKEINFLASH_STRING_OBJECT_H

#include "bakeinflash.h"

namespace bakeinflash
{

	struct as_string : public as_object_interface
	{
		// Unique id of a bakeinflash resource
		enum	{ m_class_id = AS_STRING };
		 virtual bool is(int class_id) const
		{
			return m_class_id == class_id;
		}

		as_string();
		as_string(const char* str);
		as_string(const tu_string& str);
		virtual ~as_string();

		virtual const char*	to_string();
		virtual double	to_number();
		virtual bool to_bool();
		virtual const char*	type_of();
		virtual bool is_instance_of(const as_function* constructor) const;
		virtual const tu_string&	to_tu_string() { return m_string; }

		virtual bool is_string() const { return true; }
		virtual bool get_member(const tu_string& name, as_value* val);

		void utf8_update();
		int utf8_length();
		tu_string utf8_substr(int start, int len);
		Uint16 utf8_char_code_at(int index);

		tu_string m_string;
		int m_utf8_size;
		Uint16* m_utf8;
	};

}

#endif
