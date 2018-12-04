// bakeinflash_sound_handler_xaudio2.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// xaudio2 based sound handler for mobile units

#ifndef SOUND_HANDLER_xaudio2_H
#define SOUND_HANDLER_xaudio2_H

#include "base/tu_config.h"

#include "bakeinflash/bakeinflash.h"
#include "base/container.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_mutex.h"
#include "bakeinflash/bakeinflash_listener.h"

#include <XAudio2.h>

#if TU_CONFIG_LINK_TO_FFMPEG == 1
extern "C" 
{
	#include <libavcodec/avcodec.h>
}
#endif

namespace bakeinflash
{

	struct sound;

	struct xaudio2_sound_handler : public sound_handler
	{
//    IXAudio2* m_audio;
    IXAudio2MasteringVoice* m_masteringVoice;

		tu_mutex m_mutex;
		array< smart_ptr<sound> > m_sound;
		float m_max_volume;

		// streaming
		struct stream_def
		{
	//		ALuint source, buffers[NUM_xaudio2_STREAM_BUFFERS];
			int buffer_index;
		};
		hash<int, stream_def> m_aux_streams;

		struct play_action_def
		{
			bool stop_playback;
			int handler_id;
			int loop_count;
			int skip_frame;
		};
		array<play_action_def> m_play_action;

		xaudio2_sound_handler();
		virtual ~xaudio2_sound_handler();

		virtual bool is_open() { return true; };

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

		virtual void pause(int sound_handle, bool paused);
		virtual int get_position(int sound_handle);
		virtual	void advance(float delta_time);

		virtual int create_stream();
		virtual void push_stream(int stream_id, int format, Uint8* data, int size, int sample_rate, int channels);
		virtual void close_stream(int stream_id);
		virtual bool is_stream_available(int stream_id);

		virtual void push_action(bool stop_playback, int handler_id, int loop_count);

	};


	// Used to hold the sounddata
	struct sound: public bakeinflash::ref_counted
	{
		sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
			int sample_rate, bool stereo);
		~sound();

		void stop();

		// return value is in [0..100]
		inline int get_volume() const
		{
			return (int) (m_volume * 100.0f);
		}

		// vol is in [0..100]
		void set_volume(int vol);

		void append(void* data, int size, xaudio2_sound_handler* handler);
		void play(int loops, xaudio2_sound_handler* handler);
		void pause(bool paused);
		int  get_played_bytes();
		void advance(float delta_time);
		bool is_stoped();

//	private:

		float m_volume;
		bakeinflash::sound_handler::format_type m_format;
		int m_sample_count;
		int m_sample_rate;
		bool m_stereo;
		bool m_is_paused;
		smart_ptr<listener> m_listeners;	// onSoundComplete event listeners
		int m_loops;

		XAUDIO2_BUFFER m_buffer;
    IXAudio2SourceVoice* m_source;

		int m_compressed_size;
		Uint8* m_compressed_data;
	};

}

#endif
