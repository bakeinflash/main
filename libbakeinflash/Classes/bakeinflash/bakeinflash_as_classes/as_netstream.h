// as_netstream.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_NETSTREAM_H
#define BAKEINFLASH_NETSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash.h"
#include "base/tu_queue.h"
#include "bakeinflash/bakeinflash_object.h"
#include "as_video_stream.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

#include "bakeinflash/bakeinflash_mutex.h"
extern "C" 
{
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/imgutils.h>
}

namespace bakeinflash
{

	void as_global_netstream_ctor(const fn_call& fn);
	void netstream_new(const fn_call& fn);
	void netstream_close(const fn_call& fn);
	void netstream_pause(const fn_call& fn);
	void netstream_play(const fn_call& fn);
	void netstream_seek(const fn_call& fn);
	void netstream_setbuffertime(const fn_call& fn);

	long decode_piff(const tu_string& infile, const tu_string& outfile, const tu_string& hdsfile);

	template<class T> 
	struct av_queue
	{
		void lock()
		{
			m_mutex.lock();
		}

		void unlock()
		{
			m_mutex.unlock();
		}

		void push(const T& val)
		{
			m_mutex.lock();
			m_queue.push(val);
			m_mutex.unlock();
		}

		int size()
		{
			m_mutex.lock();
			int k = int(m_queue.size());
			m_mutex.unlock();
			return k;
		}

		bool pop(T* val)
		{
			m_mutex.lock();
			bool rc = false;
			if (m_queue.size() > 0)
			{
				*val = m_queue.front();
				m_queue.pop(); 
				rc = true;
			}
			m_mutex.unlock();
			return rc;
		}

		void clear()
		{
			m_mutex.lock();
			m_queue.clear();
			m_mutex.unlock();
		}

	private:

		tu_mutex m_mutex;
		tu_queue<T> m_queue;
	};

	// container for the decoded sound
	struct decoded_sound : public ref_counted
	{
		decoded_sound(Sint16* sample, int size) :
			m_size(size),
			m_buf(sample)
		{
			m_data = (Uint8*) sample;
		}

		~decoded_sound()
		{
			// We can delete to original WAV data now (created in cvt())
			free(m_buf);
		}

		inline int size() const
		{
			return m_size;
		}

		int extract(Uint8* stream, int len)
		{
			int n = imin(m_size, len);
			if (m_data && n > 0)
			{
				memcpy(stream, m_data, n);
				m_data += n;
				m_size -= n;
				return n;
			}
			return 0;
		}

	private:

		int m_size;
		Sint16* m_buf;
		Uint8* m_data;
	};
	
	struct as_netstream : public as_video_stream
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_NETSTREAM };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
		
		// netstream status
		enum netstream_status
		{
			PLAY,
			PAUSE,
			CLOSE
		};

		// netstream event levels
		enum netstream_event_level
		{
			status,
			error
		};

		// netstream event codes
		enum netstream_event_code
		{
			playStreamNotFound,		// NetStream.Play.StreamNotFound
			playStart,		// NetStream.Play.Start
			playStop,		// NetStream.Play.Stop
			seekNotify,		//NetStream.Seek.Notify
			seekInvalidTime,		// NetStream.Seek.InvalidTime
			bufferEmpty,		// NetStream.Buffer.Empty
			bufferFull		//NetStream.Buffer.Full
		};

		typedef struct
		{
			netstream_event_level level;
			netstream_event_code code;
		}
		stream_event;

		as_netstream();
		virtual ~as_netstream();

		void	advance(float delta_time);
		virtual video_pixel_format::code get_pixel_format();

		bool decode_audio(const AVPacket& pkt, Sint16** data, int* size);
		Uint8* decode_video(const AVPacket& pkt);
		void run();
		void seek(double seek_time);

		double get_buffertime() const { return m_buffer_time; }
		void set_buffertime(double sec) { m_buffer_time = sec; }
		double get_bufferlength();

		void audio_callback(Uint8* stream, int len);
		void close_thread();
		void play(const char* url);

		//	The position of the playhead, in seconds.
		double time() const { return m_video_time; }

		// for embedded video
		bool set_decoder(AVCodecID id);
		bool decode_frame(AVCodecID id, Uint8* buf, int size, Uint8* alpha_buf, int alpha_size, bool state_only, bool flip);

		int get_width() const;
		int get_height() const;

		Uint8* get_video_data(int* w, int* h);
		void free_video_data(Uint8* data);
		void set_video_data(Uint8* data, int w, int h);
		void set_nonstop(bool val) { m_nonstop = val; }

		volatile netstream_status m_status;
		volatile double m_seek_time;


	private:

		void set_status(netstream_event_level level, netstream_event_code code);
		bool open_stream(const char* url);
		void clear_queues();
		void close_stream();
		double get_duration() const;

		// return current time in sec
		double now() const;

		AVFormatContext* m_FormatCtx;

		// video
		AVCodecContext* m_VCodecCtx;
		AVCodecContext* m_aVCodecCtx;	// alpha stream, for embedded
		AVStream* m_video_stream;

		// audio
		AVCodecContext *m_ACodecCtx;
		AVStream* m_audio_stream;
	//	smart_ptr<decoded_sound> m_sound;
		int m_openal_stream;

		double m_start_time;	// sec
		double m_video_time;	// sec

		int m_video_index;
		int m_audio_index;

		tu_string m_url;

		av_queue< AVPacket > m_aq;	// audio queue
		av_queue< AVPacket > m_vq;	// video queue
		av_queue<stream_event> m_event;	// video stream events

		smart_ptr<tu_thread> m_thread;
		tu_condition m_decoder;

		// RGBA video frame 
		Uint8* m_video_data;
		tu_mutex m_lock_video;

		// for converting video frame in RGBA
		SwsContext* m_convert_ctx;

		// Specifies how long(in sec.) to buffer messages before starting to display the stream.
		double m_buffer_time;
		bool m_nonstop;
	};

} // end of bakeinflash namespace

#else	// ffmpeg is not present

namespace bakeinflash
{
	struct as_netstream : public as_object
	{
		int get_width() const {	return 0;	}
		int get_height() const { return 0; }
		Uint8* get_video_data(int* w, int* h)	{	return NULL; }
		void free_video_data(Uint8* data) {}

		bool decode_frame(int id, Uint8* buf, int size, Uint8* alpha_buf, int alpha_size, bool state_only, bool flip) { return false; }

	};

	void as_global_netstream_ctor(const fn_call& fn);
}

#endif

// bakeinflash_NETSTREAM_H
#endif

