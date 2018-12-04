// as_xml.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "pugixml/pugixml.hpp"
#include "bakeinflash/bakeinflash_root.h"
#include "as_xml.h"
#include "as_global.h"
#include "as_urlvariables.h"
#include "as_event.h"
#include "base/tu_file.h"
#include "base/tu_timer.h"

namespace bakeinflash
{
  
	void	as_systemLoadURL(const fn_call& fn);
	
	void	as_xml_ctor(const fn_call& fn)
	{
		as_xml*	obj = new as_xml();

		// set text value
		if (fn.nargs > 0)
		{
			obj->set_data(fn.arg(0).to_tu_string());	
		}
		fn.result->set_as_object(obj);
	}

	void	as_xml_bytes_loaded(const fn_call& fn)
	{
		as_xml* obj = cast_to<as_xml>(fn.this_ptr);
		if (obj)
		{
			int size = obj->m_http.size();
			fn.result->set_int(size);
		}
	}

	void	as_xml_bytes_total(const fn_call& fn)
	{
		as_xml* obj = cast_to<as_xml>(fn.this_ptr);
		if (obj)
		{
			int size = obj->m_http.size();
			fn.result->set_int(size);
		}
	}

	void	as_global_loadvars_ctor(const fn_call& fn)
	{
		as_loadvars*	obj = new as_loadvars();
		fn.result->set_as_object(obj);
	}

	void	as_urlloader_ctor(const fn_call& fn)
	{
		as_urlloader*	obj = new as_urlloader();
		fn.result->set_as_object(obj);
	}

	void parse_doc(as_object* parent_obj, as_object* obj, pugi::xml_node& doc)
	{
		// fixme: memory leaks

		string_hash<int> node_index;
		for (pugi::xml_node node = doc.first_child(); node; node = node.next_sibling())
		{
			if (*node.name() == 0 && parent_obj)
			{
				parent_obj->set_member(doc.name(), string_to_value(node.value()));
				return;
			}

			// new child
			as_object* new_obj = new as_object();

			int k = 0;
			node_index.get(node.name(), &k);

			if (k > 0)
			{
				as_array* a;
				if (k == 1)
				{
					a = new as_array();
					as_value val;
					obj->get_member(node.name(), &val);
					a->push(val);
					obj->set_member(node.name(), a);
				}
				else
				{
					as_value val;
					obj->get_member(node.name(), &val);
					a = cast_to<as_array>(val.to_object());
				}

				if (a)
				{
					a->push(new_obj);
				}
			}
			else
			{
				obj->set_member(node.name(), new_obj);
			}

			k++;
			node_index.set(node.name(), k);

			// set attr
			for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait)
			{
				new_obj->set_member(ait->name(), string_to_value(ait->value()));
			}
			parse_doc(obj, new_obj, node);
		}
	}

	//	public decode(queryString:String) : Void
	void	as_loadvars_decode(const fn_call& fn)
	{
		as_loadvars* lv = cast_to<as_loadvars>(fn.this_ptr);
		if (lv && fn.nargs > 0)
		{
			lv->decode(fn.arg(0).to_tu_string());
		}
	}

	void	as_xml_loaded(const fn_call& fn)
	{
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml == NULL)
		{
			return;
		}

		int status = xml->m_http.get_status();
		fn.result->set_bool(status == 200);
	}

	void	as_xml_content_type_getter(const fn_call& fn)
	{
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml == NULL)
		{
			return;
		}
		fn.result->set_tu_string(xml->m_content_type);
	}

	void	as_xml_content_type_setter(const fn_call& fn)
	{
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml == NULL || fn.nargs ==  0)
		{
			return;
		}
		xml->m_content_type = fn.arg(0).to_tu_string();
	}

	

	// public addRequestHeader(header:Object, headerValue:String) : Void
	void	as_xml_addRequestHeader(const fn_call& fn)
	{
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml == NULL || fn.nargs < 2)
		{
			return;
		}
		xml->addRequest(fn.arg(0).to_tu_string(), fn.arg(1));
	}

	//load(url:String) : Boolean
	void	as_xml_load(const fn_call& fn)
	{ 
		fn.result->set_bool(false);
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml && fn.nargs >= 1)
		{
			bool rc = xml->load(fn.arg(0).to_tu_string(), xml, "");
			fn.result->set_bool(rc);
		}
	}

	// load(request:URLRequest):void
	void	as_urlloader_load(const fn_call& fn)
	{ 
		as_urlloader* loader = cast_to<as_urlloader>(fn.this_ptr);
		if (loader && fn.nargs >= 1)
		{
			as_urlrequest* request = cast_to<as_urlrequest>(fn.arg(0).to_object());
			if (request)
			{
				bool rc = loader->load(request->get_url(), loader, "");
			}
			else
			{
				// TODO: send error
			}
		}
	}

	// public sendAndLoad(url:String, resultXML:XML) : Void
	void	as_xml_sendload(const fn_call& fn)
	{ 
		fn.result->set_bool(false);
		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml && fn.nargs >= 2)
		{
			tu_string method = fn.nargs > 2 ? fn.arg(2).to_tu_string() : "";
			bool rc = xml->load(fn.arg(0).to_tu_string(), cast_to<as_xml>(fn.arg(1).to_object()), method);
			fn.result->set_bool(rc);
		}
	}

	// public parseXML(value:String) : Void
	void	as_xml_parseXML(const fn_call& fn)
	{ 
		if (fn.nargs < 1)
		{
			return;
		}

		as_xml* xml = cast_to<as_xml>(fn.this_ptr);
		if (xml)
		{
			xml->set_data(fn.arg(0).to_tu_string());
		}
	}

	// AS3
	// public function addEventListener(type:String, listener:Function, useCapture:Boolean = false, priority:int = 0, useWeakReference:Boolean = false):void
	void as_urlloader_addlistener(const fn_call& fn)
	{
		as_urlloader* loader = cast_to<as_urlloader>(fn.this_ptr);
		if (fn.nargs >= 2 && loader)
		{
			// arg #1 - event
			// arg #2 - function
			const tu_string& name = fn.arg(0).to_tu_string();
			loader->builtin_member(name, fn.arg(1).to_function());
		}
	}	

	as_xml::as_xml() :
		m_status(UNDEFINED),	// not connected

		// The default is application/x-www-form-urlencoded, which is the standard MIME content type used for most HTML forms.
		m_content_type("application/x-www-form-urlencoded")
	{
		builtin_member("load" , as_xml_load);
		builtin_member("sendAndLoad" , as_xml_sendload);
		builtin_member("parseXML" , as_xml_parseXML);
		builtin_member("addRequestHeader" , as_xml_addRequestHeader);
		builtin_member("loaded", as_value(as_xml_loaded, as_value()));
		builtin_member("toString", as_toString);
		builtin_member("bytesLoaded", as_xml_bytes_loaded);
		builtin_member("bytesTotal", as_xml_bytes_total);
		builtin_member("contentType", as_value(as_xml_content_type_getter, as_xml_content_type_setter));

		use_proto("XML");
	}

	as_loadvars::as_loadvars()
	{
		builtin_member("decode" , as_loadvars_decode);
		use_proto("LoadVars");
	}

	as_urlloader::as_urlloader()
	{
		builtin_member("load" , as_urlloader_load);		// override
		builtin_member("addEventListener", as_urlloader_addlistener);

		use_proto("URLLoader");
	}

	as_xml::~as_xml()
	{
	}

	as_loadvars::~as_loadvars()
	{
	}

	as_urlloader::~as_urlloader()
	{
	}

	void	as_xml::handle_events()
	{
		// must be here because event hadler may cause sendandload again
		m_status = UNDEFINED;
		get_root()->remove_listener(this);

		if (m_reply_xml != NULL)
		{
			as_value func;
			if (m_reply_xml->get_member("onHTTPStatus", &func))
			{
				as_environment env;
				env.push(m_http.m_status.c_str());
				call_method(func, &env, m_reply_xml.get(), 1, env.get_top_index());
			}
				
			// if there is onData then onLoad is not called
			if (m_reply_xml->get_member("onData", &func))
			{
				if (get_root()->is_as3())
				{
					as_function* f = func.to_function();

					as_environment* env = f->get_target()->get_environment();
					assert(env);

					as_event* ev = new as_event("onData", this);

					// target.data = <data>
					set_member("data", m_reply_xml->m_data.c_str());

					// use m_target from func as  THIS
					env->push(ev);
					call_method(func, env, f->get_target(), 1, env->get_top_index());
					env->drop(1);
				}
				else
				{
					as_environment env;
					env.push(m_reply_xml->m_data.c_str());
					call_method(func, &env, m_reply_xml.get(), 1, env.get_top_index());
				}
			}
			else
			if (m_reply_xml->get_member("onLoad", &func))
			{
				as_environment env;
				env.push(m_reply_xml->m_data.size() > 0 ? true : false);
				call_method(func, &env, m_reply_xml.get(), 1, env.get_top_index());
			}
		}
	}

	void	as_xml::advance(float delta_time)
	{
		switch (m_status)
		{
			case UNDEFINED:
			case LOAD_INIT:
			case READING_MEMBUF:
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
						m_status = HANDLE_EVENTS;
					}
				}
				break;
			}

			case CONNECTED:	// connected, write request
			{
				m_start = tu_timer::get_ticks();
				m_http.write_http(to_string());
				m_status = READING;	// read reply
				break;
			}

			case READING:	// read reply
			{
				// read datam_data
				const void* data = NULL;
				int size = 0;
				bool rc = m_http.read_http(&data, &size, &m_reply_xml->m_http.m_headers);
				if (rc)
				{
					if (m_reply_xml != NULL)
					{
						m_reply_xml->set_data((const char*) data, size);
					}
					m_status = HANDLE_EVENTS;
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					Uint32 timeout = HTTP_TIMEOUT * 1000;
					if (now  - m_start >= timeout)
					{
						// Timed out.
						m_status = HANDLE_EVENTS;
					}
				}
				break;
			}

			case HANDLE_EVENTS:	// read reply
			{
				if (m_reply_xml != NULL)
				{
					as_value val;
					string_hash<as_value>* headers = &m_reply_xml->m_http.m_headers;
					if (headers->get("Connection", &val) && val.to_tu_string() == "Close")
					{
						m_http.close();
					}
				}
				handle_events();
				break;
			}
			
		}
	}

	void as_xml::set_data(const tu_string& src)
	{
		m_data = src;
	//	reset_data();
	}

	void as_xml::set_data(const char* data, int size)
	{
		m_data = tu_string(data, size);
	//	reset_data();
	}

	as_object* as_xml::parse_xml(const tu_string& str)
	{
		// fixme
		if (str.size() > 0)
		{
			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load(str.c_str());
			if (result.status == pugi::status_ok)
			{
				as_object* obj = new as_object();
				parse_doc(NULL, obj, doc);
				return obj;
			}
			myprintf("XML error %d: %s\n", result.status, result.description());
			myprintf("%s\n", str.c_str());
		}
		return NULL;
	}

	bool	as_xml::get_member(const tu_string& name, as_value* val)
	{
		if (as_object::get_member(name, val))
		{
			return true;
		}

		// try xml-doc
//		if (m_doc)
//		{
//			return m_doc->get_member(name, val);
//		}

		// try headers
		if (m_http.m_headers.get(name, val))
		{
			return true;
		}

		return false;
	}

	void as_xml::addRequest(const tu_string& name, const as_value& val)
	{
		// fixme: set as array
		m_http.m_headers[name] = val;
	}

	bool as_xml::load(const tu_string& url, as_xml* reply, const tu_string& method)
	{
		m_http.close();
		m_http.set_method(method);

		//as_value trace;
		//get_member("trace", &trace);
		//m_http.set_trace(trace.to_bool());

		m_reply_xml = reply ? reply : this;
		m_reply_xml->m_data.clear();

		// first try local file
		tu_string finame = get_workdir();
		finame += url;
		tu_file fi(finame.c_str(), "r");
		if (fi.get_error() == TU_FILE_NO_ERROR)
		{
			int n = fi.size();
			char* buf = (char*) malloc(n);
			fi.read_bytes(buf, n);
			m_reply_xml->set_data(buf, n);
			free(buf);

			m_url = finame;

			// handle events in next frame if no other states
			m_status = HANDLE_EVENTS;
			get_root()->add_listener(this);
			return true;
		}


		// form param string from memebers 
		tu_string params;
		for (string_hash<as_value>::const_iterator it = m_members.begin(); it != m_members.end(); ++it ) 
		{
			// do not take functions, hack
			const as_value& val =  it->second;
			if (val.is_enum() && val.is_function() == false && val.to_tu_string().size() > 0)
			{
				params += params.size() > 0 ? '&' : '?';
				params += it->first;
				params += '=';
				params += val.to_tu_string();
			}
		}

		m_url = url + params;
		m_http.connect(m_url);

		// handle events in next frame
		m_status = CONNECTING;	// connecting
		m_start = tu_timer::get_ticks();
		get_root()->add_listener(this);

		return true;

		/*
#ifdef iOS
		as_value val;
		as_environment env();
		env.push(fn.arg(0));
		as_systemLoadURL(fn_call(&val, NULL,  &env, 1, env.get_top_index()));
		xml->m_data = val.to_tu_string();
		
//		myprintf("%s\n",val.to_string());
		fn.result->set_bool(true);
		
		// handle events in next frame
		xml->m_start = tu_timer::get_ticks();
		xml->get_root()->add_listener(xml);
		
		return;
#endif
		*/
	}

	const char*	as_xml::to_string()
	{
		return m_data.c_str();
	}

	// parse request
	void as_loadvars::decode(const tu_string& str)
	{
		if (m_content_type == "application/x-www-form-urlencoded")
		{
			if (str.size() > 0)
			{
				array<tu_string> pairs;
				str.split('&', &pairs);
				for (int k = 0; k < pairs.size(); k++)
				{
					const char* pair = pairs[k].c_str();

					// '&amp;' ==> &
					if (k > 0 && strncmp(pair, "amp;", 4) == 0)
					{
						pair += 4;
					}

					// seek the first '='
					const char* p = pair;
					for (; *p && *p != '='; p++);

					if (*p)
					{
						tu_string name(pair, p - pair);
						as_object::set_member(name, p + 1);
					}
				}
			}
		}
		else
		{
			m_data = str;
		}
	}

	void as_loadvars::encode()
	{
		if (m_content_type == "application/x-www-form-urlencoded")
		{
			tu_string str;
			string_hash<as_value>::const_iterator it = m_members.begin();
			while (it != m_members.end())
			{
				// do not take functions, hack
				if (it->second.is_enum() && it->second.is_function() == false)
				{
					if (str.size() > 0)
					{
						str += '&';
					}
					str += it->first;
					str += '=';
					str += it->second.to_tu_string();
				}
				++it;
			}
			m_data = str;
		}
		else
		{
			// assume m_data has data already
		}
	}

	void as_loadvars::set_data(const tu_string& src)
	{
		m_data = src;
		decode(src);
	}

	const char*	as_loadvars::to_string()
	{
		encode();
		return m_data.c_str();
	}

} // end of bakeinflash namespace
