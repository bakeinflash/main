// EventDispatcher -- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2014

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AS3 EventDispatcher implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_EVENTDISPATCHER_H
#define BAKEINFLASH_AS_EVENTDISPATCHER_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_listener.h"
#include "base/tu_queue.h"

namespace bakeinflash
{
	void as_global_event_dispatcher_ctor(const fn_call& fn);
	struct as_event;

	// misc
	struct clone_dispatcher_listener : public array< weak_ptr<as_function> >
	{
		clone_dispatcher_listener(const array< weak_ptr<as_function> >& listeners)
		{
			for (int i = 0, n = listeners.size(); i < n; i++)
			{
				if (listeners[i] != NULL)
				{
					push_back(listeners[i].get());
				}
			}
		}
	};

	struct as_event_dispatcher : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_EVENT_DISPATCHER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_event_dispatcher();
		virtual ~as_event_dispatcher();
		
		void	add(const fn_call& fn);
		void	remove(const fn_call& fn);
		void	broadcast(const fn_call& fn);


		string_hash< array< weak_ptr<as_function> >* > m_listeners;
	
//		private :

//		bool m_reentrance;
//		tu_queue< weak_ptr<as_object> > m_suspended_event;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_BROADCASTER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
