// bakeinflash_sound_handler_xaudio2.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// xaudio2 based sound handler for mobile units

#include "bakeinflash/bakeinflash_sound_handler_xaudio2.h"
#include "base/tu_file.h"
#include "bakeinflash/bakeinflash_stream.h"

namespace bakeinflash
{

static	    IXAudio2* m_audio;

#if TU_CONFIG_LINK_TO_FFMPEG == 1
	static AVCodec* s_avcodec = NULL;
#endif

	void	adpcm_expand(
		void* data_out,
		stream* in,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo);

	xaudio2_sound_handler::xaudio2_sound_handler() :
		m_masteringVoice(NULL),
		m_max_volume(1.0f)	// default max volume
	{

		// Иницилизируем работы с COM в отдельном потоке
		CoInitializeEx(0, COINIT_MULTITHREADED);

		// Создаём главный интерфейс
		HRESULT rc = XAudio2Create(&m_audio, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if (rc != S_OK)
		{
			myprintf("couildn't init audio\n");
			return;
		}

		// Создаём главный голос
		rc = m_audio->CreateMasteringVoice(&m_masteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, 0);
		if (rc != S_OK)
		{
			myprintf("couildn't init audio\n");
			return;
		}

	}

	xaudio2_sound_handler::~xaudio2_sound_handler()
	{
		// Сначала освобождаем все голоса (у нас пока только IXAudio2MasteringVoice)
		if (m_masteringVoice)
		{
			m_masteringVoice->DestroyVoice();
		}

		// Теперь освобождаем главный интерфейс
		if (m_audio)
		{
			m_audio->Release();
		}

		CoUninitialize();

		m_sound.clear();
	}

	void xaudio2_sound_handler::advance(float delta_time)
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
	int	xaudio2_sound_handler::load_sound(const char* url)
	{
		return -1;	//TODO
	}

	// may be used for the creation stream sound head with data_bytes=0 
	int	xaudio2_sound_handler::create_sound(
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

		m_sound[k] = new sound(data_bytes, (Uint8*) data, format, sample_count, sample_rate, stereo);
		//myprintf("create_sound: %d\n", k);

		m_mutex.unlock();
		return k;
	}

	void	xaudio2_sound_handler::set_max_volume(int vol)
	{
		if (vol >= 0 && vol <= 100)
		{
			m_max_volume = (float) vol / 100.0f;
		}
	}

	void	xaudio2_sound_handler::append_sound(int sound_handle, void* data, int data_bytes)
	{
		m_mutex.lock();
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->append(data, data_bytes, this);
		}
		m_mutex.unlock();
	}

	void xaudio2_sound_handler::pause(int sound_handle, bool paused)
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->pause(paused);
		}

		m_mutex.unlock();
	}

	void	xaudio2_sound_handler::push_action(bool stop_playback, int handler_id, int loop_count)
	{
		play_action_def pd;
		pd.handler_id = handler_id;
		pd.loop_count = loop_count;
		pd.stop_playback = stop_playback;
		pd.skip_frame = 2;
		m_play_action.push_back(pd);
	}

	void	xaudio2_sound_handler::play_sound(as_object* listener_obj, int sound_handle, int loops)
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

	void	xaudio2_sound_handler::stop_sound(int sound_handle)
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->stop();
		}

		m_mutex.unlock();
	}

	void	xaudio2_sound_handler::delete_sound(int sound_handle)
		// this gets called when it's done with a sample.
	{
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_mutex.lock();
		//vv  fault ?	m_sound[sound_handle] = NULL;
			m_mutex.unlock();
		}
	}

	void	xaudio2_sound_handler::stop_all_sounds()
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
	int	xaudio2_sound_handler::get_volume(int sound_handle)
	{
		m_mutex.lock();

		int vol = 0;
		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			vol = m_sound[sound_handle]->get_volume();
		}
		m_mutex.unlock();
		return vol;
	}


	//	A number from 0 to 100 representing a volume level.
	//	100 is full volume and 0 is no volume. The default setting is 100.
	void	xaudio2_sound_handler::set_volume(int sound_handle, int volume)
	{
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			m_sound[sound_handle]->set_volume(volume);
		}

		m_mutex.unlock();
	}

	int xaudio2_sound_handler::create_stream() 
	{
		/*
		static int s_stream_id = 0;
		m_mutex.lock();

		stream_def sd;

		alGenBuffers(NUM_xaudio2_STREAM_BUFFERS, sd.buffers);
		alGenSources(1, &sd.source);
		int n = 0;
		if (alGetError() == AL_NO_ERROR)
		{
		sd.buffer_index = 0; 

		m_aux_streams.add(++s_stream_id, sd);
		n = s_stream_id;
		}

		m_mutex.unlock();
		myprintf("xaudio2_sound_handler::create_stream %d\n", n);
		*/
		return 0; //n; 
	}

	void xaudio2_sound_handler::close_stream(int stream_id)
	{
		m_mutex.lock();

		hash<int, stream_def>::iterator sd = m_aux_streams.find(stream_id);
		if (sd != m_aux_streams.end())
		{
			//			alDeleteBuffers(NUM_xaudio2_STREAM_BUFFERS, sd->second.buffers);
			//			alSourcei(sd->second.source, AL_BUFFER, 0);
			//			alDeleteSources(1, &sd->second.source);
			//			m_aux_streams.erase(stream_id);
			//			myprintf("xaudio2_sound_handler::close_stream %d\n", stream_id);
		}

		m_mutex.unlock();
	}

	bool xaudio2_sound_handler::is_stream_available(int stream_id)
	{
		bool rc = false;
		m_mutex.lock();

		hash<int, stream_def>::iterator sd = m_aux_streams.find(stream_id);
		if (sd != m_aux_streams.end())
		{
			//			if (sd->second.buffer_index < NUM_xaudio2_STREAM_BUFFERS)
			{
				rc = true;
			}
			//		else
			{
				//		ALint val;
				//		alGetSourcei(sd->second.source, AL_BUFFERS_PROCESSED, &val);
				//	if (val > 0)
				{
					rc = true;
				}
			}
		}
		m_mutex.unlock();
		//myprintf("%d ", rc);
		return rc;
	}

	/*
	enum AVSampleFormat {
	AV_SAMPLE_FMT_NONE = -1,
	AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
	AV_SAMPLE_FMT_S16,         ///< signed 16 bits
	AV_SAMPLE_FMT_S32,         ///< signed 32 bits
	AV_SAMPLE_FMT_FLT,         ///< float
	AV_SAMPLE_FMT_DBL,         ///< double

	AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
	AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
	AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
	AV_SAMPLE_FMT_FLTP,        ///< float, planar
	AV_SAMPLE_FMT_DBLP,        ///< double, planar

	AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
	};
	*/
	void xaudio2_sound_handler::push_stream(int stream_id, int format, Uint8* data, int size, int sample_rate, int channels)
	{
		/*
		int alfmt = 0;
		switch (format)
		{
		case 0:	// AV_SAMPLE_FMT_U8,         
		alfmt = channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
		break;
		case 1:	// AV_SAMPLE_FMT_S16,      
		alfmt = channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		break;
		default:
		myprintf("xaudio2_sound_handler::push_stream, unsupported format %d\n", format);
		return;
		}

		m_mutex.lock();

		ALint val;
		hash<int, stream_def>::iterator sd = m_aux_streams.find(stream_id);
		if (sd != m_aux_streams.end())
		{
		int i = sd->second.buffer_index;
		if (i < NUM_xaudio2_STREAM_BUFFERS)
		{
		alBufferData(sd->second.buffers[i], alfmt, data, size, sample_rate);
		alSourceQueueBuffers(sd->second.source, 1, &sd->second.buffers[i]);
		sd->second.buffer_index = i + 1;
		}
		else
		{
		alGetSourcei(sd->second.source, AL_BUFFERS_PROCESSED, &val);
		if (val > 0)
		{
		// check unused buffers
		ALuint buffer;
		alSourceUnqueueBuffers(sd->second.source, 1, &buffer);

		alBufferData(buffer, alfmt, data, size, sample_rate);
		alSourceQueueBuffers(sd->second.source, 1, &buffer);
		if(alGetError() != AL_NO_ERROR)
		{
		myprintf("Error buffering :(\n");
		}
		}
		}

		//	Make sure the source is still playing, and restart it if needed.
		alGetSourcei(sd->second.source, AL_SOURCE_STATE, &val);
		if (val != AL_PLAYING)
		{
		alSourcePlay(sd->second.source);
		}
		}

		m_mutex.unlock();
		*/
	}

	int xaudio2_sound_handler::get_position(int sound_handle)
	{
		int ms = 0;
		m_mutex.lock();

		if (sound_handle >= 0 && sound_handle < m_sound.size() && m_sound[sound_handle] != NULL)
		{
			XAUDIO2_VOICE_STATE voiceState = {0};
			if (m_sound[sound_handle]->m_source)
			{
				m_sound[sound_handle]->m_source->GetState(&voiceState);
			}
			ms = voiceState.SamplesPlayed;
		}
		m_mutex.unlock();
		return ms;
	}

	sound::sound(int size, Uint8* data, sound_handler::format_type format, int sample_count, int sample_rate, bool stereo):
		m_volume(1.0f),
		m_format(format),
		m_sample_count(sample_count),
		m_sample_rate(sample_rate),
		m_stereo(stereo),
		m_is_paused(false),
		m_loops(0),
		m_source(NULL),
		m_compressed_size(size),
		m_compressed_data(data)
	{
		ZeroMemory(&m_buffer, sizeof(XAUDIO2_BUFFER));
		if (data != NULL && size > 0)
		{
			WAVEFORMATEX sd;
			ZeroMemory(&sd, sizeof(WAVEFORMATEX));
			sd.wFormatTag = WAVE_FORMAT_PCM;
			sd.nChannels = stereo ? 2 : 1;
			sd.nSamplesPerSec = sample_rate;
			sd.wBitsPerSample = 16;	// 8 or 16
			sd.cbSize = 0;
			sd.nBlockAlign = sd.nChannels * sd.wBitsPerSample / 8;
			sd.nAvgBytesPerSec = sd.nSamplesPerSec * sd.nBlockAlign;

			HRESULT rc = m_audio->CreateSourceVoice(&m_source, &sd, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
			assert(rc == S_OK);
		}
	}

	sound::~sound()
	{
		free((void*) m_buffer.pAudioData);
		free(m_compressed_data);
	}

	void sound::append(void* data, int size, xaudio2_sound_handler* handler)
	{
		// TODO
		return;
		//	m_data = (Uint8*) realloc(m_data, size + m_size);
		//	memcpy(m_data + m_size, data, size);
		//	m_size += size;
	}

	void sound::pause(bool paused)
	{
		if (m_source)
		{
			m_is_paused = paused;
			if (paused)
			{
				m_source->Stop();
			}
			else
			{
				// Play, replay, or resume a Source
				m_source->Start();
			}
		}
	}

	// returns the duration
	int  sound::get_played_bytes()
	{
		return m_sample_count;
	}

	void sound::play(int loops, xaudio2_sound_handler* handler)
	{
		// first time ?
		if (m_compressed_data)
		{
			ZeroMemory(&m_buffer, sizeof(XAUDIO2_BUFFER));
			switch (m_format)
			{
			case sound_handler::FORMAT_ADPCM :	
				{
					// Uncompress the ADPCM before handing data to host.
					int data_bytes = m_sample_count * (m_stereo ? 4 : 2);
					Uint8* data = new unsigned char[data_bytes];

					tu_file* fi = new tu_file(tu_file::memory_buffer, m_compressed_size, m_compressed_data);
					stream* in = new stream(fi);
					adpcm_expand(data, in, m_sample_count, m_stereo);
					delete in;
					delete fi;

					free(m_compressed_data);	// delete compressed data
					m_compressed_data = NULL;

					m_buffer.pAudioData = data;
					m_buffer.AudioBytes = data_bytes;
					break;
				}

			case sound_handler::FORMAT_MP3 :
				{
#if TU_CONFIG_LINK_TO_FFMPEG == 1
					AVPacket pkt;
					av_init_packet(&pkt);
					Sint16 skip = *((Sint16*) m_compressed_data);	// Number of samples to skip..see swf specification
					pkt.data = m_compressed_data + 2;	// skip 2 bytes..Number of samples to skip
					pkt.size = m_compressed_size - 2;

					AVCodecContext* avcontext = avcodec_alloc_context3(s_avcodec);
					avcontext->sample_rate = m_sample_rate;
					avcontext->channels = m_stereo ? 2 : 1;

					Uint8* outbuf = NULL;
					int outsize = 0;
					if (avcodec_open2(avcontext, s_avcodec, NULL) >= 0)
					{
						AVFrame* decoded_frame = avcodec_alloc_frame();
						int got_frame;
						while (pkt.size > 0) 
						{
							int len = avcodec_decode_audio4(avcontext, decoded_frame, &got_frame,	&pkt);
							if (len < 0)	// decoded size
							{
								free(outbuf);
								outbuf = NULL;
								outsize = 0;

								myprintf("Error while decoding MP3\n");
								break;
							}

							if (got_frame > 0)
							{
								//								int data_size = decoded_frame.nb_samples * avcontext->channels * av_get_bytes_per_sample(avcontext->sample_fmt);
								int data_size = decoded_frame->linesize[0]; //decoded_frame.nb_samples * avcontext->channels * av_get_bytes_per_sample(avcontext->sample_fmt);
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
						myprintf("could not open MP3 codec\n");
					}
					av_free(avcontext);

					// hack..из-за старой версии dll ??
#ifndef WIN32
					if (m_stereo)
					{
						m_sample_rate >>= 1;
					}
#endif

					free(m_compressed_data);	// delete compressed data
					m_compressed_data = NULL;

					m_buffer.pAudioData = outbuf;
					m_buffer.AudioBytes = outsize;
					break;

#else
					static int s_log = 0;
					if (s_log < 3)
					{
						s_log++;
						myprintf("MP3 requires FFMPEG library\n");
					}
					free(m_compressed_data);	// delete compressed data
					m_compressed_data = NULL;
					return;
#endif
				}
			case sound_handler::FORMAT_NELLYMOSER:	// Mystery proprietary format; see nellymoser.com
				assert(0);
				return;

			case sound_handler::FORMAT_RAW:		// unspecified format.	Useful for 8-bit sounds???
			case sound_handler::FORMAT_UNCOMPRESSED:	// 16 bits/sample, little-endian
			case sound_handler::FORMAT_NATIVE16:	// bakeinflash extension: 16 bits/sample, native-endian
					m_buffer.pAudioData = m_compressed_data;
					m_buffer.AudioBytes = m_compressed_size;
					m_compressed_data = NULL;
					m_compressed_size = 0;
				break;
			}
		}

		if (is_stoped())
		{
			float v = handler->m_max_volume * m_volume;
			//alSourcef(m_uiSource, AL_GAIN, v);

			m_buffer.LoopCount = imax(0, loops - 1);	// <= 1 ? 0 : XAUDIO2_LOOP_INFINITE;
			m_loops = m_buffer.LoopCount + 1;		// for handling

			// Добавляем данные в конец буфера
			if (m_source)
			{
				m_source->SubmitSourceBuffer(&m_buffer);

				// Start playing source
				m_source->Start();
			}
		}
	}

	void sound::stop()
	{
		if (m_source)
		{
			m_source->Stop();
		}
	}

	bool sound::is_stoped()
	{
		if (m_source)
		{
			XAUDIO2_VOICE_STATE voiceState = {0};
			m_source->GetState(&voiceState);
			bool rc = voiceState.BuffersQueued == 0;
			return rc;
		}
		return true;
	}
	
	void sound::advance(float delta_time)
	{
		if (m_loops > 0)
		{
			if (is_stoped())
			{
				m_loops--;
				if (m_loops == 0)
				{
					// notify onSoundComplete
					if (m_listeners)
					{
						m_listeners->notify(event_id::ON_SOUND_COMPLETE);
					}
				}
				else
				{
					// Start playing again
					if (m_source)
					{
						m_source->Start();
					}
				}
			}
		}
	}

	// vol is in [0..100]
	void sound::set_volume(int vol)
	{
		if (m_source)
		{
			m_volume = (float) vol / 100.0f;
			float v = 1.0f * m_volume;
			m_source->SetVolume(v);
		}
	}

	// Factory.
	sound_handler*	create_sound_handler_xaudio2()
	{
		return new xaudio2_sound_handler();
	}

}
