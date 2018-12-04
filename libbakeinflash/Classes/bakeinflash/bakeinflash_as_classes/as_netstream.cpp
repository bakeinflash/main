// as_netstream.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash_as_classes/as_netstream.h"
#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_root.h"
#include "base/tu_timer.h"
#include "base/png_helper.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

#if TU_CONFIG_LINK_TO_PLAYREADY == 1
	#include "piff/msplayer_piff.h"
#endif

namespace bakeinflash
{

#if TU_CONFIG_LINK_TO_PLAYREADY == 1
	long decode_piff(const tu_string& infile, const tu_string& outfile, const tu_string& hdsfile)
	{
		tu_file in(infile, "rb");
		if (in.get_error() != TU_FILE_NO_ERROR)
		{
			// no file
			myprintf("no input file %s\n", infile.c_str());
			return 0;
		}
		tu_file out(outfile, "wb");
		if (out.get_error() != TU_FILE_NO_ERROR)
		{
			// no file
			myprintf("canno create file %s\n", outfile.c_str());
			return 0;
		}

		piff f;
		long rc = f.open(in, hdsfile);
		if (rc == 0)
		if (true)
		{
			smart_ptr<atom> a;
			do
			{
				a = f.get_frame(in);
				if (a) a->write(out);
			}
			while (a);
			myprintf("'%s' is ready to play\n", outfile.c_str());
		}
		else
		{
			myprintf("failed to open/init stream, 0x%08X\n", rc);
		}
		return rc;
	}
#endif

	// it is running in decoder thread
	static void netstream_server(void* arg)
	{
		as_netstream* ns = (as_netstream*) arg;
		ns->run();
	}

	static hash<int, tu_string> s_netstream_event_level;
	static hash<int, tu_string> s_netstream_event_code;

	as_netstream::as_netstream() :
		m_FormatCtx(NULL),
		m_VCodecCtx(NULL),
		m_aVCodecCtx(NULL),
		m_video_stream(NULL),
		m_ACodecCtx(NULL),
		m_audio_stream(NULL),
		m_start_time(0.0f),
		m_video_time(0.0f),
		m_video_index(-1),              
		m_audio_index(-1),
		m_video_data(NULL),
		m_convert_ctx(NULL),
		m_status(PAUSE),
		m_seek_time(-1.0f),
		m_buffer_time(0.1f),	// The default value is 0.1 second
		m_openal_stream(0),
		m_nonstop(false)
	{
		// fill static hash once
		if (s_netstream_event_level.size() == 0)
		{
			s_netstream_event_level.add(status, "status");
			s_netstream_event_level.add(error, "error");
		}

		if (s_netstream_event_code.size() == 0)
		{
			s_netstream_event_code.add(playStreamNotFound, "NetStream.Play.StreamNotFound");
			s_netstream_event_code.add(playStart, "NetStream.Play.Start");
			s_netstream_event_code.add(playStop, "NetStream.Play.Stop");
			s_netstream_event_code.add(seekNotify, "NetStream.Seek.Notify");
			s_netstream_event_code.add(seekInvalidTime, "NetStream.Seek.InvalidTime");
			s_netstream_event_code.add(bufferEmpty, "NetStream.Buffer.Empty");
			s_netstream_event_code.add(bufferFull, "NetStream.Buffer.Full");
		}

		av_register_all();
	}

	as_netstream::~as_netstream()
	{
		close_thread();
		close_stream();
	}

	void as_netstream::play(const char* url)
	{
		switch (m_status)
		{
			default:
				break;
			case PAUSE:
			case CLOSE:
				if (m_thread == NULL)
				{
					// is path relative ?
					tu_string infile = get_curdir();
					if (strstr(url, ":") || url[0] == '/')
					{
						infile = "";
					}
					infile += url;
					m_url = infile;

					get_root()->add_listener(this);
					if (open_stream(m_url.c_str()) == true)
					{
						sound_handler* sound = get_sound_handler();
						if (sound && m_audio_index >= 0 && m_openal_stream == 0)
						{
							m_openal_stream = sound->create_stream();
						}

						m_thread = new tu_thread(netstream_server, this);
					}
				}
				m_status = PLAY;
				m_decoder.signal();
				break;
			}
	}

	void as_netstream::close_thread()
	{
		if (m_thread != NULL)
		{
			switch (m_status)
			{
			default:
				break;
			case PLAY:
				m_status = CLOSE;
				m_thread->wait();
				break;
			case PAUSE:
				m_status = CLOSE;
				m_decoder.signal();
				m_thread->wait();
				break;
			}
			m_thread = NULL;
		}

		sound_handler* sound = get_sound_handler();
		if (sound)
		{
			sound->close_stream(m_openal_stream);
		}

	}

	// seek_time in sec
	void as_netstream::seek(double seek_time)
	{
		if (seek_time < 0)
		{
			seek_time = 0;
		}

		switch (m_status)
		{
			default:
				break;

			case PLAY:
			case PAUSE:
				m_seek_time = seek_time;
//				m_status = PLAY;
//				m_decoder.signal();
				break;
			}
	}

	// return current time in sec
	double as_netstream::now() const
	{
		return (double) tu_timer::get_ticks() / 1000.0;
	}

	// it is running in decoder thread
	void as_netstream::run()
	{
		set_status(status, playStart);

		m_start_time = now();
		m_video_time = 0;

		m_status = PLAY;
		while (m_status != CLOSE)
		{
			if (m_status == PAUSE)
			{
				double paused = now();
				m_decoder.wait();
				m_start_time += now() - paused;
				continue;
			}

			// seek request
			if (m_seek_time >= 0)
			{
				int64 timestamp = (int64) (m_seek_time * AV_TIME_BASE);
				int flags = m_seek_time > m_video_time ? 0 : AVSEEK_FLAG_BACKWARD;
				//int ret = av_seek_frame(m_FormatCtx, -1, timestamp, flags);
				int ret = avformat_seek_file(m_FormatCtx, -1, (Sint64) m_seek_time, (Sint64) m_seek_time, (Sint64) m_seek_time, AVSEEK_FLAG_BYTE);
				if (ret == 0)
				{
					clear_queues();
					m_start_time += m_video_time - m_seek_time;
					m_video_time = m_seek_time;
					set_status(status, seekNotify);
				}
				else
				{
					set_status(error, seekInvalidTime);
				}
				m_seek_time = -1;
			}

			if (get_bufferlength() < m_buffer_time)
			{
				//myprintf("m_buffer_length=%f, queue_size=%d\n", get_bufferlength(), m_vq.size()); 
				AVPacket pkt;
				av_init_packet(&pkt);
				int rc = av_read_frame(m_FormatCtx, &pkt);
				if (rc < 0)
				{
					if (m_vq.size() == 0)
					{
						// video is played, restart
						if (m_nonstop)
						{
							m_seek_time = 0;
							m_start_time = now();
							m_video_time = 0;
						}
						else
						{
							set_status(status, playStop);
							m_status = PAUSE;
						}
						continue;
					}
				}
				else
				{
					if (pkt.stream_index == m_video_index)
					{
						m_vq.push(pkt);
					}
					else
					if (pkt.stream_index == m_audio_index)
					{
						m_aq.push(pkt);
					}
					else
					{
						continue;
					}
				}
			}

			// push audio
			sound_handler* sh = get_sound_handler();
			if (sh)
			{
				AVPacket audio;
				if (m_aq.pop(&audio))
				{
					Sint16* sample;
					int size;
					decode_audio(audio, &sample, &size);
				}
			}

			// skip expired video frames
			double current_time = now() - m_start_time;
			while (current_time >= m_video_time)
			{
				AVPacket packet;
				if (m_vq.pop(&packet))
				{
					// update video clock with pts, if present
					if (packet.dts > 0)
					{
						m_video_time = av_q2d(m_video_stream->time_base) * packet.dts;
					}
					m_video_time += av_q2d(m_video_stream->codec->time_base);	// +frame_delay

					set_video_data(decode_video(packet), 0, 0);
				}
				else
				{
					// no packets in queue
					//					set_status("status", "NetStream.Buffer.Empty");
					break;
				}
			}

			// Don't hog the CPU.
			// Queues have filled, video frame have shown
			// now it is possible and to have a rest

			int delay = (int) (1000 * (m_video_time - current_time));

			// hack, adjust m_start_time after seek
			if (delay > 100)
			{
				m_start_time -= (m_video_time - current_time);
				current_time = now() - m_start_time;
				delay = (int) (1000 * (m_video_time - current_time));
			}

			assert(delay <= 100);

			if (delay > 0)
			{
				if (get_bufferlength() >= m_buffer_time)
				{
					//					set_status("status", "NetStream.Buffer.Full");
					tu_timer::sleep(delay);
				}
				//	myprintf("current_time=%f, video_time=%f, delay=%d\n", current_time, m_video_time, delay);
			}
		}

		sound_handler* sound = get_sound_handler();
		if (sound)
		{
			sound->close_stream(m_openal_stream);
		}

		close_stream();

		set_status(status, playStop);
		m_status = CLOSE;
	}

	void as_netstream::advance(float delta_time)
	{
		stream_event ev;
		while (m_event.pop(&ev))
		{
			//myprintf("pop status: %d %d\n", ev.level, ev.code);		

			// keep this alive during execution!
			smart_ptr<as_object>	this_ptr(this);

			as_value function;
			if (get_member("onStatus", &function))
			{
				smart_ptr<as_object> infoObject = new as_object();
				infoObject->set_member("level", s_netstream_event_level[ev.level].c_str());
				infoObject->set_member("code", s_netstream_event_code[ev.code].c_str());

				as_environment env;
				env.push(infoObject.get());
				call_method(function, &env, this, 1, env.get_top_index());
			}

			if (ev.code == playStreamNotFound)
			{
				get_root()->remove_listener(this);
			}
		}
	}

	// it is running in decoder thread
	void as_netstream::set_status(netstream_event_level level, netstream_event_code code)
	{
		stream_event ev;
		ev.level = level;
		ev.code = code;
		m_event.push(ev);
		//myprintf("push status: %d %d %d\n", level, code, m_event.size());
	}

	void as_netstream::clear_queues()
	{
		m_aq.clear();
		m_vq.clear();
	}

	// it is running in decoder thread
	void as_netstream::close_stream()
	{
		set_video_data(NULL, 0, 0);

		clear_queues();

		if (m_FormatCtx)
		{
			if (m_VCodecCtx) avcodec_close(m_VCodecCtx);
//			av_free(m_VCodecCtx);
			m_VCodecCtx = NULL;

			if (m_aVCodecCtx) avcodec_close(m_aVCodecCtx);
//			av_free(m_aVCodecCtx);
			m_aVCodecCtx = NULL;

			if (m_ACodecCtx) avcodec_close(m_ACodecCtx);
//			av_free(m_ACodecCtx);
			m_ACodecCtx = NULL;

			avformat_close_input(&m_FormatCtx);
			m_FormatCtx = NULL;
		}
		else
		{
			// embedded video has no m_FormatCtx

			if (m_VCodecCtx) avcodec_close(m_VCodecCtx);
			av_free(m_VCodecCtx);
			m_VCodecCtx = NULL;

			if (m_aVCodecCtx) avcodec_close(m_aVCodecCtx);
			av_free(m_aVCodecCtx);
			m_aVCodecCtx = NULL;

			if (m_ACodecCtx) avcodec_close(m_ACodecCtx);
			av_free(m_ACodecCtx);
			m_ACodecCtx = NULL;
		}

		if (m_convert_ctx)
		{
			sws_freeContext(m_convert_ctx);
			m_convert_ctx = NULL;
		}
	}

	// it is running in decoder thread
	// Open video file
	bool as_netstream::open_stream(const char* c_url)
	{
		int ret = avformat_open_input(&m_FormatCtx, c_url, NULL, NULL);
		if (ret < 0)
		{
			char msg[256];
			av_strerror(ret, msg, sizeof(msg));
			myprintf("open stream error %d for '%s'\n%s\n", ret, c_url, msg);
			set_status(error, playStreamNotFound);
			return false;
		}

		// Next, we need to retrieve information about the streams contained in the file
		// This fills the streams field of the AVFormatContext with valid information
//		if (av_find_stream_info2(m_FormatCtx) < 0)
		if (avformat_find_stream_info(m_FormatCtx, NULL) < 0)
		{
			myprintf("Couldn't find stream information from '%s'\n", c_url);
			return false;
		}

		// Find the best video stream

		AVCodec *pCodec;
		m_video_index = av_find_best_stream(m_FormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);
		if (m_video_index < 0)
		{
			myprintf("Didn't find a video stream from '%s'\n", c_url);
			return false;
		}
		m_video_stream = m_FormatCtx->streams[m_video_index];

		// Get a pointer to the codec context for the video stream
		m_VCodecCtx = m_FormatCtx->streams[m_video_index]->codec;

		// Find the decoder for the video stream
		if (pCodec == NULL)
		{
			m_VCodecCtx = NULL;
			myprintf("Decoder not found\n");
			return false;
		}

		// init the video decoder
		if (avcodec_open2(m_VCodecCtx, pCodec, NULL) < 0)
		{
			m_VCodecCtx = NULL;
			myprintf("Could not open codec\n");
			return false;
		}

		// Find the best sound stream

		if (get_sound_handler() != NULL)
		{
			AVCodec *pCodec;
			m_audio_index = av_find_best_stream(m_FormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &pCodec, 0);
			if (m_audio_index < 0)
			{
				m_audio_index = -1;
//				myprintf("Didn't find an audio stream from '%s'\n", c_url);
			}
			else
			{
				m_audio_stream = m_FormatCtx->streams[m_audio_index];

				// Get a pointer to the audio codec context for the video stream
				m_ACodecCtx = m_FormatCtx->streams[m_audio_index]->codec;

				// Find the decoder for the audio stream
				AVCodec* pACodec = avcodec_find_decoder(m_ACodecCtx->codec_id);
				if (pACodec == NULL)
				{
					m_audio_index = -1;
					myprintf("No available AUDIO decoder to process MPEG file: '%s'\n", c_url);
				}

				// Open codec
				if (avcodec_open2(m_ACodecCtx, pACodec, NULL) < 0)
				{
					m_audio_index = -1;
					myprintf("Could not open AUDIO codec\n");
				}
			}
		}

		int swsFlags = SWS_FAST_BILINEAR | SWS_ACCURATE_RND;
		m_convert_ctx = sws_getContext(
			m_VCodecCtx->width, m_VCodecCtx->height, m_VCodecCtx->pix_fmt,
			m_VCodecCtx->width, m_VCodecCtx->height, AV_PIX_FMT_RGBA,	swsFlags, NULL, NULL, NULL);
		assert(m_convert_ctx);

		return true;
	}


	// it is running in sound mixer thread
	bool as_netstream::decode_audio(const AVPacket& pkt, Sint16** data, int* size)
	{
		int got = 0;
		AVFrame* frame = av_frame_alloc();
		avcodec_decode_audio4(m_ACodecCtx, frame, &got,	&pkt);
		if (got)
		{
			sound_handler* sh = get_sound_handler();
			if (sh)
			{
				// AV_SAMPLE_FMT_FLTP ==> AV_SAMPLE_FMT_S16
				if (m_ACodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
				{
					int size = frame->nb_samples * frame->channels * sizeof(Sint16);
					Sint16* buf = (Sint16*) malloc(size);
					Sint16* pbuf = buf;
					for (int i = 0; i < frame->nb_samples; i++)
					{
						for (int c = 0; c < frame->channels; c++)
						{
							float* extended_data = (float*) frame->extended_data[c];
							*pbuf++ = (Sint16) fclamp(extended_data[i], -1.0f, 1.0f) * 32767.0f;
						}
					}
					sh->push_stream(m_openal_stream, frame->channels == 2 ? 1 : 0, (Uint8*) buf, size, m_ACodecCtx->sample_rate, frame->channels);
					free(buf);
				}
				else
				{
					sh->push_stream(m_openal_stream, m_ACodecCtx->sample_fmt, frame->data[0], frame->linesize[0], m_ACodecCtx->sample_rate, m_ACodecCtx->channels);
				}
			}
			av_free(frame);
			return true;
		}
		av_free(frame);
		return false;
	}

	// it is running in decoder thread
	Uint8* as_netstream::decode_video(const AVPacket& pkt)
	{
		int got = 0;
		AVFrame* frame = av_frame_alloc();
		avcodec_decode_video2(m_VCodecCtx, frame, &got, &pkt);
		if (got)
		{
			int w = m_VCodecCtx->width;
			int h = m_VCodecCtx->height;

			// get buf for rgba picture
			// will be freed later in update_video() 
			int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 1);

			Uint8* data = (Uint8*) av_malloc(size);
//			AVPicture dst;
			AVFrame* dst = av_frame_alloc();
			dst->width = w;
			dst->height = h;
			dst->format = AV_PIX_FMT_RGBA;

			// avpicture_fill(&dst, data, AV_PIX_FMT_RGBA, w, h);
			av_image_fill_arrays(dst->data, dst->linesize, data, AV_PIX_FMT_RGBA, dst->width, dst->height, 1);

			sws_scale(m_convert_ctx, frame->data, frame->linesize, 0, h, dst->data, dst->linesize);

			av_free(dst);
			av_free(frame);
			return data;
		}

		av_free(frame);
		return NULL;
	}

	// for embedded video
	bool as_netstream::set_decoder(AVCodecID id)
	{
		// color stream

		AVCodec* codec = avcodec_find_decoder(id);
		if (codec == NULL)
		{
			myprintf("avcodec_find_decoder: no codec\n");
			return false;
		}

		assert(m_VCodecCtx == NULL);
		m_VCodecCtx = avcodec_alloc_context3(codec);
		if (m_VCodecCtx == NULL)
		{
			myprintf("avcodec_alloc_context3 failed\n");
			return false;
		}

		int ret = avcodec_open2(m_VCodecCtx, codec, NULL);
		if (ret < 0)
		{
			myprintf("avcodec_open2 failed\n");
			avcodec_close(m_VCodecCtx);
			av_free(m_VCodecCtx);
			return false;
		}

		// alpha stream

		assert(m_aVCodecCtx == NULL);
		m_aVCodecCtx = avcodec_alloc_context3(codec);
		if (m_aVCodecCtx == NULL)
		{
			myprintf("avcodec_alloc_context3 failed\n");
			return false;
		}

		ret = avcodec_open2(m_aVCodecCtx, codec, NULL);
		if (ret < 0)
		{
			myprintf("avcodec_open2 failed\n");
			avcodec_close(m_aVCodecCtx);
			av_free(m_aVCodecCtx);
			return false;
		}

		assert(m_VCodecCtx->pix_fmt == AV_PIX_FMT_YUV420P);
		assert(m_aVCodecCtx->pix_fmt == AV_PIX_FMT_YUV420P);

		//myprintf("init embedded video\n");
		return true;
	}

	// for embedded video
	bool as_netstream::decode_frame(AVCodecID id, Uint8* buf, int size, Uint8* alpha_buf, int alpha_size, bool state_only, bool flip)
	{
		if (m_VCodecCtx == NULL)
		{
			set_decoder(id);
			assert(m_aVCodecCtx);
			assert(m_VCodecCtx);
		}
		assert(buf);

		int agot = 0;
		AVFrame aframe = {}; // = av_frame_alloc();
		if (alpha_buf)
		{
			AVPacket apkt;
			av_init_packet(&apkt);
			apkt.stream_index = 0;
			apkt.data = alpha_buf;
			apkt.size = alpha_size;
			apkt.pts = apkt.dts = 0;
			apkt.flags |= AV_PKT_FLAG_KEY;
		//	apkt.priv = NULL;

			int aret = avcodec_decode_video2(m_aVCodecCtx, &aframe, &agot, &apkt);
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.stream_index = 0;
		pkt.data = buf;
		pkt.size = size;
		pkt.pts = pkt.dts = 0;
		pkt.flags |= AV_PKT_FLAG_KEY;

		int got = 0;
		AVFrame frame = {}; // = av_frame_alloc();
		int ret = avcodec_decode_video2(m_VCodecCtx, &frame, &got, &pkt);

		if (state_only)
		{
			// only goto
			return true;
		}


	//	 Uint32 t = tu_timer::get_ticks();
		if (got)
		{
			int w = frame.width;
			int h = frame.height;

			// get buf for rgba picture
			// will be freed later in update_video() 
			int pic_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 1);

			const Uint8* ta = get_ta_table();
			const Uint8* tb = get_tb_table();
			const Uint8* tc = get_tc_table();
			assert(ta && tb && tc);


			Uint8* data = (Uint8*) av_malloc(pic_size);
			Uint8* p = data;
			Uint8* d0 = frame.data[0];
			Uint8* d1 = frame.data[1];
			Uint8* d2 = frame.data[2];
			Uint8* ad0 = agot ? aframe.data[0] : NULL;

			if (flip)
			{
				int fl0 = frame.linesize[0] * h;
				int fl1 = frame.linesize[1] * (h >> 1);
				int fl2 = frame.linesize[2] * (h >> 1);

				int alinesize = agot ? aframe.linesize[0] : 0;
				int afl0 = alinesize * h;

				for (int y = h; y > 0; y--)
				{
					fl0 -= frame.linesize[0];
					afl0 -= alinesize;

					if ((y & 1) == 0)
					{
						fl1 -= frame.linesize[1];
						fl2 -= frame.linesize[2];
						//assert(fl1>=0 && fl2 >=0);
					}

					for (int x = 0; x < w; x++)
					{
						// color YUV
						Uint8 Y1 = d0[fl0 + x];

						Uint8 U1; 
						Uint8 V1;
						if ((x & 1) == 0)
						{
							int xx = x >> 1;		// div 2
							U1 = d1[fl1 + xx]; 
							V1 = d2[fl2 + xx];
						}

						// alpha YUV
						Uint8 Y2 = agot ? ad0[afl0 + x] : 255;

						Uint8 b;
						b = ta[(Y1 << 8) + V1];
						*p++ = Y2 < b ? Y2 : b;		// R

						b = tb[(Y1 << 16) + (V1 << 8) + U1];
						*p++ = Y2 < b ? Y2 : b;		// G

						b = tc[(Y1 << 8) + U1];
						*p++ = Y2 < b ? Y2 : b;		// B

						*p++ = Y2;		// A
					}
				}
			}
			else
			{
				for (int y = 0; y < h; y++)
				{
					int yy = y >> 1;	// div 2

					int fl0 = frame.linesize[0] * y;
					int fl1 = frame.linesize[1] * yy;
					int fl2 = frame.linesize[2] * yy;

					int afl0 = agot ? aframe.linesize[0] * y : 0;

					for (int x = 0; x < w; x++)
					{
						int xx = x >> 1;		// div 2

						// color YUV
						int Y1 = d0[fl0 + x];
						int U1 = d1[fl1 + xx]; //frame.data[1][frame.linesize[1] * yy + xx];
						int V1 = d2[fl2 + xx];	// frame.data[2][frame.linesize[2] * yy + xx];

						// alpha YUV
						Uint8 Y2 = agot ? ad0[afl0 + x] : 255; // aframe.data[0][aframe.linesize[0] * y + x];

						// YUV ==> RGB conversion
						// must be high optimized !!!
						if (ta && tb && tc)
						{
							Uint8 b = ta[(Y1 << 8) + V1];
							*p++ = Y2 < b ? Y2 : b;		// R
							b = tb[(Y1 << 16) + (V1 << 8) + U1];
							*p++ = Y2 < b ? Y2 : b;		// G
							b = tc[(Y1 << 8) + U1];
							*p++ = Y2 < b ? Y2 : b;		// B
							*p++ = Y2;		// A
						}
						else
						{
							*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) + 1.596f * (V1 - 128)));		// R
							*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) - 0.813f * (V1 - 128) - 0.391f * (U1 - 128)));		// G
							*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) + 2.018f * (U1 - 128)));		// B
							*p++ = Y2;		// A
						}
					}
				}
			}

		//	png_helper::write_rgba("C:\\Users\\vitaly\\Documents\\p.png", data, w, h, 4);
			set_video_data(data, 0, 0);
		//	myprintf("video time: %d\n",	tu_timer::get_ticks()-t);
			return true;
		}
		return false;
	}

	int as_netstream::get_width() const
	{
		if (m_VCodecCtx)
		{
			return m_VCodecCtx->width;
		}
		return 0;
	}

	int as_netstream::get_height() const
	{
		if (m_VCodecCtx)
		{
			return m_VCodecCtx->height;
		}
		return 0;
	}

	// pass video data in video_stream_instance()
	Uint8* as_netstream::get_video_data(int* w, int* h)
	{
		tu_autolock locker(m_lock_video);
		Uint8* video_data = m_video_data;
		m_video_data = NULL;
		if (w) *w = get_width();
		if (h) *h = get_height();
		return video_data;
	}

	void as_netstream::free_video_data(Uint8* data)
	{
		av_free(data);
	}

	void as_netstream::set_video_data(Uint8* data, int w, int h)
	{
		tu_autolock locker(m_lock_video);
		if (m_video_data)
		{
			av_free(m_video_data);
		}
		m_video_data = data;
	}

	video_pixel_format::code as_netstream::get_pixel_format() { return video_pixel_format::RGBA; }

	double as_netstream::get_duration() const
	{
		return (double) m_FormatCtx->duration / 1000000;
	}

	double as_netstream::get_bufferlength()
	{
		if (m_video_stream != NULL)
		{
			// hack,
			// TODO: take account of PTS (presentaion time stamp)
			return m_vq.size() * av_q2d(m_video_stream->codec->time_base);	// frame_delay
		}
		return 0;
	}

	void netstream_close(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		ns->close_thread();
	}

	// flag:Boolean [optional] - A Boolean value specifying whether to pause play (true) or resume play (false).
	// If you omit this parameter, NetStream.pause() acts as a toggle: the first time it is called on a specified stream,
	// it pauses play, and the next time it is called, it resumes play.
	void netstream_pause(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs > 0)
		{
			bool pause = fn.arg(0).to_bool();
			if (pause == false && ns->m_status == as_netstream::PAUSE)	// play
			{
				ns->play(NULL);
			}
			else
				if (pause == true && ns->m_status == as_netstream::PLAY)	// pause
				{
					ns->m_status = as_netstream::PAUSE;
				}
		}
		else
		{
			// toggle mode
			if (ns->m_status == as_netstream::PAUSE)	// play
			{
				ns->play(NULL);
			}
			else
				if (ns->m_status == as_netstream::PLAY)	// pause
				{
					ns->m_status = as_netstream::PAUSE;
				}
		}
	}

	void netstream_play(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs < 1)
		{
			myprintf("NetStream play needs args\n");
			return;
		}

		ns->set_nonstop(false);
		if (fn.nargs > 1)
		{
			ns->set_nonstop(fn.arg(1).to_bool());
		}
		ns->play(fn.arg(0).to_tu_string());
	}

	// Seeks the keyframe closest to the specified number of seconds from the beginning
	// of the stream.
	void netstream_seek(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs < 1)
		{
			myprintf("NetStream seek needs args\n");
			return;
		}

		ns->seek(fn.arg(0).to_number());
	}

	// public setBufferTime(bufferTime:Number) : Void
	// bufferTime:Number - The number of seconds of data to be buffered before 
	// Flash begins displaying data.
	// The default value is 0.1 (one-tenth of a second).
	void netstream_setbuffertime(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);

		if (fn.nargs < 1)
		{
			myprintf("setBufferTime needs args\n");
			return;
		}

		ns->set_buffertime(fn.arg(0).to_number());
	}

	void netstream_time(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_double(ns->time());
	}

	void netstream_buffer_length(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_double(ns->get_bufferlength());
	}

	void netstream_buffer_time(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_double(ns->get_buffertime());
	}

	void netstream_video_width(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_int(ns->get_width());
	}

	void netstream_video_height(const fn_call& fn)
	{
		as_netstream* ns = cast_to<as_netstream>(fn.this_ptr);
		assert(ns);
		fn.result->set_int(ns->get_height());
	}

	void	as_global_netstream_ctor(const fn_call& fn)
		// Constructor for ActionScript class NetStream.
	{
		as_object* netstream = new as_netstream();

		// properties
		netstream->builtin_member("time", as_value(netstream_time, as_value()));
		netstream->builtin_member("bufferLength", as_value(netstream_buffer_length, as_value()));
		netstream->builtin_member("bufferTime", as_value(netstream_buffer_time, as_value()));

		// methods
		netstream->builtin_member("close", netstream_close);
		netstream->builtin_member("pause", netstream_pause);
		netstream->builtin_member("play", netstream_play);
		netstream->builtin_member("seek", netstream_seek);
		netstream->builtin_member("setbuffertime", netstream_setbuffertime);

		// bakeinflash extension, video width & height
		netstream->builtin_member("_width", as_value(netstream_video_width, as_value()));
		netstream->builtin_member("_height", as_value(netstream_video_height, as_value()));

		fn.result->set_as_object(netstream);
	}


} // end of bakeinflash namespace

#else

#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_log.h"

namespace bakeinflash
{
	void as_global_netstream_ctor(const fn_call& fn)
	{
		myprintf("video requires FFMPEG library\n");
	}
}

#endif
