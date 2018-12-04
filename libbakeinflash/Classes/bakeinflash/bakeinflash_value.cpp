// bakeinflash_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.

#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_movie_def.h"
#include "bakeinflash/bakeinflash_string_object.h"
#include "bakeinflash/bakeinflash_as_classes/as_number.h"
#include "bakeinflash/bakeinflash_as_classes/as_boolean.h"
#include "bakeinflash/bakeinflash_as_classes/as_string.h"
#include <float.h>

namespace bakeinflash
{

	// parse request
	void parse(const tu_string& str, string_hash<as_value>* map)
	{
		array<tu_string> pairs;
		str.split('&', &pairs);
		for (int k = 0; k < pairs.size(); k++)
		{
			array<tu_string> val;
			pairs[k].split('=', &val);

			if (val.size() == 2)
			{
				map->set(val[0], val[1].c_str());
			}
			else
				if (val.size() == 1)
				{
					map->set(val[0], "");	// no arg value
				}
		}
	}

	bool string_to_number(int64* result, const char* str, int base)
	// Utility.  Try to convert str to a number.  If successful,
	// put the result in *result, and return true.  If not
	// successful, put 0 in *result, and return false.
	{
		// spec case
		if (base == 2)
		{
			int n = strlen(str);
			if (n > 64 || n < 1)
			{
				return false;
			}

			*result = 0;
			uint64 b = 1;
			for (const char* p = str + n - 1; p >= str; p--)
			{
				*result |= (*p == '1') ? b : 0;
				b <<= 1;
			}
			return true;
		}

		char* tail = 0;
		*result = strtol(str, &tail, base);
		if (tail == str || *tail != 0)
		{
			// Failed conversion to Number.
			return false;
		}
		return true;
	}

	bool string_to_number(double* result, const char* str)
	// Utility.  Try to convert str to a number.  If successful,
	// put the result in *result, and return true.  If not
	// successful, put 0 in *result, and return false.
	{
		char* tail = 0;
		*result = strtod(str, &tail);
		if (tail == str || *tail != 0)
		{
			// Failed conversion to Number.
			return false;
		}
		return true;
	}

	as_value string_to_value(const char* str)
	{
		double res;
		if (string_to_number(&res, str))
		{
			return as_value(res);
		}
		return as_value(str);
	}

	as_value::as_value(as_object_interface* obj) :
		m_type(OBJECT),
		m_flags(0),
		m_object(obj),
		m_string(NULL)
	{
		if (m_object)
		{
			m_object->add_ref();
		}
	}


	as_value::as_value(as_s_function* func)	:
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_as_object(func);
	}

	as_value::as_value(const as_value& getter, const as_value& setter) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_as_object(new as_property(getter, setter));
	}

	const char*	as_value::to_string() const
	// Conversion to string.
	{
		return to_tu_string().c_str();
	}

	const tu_string&	as_value::to_tu_string() const
	// Conversion to const tu_string&.
	{
		switch (m_type)
		{
			case UNDEFINED:
			{
				static tu_string s_str_undefined("undefined");
				return s_str_undefined;
			}

			case BOOLEAN:
			{
				static tu_string s_str_true("true");
				static tu_string s_str_false("false");
				return m_bool ? s_str_true : s_str_false;
			}

			case NUMBER:
				// @@ Moock says if value is a NAN, then result is "NaN"
				// INF goes to "Infinity"
				// -INF goes to "-Infinity"
				if (isnan(m_number))
				{
					static tu_string s_str_nan("NaN");
					return s_str_nan;
				} 
				else
				{
					char buffer[50];
					snprintf(buffer, 50, "%.14g", m_number);
					if (m_string == NULL)
					{
						m_string = new tu_string();
					}
					*m_string = buffer;
				}
				break;

			case OBJECT:
			{
				// Moock says, "the value that results from
				// calling toString() on the object".
				//
				// The default toString() returns "[object
				// Object]" but may be customized.
				if (m_object)
				{
					return m_object->to_tu_string();
				}
				static tu_string s_str_null("null");
				return s_str_null;
			}
	
			default:
				assert(0);
		}
		return *m_string;
	}

	double	as_value::to_number() const
	// Conversion to double.
	{
		switch (m_type)
		{
			case NUMBER:
				return m_number;

			case BOOLEAN:
				return m_bool ? 1 : 0;

			case OBJECT:
				if (m_object)
				{
					return m_object->to_number();
				}
	 			// Evan: from my tests
				return get_nan();
	
			case UNDEFINED:
			{
				return get_nan();
			}

			default:
				return 0.0;
		}
	}


	bool	as_value::to_bool() const
	// Conversion to boolean.
	{
		switch (m_type)
		{
			case OBJECT:
				return m_object ? m_object->to_bool() : false;

			case NUMBER:
				return m_number != 0;

			case BOOLEAN:
				return m_bool;

			case UNDEFINED:
				return false;

			default:
				assert(0);
		}
		return false;
	}

	
	as_object*	as_value::to_object() const
	// Return value as an object.
	{
		switch (m_type)
		{
			case OBJECT:
				return cast_to<as_object>(m_object);
			default:
				break;
		}
		return NULL;
	}

	as_property* as_value::to_property() const
	{
		switch (m_type)
		{
			case OBJECT:
				return cast_to<as_property>(m_object);
			default:
				break;
		}
		return NULL;
	}

	as_object_interface*	as_value::to_object_interface() const
	// Return value as an object.
	{
		switch (m_type)
		{
			case OBJECT:
				return m_object;
			default:
				break;
		}
		return NULL;
	}

	as_function*	as_value::to_function() const
	// Return value as an function.
	{
		switch (m_type)
		{
			case OBJECT:
				return cast_to<as_function>(m_object);
			default:
				break;
		}
		return NULL;
	}

	void	as_value::set_as_object(as_object_interface* obj)
	{
		if (m_type != OBJECT || m_object != obj)
		{
			drop_refs();
			m_type = OBJECT;
			m_object = obj;
			if (m_object)
			{
				m_object->add_ref();
			}
		}
	}

	void	as_value::operator=(const as_value& v)
	{
		switch (v.m_type)
		{
			case UNDEFINED:
				set_undefined();
				break;

			case NUMBER:
				set_double(v.m_number);
				m_flags = v.m_flags;
				break;

			case BOOLEAN:
				set_bool(v.m_bool);
				m_flags = v.m_flags;
				break;

			case OBJECT:
				set_as_object(v.m_object);
				m_flags = v.m_flags;
				break;

			default:
				assert(0);
		}
	}

	bool	as_value::operator==(const as_value& v) const
	// Return true if operands are equal.
	{
		if ((is_undefined() && v.is_null()) || (is_null() && v.is_undefined()))
		{
			return true;
		}

		switch (m_type)
		{
			case UNDEFINED:
				return v.m_type == UNDEFINED;

			case NUMBER:
				return m_number == v.to_number();

			case BOOLEAN:
				return m_bool == v.to_bool();

			case OBJECT:
				if (m_object == v.to_object_interface())
				{
					return true;
				}
				if (is_string())
				{
					return to_tu_string() == v.to_tu_string();
				}
				return false;

			default:
				assert(0);
				return false;
		}
	}

	
	bool	as_value::operator!=(const as_value& v) const
	// Return true if operands are not equal.
	{
		return ! (*this == v);
	}

	void	as_value::drop_refs()
	// Drop any ref counts we have; this happens prior to changing our value.
	{
		m_flags = 0;
		if (m_type == OBJECT && m_object)
		{
			m_object->drop_ref();
			m_object = NULL;
			return;
		}
	}

	void	as_value::set_property(as_object* this_ptr, const as_value& val)
	{
		as_property* prop = to_property();
		assert(prop);
		prop->set(this_ptr, val);
	}

	// get property of primitive value, like Number
	void as_value::get_property(const as_value& primitive, as_value* val) const
	{
		as_property* prop = to_property();
		assert(prop);
		prop->get(primitive, val);
	}

	void as_value::get_property(as_object* this_ptr, as_value* val) const
	{
		as_property* prop = to_property();
		assert(prop);
		prop->get(this_ptr, val);
	}

	as_value::as_value(float val) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_double(val);
	}

	as_value::as_value(int val) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_double(val);
	}

	as_value::as_value(double val) :
		m_type(NUMBER),
		m_number(val),
		m_flags(0),
		m_string(NULL)
	{
	}

	void	as_value::set_double(double val)
	{
		drop_refs();
		m_type = NUMBER;
		m_number = val;
	}

	as_value::as_value(bool val) :
		m_type(BOOLEAN),
		m_bool(val),
		m_flags(0),
		m_string(NULL)
	{
	}

	void	as_value::set_bool(bool val)
	{
		drop_refs();
		m_type = BOOLEAN;
		m_bool = val;
	}


	bool as_value::is_function() const
	{
		if (m_type == OBJECT)
		{
			return cast_to<as_function>(m_object) ? true : false;
		}
		return false;
	}

	as_value::as_value(as_c_function_ptr func) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_as_c_function(func);
	}

	void	as_value::set_as_c_function(as_c_function_ptr func)
	{
		// c_function object has no pointer to player instance
		set_as_object(new as_c_function(func));
	}

		
	bool as_value::is_instance_of(const as_function* constructor) const
	{
		switch (m_type)
		{
			case UNDEFINED:
				break;

			case NUMBER:
			{
				const as_c_function* func = cast_to<as_c_function>(constructor);
				if (func)
				{
					return 
						(func->m_func == as_global_number_ctor) |
						(func->m_func == as_global_object_ctor);
				}
				break;
			}

			case BOOLEAN:
			{
				const as_c_function* func = cast_to<as_c_function>(constructor);
				if (func)
				{
					return 
						(func->m_func == as_global_boolean_ctor) |
						(func->m_func == as_global_object_ctor);
				}
				break;
			}

			case OBJECT:
				if (m_object)
				{
					return m_object->is_instance_of(constructor);
				}
				break;

			default:
				break;

		}
		return false;
	}

	const char*	as_value::type_of() const
	{
		switch (m_type)
		{
			case UNDEFINED:
				return "undefined";

			case NUMBER:
				return "number";

			case BOOLEAN:
				return "boolean";

			case OBJECT:
				if (m_object)
				{
					return m_object->type_of();
				}
				return "null";


			default:
				assert(0);

		}
		return 0;
	}


	bool as_value::find_property( const tu_string& name, as_value* val)
	{
		switch (m_type)
		{
			default:
				break;

			case NUMBER:
			{
				return get_builtin(get_root()->is_as3() ? BUILTIN_NUMBER_METHOD_AS3 : BUILTIN_NUMBER_METHOD, name, val);
			}

			case BOOLEAN:
			{
				return get_builtin(get_root()->is_as3() ? BUILTIN_BOOLEAN_METHOD_AS3 : BUILTIN_BOOLEAN_METHOD, name, val);
			}

			case OBJECT:
			{
				if (m_object)
				{
					return m_object->get_member(name, val);
				}
			}
		}
		return false;
	}


	bool as_value::get_property_owner(const tu_string& name, as_value* val)
	{
		switch (m_type)
		{
			default:
				break;

			case NUMBER:
				{
					if (get_builtin(get_root()->is_as3() ? BUILTIN_NUMBER_METHOD_AS3 : BUILTIN_NUMBER_METHOD, name, NULL))
					{
						*val = *this;
					}
					else
					{
						return false;
					}
				}

			case BOOLEAN:
				{
					if (get_builtin(get_root()->is_as3() ? BUILTIN_BOOLEAN_METHOD_AS3 : BUILTIN_BOOLEAN_METHOD, name, NULL))
					{
						*val = *this;
					}
					else
					{
						return false;
					}
				}

			case OBJECT:
				{
					if (m_object)
					{
						return m_object->find_property(name, val);
					}
				}
		}
		return false;
	}

	void	as_value::set_tu_string(const tu_string& str)
	{
		set_as_object(new as_string(str));
	}
	
	void	as_value::set_string(const char* str)
	{
		set_as_object(new as_string(str));
	}
	
	as_value::as_value(const char* str) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		set_as_object(new as_string(str));
	}

	as_value::as_value() :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
	}

	as_value::as_value(const as_value& v) :
		m_type(UNDEFINED),
		m_flags(0),
		m_string(NULL)
	{
		*this = v;
	}

	as_value::~as_value()
	{
		drop_refs(); 
		delete m_string;
	}

	void as_value::print(const char* name, int tabs) const
	{
		tu_string tab;
		for (int i = 0; i < tabs; i++)
		{
			tab += ' ';
		}

		if (is_property())
		{
			myprintf("%s%s: <as_property %p>\n", tab.c_str(), name, to_property());
		}
		else
		if (is_function())
		{
			myprintf("%s%s: %s\n", tab.c_str(), name, to_object()->to_string());
		}
		else
		if (is_object())
		{
			myprintf("%s%s: %s\n", tab.c_str(), name, to_object()->to_string());
			to_object()->dump(tabs);
		}
		else
		{
			myprintf("%s%s: %s\n", tab.c_str(), name, to_string());
		}
	}

	void	as_value::shl(int v)
	{
		double d = to_number();
		int val = isnan(d) ? 0 : int(d); 
		set_int(val << v); 
	}
	void	as_value::asr(int v) 
	{
		double d = to_number();
		int val = isnan(d) ? 0 : int(d); 
		set_int(val >> v); 
	}
	void	as_value::lsr(int v)
	{
		double d = to_number();
		Uint32 val = isnan(d) ? 0 : Uint32(d); 
		set_int(val >> v); 
	}
	void	as_value::operator&=(int v)
	{
		double d = to_number();
		Uint32 val = isnan(d) ? 0 : Uint32(d); 
		set_int(val & v); 
	}
	void	as_value::operator|=(int v) 
	{
		double d = to_number();
		Uint32 val = isnan(d) ? 0 : Uint32(d); 
		set_int(val | v); 
	}
	void	as_value::operator^=(int v)
	{
		double d = to_number();
		Uint32 val = isnan(d) ? 0 : Uint32(d); 
		set_int(val ^ v); 
	}


	bool as_value::is_bool() const
	{
		return m_type == BOOLEAN; 
	}
	
	bool as_value::is_string() const
	{
		return m_type == OBJECT && m_object && m_object->is_string(); 
	}
	
	bool as_value::is_number() const
	{
		return m_type == NUMBER && isnan(m_number) == false; 
	}
	
	bool as_value::is_object() const
	{
		return m_type == OBJECT && m_object && m_object->is_object();
	}

	bool as_value::is_property() const
	{
		return m_type == OBJECT && m_object && m_object->is_property();
	}
	
	bool as_value::is_null() const
	{
		return m_type == OBJECT && m_object == NULL; 
	}
	
	bool as_value::is_undefined() const
	{
		return m_type == UNDEFINED; 
	}

	//
	//	as_property
	//

	as_property::as_property(const as_value& getter,	const as_value& setter)
	{
		m_getter = cast_to<as_function>(getter.to_object());
		m_setter = cast_to<as_function>(setter.to_object());
	}

	as_property::~as_property()
	{
	}

	void	as_property::set_getter(as_object* getter)
	{
		m_getter = cast_to<as_function>(getter);
	}

	void	as_property::set_setter(as_object* setter)
	{
		m_setter = cast_to<as_function>(setter);
	}

	void	as_property::set(as_object* target, const as_value& val)
	{
		if (target != NULL)
		{
			as_environment env;
			env.push(val);
			if (m_setter != NULL)
			{
				smart_ptr<as_object> tar = target;
				(*m_setter.get())(fn_call(NULL, tar.get(),	&env, 1, env.get_top_index()));
			}
		}
	}

	void as_property::get(as_object* target, as_value* val) const
	{
		if (target == NULL)
		{
			val->set_undefined();
			return;
		}

		// env is used when m_getter->m_env is NULL
		as_environment env;
		if (m_getter != NULL)
		{
			smart_ptr<as_object> tar = target;
			(*m_getter.get())(fn_call(val, tar.get(), &env, 0,	0));
		}
	}

	// call static method
	void as_property::get(const as_value& primitive, as_value* val) const
	{
		if (m_getter != NULL)
		{
			(*m_getter.get())(fn_call(val, primitive, NULL, 0,	0));
		}
	}

	const char*	as_property::to_string()
	{
		return to_tu_string().c_str();
	}

	const tu_string&	as_property::to_tu_string()
	{
		static tu_string s_string("[property]"); 
		return s_string; 
	}

	double	as_property::to_number()
	{
		return get_nan();
	}

	bool as_property::to_bool()
	{
		return false;
	}

}
