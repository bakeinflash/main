// as_event.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

//
//  AS3, as scrip Event class
//

#include "bakeinflash/bakeinflash_as_classes/as_event.h"

namespace bakeinflash
{

	void	as_global_event_ctor(const fn_call& fn)
	// Constructor for ActionScript class Array.
	{
		const char* eventname = fn.nargs > 0 ? fn.arg(0).to_string() : "";

		// first try pre-allocated object as this_ptr
		as_object* obj = cast_to<as_object>(fn.this_ptr);

		if (fn.this_ptr == NULL)
		{
			obj = new as_event(eventname, NULL);
		}
		else
		{
			obj->set_member("type", eventname);
			obj->set_member("target", as_value());
		}

		fn.result->set_as_object(obj);
	}

	void	as_event_tostring(const fn_call& fn)
	{
		as_event* obj = cast_to<as_event>(fn.this_ptr);
		if (obj)
		{
			// [Event type="onMyEvent" bubbles=false cancelable=false eventPhase=2]
			tu_string str = "[Event type='";

			as_value val;
			obj->get_member("type", &val);

			str += val.to_tu_string();
			str += "']";
			fn.result->set_tu_string(str);
		}
	}

	//
	//	 for new Event("onMyEvent")
	//
	as_event::as_event(const tu_string& type, as_object* target)
	{
		// methods
		builtin_member("toString", as_event_tostring);

		// properties
		// todo read-only
		builtin_member("type", type.c_str());
		builtin_member("target", target);

		set_ctor(as_global_event_ctor);
	}

	as_event::~as_event()
	{
	}

	//
	// this is "_global.Event" object
	//
	as_global_event::as_global_event() :
		as_c_function(as_global_event_ctor)
	{
		builtin_member("ENTER_FRAME", "onEnterFrame");
		builtin_member("CONNECT", "onConnect");
		builtin_member("CLOSE", "onClose");
		builtin_member("COMPLETE", "onData"); //"onComplete");
	}

	//
	// this is "_global.MouseEvent" object
	//
	as_global_mouseevent::as_global_mouseevent() :
		as_c_function(as_global_event_ctor)
	{
		builtin_member("CLICK", "onRelease");
		builtin_member("MOUSE_DOWN", "onPress");
		builtin_member("MOUSE_UP", "onRelease");
		builtin_member("MOUSE_OVER", "onRollOver");
		builtin_member("MOUSE_MOVE", "onMouseMove");
		builtin_member("MOUSE_OUT", "onRollOut");
	}

	//
	// this is "_global.DataEvent" object
	//
	as_global_dataevent::as_global_dataevent() :
		as_c_function(as_global_event_ctor)
	{
		builtin_member("DATA", "onData");
	}

		//
	// this is "_global.IOErrorEvent" object
	//
	as_global_ioerrorevent::as_global_ioerrorevent() :
		as_c_function(as_global_event_ctor)
	{
		builtin_member("IO_ERROR", "onError");
	}

		//
	// this is "_global.SecurityErrorEvent" object
	//
	as_global_securityerrorevent::as_global_securityerrorevent() :
		as_c_function(as_global_event_ctor)
	{
		builtin_member("SECURITY_ERROR", "onSecurityError");
	}


};
