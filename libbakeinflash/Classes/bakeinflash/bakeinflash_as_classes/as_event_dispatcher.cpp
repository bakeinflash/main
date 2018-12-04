// as_event_dispatcherer.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2014

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Provides event notification and listener management capabilities that
// you can add to user-defined objects. This class is intended for advanced 
// users who want to create custom event handling mechanisms. 
// You can use this class to make any object an event event_dispatcherer and 
// to create one or more listener objects that receive notification anytime 
// the event_dispatchering object calls the event_dispatcherMessage() method. 

#include "bakeinflash/bakeinflash_as_classes/as_event_dispatcher.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "bakeinflash/bakeinflash_as_classes/as_event.h"
#include "bakeinflash/bakeinflash_function.h"

namespace bakeinflash
{
	void	as_global_event_dispatcher_ctor(const fn_call& fn)
	// Constructor for ActionScript class MovieClipLoader
	{
		fn.result->set_as_object(new as_event_dispatcher());
	}

	// (listenerObj:Object) : Boolean
	void	as_event_dispatcher_addlistener(const fn_call& fn)
	// Registers an object to receive event notification messages.
	// This method is called on the event_dispatchering object and
	// the listener object is sent as an argument.
	{
		as_event_dispatcher* id = cast_to<as_event_dispatcher>(fn.this_ptr);
		if (id)
		{
			id->add(fn);
		}
	}

	// (listenerObj:Object) : Boolean
	void	as_event_dispatcher_removelistener(const fn_call& fn)
	// Removes an object from the list of objects that receive event notification messages. 
	{
		as_event_dispatcher* id = cast_to<as_event_dispatcher>(fn.this_ptr);
		if (id)
		{
			id->remove(fn);
		}
	}

	// (eventName:String, ...) : Void
	void	as_event_dispatcher_sendmessage(const fn_call& fn)
	// Sends an event message to each object in the list of listeners.
	// When the message is received by the listening object,
	// bakeinflash attempts to invoke a function of the same name on the listening object. 
	{
		as_event_dispatcher* id = cast_to<as_event_dispatcher>(fn.this_ptr);
		if (id)
		{
			id->broadcast(fn);
		}
	}

	as_event_dispatcher::as_event_dispatcher()
	//	m_reentrance(false)
	{
		builtin_member("addEventListener", as_event_dispatcher_addlistener);
		builtin_member("removeEventListener", as_event_dispatcher_removelistener);
		builtin_member("dispatchEvent", as_event_dispatcher_sendmessage);
	}

	as_event_dispatcher::~as_event_dispatcher()
	{
		for (string_hash< array< weak_ptr<as_function> >* >::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
		{
			delete it->second;
		}
	}

	void	as_event_dispatcher::add(const fn_call& fn)
	{
		const tu_string& name = fn.arg(0).to_tu_string();
		as_function* func = fn.arg(1).to_function();
		as_object* target;
		if (func && (target = func->get_target()))
		{
			array< weak_ptr<as_function> >* l;
			if (m_listeners.get(name, &l) == false)
			{
				l = new array< weak_ptr<as_function> >();
				m_listeners.set(name, l);
			}

			// seek a room in the array
			int free_item = -1;
			for (int i = 0, n = l->size(); i < n; i++)
			{
				const weak_ptr<as_function>& f = l->operator[](i);
				if (f == func && f->get_target() == target)
				{
					// found same item
					return;
				}

				if (f == NULL)
				{
					free_item = i;
				}
			}

			if (free_item >= 0)
			{
				l->operator[](free_item) = func;
			}
			else
			{
				l->push_back(func);
			}
		}
	}

	void	as_event_dispatcher::remove(const fn_call& fn)
	{
		const tu_string& name = fn.arg(0).to_tu_string();
		as_function* func = fn.arg(1).to_function();
		as_object* target;
		if (func && (target = func->get_target()))
		{
			array< weak_ptr<as_function> >* l;
			if (m_listeners.get(name, &l) == false)
			{
				// no event listener
				return;
			}

			// seek and delete event listeners
			for (int i = 0, n = l->size(); i < n; i++)
			{
				const weak_ptr<as_function>& f = l->operator[](i);
				if (f == func && f->get_target() == target)
				{
					// found..delete it
					l->operator[](i) = NULL;
				}
			}
		}
	}

	// always 1 arg
	void	as_event_dispatcher::broadcast(const fn_call& fn)
	{
		//fn.arg(0).to_object()->dump();
		as_object* ev = fn.arg(0).to_object();
		if (ev == NULL)
		{
			return;
		}

//		if (m_reentrance)
//		{
			// keep call args
			// we must process one event completely then another
//			m_suspended_event.push(ev);
//			return;
//		}

//		m_reentrance = true;

		array< weak_ptr<as_function> >* ll;
		as_value event_name;
		ev->get_member("type", &event_name);

		if (m_listeners.get(event_name.to_tu_string(), &ll))
		{
			// event handler may affects m_listeners using addListener & removeListener
			// iterate through a copy of it
			clone_dispatcher_listener listeners(*ll);
			for (int i = 0, n = listeners.size(); i < n; i++)
			{
				const weak_ptr<as_function>& func = listeners[i];
				call_method(func.get(), fn.env, func->get_target(),	fn.nargs, fn.env->get_top_index());
			}
		}

		// check reentrances
/*		while (m_suspended_event.size() > 0)
		{
			// event handler may affects m_suspended_event using event_dispatcherMessage
			// so we iterate through the copy of args
			weak_ptr<as_object> ev = m_suspended_event.front();
			if (ev != NULL)
			{
				ev->get_member("type", &event_name);
				if (m_listeners.get(event_name.to_tu_string(), &ll))
				{
					// event handler may affects m_listeners using addListener & removeListener
					// iterate through a copy of it
					clone_dispatcher_listener listeners(*ll);
					for (int i = 0, n = listeners.size(); i < n; i++)
					{
						const weak_ptr<as_function>& func = listeners[i];
						call_method(func.get(), fn.env, func->get_target(),	fn.nargs, fn.env->get_top_index());
					}
				}
			}
			m_suspended_event.pop();
		}

		m_reentrance = false;
		*/
	}

};
