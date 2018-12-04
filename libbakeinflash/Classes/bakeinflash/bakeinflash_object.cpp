// bakeinflash_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_movie_def.h"

namespace bakeinflash
{

	const char*	next_slash_or_dot(const char* word);

	void	as_object_addproperty(const fn_call& fn)
	{
		if (fn.nargs == 3)
		{
			assert(fn.this_ptr);

			// creates unbinded property
			// fn.this_ptr->set_member(fn.arg(0).to_string(), as_value(fn.arg(1), fn.arg(2)));
			// force direct rewriting of member
			fn.this_ptr->builtin_member(fn.arg(0).to_string(), as_value(fn.arg(1), fn.arg(2)));
			
			fn.result->set_bool(true);
			return;
		}
		fn.result->set_bool(false);
	}
	
	//static registerClass(name:String, theClass:Function) : Boolean
	void	as_object_registerclass(const fn_call& fn)
	{
		fn.result->set_bool(false);
		if (fn.nargs == 2 && fn.env->get_target() != NULL)
		{
			character_def* def = fn.env->get_target()->find_exported_resource(fn.arg(0).to_tu_string());
			if (def)
			{
				as_function* func = cast_to<as_function>(fn.arg(1).to_object());
				if (func)
				{
					IF_VERBOSE_ACTION(myprintf("registerClass '%s' (overwrite)\n",	fn.arg(0).to_string()));
					fn.result->set_bool(true);
					def->set_registered_class_constructor(func);
				}
			}
			else
			{
				// HST:
				// From the documentation of registerClass:
				//		"Associates a movie clip symbol with an ActionScript object class. 
				//		If a symbol doesn't exist, Flash creates an association between a string identifier and an object class. 
				//
				//		If a symbol is already registered to a class, this method replaces it with the new registration."
				//
				// I take that to mean that the not-found case shouldn't be handled as an error, but rather as
				// as a perfectly normal case.

				character* target = fn.env->get_target();
				def = new character_def();
				movie_definition_sub* movie = (movie_definition_sub*) target->get_root_movie();
				as_function* func = cast_to<as_function>(fn.arg(1).to_object());
				if (func)
				{
					IF_VERBOSE_ACTION(myprintf("registerClass '%s' (new)\n",	fn.arg(0).to_string()));
					movie->export_resource(fn.arg(0).to_tu_string(), def);
					fn.result->set_bool(true);
					def->set_registered_class_constructor(func);
				}
			}
		}
	}

	// public hasOwnProperty(name:String) : Boolean
	// Indicates whether an object has a specified property defined. 
	// This method returns true if the target object has a property that
	// matches the string specified by the name parameter, and false otherwise.
	// This method does not check the object's prototype chain and returns true only 
	// if the property exists on the object itself.
	void	as_object_hasownproperty(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			as_value m;
			const tu_string& name = fn.arg(0).to_tu_string();
			if (fn.this_ptr->get_member(name, &m))
			{
				fn.result->set_bool(true);
				return;
			}
		}
		fn.result->set_bool(false);
	}


	// public watch(name:String, callback:Function, [userData:Object]) : Boolean
	// Registers an event handler to be invoked when a specified property of
	// an ActionScript object changes. When the property changes,
	// the event handler is invoked with myObject as the containing object. 
	void	as_object_watch(const fn_call& fn)
	{
		bool ret = false;
		if (fn.nargs >= 2)
		{
			assert(fn.this_ptr);

			ret = fn.this_ptr->watch(fn.arg(0).to_tu_string(),
							fn.arg(1).to_function(), fn.nargs > 2 ? fn.arg(2) : as_value());
		}
		fn.result->set_bool(ret);
	}

	//public unwatch(name:String) : Boolean
	// Removes a watchpoint that Object.watch() created.
	// This method returns a value of true if the watchpoint is successfully removed,
	// false otherwise.
	void	as_object_unwatch(const fn_call& fn)
	{
		bool ret = false;
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			ret = fn.this_ptr->unwatch(fn.arg(0).to_tu_string());
		}
		fn.result->set_bool(ret);
	}


	// for debugging
	void	as_object_dump(const fn_call& fn)
	{
		if (fn.this_ptr)
		{
			fn.this_ptr->dump();
		}
	}

	void	as_global_object_ctor(const fn_call& fn)
	// Constructor for ActionScript class Object.
	{
		fn.result->set_as_object(new as_object());
	}


	//
	//	as_object_interface implementation
	//

	const char*	as_object_interface::to_string()
	{
		return to_tu_string().c_str();
	}
	
	const tu_string&	as_object_interface::to_tu_string() 
	{
#ifdef _DEBUG
		char str[32];
		snprintf(str, 32, "[object %p]", this);

		static tu_string s_string;
		s_string = str;
		return s_string; 
#else
		static tu_string s_string("[object Object]"); 
		return s_string; 
#endif
	}

	double	as_object_interface::to_number()
	{
		const char* str = to_string();
		if (str)
		{
			return atof(str);
		}
		return 0;
	}

	bool as_object_interface::to_bool()
	{
		return true; 
	}

	// this stuff should be high optimized
	// therefore we can't use here set_member(...)

	static string_hash<Uint32>	s_mouse_events_map;

	as_object::as_object() :
		m_watch(NULL),
		m_on_mouse_event(0)
	{
		if (s_mouse_events_map.size() == 0)
		{
			s_mouse_events_map.set_capacity(9);
			s_mouse_events_map.add("onKeyPress", onKeyPress);
			s_mouse_events_map.add("onRelease", onRelease);
			s_mouse_events_map.add("onDragOver", onDragOver);
			s_mouse_events_map.add("onDragOut", onDragOut);
			s_mouse_events_map.add("onPress", onPress);
			s_mouse_events_map.add("onReleaseOutside", onReleaseOutside);
			s_mouse_events_map.add("onRollOut", onRollout);
			s_mouse_events_map.add("onRollOver", onRollover);
			s_mouse_events_map.add("onMouseMove", onMouseMove);
		}

	}

	as_object::~as_object()
	{
		set_clear_garbage(true);
		delete m_watch;
	}

	// called from a object constructor only
	void	as_object::builtin_member(const tu_string& name, const as_value& val)
	{
		val.set_flags(as_value::DONT_ENUM);
		m_members.set(name, val);
		set_mouse_flag(name, val);
	}


	void as_object::call_watcher(const tu_string& name, const as_value& old_val, as_value* new_val)
	{
		if (m_watch)
		{
			as_watch watch;
			m_watch->get(name, &watch);
			if (watch.m_func)
			{
				as_environment env;
				env.push(watch.m_user_data);	// params
				env.push(*new_val);		// newVal
				env.push(old_val);	// oldVal
				env.push(as_value(name));	// property
				new_val->set_undefined();
				(*watch.m_func)(fn_call(new_val, this, &env, 4, env.get_top_index()));
			}
		}
	}


	bool	as_object::set_member(const tu_string& name, const as_value& new_val)
	{
		//if (name=="global")
		//{
		//	myprintf("SET MEMBER: %s at %p for object %p\n", name.c_str(), new_val.to_object(), this);
		//}

		as_value old_val;
		m_members.get(name, &old_val);
		if (old_val.is_readonly())
		{
			return true;
		}

		if (old_val.is_property())
		{
			old_val.set_property(this, new_val);
			return true;
		}

		// try watcher
		if (m_watch)
		{
			as_value val(new_val);
			call_watcher(name, old_val, &val);
			m_members.set(name, val);
			return true;
		}

		m_members.set(name, new_val);
		set_mouse_flag(name, new_val);

		// spec case
		// var xxx=new Object();  xxx=new Object();
		as_object* old_obj = old_val.to_object();
		if (old_obj && old_obj->get_ref_count() == 2)
		{
			set_alive(old_obj, false);
		}
		return true;
	}

	as_object* as_object::get_proto() const
	{
		return m_proto.get();
	}

	void as_object::set_proto(as_object* proto)
	{
		return m_proto = proto;
	}

	bool	as_object::get_member(const tu_string& name, as_value* val)
	{
//		if (name=="length")
//		{
//				myprintf("get_member: %s at %p for object %p\n", name.c_str(), val, this);
//		}

		if (m_members.get(name, val) == false)
		{
			root* r = get_root();
			if (r && get_builtin(r->is_as3() ? BUILTIN_OBJECT_METHOD_AS3 : BUILTIN_OBJECT_METHOD, name, val) == false)
			{
				as_object* proto = get_proto();
				if (proto == NULL)
				{
					return false;
				}

				if (proto->get_member(name, val) == false)
				{
					return false;
				}
			}
		}

		if (val && val->is_property())
		{
			val->get_property(this, val);
		}

		return true;
	}

	// get owner
	bool	as_object::find_property( const tu_string& name, as_value* val )
	{
		as_value dummy;
		if (get_member(name, &dummy))
		{
			val->set_as_object(this);
			return true;
		}

		as_object *proto = get_proto();
		if (proto)
		{
			return proto->find_property(name, val);
		}
		return false;
	}

	void	as_object::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		// Is it a reentrance ?
		if (visited_objects->get(this, NULL))
		{
			return;
		}
		visited_objects->set(this, true);

		as_value undefined;
		for (string_hash<as_value>::iterator it = m_members.begin();
			it != m_members.end(); ++it)
		{
			as_object* obj = it->second.to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					it->second.set_undefined();
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
				continue;
			}
		}
	}

	bool	as_object::on_event(const event_id& id)
	{
		// Check for member function.
		bool called = false;
		{
			const tu_string&	method_name = id.get_function_name();
			if (method_name.size() > 0)
			{
				as_value	method;
				if (get_member(method_name, &method))
				{
//					as_environment env;

					// use _root environment
					character* mroot = get_root_movie();
					as_environment* env = mroot->get_environment();
					
					// keep stack size
					int stack_size = env->get_stack_size();

					int nargs = 0;
					if (id.m_args)
					{
						nargs = id.m_args->size();
						for (int i = nargs - 1; i >=0; i--)
						{
							env->push((*id.m_args)[i]);
						}
					}
					call_method(method, env, this, nargs, env->get_top_index());
					called = true;

					// restore stack size
					env->set_stack_size(stack_size);
				}
			}
		}

		return called;
	}

	void as_object::enumerate(as_environment* env)
	// retrieves members & pushes them into env
	{
		string_hash<as_value>::const_iterator it = m_members.begin();
		while (it != m_members.end())
		{
			if (it->second.is_enum())
			{
				env->push(as_value(it->first.c_str()));
				IF_VERBOSE_ACTION(myprintf("-------------- enumerate - push: %s\n", it->first.c_str()));
			}

			++it;
		}
	}

	void as_object::enumerate(as_array* a)
	// retrieves members & pushes them into a
	{
		string_hash<as_value>::const_iterator it = m_members.begin();
		while (it != m_members.end())
		{
			if (it->second.is_enum())
			{
				a->push(as_value(it->first.c_str()));
				IF_VERBOSE_ACTION(myprintf("-------------- enumerate - push: %s\n", it->first.c_str()));
			}
			++it;
		}
	}

	bool as_object::watch(const tu_string& name, as_function* callback,
		const as_value& user_data)
	{
		if (callback == NULL)
		{
			return false;
		}

		as_watch watch;
		watch.m_func = callback;
		watch.m_user_data = user_data;
		
		if (m_watch == NULL)
		{
			m_watch = new string_hash<as_watch>;
		}
		m_watch->set(name, watch);
		return true;
	}

	bool as_object::unwatch(const tu_string& name)
	{
		if (m_watch)
		{
			as_watch watch;
			if (m_watch->get(name, &watch))
			{
				m_watch->erase(name);
				return true;
			}
		}
		return false;
	}

	void as_object::copy_to(as_object* target)
	// Copy all members from 'this' to target
	{
		if (target)
		{
			for (string_hash<as_value>::const_iterator it = m_members.begin(); 
				it != m_members.end(); ++it ) 
			{ 
				target->builtin_member(it->first, it->second); 
			} 
		}
	}

	void as_object::dump(int tabs, bool prototype)
	// for debugging, used from action script
	// retrieves members & print them
	{
		tu_string tab;
		for (int i = 0; i < tabs; i++)
		{
			tab += ' ';
		}

		myprintf("%s*** %s %p ***\n", tab.c_str(), prototype ? "prototype" : to_string(), this);
		for (string_hash<as_value>::const_iterator it = m_members.begin(); 
			it != m_members.end(); ++it)
		{
			const as_value& val = it->second;
			val.print(it->first.c_str(), tabs + 2);
		}

		tabs += 4;
		for (int i = 0; i < 4; i++)
		{
			tab += ' ';
		}

		// dump proto
		if (m_proto != NULL)
		{
			m_proto->dump(tabs, true);
		}
	}

	as_object*	as_object::find_target(const as_value& target)
	// Find the object referenced by the given target.
	{
		if (target.is_string() == false)
		{
			return target.to_object();
		}

		const tu_string& path = target.to_tu_string();
		if (path.size() == 0)
		{
			return this;
		}

		as_value val;
		as_object* tar = NULL;

		// absolute path ?
		if (*path.c_str() == '/')
		{
			return get_root_movie()->find_target(path.c_str() + 1);
		}

		const char* slash = strchr(path.c_str(), '/');
		if (slash == NULL)
		{
			slash = strchr(path.c_str(), '.');
			if (slash)
			{
				if (slash[1] == '.')
				{
					slash = NULL;
				}
			}
		}

		if (slash)
		{
			tu_string name(path.c_str(), int(slash - path.c_str()));
			get_member(name, &val);
			tar = val.to_object();
			if (tar)	
			{
				return tar->find_target(slash + 1);
			}
		}
		else
		{
			get_member(path, &val);
			tar = val.to_object();
		}

		if (tar == NULL)
		{
			myprintf("can't find target %s\n", path.c_str());
		}
		return tar;
	}

	// mark 'this' as alive
	void as_object::this_alive()
	{
		// Whether there were we here already ?
		if (get_clear_garbage() == false)
		{
			return;
		}

		if (is_garbage(this))
		{
			// 'this' and its members is alive
			set_alive(this);
			for (string_hash<as_value>::iterator it = m_members.begin();
				it != m_members.end(); ++it)
			{
				as_object* obj = it->second.to_object();
				if (obj)
				{
					if (obj->is_alive())
					{
						obj->this_alive();
					}
					else
					{
						// deleted movieclip.. it's garbage

						// maybe not garbage.. for example, in case createEmptyMovieClip
						//it->second.set_undefined();
					}
				}
			}
		}

	}

	// for optimization I don't want to put '__constructor__' into as_object
	// so the missing of '__constructor__' in a object means 
	// that it is a instance of as_object
	bool	as_object::is_instance_of(const as_function* constructor) const
	{
		// by default ctor is as_global_object_ctor
		as_value ctor;
		get_ctor(&ctor);
		if (ctor.is_undefined())
		{
			ctor.set_as_c_function(as_global_object_ctor);
		}

		const as_s_function* sf = cast_to<as_s_function>(constructor);
		if (sf && sf == cast_to<as_s_function>(ctor.to_function()))
		{
			return true;
		}

		const as_c_function* cf1 = cast_to<as_c_function>(constructor);
		const as_c_function* cf2 = cast_to<as_c_function>(ctor.to_function());
		if (cf1 && cf2 && cf1->m_func == cf2->m_func)
		{
			return true;
		}

		as_object* proto = get_proto();
		if (proto)
		{
			return proto->is_instance_of(constructor);
		}
		return false;
	}

	static tu_string s_constructor("__constructor__");
	bool as_object::get_ctor(as_value* val) const
	{
		return m_members.get(s_constructor, val);
	}

	void as_object::set_ctor(const as_value& val)
	{
		builtin_member(s_constructor, val);
	}

	as_object* as_object::create_proto(const as_value& constructor)
	{ 	 
		m_proto = new as_object(); 	 
		m_proto->m_this_ptr = m_this_ptr; 	

		if (constructor.to_object()) 	 
		{ 	 
			// constructor is as_s_function 	 
			as_value val;
			constructor.to_object()->get_member("prototype", &val);
			as_object* prototype = val.to_object();
			assert(prototype);
			prototype->copy_to(this);

			as_value prototype_constructor;
			if (prototype->get_ctor(&prototype_constructor))
			{
				m_proto->create_proto(prototype_constructor);
			}
		}

		set_ctor(constructor);
		return m_proto.get();
	}

	void as_object::use_proto(const tu_string& classname)
	{
		// set prototype
		as_value val;
		get_global()->get_member(classname, &val);
		as_object* obj = val.to_object();
		if (obj)
		{
			static tu_string prototype("prototype");
			obj->get_member(prototype, &val);
			m_proto = val.to_object();
		}
	}

	bool as_object::get_mouse_flag() const
	{
		if (m_on_mouse_event != 0)
		{
			return true;
		}
		if (m_proto)
		{
			return m_proto->get_mouse_flag();
		}
		return false;
	}

	void as_object::set_mouse_flag(const tu_string& name, const as_value& val)
	{
		Uint32 f;
		if (s_mouse_events_map.get(name, &f))
		{
			if (val.is_function())
			{
				m_on_mouse_event |= f;
			}
			else
			{
				f = ~f;
				m_on_mouse_event &= f;
			}
		}
	}


}
