// as_netstream.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_CAMERA_H
#define BAKEINFLASH_CAMERA_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/tu_queue.h"
#include "bakeinflash/bakeinflash_object.h"
#include "as_video_stream.h"

// USB video
#if TU_CONFIG_LINK_TO_OPENCV == 1
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
//#include "opencv2/face.hpp"
#endif

namespace bakeinflash
{
	
	struct as_camera : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CAMERA };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
	
		as_camera();
		virtual ~as_camera();
		void notify_image(bool rc);
		as_object* get();
		void take_image(character* target, float quality);
	};

	struct as_camera_object : public as_video_stream
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CAMERA_OBJECT };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
	
		as_camera_object();
		virtual ~as_camera_object();

		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& new_val);
		virtual int get_width() const;
		virtual int get_height() const;

		virtual Uint8* get_video_data(int* w, int* h);
		virtual void free_video_data(Uint8* data);
		virtual void set_video_data(Uint8* data, int w, int h);

		virtual bool open_stream(const char* url);
		virtual void close_stream();
		virtual void	advance(float delta_time) {};
		

#ifdef iOS
    virtual video_pixel_format::code get_pixel_format() { return video_pixel_format::RGBA; };
#else
    virtual video_pixel_format::code get_pixel_format();
#endif
    
		void run();
		tu_string takeImage();
		tu_string videoClick();
		tu_string faceRecognize();


		// RGBA video frame 
		Uint8* m_video_data;
		tu_mutex m_lock_video;

		smart_ptr<tu_thread> m_thread;
		bool m_go;

#if TU_CONFIG_LINK_TO_OPENCV == 1
		cv::VideoCapture m_cap; // open the default camera
		cv::VideoWriter m_avi;  
		cv::Mat m_frame;
		CvFont m_font;
#endif

		tu_mutex m_mutex;

		volatile int m_hue;
		volatile int m_contrast;
		volatile int m_brightness;
		volatile int m_saturation;
		volatile int m_width;
		volatile int m_height;
		volatile int m_timeStamp;
		volatile int m_fps;
	};

	as_object* camera_init();

} // end of bakeinflash namespace

// bakeinflash_NETSTREAM_H
#endif

