// as_cef.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2015

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// CEF embedd browser implementation


#ifndef bakeinflash_AS_CEF_H
#define bakeinflash_AS_CEF_H

#if TU_CONFIG_LINK_CEF == 1

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_types.h"	// for rgba
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

#include "cef/include/capi/cef_app_capi.h"	
#include "cef/include/capi/cef_browser_capi.h"		// browser API
#include "cef/include/capi/cef_client_capi.h"		// client API

namespace bakeinflash
{

	struct as_cef : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IWEB_VIEW };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_cef(sprite_instance* mc);
		~as_cef();
		virtual void advance(float delta_time);

		tu_string m_url;
		cef_browser_t* m_browser;

		cef_window_info_t m_window_info;
		cef_browser_settings_t m_bsettings;
		smart_ptr<bitmap_info> m_bi;

		cef_client_t m_client;
		weak_ptr<as_cef> this_ptr_for_client;

		cef_render_handler_t m_render;
		weak_ptr<as_cef> this_ptr_for_render;

		cef_request_handler_t m_requesthandler;
		weak_ptr<as_cef> this_ptr_for_requesthandler;

		weak_ptr<sprite_instance> m_parent;
	};

	void	as_global_cef_ctor(const fn_call& fn);

}	// namespace bakeinflash

#endif
#endif
