// bakeinflash_environment.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript "environment", essentially VM state?

#ifndef BAKEINFLASH_ENVIRONMENT_H
#define BAKEINFLASH_ENVIRONMENT_H

#include "base/smart_ptr.h"
#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_value.h"

namespace bakeinflash
{

	#define GLOBAL_REGISTER_COUNT 4

	struct character;
	struct sprite_instance;

	 tu_string get_full_url(const tu_string& workdir, const char* url);

	//
	// with_stack_entry
	//
	// The "with" stack is for Pascal-like with-scoping.

	struct with_stack_entry
	{
		smart_ptr<as_object>	m_object;
		int	m_block_end_pc;
		
		with_stack_entry() :
			m_object(NULL),
			m_block_end_pc(0)
		{
		}

		with_stack_entry(as_object* obj, int end)	:
			m_object(obj),
			m_block_end_pc(end)
		{
		}
	};

	// stack access/manipulation
	// @@ TODO do more checking on these
	struct vm_stack : private array<as_value>
	{
		vm_stack() :
			m_stack_size(0)
		{
		}

		inline as_value&	operator[](int index) 
		{
//			assert(index >= 0 && index < m_stack_size);
			return array<as_value>::operator[](index);
		}

		inline const as_value&	operator[](int index) const 
		{
//			assert(index >= 0 && index < m_stack_size);
			return array<as_value>::operator[](index);
		}

		void reset(const as_value& val)
		{
			(*this)[m_stack_size] = val;
		}

		void reset(const char* val)
		{
			(*this)[m_stack_size].set_string(val);
		}

//		void reset(const wchar_t* val)
//		{
//			(*this)[m_stack_size] = as_value(val);
//		}

		void reset(bool val)
		{
			(*this)[m_stack_size].set_bool(val);
		}

		void reset(int val)
		{
			(*this)[m_stack_size].set_int(val);
		}

		void reset(float val)
		{
			(*this)[m_stack_size].set_double(val);
		}

		void reset(double val)
		{
			(*this)[m_stack_size].set_double(val);
		}

		void reset(as_object_interface* val)
		{
			(*this)[m_stack_size].set_as_object(val);
		}

		void	push(const as_value& val); 

		as_value&	pop();
		 void	drop(int count);
		as_value&	top(int dist) { return (*this)[m_stack_size - 1 - dist]; }
		as_value&	bottom(int index) { return (*this)[index]; }
		inline int	get_top_index() const { return m_stack_size - 1; }
		inline int	size() const { return m_stack_size; }
		void resize(int new_size);
		void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);

		// get value of property
		as_object* get_property_owner(const tu_string& name);
		bool get_property(const tu_string& name, as_value* val);
		void dump();		// for debuging


	private:

		int m_stack_size;
	};

	struct as_environment : public vm_stack
	{
		as_value	m_global_register[GLOBAL_REGISTER_COUNT];
		array<as_value>	m_local_register;	// function2 uses this
		smart_ptr<as_object>	m_target;

		// For local vars.  Use empty names to separate frames.
		array< string_hash<as_value>* >	m_local_frames;

		as_environment();
		virtual ~as_environment();

		bool	set_member(const tu_string& name, const as_value& val);
		bool	get_member(const tu_string& name, as_value* val);

		int get_stack_size() const { return size(); }
		void set_stack_size(int n) { resize(n); }

		as_object*	get_target_object() const { return m_target.get(); };
		character*	get_target() const;
		void set_target(as_object* target);

		void set_target(as_value& target, character* original_target);

		bool	get_variable(const tu_string& varname, as_value* val, const array<with_stack_entry>& with_stack) const;
		// no path stuff:
		bool	get_variable_raw(const tu_string& varname, as_value* val, const array<with_stack_entry>& with_stack) const;

		void	set_variable(const tu_string& path, const as_value& val, const array<with_stack_entry>& with_stack);
		// no path stuff:
		void	set_variable_raw(const tu_string& path, const as_value& val, const array<with_stack_entry>& with_stack);

		bool	get_local(const tu_string& name, as_value* val) const;
		bool	set_local(const tu_string& varname, const as_value& val);
		bool	update_local(const tu_string& varname, const as_value& val);
		void	declare_local(const tu_string& varname);	// Declare varname; undefined unless it already exists.

		// Local registers.
		void	add_local_registers(int register_count)
		{
			// Flash 8 can have zero register (+1 for zero)
			m_local_register.resize(m_local_register.size() + register_count + 1);
		}
		void	drop_local_registers(int register_count)
		{
			// Flash 8 can have zero register (-1 for zero)
			m_local_register.resize(m_local_register.size() - register_count - 1);
		}

		as_value* get_register(int reg);
		void set_register(int reg, const as_value& val);

		// may be used in outside of class instance
		static bool	parse_path(const tu_string& var_path, tu_string* path, tu_string* var);

		// Internal.
		character* load_file(const char* url, const as_value& target, int method = 0);
		as_object*	find_target(const as_value& target) const;
		void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);

		// swap stack items
		void swap(int n);

		private:

		as_value*	local_register_ptr(int reg);

	};


	// Parameters/environment for C functions callable from ActionScript.
	struct fn_call
	{
		as_value* result;
		as_object* this_ptr;
		const as_value& this_value;	// Number or String or Boolean value
		as_environment* env;
		int nargs;
		int first_arg_bottom_index;

		fn_call(as_value* res_in, const as_value& this_in, as_environment* env_in, int nargs_in, int first_in) :
			result(res_in),
			this_value(this_in),
			env(env_in),
			nargs(nargs_in),
			first_arg_bottom_index(first_in)
		{
			this_ptr = this_in.to_object();
		}

		as_value& arg(int n) const
		// Access a particular argument.
		{
			assert(n < nargs);
			return env->bottom(first_arg_bottom_index - n);
		}

	};

}

#endif
