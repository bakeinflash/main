// bakeinflash_sound_handler_sdl.cpp	-- Thatcher Ulrich http://tulrich.com 2003
// bakeinflash_sound_handler_sdl.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_sound_handler_sdl.h"

#ifdef TU_USE_SDL_SOUND

namespace bakeinflash
{

#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
	static AVCodec* s_avcodec = NULL;
#endif

	tu_mutex& bakeinflash_engine_mutex();
	static void sdl_audio_callback(void *udata, Uint8 *stream, int len); // SDL C audio handler

	SDL_sound_handler::SDL_sound_handler():
		m_max_volume(1.0f),
		m_is_open(true)
	{
		// This is our sound settings
		m_audioSpec.freq = 44100;
		m_audioSpec.format = AUDIO_S16SYS;
		m_audioSpec.channels = 2;
		m_audioSpec.callback = sdl_audio_callback;
		m_audioSpec.userdata = this;
		m_audioSpec.samples = 4096;

#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
				avcodec_register_all();
				s_avcodec = avcodec_find_decoder(AV_CODEC_ID_MP3);
				if (s_avcodec == NULL)
				{
					printf("MP3 codec not found\n");
				}
#endif

		if (SDL_OpenAudio(&m_audioSpec, NULL) < 0 )
		{
			myprintf("Unable to start sound handler: %s\n", SDL_GetError());
			m_is_open = false;
			return;
		}

		SDL_PauseAudio(1);
	}

	SDL_sound_handler::~SDL_sound_handler()
	{
		if (m_is_open) SDL_CloseAudio();
		m_sound.clear();
	}

	// loads external sound file
	// TODO: load MP3, ...
	int	SDL_sound_handler::load_sound(const char* url)
	{
		SDL_AudioSpec wav_spec;
		Uint32	data_bytes;
		Uint8*	data;

		if (SDL_LoadWAV(url, &wav_spec, &data, &data_bytes) == NULL)
		{
			myprintf("loadSound: can't load %s\n", url);
			return -1;
		}

		int id = create_sound(
			data,
			data_bytes,
			wav_spec.samples,
			FORMAT_NATIVE16,
			wav_spec.freq, 
			wav_spec.channels < 2 ? false : true);

		SDL_FreeWAV(data);

		return id;
	}

	// may be used for the creation stream sound head with data_bytes=0 
	int	SDL_sound_handler::create_sound(
		void* data,
		int data_bytes,
		int sample_count,
		format_type format,
		int sample_rate,
		bool stereo)
		// Called to create a sample.  We'll return a sample ID that
		// can be use for playing it.
	{
		m_mutex.lock();

		int sound_id = m_sound.size();
		m_sound[sound_id] = new sound(data_bytes, (Uint8*) data, format, sample_count, sample_rate, stereo);
		m_mutex.unlock();
		return sound_id;
	}

	void	SDL_sound_handler::set_max_volume(int vol)
	{
		if (vol >= 0 && vol <= 100)
		{
			m_max_volume = (float) vol / 100.0f;
		}
	}

	void	SDL_sound_handler::append_sound(int sound_handle, void* data, int data_bytes)
	{
		m_mutex.lock();
		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->append(data, data_bytes, this);
		}
		m_mutex.unlock();
	}

	void SDL_sound_handler::pause(int sound_handle, bool paused)
	{
		m_mutex.lock();

		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->pause(paused);
		}

		m_mutex.unlock();
	}

	void	SDL_sound_handler::play_sound(as_object* listener_obj, int sound_handle, int loops)
	// Play the index'd sample.
	{
		m_mutex.lock();

		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			if (listener_obj)
			{
				// create listener
				if (it->second->m_listeners == NULL)
				{
					it->second->m_listeners = new listener();
				}
				it->second->m_listeners->add(listener_obj);
			}

			it->second->play(loops, this);
		}

		m_mutex.unlock();
	}

	void	SDL_sound_handler::stop_sound(int sound_handle)
	{
		m_mutex.lock();

		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->clear_playlist();
		}

		m_mutex.unlock();
	}


	void	SDL_sound_handler::delete_sound(int sound_handle)
	// this gets called when it's done with a sample.
	{
		m_mutex.lock();

		//		stop_sound(sound_handle);
		m_sound.erase(sound_handle);

		m_mutex.unlock();
	}

	void	SDL_sound_handler::stop_all_sounds()
	{
//		m_mutex.lock();

//		for (hash<int, smart_ptr<sound> >::iterator it = m_sound.begin(); it != m_sound.end(); ++it)
//		{
//			it->second->clear_playlist();
//		}
//		m_mutex.unlock();
	}

	//	returns the sound volume level as an integer from 0 to 100,
	//	where 0 is off and 100 is full volume. The default setting is 100.
	int	SDL_sound_handler::get_volume(int sound_handle)
	{
		m_mutex.lock();

		int vol = 0;
		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			vol = it->second->get_volume();
		}
		m_mutex.unlock();
		return vol;
	}


	//	A number from 0 to 100 representing a volume level.
	//	100 is full volume and 0 is no volume. The default setting is 100.
	void	SDL_sound_handler::set_volume(int sound_handle, int volume)
	{
		m_mutex.lock();

		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			it->second->set_volume(volume);
		}

		m_mutex.unlock();
	}

	void	SDL_sound_handler::attach_aux_streamer(aux_streamer_ptr ptr, as_object* netstream)
	{
//		assert(netstream);
//		assert(ptr);

//		m_mutex.lock();

//		m_aux_streamer[netstream] = ptr;
//		SDL_PauseAudio(0);

//		m_mutex.unlock();
	}

	void SDL_sound_handler::detach_aux_streamer(as_object* netstream)
	{
//		m_mutex.lock();
//		m_aux_streamer.erase(netstream);
//		m_mutex.unlock();
	}

	void SDL_sound_handler::cvt(short int** adjusted_data, int* adjusted_size, unsigned char* data, 
		int size, int channels, int freq)
	{
		*adjusted_data = NULL;
		*adjusted_size = 0;

		SDL_AudioCVT  wav_cvt;
		int rc = SDL_BuildAudioCVT(&wav_cvt, AUDIO_S16SYS, channels, freq,
			m_audioSpec.format, m_audioSpec.channels, m_audioSpec.freq);
		if (rc == -1)
		{
			myprintf("Couldn't build SDL audio converter\n");
			return;
		}

		// Setup for conversion 
		wav_cvt.buf = (Uint8*) malloc(size * wav_cvt.len_mult);
		wav_cvt.len = size;
		memcpy(wav_cvt.buf, data, size);

		// And now we're ready to convert
		SDL_ConvertAudio(&wav_cvt);

		*adjusted_data = (int16*) wav_cvt.buf;
		*adjusted_size = size * wav_cvt.len_mult;
	}

	int SDL_sound_handler::get_position(int sound_handle)
	{
		int ms = 0;
		m_mutex.lock();

		hash<int, smart_ptr<sound> >::iterator it = m_sound.find(sound_handle);
		if (it != m_sound.end())
		{
			int bytes = it->second->get_played_bytes();

			// convert to the time in ms
			ms = bytes / (m_audioSpec.freq *(m_audioSpec.channels > 1 ? 4 : 2) / 1000);
		}

		m_mutex.unlock();
		return ms;
	}

	sound::sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
		int sample_rate, bool stereo):
		m_data(data),
		m_size(size),
		m_volume(1.0f),
		m_format(format),
		m_sample_count(sample_count),
		m_sample_rate(sample_rate),
		m_stereo(stereo),
		m_is_paused(false)
	{
		if (m_data)
		switch (m_format)
		{
			case sound_handler::FORMAT_ADPCM :	
			{
				// Uncompress the ADPCM before handing data to host.
				int data_bytes = m_sample_count * (m_stereo ? 4 : 2);
				Uint8* data = new unsigned char[data_bytes];

				tu_file* fi = new tu_file(tu_file::memory_buffer, m_size, m_data);
				stream* in = new stream(fi);
				adpcm_expand(data, in, m_sample_count, m_stereo);
				delete in;
				delete fi;

				free(m_data);	// delete compressed data

				m_data = data;
				m_size = data_bytes;
				break;
			}

			case sound_handler::FORMAT_MP3 :
			{
	#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
				AVPacket pkt;
				av_init_packet(&pkt);
				Sint16 skip = *((Sint16*) m_data);	// Number of samples to skip..see swf specification
				pkt.data = m_data + 2;	// skip 2 bytes..Number of samples to skip
				pkt.size = m_size - 2;

				AVCodecContext* avcontext = avcodec_alloc_context3(s_avcodec);
				avcontext->sample_rate = m_sample_rate;
				avcontext->channels = m_stereo ? 2 : 1;

				Uint8* outbuf = NULL;
				int outsize = 0;
				if (avcodec_open2(avcontext, s_avcodec, NULL) >= 0)
				{
					AVFrame* decoded_frame = av_frame_alloc();
					int got_frame;
					while (pkt.size > 0) 
					{
						int len = avcodec_decode_audio4(avcontext, decoded_frame, &got_frame,	&pkt);
						if (len < 0)	// decoded size
						{
							free(outbuf);
							outbuf = NULL;
							outsize = 0;

							printf("Error while decoding MP3\n");
							break;
						}

						if (got_frame > 0)
						{
							int data_size = decoded_frame->linesize[0];
							outbuf = (Uint8*) realloc(outbuf, outsize + data_size);
							memcpy(outbuf + outsize, decoded_frame->data[0], data_size);
							outsize += data_size;
						}

						pkt.size -= len;
						pkt.data += len;
					}
					av_free(decoded_frame);
				}
				else
				{
					printf("could not open MP3 codec\n");
				}
				av_free(avcontext);

				// hack..из-за старой версии dll ??
	#ifndef WIN32
				if (m_stereo)
				{
					m_sample_rate >>= 1;
				}
	#endif

				free(m_data);	// delete compressed data
				m_data = outbuf;
				m_size = outsize;
				break;
	#else
		//		static int s_log = 0;
		//		if (s_log < 3)
		//		{
		//			s_log++;
					myprintf("MP3 requires FFMPEG library\n");
		//		}
				return;
	#endif
			}
			case sound_handler::FORMAT_NELLYMOSER:	// Mystery proprietary format; see nellymoser.com
				assert(0);
				return;

			case sound_handler::FORMAT_RAW:		// unspecified format.	Useful for 8-bit sounds???
			case sound_handler::FORMAT_UNCOMPRESSED:	// 16 bits/sample, little-endian
			case sound_handler::FORMAT_NATIVE16:	// bakeinflash extension: 16 bits/sample, native-endian
				break;
		}
	}

	sound::~sound()
	{
		free(m_data);
	}

	void sound::append(void* data, int size, SDL_sound_handler* handler)
	{
		m_data = (Uint8*) realloc(m_data, size + m_size);
		memcpy(m_data + m_size, data, size);
		m_size += size;
	}

	void sound::pause(bool paused)
	{
		m_is_paused = paused;
	}

	// returns the current sound position in ms
	int  sound::get_played_bytes()
	{
		// What to do if m_playlist.size() > 1 ???
		if (m_playlist.size() > 0)
		{
			return m_playlist[0]->get_played_bytes();
		}
		return 0;
	}

	void sound::play(int loops, SDL_sound_handler* handler)
	{
		if (m_size == 0)
		{
			myprintf("the attempt to play the empty sound\n");
			return;
		}

#if TU_CONFIG_LINK_TO_FFMPEGzz == 1
		m_playlist.push_back(new active_sound(this, loops));
		SDL_PauseAudio(0);
#else
		if (m_format != sound_handler::FORMAT_MP3)
		{
			m_playlist.push_back(new active_sound(this, loops));
			SDL_PauseAudio(0);
		}
		else
		{
			myprintf("MP3 requires FFMPEG library\n");
		}
#endif
	}

	// called from audio callback
	bool sound::mix(Uint8* stream, int len, array< smart_ptr<listener> >* listeners, float max_volume)
	{
		if (m_is_paused)
		{
			return true;
		}

		Uint8* buf = new Uint8[len];

		bool play = false;
		for (int i = 0; i < m_playlist.size(); )
		{
			play = true;
			if (m_playlist[i]->mix(buf, len))
			{
				i++;
			}
			else
			{
				if (m_listeners != NULL)
				{
					listeners->push_back(m_listeners);
				}

				m_playlist.remove(i);
			}

			int volume = (int) floorf(SDL_MIX_MAXVOLUME * max_volume * m_volume);
			SDL_MixAudio(stream, buf, len, volume);
		}

		delete [] buf;
		return play;
	}

	sound_handler*	create_sound_handler_sdl()
	// Factory.
	{
		return new SDL_sound_handler;
	}

	/// The callback function which refills the buffer with data
	/// We run through all of the sounds, and mix all of the active sounds 
	/// into the stream given by the callback.
	/// If sound is compresssed (mp3) a mp3-frame is decoded into a buffer,
	/// and resampled if needed. When the buffer has been sampled, another
	/// frame is decoded until all frames has been decoded.
	/// If a sound is looping it will be decoded from the beginning again.

	static void	sdl_audio_callback (void *udata, Uint8 *stream, int len)
	{
		// Get the soundhandler
		SDL_sound_handler* handler = static_cast<SDL_sound_handler*>(udata);

		handler->m_mutex.lock();

		int pause = 1;
		memset(stream, 0, len);

		array< smart_ptr<listener> > listeners;

		// mix Flash audio
		for (hash<int, smart_ptr<sound> >::iterator snd = handler->m_sound.begin();
			snd != handler->m_sound.end(); ++snd)
		{
			bool play = snd->second->mix(stream, len, &listeners, handler->m_max_volume);
			pause = play ? 0 : pause;
		}

		/*
		// mix audio of video 
		if (handler->m_aux_streamer.size() > 0)
		{
			Uint8* mix_buf = new Uint8[len];
			for (hash< as_object*, bakeinflash::sound_handler::aux_streamer_ptr>::const_iterator it = handler->m_aux_streamer.begin();
			     it != handler->m_aux_streamer.end();
			     ++it)
			{
				memset(mix_buf, 0, len); 

				bakeinflash::sound_handler::aux_streamer_ptr aux_streamer = it->second;
				(aux_streamer)(it->first, mix_buf, len);

				int volume = (int) floorf(SDL_MIX_MAXVOLUME * handler->m_max_volume);
				SDL_MixAudio(stream, mix_buf, len, volume);
			}
			pause = 0;
			delete [] mix_buf;
		}
		*/
		SDL_PauseAudio(pause);
		handler->m_mutex.unlock();

		// notify onSoundComplete
//		bakeinflash_engine_mutex().lock();
//		for (int i = 0, n = listeners.size(); i < n; i++)
//		{
//			listeners[i]->notify(event_id::ON_SOUND_COMPLETE);
//		}
//		bakeinflash_engine_mutex().unlock();

	}

	bool active_sound::mix(Uint8* mixbuf , int mixbuf_len)
	{
		memset(mixbuf, 0, mixbuf_len);

		// m_pos ==> current position of decoded sound which should be played
		l1:;
		int left = m_parent->m_size - m_pos;
		assert(left >= 0);
		if (left <= 0)
		{
			if (m_loops <= 0)
			{
				return false;
			}
			return true;
		}
		else
		if (left >= mixbuf_len)
		{
			memcpy(mixbuf, m_parent->m_data + m_pos, mixbuf_len);
			m_played_bytes += mixbuf_len;
			m_pos += mixbuf_len;
		}
		else
		{
			memcpy(mixbuf, m_parent->m_data + m_pos, left);
			m_played_bytes += left;
			m_pos = 0;
			m_loops--;
			if (m_loops <= 0)
			{
				return false;
			}
			goto l1;
		}
		return true;
	}


}

#endif  // TU_USE_SDL

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
