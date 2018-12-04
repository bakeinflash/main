
#include "bakeinflash/bakeinflash_string_object.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_as_classes/as_string.h"
#include "base/utf8.h"

namespace bakeinflash
{
	bool string_to_number(double* result, const char* str);

	as_string::as_string() :
		m_utf8_size(0),
		m_utf8(NULL)
	{
	}

	as_string::as_string(const char* str) :
		m_string(str),
		m_utf8_size(0),
		m_utf8(NULL)
	{
	}

	as_string::as_string(const tu_string& str) :
		m_string(str),
		m_utf8_size(0),
		m_utf8(NULL)
	{
	}
	
	as_string::~as_string()
	{
		if (m_utf8)
		{
			delete m_utf8;
		}
	}
	
	const char*	as_string::to_string()
	{
		return m_string.c_str(); 
	}
	
	double	as_string::to_number()
	{
		double val;
		if (! string_to_number(&val, m_string.c_str()))
		{
			// Failed conversion to Number.
			val = get_nan();
		}
		return val;
	}
		
	bool as_string::to_bool()
	{
		return m_string.size() > 0 ? true : false;
	}
	
	const char*	as_string::type_of()
	{
		return "string"; 
	}
		
	bool as_string::is_instance_of(const as_function* constructor) const
	{
		const as_c_function* func = cast_to<as_c_function>(constructor);
		if (func)
		{
			return (func->m_func == as_global_string_ctor) | (func->m_func == as_global_object_ctor);
		}
		return false;
	}

	bool	as_string::get_member(const tu_string& name, as_value* val)
	{
		return get_builtin(get_root()->is_as3() ? BUILTIN_STRING_METHOD_AS3 : BUILTIN_STRING_METHOD, name, val);
	}

	void as_string::utf8_update()
	{
		if (!m_string.get_updated_flag())
		{
			m_string.set_updated_flag();

			m_utf8_size = 0;
			const char*	buf = m_string.c_str();
			const char*	p = buf;
			int buflen = m_string.size();

			delete m_utf8;
			m_utf8 = (Uint16*) malloc(sizeof(Uint16) * buflen);

			while (p - buf < buflen)
			{
				uint32	c = utf8::decode_next_unicode_character(&p);
				if (c == 0)
				{
					break;
				}
				m_utf8[m_utf8_size] = c;
				m_utf8_size++;
			}
		}
	}

	int as_string::utf8_length()
	{
		utf8_update();
		return m_utf8_size;
	}

	tu_string as_string::utf8_substr(int start, int len)
	{
		utf8_update();

		start = iclamp(start, 0, m_utf8_size);
		len = len == -1 ? m_utf8_size : iclamp(len, 0, m_utf8_size);
		int end = iclamp(start + len, 0, m_utf8_size);

		tu_string str;
		for (int i = start; i < end; i++)
		{
			str += m_utf8[i];
		}
		return str;
	}

	Uint16 as_string::utf8_char_code_at(int index)
	{
		utf8_update();

		if (index >= 0 && index < m_utf8_size)
		{
			return m_utf8[index];
		}
		return 0xFFFF;	// invalid
	}


}
