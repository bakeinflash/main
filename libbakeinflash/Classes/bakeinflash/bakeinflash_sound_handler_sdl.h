// bakeinflash_sound_handler_sdl.h	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef SOUND_HANDLER_SDL_H
#define SOUND_HANDLER_SDL_H

#include "base/tu_config.h"

#ifdef TU_USE_SDL_SOUND

#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_thread.h>

#include "bakeinflash/bakeinflash.h"
#include "base/container.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_mutex.h"
#include "bakeinflash/bakeinflash_listener.h"
#include "bakeinflash/bakeinflash_sound.h"
#include "base/tu_file.h"
#include "bakeinflash/bakeinflash_stream.h"

#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
extern "C" 
{
	#include <libavformat/avformat.h>
}

#endif

// Used to hold the info about active sounds

namespace bakeinflash
{

	struct sound;
	struct active_sound;
	sound_handler*	create_sound_handler_sdl();

	// Use SDL and ffmpeg to handle sounds.
	struct SDL_sound_handler : public sound_handler
	{

		// NetStream audio callbacks
		hash<as_object* /* netstream */, aux_streamer_ptr /* callback */> m_aux_streamer;
		hash<int, smart_ptr<sound> > m_sound;
		float m_max_volume;
		tu_mutex m_mutex;

		// SDL_audio specs
		SDL_AudioSpec m_audioSpec;

		// Is sound device opened?
		bool m_is_open;

		SDL_sound_handler();
		virtual ~SDL_sound_handler();

		virtual bool is_open() { return m_is_open; };

		// loads external sound file, only .WAV for now
		virtual int	load_sound(const char* url);

		// Called to create a sample.
		virtual int	create_sound(void* data, int data_bytes,
			int sample_count, format_type format,
			int sample_rate, bool stereo);

		virtual void append_sound(int sound_handle, void* data, int data_bytes);

		// Play the index'd sample.
		virtual void	play_sound(as_object* listener_obj, int sound_handle, int loop_count);

		virtual void	set_max_volume(int vol);

		virtual void	stop_sound(int sound_handle);

		// this gets called when it's done with a sample.
		virtual void	delete_sound(int sound_handle);

		// this will stop all sounds playing.
		virtual void	stop_all_sounds();

		// returns the sound volume level as an integer from 0 to 100.
		virtual int	get_volume(int sound_handle);

		virtual void	set_volume(int sound_handle, int volume);

		virtual void	attach_aux_streamer(bakeinflash::sound_handler::aux_streamer_ptr ptr,
			as_object* netstream);
		virtual void	detach_aux_streamer(as_object* netstream);

		// Converts input data to the SDL output format.
		void cvt(short int** adjusted_data, int* adjusted_size, unsigned char* data, int size, 
			int channels, int freq);

		virtual void pause(int sound_handle, bool paused);
		virtual int get_position(int sound_handle);
	};

	// Used to hold the sounddata
	struct sound: public ref_counted
	{
		sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
			int sample_rate, bool stereo);
		virtual ~sound();

		inline void clear_playlist()
		{
			m_playlist.clear();
		}

		// return value is in [0..100]
		inline int get_volume() const
		{
			return (int) (m_volume * 100.0f);
		}

		// vol is in [0..100]
		inline void set_volume(int vol)
		{
			m_volume = (float) vol / 100.0f;
		}

		void append(void* data, int size, SDL_sound_handler* handler);
		void play(int loops, SDL_sound_handler* handler);
		bool mix(Uint8* stream,	int len, array< smart_ptr<listener> >* listeners, float max_volume);
		void pause(bool paused);
		int  get_played_bytes();

//	private:

		Uint8* m_data;
		int m_size;
		float m_volume;
		bakeinflash::sound_handler::format_type m_format;
		int m_sample_count;
		int m_sample_rate;
		bool m_stereo;
		array<smart_ptr<active_sound> > m_playlist;
		bool m_is_paused;
		smart_ptr<listener> m_listeners;	// onSoundComplete event listeners
	};


	struct active_sound: public ref_counted
	{
		active_sound(sound* parent, int loops):
		m_pos(0),
		m_played_bytes(0),
		m_loops(loops),
		m_parent(parent)
	{
		m_handler = (SDL_sound_handler*) get_sound_handler();

		if (m_handler == NULL)
		{
			return;
		}
		
#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
		if (m_parent->m_format == sound_handler::FORMAT_MP3)
		{
			// Init MP3 the parser
			m_parser = av_parser_init(AV_CODEC_ID_MP3);

			m_cc = avcodec_alloc_context3(m_handler->m_MP3_codec);
			if (m_cc == NULL)
			{
				myprintf("Could not alloc MP3 codec context\n");
				return;
			}

			if (avcodec_open2(m_cc, m_handler->m_MP3_codec, NULL) < 0)
			{
				myprintf("Could not open MP3 codec\n");
				avcodec_close(m_cc);
				return;
			}
		}
#endif
	}

	virtual ~active_sound()
	{
#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
		if (m_parent->m_format == sound_handler::FORMAT_MP3)
		{
			if (m_cc)
			{
				avcodec_close(m_cc);
				av_free(m_cc);
			}

			if (m_parser)
			{
				av_parser_close(m_parser);
			}
		}
#endif
	}


	// returns the current sound position
	inline int get_played_bytes()
	{
		return m_played_bytes;
	}

	// returns true if the sound is played
	bool mix(Uint8* mixbuf , int mixbuf_len);

	private:

		int m_pos;
		int m_played_bytes;	// total played bytes, it's used in get_position()
		int m_loops;
		sound* m_parent;
		int m_bufsize;
		SDL_sound_handler* m_handler;
	};

}

#endif // SOUND_HANDLER_SDL_H

#endif // TU_USE_SDL

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
