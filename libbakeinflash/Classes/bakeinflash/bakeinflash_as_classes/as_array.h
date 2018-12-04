// array.h	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_ARRAY_H
#define BAKEINFLASH_AS_ARRAY_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_function.h"

namespace bakeinflash
{

	// constructor of an Array object
	void	as_global_array_ctor(const fn_call& fn);

	// this is an Array object
	struct as_array : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_ARRAY };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		 virtual bool	get_member(const tu_string& name, as_value* val);
		 virtual bool	set_member(const tu_string& name, const as_value& val);
		virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
		 virtual	void enumerate(as_environment* env);

		// Basic array access.
		as_value&	operator[](int index) { assert(index >= 0 && index < size()); return m_array[index]; }
		const as_value&	operator[](int index) const { assert(index >= 0 && index < size()); return m_array[index]; }

		 as_array();
		 virtual const char* to_string();
		virtual const tu_string&	to_tu_string();
		 void push(const as_value& val) { m_array.push_back(val); }
		 void remove(int index) { m_array.remove(index); }
		 void insert(int index, const as_value& val)	{ m_array.insert(index, val); }
		 void sort(int options, as_function* compare_function);
		 int size() const { return m_array.size(); }
		 void resize(int size) { m_array.resize(size); }

		 virtual void dump(int tabs = 0);
		 virtual void this_alive();

		tu_string m_string_value;
		array<as_value> m_array;
	};

	// this is "_global.Array" object
	struct as_global_array : public as_c_function
	{
		enum option
		{
			CASEINSENSITIVE = 1,
			DESCENDING = 2,
			UNIQUESORT = 4,
			RETURNINDEXEDARRAY = 8,
			NUMERIC = 16
		};

		as_global_array();
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_ARRAY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
