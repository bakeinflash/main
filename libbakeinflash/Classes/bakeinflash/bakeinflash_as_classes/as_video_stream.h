// as_video_stream.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2016
// This source code has been donated to the Public Domain.  Do whatever you want with it.
// video stream interface, it's used in netstream and camera

#ifndef BAKEINFLASH_VIDEO_STREAM_H
#define BAKEINFLASH_VIDEO_STREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash_object.h"

namespace bakeinflash
{

	struct as_video_stream : public as_object
	{

		virtual void	advance(float delta_time) = 0;

//		bool decode_audio(const AVPacket& pkt, Sint16** data, int* size) {};
//		Uint8* decode_video(const AVPacket& pkt) ;

		virtual int get_width() const = 0;
		virtual int get_height() const = 0;

		virtual Uint8* get_video_data(int* w, int* h) = 0;
		virtual void free_video_data(Uint8* data) = 0;
		virtual void set_video_data(Uint8* data, int w, int h) = 0;

		virtual bool open_stream(const char* url) = 0;
		virtual void close_stream() = 0;
		virtual video_pixel_format::code get_pixel_format() = 0;
	};

} // end of bakeinflash namespace

#endif
