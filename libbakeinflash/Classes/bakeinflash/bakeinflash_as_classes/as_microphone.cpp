// as_netstream.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_as_classes/as_microphone.h"

namespace bakeinflash
{
	// audio callback is running in sound handler thread
//	static void audio_streamer(as_object* obj, Uint8* stream, int len)
//	{
//		as_microphone* mic = cast_to<as_microphone>(obj);
//		assert(mic);
//		mic->audio_callback(stream, len);
//	}


	//	public static get([index:Number]) : Microphone
	void	microphone_get(const fn_call& fn)
	{
		as_microphone* mic = cast_to<as_microphone>(fn.this_ptr);
		assert(mic);
		get_root()->add_listener(mic);
		fn.result->set_as_object(mic);	// hack, returns this
	}

	as_microphone::as_microphone()
	{
		// no root yet !!!
	}

	as_microphone::~as_microphone()
	{
	}

	void as_microphone::advance(float delta_time)
	{
		bool has_listeners = false;
		for (int i = 0, n = m_listeners.size(); i < n; i++)
		{
			if (m_listeners[i] != NULL)
			{
				has_listeners = true;
				break;
			}
		}

		if (has_listeners == false)
		{
			sound_handler* sound = get_sound_handler();
			if (sound)
			{
			//	sound->detach_aux_streamer(this);
			}
		}
	}

	// it is running in sound mixer thread
	void as_microphone::audio_callback(Uint8* stream, int len)
	{
		return;
	}

	// it is running in sound mixer thread
	void as_microphone::add_listener(as_object* mc)
	{
		m_listeners.push_back(mc);
	}

	as_object* microphone_init()
	{
		// Create built-in math object.
		as_object*	obj = new as_microphone();
		obj->builtin_member("get", microphone_get);
		return obj;
	}

} // end of bakeinflash namespace
