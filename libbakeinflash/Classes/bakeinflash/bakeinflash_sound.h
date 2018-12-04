// bakeinflash_sound.h   -- Thatcher Ulrich, Vitaly Alexeev

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_SOUND_H
#define BAKEINFLASH_SOUND_H


#include "bakeinflash/bakeinflash_impl.h"


namespace bakeinflash
{
	int get_sample_rate(int index);

	// Utility function to uncompress ADPCM.
	void	adpcm_expand(
		void* data_out,
		stream* in,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo);

	struct sound_envelope
	{
		Uint32 m_mark44;
		Uint16 m_level0;
		Uint16 m_level1;
	};

	struct sound_sample : public character_def
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_SOUND_SAMPLE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character_def::is(class_id);
		}

		int	m_sound_handler_id;

		sound_sample(int id) :
			m_sound_handler_id(id)
		{
		}

		virtual ~sound_sample();
	};

}


#endif // bakeinflash_SOUND_H
