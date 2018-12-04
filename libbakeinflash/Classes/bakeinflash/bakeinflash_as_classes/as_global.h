// as_global.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script global functions implementation


#ifndef BAKEINFLASH_AS_GLOBAL_H
#define BAKEINFLASH_AS_GLOBAL_H

namespace bakeinflash
{
	void	as_global_trace(const fn_call& fn);
	void	as_global_get_version(const fn_call& fn);
	void	as_global_assetpropflags(const fn_call& fn);
	void  as_global_setinterval(const fn_call& fn);
	void  as_global_clearinterval(const fn_call& fn);
	void  as_global_settimeout(const fn_call& fn);
	void  as_global_cleartimeout(const fn_call& fn);
	void	as_global_update_after_event(const fn_call& fn);
	void	as_global_geturl(const fn_call& fn);
	void	as_toString(const fn_call& fn);
	void	as_global_print(const fn_call& fn);

	// for setInterval and setTimeout
	struct as_timer : public as_object
	{
		weak_ptr<as_object> m_this_ptr;
		weak_ptr<as_function> m_func;
		float m_interval;	// sec
		float m_time_remainder;
		array<as_value> m_arg;
		bool m_do_once;

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_TIMER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_timer() :
			m_interval(0.0f),
			m_time_remainder(0.0f),
			m_do_once(false)
		{
		}

		virtual void advance(float delta_time);
		virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
	};

	// Stage object
	struct as_stage : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_STAGE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_stage();
		virtual ~as_stage();
		virtual bool	get_member(const tu_string& name, as_value* val);

		void on_resize();
		listener m_listener;
	};

	// creates 'Stage' object
	as_stage* stage_init();

}	// end namespace bakeinflash


#endif // bakeinflash_AS_GLOBAL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
