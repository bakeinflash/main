// bakeinflash_value.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.


#ifndef BAKEINFLASH_VALUE_H
#define BAKEINFLASH_VALUE_H

#include "base/container.h"
#include "bakeinflash/bakeinflash.h"	// for ref_counted

namespace bakeinflash
{
	struct fn_call;
	struct as_c_function;
	struct as_function;
	struct as_object;
	struct as_environment;

	void parse(const tu_string& str, string_hash<as_value>* map);
	bool string_to_number(int64* result, const char* str, int base = 10);
	bool string_to_number(double* result, const char* str);
	as_value string_to_value(const char* str);

	typedef void (*as_c_function_ptr)(const fn_call& fn);

	// helper, used in as_value
	struct as_property : public as_object_interface
	{
		// Unique id of a bakeinflash resource
		enum	{ m_class_id = AS_PROPERTY };
		 virtual bool is(int class_id) const
		{
			return m_class_id == class_id;
		}

		as_property(const as_value& getter,	const as_value& setter);
		virtual ~as_property();
	
		void	set(as_object* target, const as_value& val);
		void	get(as_object* target, as_value* val) const;
		void	get(const as_value& primitive, as_value* val) const;
		void	set_getter(as_object* getter);
		void	set_setter(as_object* setter);

		virtual const char*	to_string();
		virtual const tu_string&	to_tu_string();
		virtual double	to_number();
		virtual bool to_bool();
		virtual bool is_property() const { return true; }
		virtual const char*	type_of() { return "property"; }

	private:

		smart_ptr<as_function>	m_getter;
		smart_ptr<as_function>	m_setter;

	};

	struct as_value
	{
		// flags defining the level of protection of a value
		enum value_flag
		{
			DONT_ENUM = 0x01,
			DONT_DELETE = 0x02,
			READ_ONLY = 0x04
		};

		private:

		enum type
		{
			UNDEFINED,
			BOOLEAN,
			NUMBER,
			OBJECT
		};
		type	m_type;

		// Numeric flags
		mutable int m_flags;
		mutable tu_string* m_string;
		union
		{
			double m_number;
			bool m_bool;
			as_object_interface* m_object;
		};

	public:

		// constructors
		 as_value();
		 as_value(const as_value& v);
		 as_value(const char* str);
		 as_value(bool val);
		 as_value(int val);
		 as_value(float val);
		 as_value(double val);
		 as_value(as_object_interface* obj);
		 as_value(as_c_function_ptr func);
		 as_value(as_s_function* func);
		 as_value(const as_value& getter, const as_value& setter);

		~as_value();

		// Useful when changing types/values.
		 void	drop_refs();

		 const char*	to_string() const;
		 const tu_string&	to_tu_string() const;
		 double	to_number() const;
		 int	to_int() const { return (int) to_number(); };
		 float	to_float() const { return (float) to_number(); };
		 bool	to_bool() const;
		 as_function*	to_function() const;
		 as_object*	to_object() const;
		 as_object_interface*	to_object_interface() const;
		 as_property*	to_property() const;

		// These set_*()'s are more type-safe; should be used
		// in preference to generic overloaded set().  You are
		// more likely to get a warning/error if misused.
		 void	set_tu_string(const tu_string& str);
		 void	set_string(const char* str);
		 void	set_double(double val);
		 void	set_bool(bool val);
		 void	set_int(int val) { set_double(val); }
		 void	set_nan() { set_double(get_nan()); }
		 void	set_as_object(as_object_interface* obj);
		 void	set_as_c_function(as_c_function_ptr func);
		 void	set_undefined() { drop_refs(); m_type = UNDEFINED; }
		 void	set_null() { set_as_object(NULL); }

		void	set_property(as_object* this_ptr, const as_value& val);
		void	get_property(as_object* this_ptr, as_value* val) const;
		void	get_property(const as_value& primitive, as_value* val) const;

		 void	operator=(const as_value& v);
		 bool	operator==(const as_value& v) const;
		 bool	operator!=(const as_value& v) const;
		 bool	operator<(double v) const { return to_number() < v; }
		 void	operator+=(double v) { set_double(to_number() + v); }
		 void	operator-=(double v) { set_double(to_number() - v); }
		 void	operator*=(double v) { set_double(to_number() * v); }
		 void	operator/=(double v) { set_double(to_number() / v); }  // @@ check for div/0
		 void	operator&=(int v);
		 void	operator|=(int v);
		 void	operator^=(int v);
		 void	shl(int v);
		 void	asr(int v);
		 void	lsr(int v);

		bool is_function() const;
		bool is_bool() const;
		bool is_string() const;
		bool is_number() const;
		bool is_object() const;
		bool is_property() const;
		bool is_null() const;
		bool is_undefined() const;

		const char* type_of() const;
		bool is_instance_of(const as_function* constructor) const;
		bool find_property(const tu_string& name, as_value* val);
		bool get_property_owner(const tu_string& name, as_value* val);

		// flags
		inline bool is_enum() const { return m_flags & DONT_ENUM ? false : true; }
		inline bool is_readonly() const { return m_flags & READ_ONLY ? true : false; }
		inline bool is_protected() const { return m_flags & DONT_DELETE ? true : false; }
		inline int get_flags() const { return m_flags; }
		inline void set_flags(int flags) const  { m_flags = flags; }

		void print(const char* name, int tabs = 0) const;

	};

}


#endif
