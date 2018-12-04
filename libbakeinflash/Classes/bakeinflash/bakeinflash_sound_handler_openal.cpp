// bakeinflash_sound_handler_openal.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenAL based sound handler for mobile units

#include "bakeinflash/bakeinflash_sound_handler_openal.h"
#include "base/tu_file.h"
#include "bakeinflash/bakeinflash_stream.h"

namespace bakeinflash
{

#if TU_CONFIG_LINK_TO_FFMPEG == 1
	static AVCodec* s_avcodec = NULL;
#endif

	void	adpcm_expand(void* data_out, stream* in, int sample_count,	bool stereo);

	int clearBuffers(ALuint source)
	{
		// unqueue unused buffers
		ALint val = 0;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
		if (val > 0)
		{
			ALuint* buffer = new ALuint[val];
			alSourceUnqueueBuffers(source, val, buffer);
			alDeleteBuffers(val, buffer);
			delete [] buffer;

			//printf("deleted %d audiobuffres\n", val);
		}
		return val;
	}

	openal_sound_handler::openal_sound_handler() :
		m_device(NULL),
		m_context(NULL),
		m_max_volume(1.0f)	// default max volume
	{
		// open default device
		m_device = alcOpenDevice(NULL);
		if (m_device)
		{
			m_context = alcCreateContext(m_device, NULL);
			if (m_context)
			{
				printf("\nopenAL uses '%s' device\n", alcGetString(m_device, ALC_DEVICE_SPECIFIER));
//				__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "\nopenAL uses '%s' device\n", alcGetString(m_device, ALC_DEVICE_SPECIFIER));
				alcMakeContextCurrent(m_context);

#if TU_CONFIG_LINK_TO_FFMPEG == 1
				avcodec_register_all();
				s_avcodec = avcodec_find_decoder(AV_CODEC_ID_MP3);
				if (s_avcodec == NULL)
				{
					printf("MP3 codec not found\n");
				}

#endif
			}
			else
			{
				alcCloseDevice(m_device);
				m_device = NULL;
			}
		}
		else
		{
			myprintf("Unable to start openAL sound handler\n");
		}
	}

	openal_sound_handler::~openal_sound_handler()
	{
		stop_all_sounds();
		for (int i = 0; i < m_sound.size(); i++)
		{
			smart_ptr<sound> snd = m_sound[i];	// keep alive
			if (snd != NULL)
			{
				snd->clear_all_sources();
			}
		}

		alcDestroyContext(m_context);
		alcCloseDevice(m_device);
	}

	void openal_sound_handler::advance(float delta_time)
	{
		for (int i = 0; i < m_play_action.size(); )
		{
			play_action_def& pd = m_play_action[i];
			if (--pd.skip_frame == 0)
			{
				if (pd.stop_playback)
				{
					stop_sound(pd.handler_id);
				}
				else
				{
					play_sound(NULL, pd.handler_id, pd.loop_count);
				}
				m_play_action.remove(i);
			}
			else
			{
				i++;
			}
		}

		// causes deadlock
		//m_mutex.lock();

		for (int i = 0; i < m_sound.size(); i++)
		{
			smart_ptr<sound> snd = m_sound[i];	// keep alive
			if (snd != NULL)
			{
				snd->advance(delta_time);
			}
		}
		//m_mutex.unlock();
	}

	// loads external sound file
	// TODO: load MP3, ...
	int	openal_sound_handler::load_sound(const char* url)
	{
		return -1;	//TODO
	}

	// may be used for the creation stream sound head with data_bytes=0 
	int	openal_sound_handler::create_sound(
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

		int k = -1;
		for (int i = 0; i < m_sound.size(); i++)
		{
			if (m_sound[i] == NULL)
			{
				k = i;
				break;
			}
		}

		if (k < 0)
		{
			m_sound.push_back(NULL);
			k = m_sound.size() - 1;
		}
				
		m_sound[k] = new sound(data_bytes, (Uint8*) data, format, sample_count, sample_rate, stereo, 100);
		//printf("create_sound: %d\n", k);

		m_mutex.unlock();
		return k;
	}

	void	openal_sound_handler::set_max_volume(int vol)
	{
		if (vol >= 0 && vol <= 100)
		{
			m_max_volume = (float) vol / 100.0f;

			// reset active volume
			for (int i = 0; i < m_sound.size(); i++)
			{
				sound* snd = m_sound[i];
				if (snd)
				{
					int volume = snd->get_volume();
					snd->set_gain(volume * m_max_volume);
				}
			}
		}
	}

	void	openal_sound_handler::append_sound(int sound_handle, void* data, int data_bytes)
	{
		m_mutex.lock();
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->append(data, data_bytes, this);
		}
		m_mutex.unlock();
	}

	void openal_sound_handler::pause(int sound_handle, bool paused)
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->pause(paused);
		}

		m_mutex.unlock();
	}

	void	openal_sound_handler::push_action(bool stop_playback, int handler_id, int loop_count)
	{
		play_action_def pd;
		pd.handler_id = handler_id;
		pd.loop_count = loop_count;
		pd.stop_playback = stop_playback;
		pd.skip_frame = 2;
		m_play_action.push_back(pd);
	}

	void	openal_sound_handler::play_sound(as_object* listener_obj, int sound_handle, int loops)
	// Play the index'd sample.
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			if (listener_obj)
			{
				// create listener
				if (m_sound[sound_handle]->m_listeners == NULL)
				{
					m_sound[sound_handle]->m_listeners = new listener();
				}
				m_sound[sound_handle]->m_listeners->add(listener_obj);
			}

			m_sound[sound_handle]->play(loops, this);
		}

		m_mutex.unlock();
	}

	void	openal_sound_handler::stop_sound(int sound_handle)
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->stop();
		}

		m_mutex.unlock();
	}

	void	openal_sound_handler::delete_sound(int sound_handle)
	// this gets called when it's done with a sample.
	{
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_mutex.lock();
			m_sound[sound_handle] = NULL;
			m_mutex.unlock();
		}
	}

	void	openal_sound_handler::stop_all_sounds()
	{
		m_mutex.lock();

		for (int i = 0; i < m_sound.size(); i++)
		{
			if (m_sound[i] != NULL)
			{
				m_sound[i]->stop();
			}
		}
		m_mutex.unlock();
	}

	//	returns the sound volume level as an integer from 0 to 100,
	//	where 0 is off and 100 is full volume. The default setting is 100.
	int	openal_sound_handler::get_volume(int sound_handle)
	{
//		return m_volume;

		m_mutex.lock();

		int vol = 0;
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			vol = m_sound[sound_handle]->get_volume();
		}
		m_mutex.unlock();
		return vol;
	}

	// GLOBAL volume !!!
	//	A number from 0 to 100 representing a volume level.
	//	100 is full volume and 0 is no volume. The default setting is 100.
	void	openal_sound_handler::set_volume(int sound_handle, int volume)
	{
		volume = iclamp(volume, 0, 100);
		m_mutex.lock();
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->set_volume(volume);
		}
		m_mutex.unlock();
	}

	int openal_sound_handler::create_stream() 
	{
		static int s_stream_id = 0;
		m_mutex.lock();

		ALuint source;
		alGenSources(1, &source);

		int n = 0;
		if (alGetError() == AL_NO_ERROR)
		{
			m_aux_streams.add(++s_stream_id, source);
			n = s_stream_id;
		}

		m_mutex.unlock();
		myprintf("openal_sound_handler::create_stream %d\n", n);
		return n; 
	}

	void openal_sound_handler::close_stream(int stream_id)
	{
		m_mutex.lock();

		hash<int, ALuint>::iterator it = m_aux_streams.find(stream_id);
		if (it != m_aux_streams.end())
		{
			ALuint source = it->second;

			// one can only unqueue buffers that have been processed.
			// When you stop a source, this marks all the buffers in the queue as processed. 
			alSourceStop(source);
			clearBuffers(source);

			alDeleteSources(1, &source);
			m_aux_streams.erase(stream_id);
			myprintf("openal_sound_handler::close_stream %d\n", stream_id);
		}

		m_mutex.unlock();
	}

	void openal_sound_handler::push_stream(int stream_id, int format, Uint8* data, int size, int sample_rate, int channels)
	{
		int alfmt = 0;
		switch (format)
		{
		case 0: // AV_SAMPLE_FMT_U8:   hack, fix compile error if no ffmpeg
			alfmt = channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
			break;
		case 1: // AV_SAMPLE_FMT_S16:	hack, fix compile error if no ffmpeg
			alfmt = channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			break;
		case 6:		// AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
			alfmt = channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			break;
		default:
			myprintf("openal_sound_handler::push_stream, unsupported format %d\n", format);
			return;
		}

		m_mutex.lock();

		hash<int, ALuint>::iterator it = m_aux_streams.find(stream_id);
		if (it != m_aux_streams.end())
		{
			ALuint source = it->second;

			clearBuffers(source);

			// Generate AL Buffers for streaming
			ALuint buffer;
			alGenBuffers(1, &buffer);

			alBufferData(buffer, alfmt, data, size, sample_rate);
			alSourceQueueBuffers(source, 1, &buffer);

			alSourcePlay(source);
		}

		m_mutex.unlock();
	}

	int openal_sound_handler::get_position(int sound_handle)
	{
		int ms = 0;
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL && m_sound[sound_handle]->m_source.size() > 0)
		{
			ALfloat sec = 0;
			alGetSourcef(m_sound[sound_handle]->m_source[0], AL_SEC_OFFSET, &sec);		// hack
			ms = (int) (sec * 1000);
		}
		m_mutex.unlock();
		return ms;
	}

	sound::sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, 
		int sample_rate, bool stereo, int volume):
		m_volume(1.0f),
		m_format(format),
		m_sample_count(sample_count),
		m_sample_rate(sample_rate),
		m_stereo(stereo),
		m_is_paused(false),
		m_size(size),
		m_data(data)
	{
		m_volume = iclamp(volume, 0, 100) / 100.0f;

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
	#if TU_CONFIG_LINK_TO_FFMPEG == 1
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
	//#ifndef WIN32
				if (m_stereo)
				{
					m_sample_rate >>= 1;
				}
	//#endif

				free(m_data);	// delete compressed data
				m_data = outbuf;
				m_size = outsize;
				break;
	#else
				static int s_log = 0;
				if (s_log < 3)
				{
					s_log++;
					myprintf("MP3 requires FFMPEG library\n");
				}
				free(m_data);
				m_data = NULL;
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
		// one can only unqueue buffers that have been processed.
		// When you stop a source, this marks all the buffers in the queue as processed. 
		clear_all_sources();
		free(m_data);
	}

	void sound::append(void* data, int size, openal_sound_handler* handler)
	{
		// TODO
		return;
	//	m_data = (Uint8*) realloc(m_data, size + m_size);
	//	memcpy(m_data + m_size, data, size);
	//	m_size += size;
	}

	void sound::pause(bool paused)
	{
		m_is_paused = paused;
		for (int i = 0; i < m_source.size(); i++)
		{
			if (paused)
			{
				alSourcePause(m_source[i]);
			}
			else
			{
				// Play, replay, or resume a Source
				alSourcePlay(m_source[i]);
			}
		}
	}

	// returns the duration
	int  sound::get_played_bytes()
	{
		ALint duration = 0;
		if (m_source.size() > 0)
		{
			alGetSourcei(m_source[0], AL_BYTE_OFFSET, &duration);		// hack
		}
		return duration;
	}

	void sound::play(int loops, openal_sound_handler* handler)
	{
		// first time ?
		if (m_data)
		{
			// Generate a Source to playback the Buffers
			ALuint source;
			alGenSources(1, &source);

			m_source.push_back(source);

			float v = handler->m_max_volume * m_volume;
			alSourcef(source, AL_GAIN, v);

			// hack .. play infinitive
			if (loops > 10)
			{
				loops = -1;
			}

			alSourcei(source, AL_LOOPING, loops < 0 ? AL_TRUE : AL_FALSE);
		
			// Generate AL Buffers for streaming
			ALuint buffer;
			alGenBuffers(1, &buffer);

			ALenum al_fmt = m_stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			alBufferData(buffer, al_fmt, m_data, m_size, m_sample_rate);

			// play at least 1 time
			alSourceQueueBuffers(source, 1, &buffer);
			for (int i = 0; i < loops - 1; i++)
			{
				alSourceQueueBuffers(source, 1, &buffer);
			}

			// Start playing source
			alSourcePlay(source);
			//printf("play sound %X, looping %d\n", m_uiSource, m_loops);
		}
	}

	void sound::clear_source(int i)
	{
		alSourceStop(m_source[i]);
		clearBuffers(m_source[i]);

		alDeleteSources(1, &m_source[i]);
	}

	void sound::clear_all_sources()
	{
		for (int i = 0; i < m_source.size(); i++)
		{
			clear_source(i);
		}
		m_source.clear();
	}

	void sound::stop()
	{
		clear_all_sources();
	}

	void sound::advance(float delta_time)
	{
		for (int i = 0; i < m_source.size(); )
		{
			int n = clearBuffers(m_source[i]);

			ALint state;
			alGetSourcei(m_source[i], AL_SOURCE_STATE, &state);
			if (state == AL_STOPPED)
			{
				if (n > 0 && m_listeners)
				{
					m_listeners->notify(event_id::ON_SOUND_COMPLETE);
					clear_source(i);
					m_source.remove(i);
					continue;
				}
			}
			i++;
		}
	}

	// vol is in [0..100]
	void sound::set_volume(int vol)
	{
		m_volume = (float) vol / 100.0f;
		set_gain(m_volume);
	}

	void sound::set_gain(float vol)
	{
		for (int i = 0; i < m_source.size(); i++)
		{
			alSourcef(m_source[0], AL_GAIN, vol);
		}
	}

	// Factory.
	sound_handler*	create_sound_handler_openal()
	{
		return new openal_sound_handler();
	}

}
