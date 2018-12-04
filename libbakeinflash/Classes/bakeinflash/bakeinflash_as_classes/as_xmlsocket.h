// as_xmlsocket.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script XMLSocket implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_XMLSOCKET_H
#define BAKEINFLASH_AS_XMLSOCKET_H

#include "base/tu_config.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object

#include "bakeinflash/bakeinflash_tcp.h"

namespace bakeinflash
{

	void	as_global_xmlsock_ctor(const fn_call& fn);

	struct as_xmlsock : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_XML_SOCKET };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		tcp m_http;
		Uint32 m_start;
		net_status m_status;

		as_xmlsock();
		virtual ~as_xmlsock();

		virtual void advance(float delta_time);
		void add_event_listener(const tu_string& eventname, as_function* handler);

		bool connect(const char* host, int port);
		void close();
		void send(const as_value& val);
	};

}	// end namespace bakeinflash

#endif // bakeinflash_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
