// as_mcloader.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_MCLOADER_H
#define BAKEINFLASH_AS_MCLOADER_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_as_classes/as_xml.h"

namespace bakeinflash
{

	void	as_global_mcloader_ctor(const fn_call& fn);

	struct as_mcloader : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_MCLOADER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		enum mcl_event
		{
			mcl_UNDEFINED,
			onLoadComplete,
			onLoadError,
			onLoadInit ,
			onLoadProgress,
			onLoadStart
		};

		struct loaderdef : public ref_counted
		{
			net_status m_status;
			tcp m_http;
			Uint32 m_start;
			tu_string m_url;

			weak_ptr<sprite_instance> m_target;
		};
		array< smart_ptr<loaderdef> > m_ld;
		array< weak_ptr<as_object> > m_listeners;

		as_mcloader();
		virtual ~as_mcloader();
		virtual void advance(float delta_time);
		virtual void handle_events() { assert(0); };
		void handle(int index, mcl_event ev);
		bool load(const tu_string& url, sprite_instance* target);
		//void set_target(const char* data, int size);
		void set_target(int index, tu_file* in);
		void set_target(int index, sprite_instance* mc);

	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
