// bakeinflash_video_impl.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef BAKEINFLASH_VIDEO_H
#define BAKEINFLASH_VIDEO_H

#include "bakeinflash_impl.h"
#include "bakeinflash_as_classes/as_video_stream.h"

namespace bakeinflash
{
	struct video_packet : public ref_counted
	{
		video_packet(int codec, int color_size, int alpha_size, bool flip) :
			m_codec(codec),
			m_color_size(color_size),
			m_alpha_size(alpha_size),
			m_color_channel(NULL),
			m_alpha_channel(NULL),
			m_flip(flip)
		{
			if (color_size > 0)
			{
				m_color_channel = new Uint8[color_size];
			}

			if (alpha_size > 0)
			{
				m_alpha_channel = new Uint8[alpha_size];
			}
		}

		virtual ~video_packet()
		{
			delete [] m_color_channel;
			delete [] m_alpha_channel;
		}


		void decode(as_video_stream* ns, bool state_only);

		int m_codec;
		int m_color_size;
		int m_alpha_size;
		Uint8* m_color_channel;
		Uint8* m_alpha_channel;
		bool m_flip;
	};

	struct video_stream_definition : public character_def
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_VIDEO_DEF };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character_def::is(class_id);
		}

		video_stream_definition();
		//	virtual ~video_stream_definition();


		character* create_character_instance(character* parent, int id);
		void	read(stream* in, int tag, movie_definition* m);
		virtual void get_bound(rect* bound);
		bool has_embedded_video() const { return m_frames.size() > 0; }

		Uint16 m_width;
		Uint16 m_height;
		array< smart_ptr<video_packet> >	m_frames;

	private:

		//	uint8_t reserved_flags;
		Uint8 m_deblocking_flags;
		bool m_smoothing_flags;

		// 0: extern file
		// 2: H.263
		// 3: screen video (Flash 7+ only)
		// 4: VP6
		Uint8 m_codec_id;
	};

	struct video_stream_instance : public character
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_VIDEO_INST };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		video_stream_instance(video_stream_definition* def,	character* parent, int id);
		~video_stream_instance();

		virtual void	display();
		virtual character_def* get_character_def() { return m_def.get();	}

		//
		// ActionScript overrides
		//

		// To drop the connection to the Video object, pass null for source.
		void attach_netstream(as_video_stream* ns)
		{
			video_stream_definition* vdef = cast_to<video_stream_definition>(m_def);
			if (vdef->has_embedded_video() == false)
			{
				m_ns = ns;
			}
		}

		smart_ptr<video_handler> m_video_handler;

		private:

		smart_ptr<video_stream_definition>	m_def;

		// A Camera object that is capturing video data or a NetStream object.
		smart_ptr<as_video_stream> m_ns;

		// for embedded video
		int m_current_frame;

		// фрейм с которогот начинается видео
		int m_parent_base_frame;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_VIDEO_H
