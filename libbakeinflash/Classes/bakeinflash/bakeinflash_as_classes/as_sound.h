// as_sound.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_SOUND_H
#define BAKEINFLASH_AS_SOUND_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	void	as_global_sound_ctor(const fn_call& fn);

	struct as_sound : public as_object
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_SOUND };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_sound() :
			m_id(-1),
			m_is_loaded_sound(false),
			iOS_sound(NULL)
		{
		}

		virtual ~as_sound();
		void clear();

		// id of the sound
		int m_id;
		void* iOS_sound;

		bool m_is_loaded_sound;
		weak_ptr<character> m_target;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_SOUND_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
