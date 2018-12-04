// as_xml.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_AS_XML_H
#define BAKEINFLASH_AS_XML_H

#include "base/tu_config.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_tcp.h"

namespace bakeinflash
{

	void	as_xml_ctor(const fn_call& fn);
	void	as_global_loadvars_ctor(const fn_call& fn);
	void	as_urlloader_ctor(const fn_call& fn);

	struct as_xml : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_XML };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		tcp m_http;
		tu_string m_data;
		tu_string m_url;
	//	smart_ptr<as_object> m_doc;
		weak_ptr<as_xml> m_reply_xml;
		Uint32 m_start;
		net_status m_status;
		tu_string m_content_type;

		as_xml();
		virtual ~as_xml();

		virtual void	advance(float delta_time);
		virtual void set_data(const tu_string& str);
		virtual void set_data(const char* data, int size);
		as_object* parse_xml(const tu_string& str);
		virtual void addRequest(const tu_string& name, const as_value& val);
		virtual bool load(const tu_string& url, as_xml* reply, const tu_string& method);
		virtual void handle_events();

		//		 virtual bool	set_member(const tu_string& name, const as_value& val);
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual const char*	to_string();
	};

	struct as_loadvars : public as_xml
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_LOADVARS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_xml::is(class_id);
		}

		as_loadvars();
		virtual ~as_loadvars();

		void decode(const tu_string& str);
		void encode();
		virtual void set_data(const tu_string& str);
		virtual const char*	to_string();
	};

	struct as_urlloader : public as_xml
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_URLLOADER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_xml::is(class_id);
		}

		as_urlloader();
		virtual ~as_urlloader();

		//	void decode(const tu_string& str);
		//	void encode();
		//	virtual void set_data(const tu_string& str);
		//	virtual const char*	to_string();
	};


}	// end namespace bakeinflash

#endif // bakeinflash_AS_XML_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
