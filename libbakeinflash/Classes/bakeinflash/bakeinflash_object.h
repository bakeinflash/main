// bakeinflash_object.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#ifndef BAKEINFLASH_OBJECT_H
#define BAKEINFLASH_OBJECT_H

#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_environment.h"
#include "bakeinflash/bakeinflash_types.h"
#include "base/container.h"
#include "base/smart_ptr.h"
#include "base/tu_loadlib.h"

namespace bakeinflash
{
	void	as_object_addproperty(const fn_call& fn);
	void	as_object_registerclass( const fn_call& fn );
	void	as_object_hasownproperty(const fn_call& fn);
	void	as_object_watch(const fn_call& fn);
	void	as_object_unwatch(const fn_call& fn);
	void	as_object_dump(const fn_call& fn);
	void	as_global_object_ctor(const fn_call& fn);

	struct instance_info; 
	struct as_array; 

	struct as_object : public as_object_interface
	{
		// Unique id of a bakeinflash resource
		enum	{ m_class_id = AS_OBJECT };
		virtual bool is(int class_id) const
		{
			return m_class_id == class_id;
		}

		string_hash<as_value>	m_members;

		// It is used to register an event handler to be invoked when
		// a specified property of object changes.
		// TODO: create container based on string_hash<as_value>
		// watch should be coomon
		struct as_watch
		{
			as_watch() :	m_func(NULL)
			{
			}

			as_function* m_func;
			as_value m_user_data;
		};

		// primitive data type has no dynamic members
		string_hash<as_watch>*	m_watch;

		// it's used for passing new created object pointer to constructors chain
		weak_ptr<as_object> m_this_ptr;

		// We can place reference to __proto__ into members but it's used very often
		// so for optimization we place it into instance
		smart_ptr<as_object> m_proto;	// for optimization

		// for optimization
		enum on_mouse_events
		{
			onKeyPress = 0x01,
			onRelease = 0x02,
			onDragOver = 0x04,
			onDragOut = 0x08,
			onPress = 0x10,
			onReleaseOutside = 0x20,
			onRollout = 0x40,
			onRollover = 0x80,
			onMouseMove = 0x0100
		};
		Uint32	m_on_mouse_event;	// mouse events


		as_object();
		virtual ~as_object();

		virtual bool is_object() const { return true; }

		//		 virtual const char*	to_string() { return "[object Object]"; }
		//		 virtual double	to_number();
		//		 virtual bool to_bool() { return true; }
		//		 virtual const char*	type_of() { return "object"; }

		void	builtin_member(const tu_string& name, const as_value& val); 
		void	call_watcher(const tu_string& name, const as_value& old_val, as_value* new_val);
		virtual bool	set_member(const tu_string& name, const as_value& val);
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	find_property( const tu_string& name, as_value* val);
		virtual bool	on_event(const event_id& id);
		virtual	void enumerate(as_environment* env);
		virtual	void enumerate(as_array* a);
		virtual as_object* get_proto() const;
		virtual void set_proto(as_object* proto);
		virtual bool watch(const tu_string& name, as_function* callback, const as_value& user_data);
		virtual bool unwatch(const tu_string& name);
		virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
		virtual void this_alive();
		virtual void copy_to(as_object* target);
		virtual void dump(int tab = 0, bool prototype = false);
		as_object* find_target(const as_value& target);
		virtual as_environment*	get_environment() { return 0; }
		virtual void advance(float delta_time) { assert(0); }

		virtual bool	is_instance_of(const as_function* constructor) const;

		// get/set constructor of object
		bool get_ctor(as_value* val) const;
		void set_ctor(const as_value& val);

		as_object* create_proto(const as_value& constructor);
		void use_proto(const tu_string& classname);

		bool get_mouse_flag() const;
		void set_mouse_flag(const tu_string& name, const as_value& val);
	};

}

#endif
