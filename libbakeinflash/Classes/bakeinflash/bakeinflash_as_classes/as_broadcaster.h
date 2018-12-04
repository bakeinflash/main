// as_broadcaster.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script AsBroadcaster implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_BROADCASTER_H
#define BAKEINFLASH_AS_BROADCASTER_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_listener.h"
#include "base/tu_queue.h"

namespace bakeinflash
{

	// Create built-in broadcaster object.
	as_object* broadcaster_init();

	struct as_listener : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_LISTENER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_listener();
		virtual bool	get_member(const tu_string& name, as_value* val);
		void add(as_object* listener);
		void remove(as_object* listener);
		int size() const;
		virtual void enumerate(as_environment* env);
		void	broadcast(const fn_call& fn);
	
		private :

		listener m_listeners;
		bool m_reentrance;
		tu_queue< array<as_value> > m_suspended_event;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_BROADCASTER_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
