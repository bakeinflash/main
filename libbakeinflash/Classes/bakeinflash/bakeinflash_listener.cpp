// bakeinflash_listener.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_listener.h"
#include "bakeinflash/bakeinflash_character.h"

namespace bakeinflash
{

	// for Key, keyPress, ...
	void listener::notify(const event_id& ev)
	{
		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		clone_listener listeners(m_listeners);
		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			listeners[i]->on_event(ev);
		}
	}

		// sort by global level/depth
	void listener::sort_by_level()
	{
		for (int i = 0, n = m_listeners.size(); i < n - 1; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				character* ch1 = cast_to<character>(m_listeners[i].get());
				character* ch2 = cast_to<character>(m_listeners[j].get());

				// both movieclips ?
				if (ch1 && ch2)
				{
					// get global level/depth
					int level1 = 0;
					for (character* parent = ch1->get_parent(); parent; parent = parent->get_parent())
					{
						level1++;
					}
					int level2 = 0;
					for (character* parent = ch2->get_parent(); parent; parent = parent->get_parent())
					{
						level2++;
					}


					if (level1 > level2)
					{
						as_object* obj = m_listeners[i].get();
						m_listeners[i] = m_listeners[j];
						m_listeners[j] = obj;
					}
				}
			}
		}
	}

	// for asBroadcaster, ...
	void listener::notify(const tu_string& event_name, const fn_call& fn)
	{
		// may be called from multithread plugin ==>
		// we should check current root
		if (get_root() == NULL)
		{
			return;
		}

		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		clone_listener listeners(m_listeners);

		// iterate through a copy of environment
		int nargs = fn.nargs;
		as_environment env;
		for (int i = 0; i < nargs; i++)
		{
			env.push(fn.env->top(nargs - i - 1));
		}

		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			as_value function;
			if (listeners[i]->get_member(event_name, &function))
			{
				call_method(function, &env, listeners[i].get(),
					nargs, env.get_top_index());
			}
		}
	}

	// for video, timer, ...
	void	listener::advance(float delta_time)
	{
		// event handler may affects m_listeners using addListener & removeListener
		// iterate through a copy of it
		clone_listener listeners(m_listeners);
		for (int i = 0, n = listeners.size(); i < n; i++)
		{
			listeners[i]->advance(delta_time);
		}
	}

	void listener::add(as_object* listener) 
	{
		// sanity check
		assert(m_listeners.size() < 1000);
		//myprintf("m_listeners size=%d\n", m_listeners.size());

		if (listener)
		{
			int free_item = -1;
			for (int i = 0, n = m_listeners.size(); i < n; i++)
			{
				if (m_listeners[i] == listener)
				{
					return;
				}
				if (m_listeners[i] == NULL)
				{
					free_item = i;
				}
			}

			if (free_item >= 0)
			{
				m_listeners[free_item] = listener;
			}
			else
			{
				m_listeners.push_back(listener);
			}

			sort_by_level();
		}
	} 

	bool listener::remove(as_object* listener) 
	{
		bool is_removed = false;

		// null out a item
		for (int i = 0, n = m_listeners.size(); i < n; i++)
		{
			if (m_listeners[i] == listener)
			{
				m_listeners[i] = NULL;
				is_removed = true;
			}
		}
		return is_removed;
	} 

	as_object*	listener::operator[](const tu_string& name) const
	{
		return operator[](atoi(name.c_str()));
	}
	
	as_object*	listener::operator[](int index) const
	{
		if (index >= 0 && index < m_listeners.size())
		{
			int nonzero = 0;
			for (int i = 0, n = m_listeners.size(); i < n; i++)
			{
				if (m_listeners[i] != NULL)
				{
					if (nonzero == index)
					{
						return m_listeners[i].get();
					}
					nonzero++;
				}
			}
		}
		return NULL;
	}

	int	listener::size() const
	{
		int nonzero = 0;
		for (int i = 0, n = m_listeners.size(); i < n; i++)
		{
			if (m_listeners[i] != NULL)
			{
				nonzero++;
			}
		}
		return nonzero;
	}

	void listener::enumerate(as_environment* env) const
	{
		int nonzero = 0;
		for (int i = 0, n = m_listeners.size(); i < n; i++)
		{
			if (m_listeners[i] != NULL)
			{
				env->push(nonzero);
				nonzero++;
			}
		}
	}

}
