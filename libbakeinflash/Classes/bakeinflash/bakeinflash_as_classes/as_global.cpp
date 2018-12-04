// as_global.h	-- Thatcher Ulrich <tu@tulrich.com> 2003
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script global functions implementation

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_print.h"
#include "as_global.h"

namespace bakeinflash
{

	//
	// Built-in objects
	//
	void	as_systemLaunchUrl(const fn_call& fn);
	void	as_global_geturl(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}
#ifdef iOS
		as_systemLaunchUrl(fn);
#endif
	}
	
	void	as_toString(const fn_call& fn)
	{
		if (fn.this_ptr)
		{
			fn.result->set_tu_string(fn.this_ptr->to_string());
		}
	}

	void	as_global_trace(const fn_call& fn)
	{
		tu_string s;
		for (int i = 0; i < fn.nargs; i++)
		{
			if (i > 0)
			{
				s += ' ';
			}
			s += fn.arg(i).to_string();
		}
		myprintf("%s\n", s.c_str());
	}

	void	as_global_print(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			// print movieclip
			sprite_instance* mc = cast_to<sprite_instance>(fn.arg(0).to_object());
			if (mc)
			{
				mc->print();
				return;
			}

			// print esc sequence
			if (fn.arg(0).is_string() && fn.arg(1).is_string())
			{
				// text, device
#if TU_CONFIG_LINK_TO_LIBHPDF == 1
				print_text(fn.arg(0).to_tu_string(), fn.arg(1).to_tu_string());
#endif
				return;
			}
		}
	}

	// setInterval(functionReference:Function, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	// setInterval(objectReference:Object, methodName:String, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	void  as_global_setinterval(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			smart_ptr<as_timer> t = new as_timer();

			int first_arg_index = 2;
			if (fn.arg(0).is_function())
			{
				t->m_func = fn.arg(0).to_function();
				t->m_interval = fn.arg(1).to_float() / 1000.0f;
				assert(fn.env);
				t->m_this_ptr = fn.env->get_target();
			}
			else
			if (fn.arg(0).to_object() != NULL)
			{
				as_value func;
				as_object* this_ptr = fn.arg(0).to_object();
				this_ptr->get_member(fn.arg(1).to_tu_string(), &func);

				t->m_func = func.to_function();
				t->m_interval = fn.arg(2).to_float() / 1000.0f;
				t->m_this_ptr = this_ptr;
				first_arg_index = 3;
			}
			else
			{
				// invalid args
				return;
			}

			// pass args
			t->m_arg.resize(fn.nargs - first_arg_index);
			for (int i = first_arg_index; i < fn.nargs; i++)
			{
				t->m_arg.push_back(fn.arg(i));
			}

			get_root()->add_listener(t);
			fn.result->set_as_object(t);
		}
	}

	// setTimeout(functionReference:Function, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	// setTimeout(objectReference:Object, methodName:String, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	void  as_global_settimeout(const fn_call& fn)
	{
		// create interval timer
		as_global_setinterval(fn);
		as_timer* t = cast_to<as_timer>(fn.result->to_object());

		if (t)
		{
			t->m_do_once = true;
		}
	}

	void  as_global_clearinterval(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			get_root()->remove_listener(fn.arg(0).to_object());
		}
	}

	void	as_global_update_after_event(const fn_call& fn)
	{
		// isn't required for bakeinflash
	}
	
	void	as_global_assetpropflags(const fn_call& fn)
	// Undocumented ASSetPropFlags function
	// Works only for as_object for now
	{
		int version = get_root()->get_movie_version();

		// Check the arguments
		assert(fn.nargs == 3 || fn.nargs == 4);
		assert((version == 5) ? (fn.nargs == 3) : true);

		// object
		as_object* obj = fn.arg(0).to_object();
		if (obj == NULL)
		{
			myprintf("error: assetpropflags for NULL object\n");
			return;
		}

		// The second argument is a list of child names,
		// may be in the form array(like ["abc", "def", "ggggg"]) or in the form a string(like "abc, def, ggggg")
		// the NULL second parameter means that assetpropflags is applied to all children
		as_object* props = fn.arg(1).to_object();

		int as_prop_flags_mask = 7; // DONT_ENUM | DONT_DELETE | READ_ONLY;

		// a number which represents three bitwise flags which
		// are used to determine whether the list of child names should be hidden,
		// un-hidden, protected from over-write, un-protected from over-write,
		// protected from deletion and un-protected from deletion
		int true_flags = fn.arg(2).to_int() & as_prop_flags_mask;

		// Is another integer bitmask that works like true_flags,
		// except it sets the attributes to false. The
		// false_flags bitmask is applied before true_flags is applied

		// ASSetPropFlags was exposed in Flash 5, however the fourth argument 'false_flags'
		// was not required as it always defaulted to the value '~0'. 
		int false_flags = (fn.nargs == 3 ? 
				 (version == 5 ? ~0 : 0) : fn.arg(3).to_int()) & as_prop_flags_mask;

		// Evan: it seems that if true_flags == 0 and false_flags == 0, this function
		// acts as if the parameters where (object, null, 0x1, 0) ...
		if (false_flags == 0 && true_flags == 0)
		{
			props = NULL;
			false_flags = 0;
			true_flags = 0x1;
		}

		if (props == NULL)
		{
			// Takes all members of the object and sets its property flags
			for (string_hash<as_value>::const_iterator it = obj->m_members.begin(); 
				it != obj->m_members.end(); ++it)
			{
				const as_value& val = it->second;
				int flags = val.get_flags();
				flags = flags & (~false_flags);
				flags |= true_flags;
				val.set_flags(flags);
			}
		}
		else
		{
			// Takes all string type prop and sets property flags of obj[prop]
			for (string_hash<as_value>::const_iterator it = props->m_members.begin(); 
				it != props->m_members.end(); ++it)
			{
				const as_value& key = it->second;
				if (key.is_string())
				{
					string_hash<as_value>::iterator obj_it = obj->m_members.find(key.to_tu_string());
					if (obj_it != obj->m_members.end())
					{
						const as_value& val = obj_it->second;
						int flags = val.get_flags();
						flags = flags & (~false_flags);
						flags |= true_flags;
						val.set_flags(flags);
					}
				}
			}
		}
	}

	// getVersion() : String
	void	as_global_get_version(const fn_call& fn)
	// Returns a string containing Flash Player version and platform information.
	{
		fn.result->set_tu_string(get_bakeinflash_version());
	}


	void as_timer::advance(float delta_time)
	{
		if (m_func == NULL)
		{
			return;
		}

		m_time_remainder += delta_time;
		if (m_time_remainder >= m_interval)
		{
			m_time_remainder = (float) fmod(m_time_remainder - m_interval, m_interval);

			as_environment env;
			int n = m_arg.size();
			{
				for (int i = 0; i < n; i++)
				{
					env.push(m_arg[i]);
				}
			}

			// keep alive
			smart_ptr<as_object> obj = m_this_ptr.get();
			as_value callback(m_func.get());

			call_method(callback, &env, obj.get(), n, env.get_top_index());

			if (m_do_once)
			{
				get_root()->remove_listener(this);
			}
		}
	}


	void as_timer::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		// Is it a reentrance ?
		if (visited_objects->get(this, NULL))
		{
			return;
		}

		as_object::clear_refs(visited_objects, this_ptr);

		// clear m_arg
		for (int i = 0; i < m_arg.size(); i++)
		{
			as_object* obj = m_arg[i].to_object();
			if (obj)
			{
				obj->clear_refs(visited_objects, this_ptr);
			}
		}
		m_arg.clear();
	}


	// creates 'Stage' object

		void	as_stage_addlistener(const fn_call& fn)
	{
		as_stage* stage = cast_to<as_stage>(fn.this_ptr);
		fn.result->set_bool(false);
		as_object* obj = fn.arg(0).to_object();
		if (stage && fn.nargs > 0 && obj)
		{
			stage->m_listener.add(obj);
			fn.result->set_bool(true);
		}
	}

	void	as_stage_removelistener(const fn_call& fn)
	{
		as_stage* stage = cast_to<as_stage>(fn.this_ptr);
		fn.result->set_bool(false);
		as_object* obj = fn.arg(0).to_object();
		if (stage && fn.nargs > 0 && obj)
		{
			stage->m_listener.remove(obj);
			fn.result->set_bool(true);
		}
	}


	as_stage* stage_init()
	{
		return new as_stage();
	}

	as_stage::as_stage()
	{
		builtin_member("addListener", as_stage_addlistener);
		builtin_member("removeListener", as_stage_removelistener);
	}

	as_stage::~as_stage()
	{
	}

	bool	as_stage::get_member(const tu_string& name, as_value* val)
	{
		if (name == "width")
		{
			smart_ptr<root>	m = get_root();

//			int width = m->get_movie_width();
			int x0, y0, width, height = 0;
			m->get_display_viewport(&x0, &y0, &width, &height);
			val->set_int(width);
			return true;
		}
		else
		if (name == "height")
		{
			smart_ptr<root>	m = get_root();

//			int height = m->get_movie_height();
			int x0, y0, width, height = 0;
			m->get_display_viewport(&x0, &y0, &width, &height);
			val->set_int(height);
			return true;
		}
		else
		if (name == "x")
		{
			smart_ptr<root>	m = get_root();
//			int height = m->get_movie_height();
			int x0, y0, w, h = 0;
			m->get_display_viewport(&x0, &y0, &w, &h);
			val->set_int(x0);
			return true;
		}
		else
		if (name == "y")
		{
			smart_ptr<root>	m = get_root();
//			int height = m->get_movie_height();
			int x0, y0, w, h = 0;
			m->get_display_viewport(&x0, &y0, &w, &h);
			val->set_int(y0);
			return true;
		}
		else
		if (name == "scale")
		{
			smart_ptr<root>	m = get_root();
			float height = (float) m->get_movie_height();
			int x0, y0, w, h = 0;
			m->get_display_viewport(&x0, &y0, &w, &h);
			val->set_double(h / height);
			return true;
		}

		return as_object::get_member(name, val);
	}


	void as_stage::on_resize()
	{
		as_environment env;
		m_listener.notify("onResize", fn_call(NULL, 0, &env, 0, env.get_top_index()));
	}
};
