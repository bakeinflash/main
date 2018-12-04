// as_netstream.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_MICROPHONE_H
#define BAKEINFLASH_MICROPHONE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/tu_queue.h"
#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_mutex.h"

namespace bakeinflash
{
	
	struct as_microphone : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_MICROPHONE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_microphone();
		virtual ~as_microphone();
		void audio_callback(Uint8* stream, int len);
		void add_listener(as_object* mc);
		virtual void advance(float delta_time);


	private:

		array< weak_ptr<as_object> > m_listeners;

	};

	as_object* microphone_init();

} // end of bakeinflash namespace

// bakeinflash_NETSTREAM_H
#endif

