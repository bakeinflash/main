// as_xmlsocket.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script XMLSocket implementation code for the bakeinflash SWF player library.

#include "bakeinflash/bakeinflash_as_classes/as_xmlsocket.h"
#include "bakeinflash/bakeinflash_as_classes/as_event.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_function.h"
#include "base/tu_timer.h"

namespace bakeinflash
{

	void	as_xmlsock_connect(const fn_call& fn)
	{
		fn.result->set_bool(false);

		as_xmlsock* xmls = cast_to<as_xmlsock>(fn.this_ptr);
		if (xmls && fn.nargs >= 2)
		{
			fn.result->set_bool(xmls->connect(fn.arg(0).to_string(), fn.arg(1).to_int()));
		}
	}

	void	as_xmlsock_close(const fn_call& fn)
	{
		as_xmlsock* xmls = cast_to<as_xmlsock>(fn.this_ptr);
		if (xmls)
		{
			xmls->close();
		}
	}

	void	as_xmlsock_send(const fn_call& fn)
	{
		as_xmlsock* xmls = cast_to<as_xmlsock>(fn.this_ptr);
		if (fn.nargs > 0 && xmls)
		{
			xmls->send(fn.arg(0));
		}
	}

	// AS3
	// public function addEventListener(type:String, listener:Function, useCapture:Boolean = false, priority:int = 0, useWeakReference:Boolean = false):void
	void as_xmlsock_addeventlistener(const fn_call& fn)
	{
		as_xmlsock* xmls = cast_to<as_xmlsock>(fn.this_ptr);
		if (fn.nargs >= 2 && xmls)
		{
			// arg #1 - event
			// arg #2 - function
			xmls->add_event_listener(fn.arg(0).to_tu_string(), fn.arg(1).to_function());
		}
	}

	void	as_global_xmlsock_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
		fn.result->set_as_object(new as_xmlsock());
	}

	as_xmlsock::as_xmlsock()
	{
		builtin_member("connect", as_xmlsock_connect);
		builtin_member("close", as_xmlsock_close);
		builtin_member("send", as_xmlsock_send);

		// as3
		builtin_member("addEventListener", as_xmlsock_addeventlistener);
	}

	as_xmlsock::~as_xmlsock()
	{
		close();
	}

	bool as_xmlsock::connect(const char* host, int port)
	{
		m_http.close();
		m_http.connect(host, port);

		// handle events in next frame
		m_status = CONNECTING;	// connecting
		m_start = tu_timer::get_ticks();
		get_root()->add_listener(this);
		return true;
	}

	void as_xmlsock::close()
	{
		m_http.close();
		root* r = get_root();
		if (r)
		{
			r->remove_listener(this);
		}
	}

	// as3
	void as_xmlsock::add_event_listener(const tu_string& eventname, as_function* handler)
	{
		if (handler && eventname.size() > 0)
		{
			builtin_member(eventname, handler);
		}
	}

	void as_xmlsock::send(const as_value& val)
	{
		m_http.write(val.to_string(), val.to_tu_string().size() + 1);
	}

	// called from root
	void	as_xmlsock::advance(float delta_time)
	{
		switch (m_status)
		{
            case UNDEFINED:
            case READING_MEMBUF:
            case HANDLE_EVENTS:
            case LOAD_INIT:
            case DOWNLOADING:
                break;
                
			case CONNECTING:	// connecting
			{
				bool rc = m_http.is_connected();
				if (rc)
				{
					// established connection
					m_status = CONNECTED;	// connected
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					Uint32 timeout = HTTP_TIMEOUT * 1000;
					if (now  - m_start >= timeout || m_http.is_alive() == false)
					{
						// Timed out.
						as_value function;
						if (get_member("onConnect", &function))
						{
							if (get_root()->is_as3())
							{
								as_function* func = function.to_function();

								as_environment* env = func->get_target()->get_environment();
								assert(env);

								as_event* ev = new as_event("onConnect", this);

								// use m_target from func as  THIS
								env->push(ev);
								call_method(func, env, func->get_target(), 1, env->get_top_index());
								env->drop(1);
							}
							else
							{
								as_environment env;
								env.push(false);
								call_method(function, &env, this, 1, env.get_top_index());
							}

							close();
						}
					}
				}
				break;
			}

			case CONNECTED:	// connected, write request
			{
				as_value function;
				if (get_member("onConnect", &function))
				{
					if (get_root()->is_as3())
					{
						as_function* func = function.to_function();

						as_environment* env = func->get_target()->get_environment();
						assert(env);

						as_event* ev = new as_event("onConnect", this);

						// use m_target from func as  THIS
						env->push(ev);
						call_method(func, env, func->get_target(), 1, env->get_top_index());
						env->drop(1);
					}
					else
					{
						as_environment env;
						env.push(true);
						call_method(function, &env, this, 1, env.get_top_index());
					}
				}
				m_status = READING;	// read reply
				break;
			}

			case READING:	// read reply
			{
				// check data
				int rc = m_http.read_data();

				tu_string str;
				if (m_http.get_string(&str))
				{
					// there is a string
					as_value function;
					if (get_member("onData", &function))
					{
						if (get_root()->is_as3())
						{
							as_function* func = function.to_function();

							as_environment* env = func->get_target()->get_environment();
							assert(env);

							as_event* ev = new as_event("onData", this);
							ev->set_member("data", str.c_str());

							// use m_target from func as  THIS
							env->push(ev);
							call_method(func, env, func->get_target(), 1, env->get_top_index());
							env->drop(1);
						}
						else
						{
							as_environment env;
							env.push(str.c_str());
							call_method(function, &env, this, 1, env.get_top_index());
						}
					}
				}
				
				if (rc == -1)
				{
					close();
					as_value function;
					if (get_member("onClose", &function))
					{
						if (get_root()->is_as3())
						{
							as_function* func = function.to_function();

							as_environment* env = func->get_target()->get_environment();
							assert(env);

							as_event* ev = new as_event("onClose", this);

							// use m_target from func as  THIS
							env->push(ev);
							call_method(func, env, func->get_target(), 1, env->get_top_index());
							env->drop(1);
						}
						else
						{
							as_environment env;
							call_method(function, &env, this, 0, env.get_top_index());
						}
					}
				}
				break;
			}
		}
	}

};
