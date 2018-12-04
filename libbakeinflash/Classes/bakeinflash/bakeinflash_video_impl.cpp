// bakeinflash_video_impl.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// video implementation

#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_video_impl.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_netstream.h"
#include "bakeinflash/bakeinflash_as_classes/as_camera.h"

namespace bakeinflash
{

	video_stream_definition::video_stream_definition()
	{
		if (bakeinflash::get_yuv2rgb_table())
		{
			create_YUV2RGBtable();
		}
	}

	void video_stream_definition::get_bound(rect* bound)
	{
		bound->m_x_min = 0;
		bound->m_y_min = 0;
		bound->m_x_max = PIXELS_TO_TWIPS(m_width);
		bound->m_y_max = PIXELS_TO_TWIPS(m_height);
	}

	void video_stream_definition::read(stream* in, int tag, movie_definition* m)
	{
		// Character ID has been read already 
	
		assert(tag == 60 ||	tag == 61);

		// video
		if (tag == 60)
		{
	
			Uint16 numframes = in->read_u16();
			m_frames.resize(numframes);
	
			m_width = in->read_u16();
			m_height = in->read_u16();
			Uint8 reserved_flags = in->read_uint(5);
                        UNUSED(reserved_flags);
			m_deblocking_flags = in->read_uint(2);
			m_smoothing_flags = in->read_uint(1) ? true : false;
	
			m_codec_id = in->read_u8();
		}
		else 
		// video frame
		if (tag == 61)
		{
#if TU_CONFIG_LINK_TO_FFMPEG == 1
			Uint16 frame = in->read_u16();
			IF_VERBOSE_PARSE(myprintf("video frame %d\n", frame));
			m_frames[frame] = NULL;

			// videodata
			switch (m_codec_id)
			{
				case 2:		// H263VIDEOPACKET = FLV
				{
					int tag_length = in->get_tag_end_position() - in->get_position();
					video_packet* vp = new video_packet(AV_CODEC_ID_FLV1, tag_length, 0, false);
					for (int i = 0; i < tag_length; i++)
					{
						vp->m_color_channel[i] = in->read_uint(8);
					}
					m_frames[frame] = vp;
					break;
				}

				case 3:	//	SCREENVIDEOPACKET
					myprintf("SCREENVIDEOPACKET is not implememnted yet\n");
					break;

				case 4:	//	VP6SWFVIDEOPACKET
				{
					int tag_length = in->get_tag_end_position() - in->get_position();
					video_packet* vp = new video_packet(AV_CODEC_ID_VP6, tag_length, 0, true);
					for (int i = 0; i < tag_length; i++)
					{
						vp->m_color_channel[i] = in->read_uint(8);		// color channel
					}

					m_frames[frame] = vp;
					break;
				}

				case 5:	// VP6SWFALPHAVIDEOPACKET
				{
					int color_size = in->read_uint(24);	// size
					int tag_length = in->get_tag_end_position() - in->get_position();
					int alpha_size = tag_length - color_size;
					assert(alpha_size > 0);
					video_packet* vp = new video_packet(AV_CODEC_ID_VP6, color_size, alpha_size, true);
					for (int i = 0; i < color_size; i++)
					{
						vp->m_color_channel[i] = in->read_uint(8);		// color channel
					}

					for (int i = 0; i < alpha_size; i++)
					{
						vp->m_alpha_channel[i] = in->read_uint(8);	// alpha channel
					}

					m_frames[frame] = vp;
					break;
				}

				case 6:	//	SCREENV2VIDEOPACKET
					myprintf("SCREENV2VIDEOPACKET is not implememnted yet\n");
					break;

				default:
					myprintf("invalid codec id %d\n", m_codec_id);
					break;
			}
#endif
		}
	}
	
	character* video_stream_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new video_stream_instance(this, parent, id);
		// instanciate_registered_class(ch);	//TODO: test it
		return ch;
	}

	void attach_video(const fn_call& fn)
	{
		video_stream_instance* video = cast_to<video_stream_instance>(fn.this_ptr);
		if (video && fn.nargs > 0)
		{
			// fn.arg(0) may be null
			video->attach_netstream(cast_to<as_video_stream>(fn.arg(0).to_object()));
		}
	}

	void clear_video_background(const fn_call& fn)
	{
		video_stream_instance* video = cast_to<video_stream_instance>(fn.this_ptr);
		if (video && fn.nargs > 0)
		{
			assert(video->m_video_handler);
			video->m_video_handler->clear_background(fn.arg(0).to_bool());
		}
	}

	video_stream_instance::video_stream_instance(video_stream_definition* def, character* parent, int id)	:
		character(parent, id),
		m_def(def),
		m_current_frame(0),		// for embedded video
		m_parent_base_frame(0)
	{
		assert(m_def != NULL);

		m_video_handler = render::create_video_handler();
		if (m_video_handler == NULL)
		{
			myprintf("No available video render\n");
		}

		builtin_member("attachVideo", attach_video);
        
		// bakeinflash extension, set video background cleanup option, true/false
		builtin_member("clearBackgroundColor", as_value(as_value(), clear_video_background));

		// set frame from which starts video
		m_parent_base_frame = m_parent->get_target_frame();
		assert(m_parent_base_frame >= 0 && m_parent_base_frame < m_parent->get_frame_count());
	}

	video_stream_instance::~video_stream_instance()
	{
	}

	void video_packet::decode(as_video_stream* vs, bool state_only)
	{
#if TU_CONFIG_LINK_TO_FFMPEG == 1
		as_netstream* ns = cast_to<as_netstream>(vs);
		if (ns)
		{
			ns->decode_frame((AVCodecID) m_codec, m_color_channel, m_color_size, m_alpha_channel, m_alpha_size, state_only, m_flip);
		}
#endif
	}

	void video_stream_instance::display()
	{
#if TU_CONFIG_LINK_TO_FFMPEG == 0
    return;
#endif
    
		video_stream_definition* vdef = cast_to<video_stream_definition>(m_def);
		if (vdef->has_embedded_video())
		{
			if (m_ns == NULL)
			{
				m_ns = (as_video_stream*) new as_netstream();
				m_current_frame = 0;
			}

			sprite_instance* parent = cast_to<sprite_instance>(get_parent());
			int frame = (parent->get_current_frame() - m_parent_base_frame) + 1;	// 1 based
			if (m_current_frame < frame)
			{
				// goto forward
				for (int i = m_current_frame; i < frame - 1; i++)
				{
					if (i < vdef->m_frames.size() && vdef->m_frames[i])
					{
						vdef->m_frames[i]->decode(m_ns, true);
					}
				}

				// seek and display last NON NULL frame
				for (int i = frame - 1; i >= m_current_frame; i--)
				{
					if (i < vdef->m_frames.size() && vdef->m_frames[i])
					{
						Uint8* video_data = m_ns->get_video_data(NULL, NULL);
						m_ns->free_video_data(video_data);
						vdef->m_frames[i]->decode(m_ns, false);
						break;
					}
				}

			}
			else
			if (m_current_frame > frame)
			{
				// seek NON NULL frame
				int target = 0;
				for (int i = frame - 1; i >= 0; i--)
				{
					if (i < vdef->m_frames.size() && vdef->m_frames[i].get())
					{
						target = i + 1;
						break;
					}
				}
				
				if (target ==  0)
				{
					// no video
					return;
				}

				// goto back
				for (int i = 0; i < target - 1; i++)
				{
					if (i < vdef->m_frames.size() && vdef->m_frames[i])
					{
						vdef->m_frames[i]->decode(m_ns, true);
					}
				}

				// display
				if (target <= vdef->m_frames.size() && vdef->m_frames[target - 1])
				{
					Uint8* video_data = m_ns->get_video_data(NULL, NULL);
					m_ns->free_video_data(video_data);
					vdef->m_frames[target - 1]->decode(m_ns, false);
				}
			}
			else
			{
//				int k=0;
			}
			m_current_frame = frame;

			// display
			rect bounds;
			bounds.m_x_min = 0.0f;
			bounds.m_y_min = 0.0f;
			bounds.m_x_max = PIXELS_TO_TWIPS(m_def->m_width);
			bounds.m_y_max = PIXELS_TO_TWIPS(m_def->m_height);

			cxform cx;
			get_world_cxform(&cx);
			bakeinflash::rgba color = cx.transform(bakeinflash::rgba());

			matrix m;
			get_world_matrix(&m);

			Uint8* video_data = m_ns->get_video_data(NULL, NULL);

			// video_data==NULL means that video is not updated and video_handler will draw the last video frame from last texture
			int video_w = m_ns->get_width();
			int video_h = m_ns->get_height();

			// embedd video frame size may be different from flash video frame size !!!
			// so needs to rescale
			float scale_x = (float) video_w / m_def->m_width;
			float scale_y = (float) video_h / m_def->m_height;
			m.concatenate_xyscale(scale_x, scale_y);

			video_pixel_format::code bpp = m_ns->get_pixel_format();
			m_video_handler->display(bpp, video_data, video_w, video_h, &m, &bounds, color);

			m_ns->free_video_data(video_data);
		}
		else
		{
			if (m_ns != NULL && m_video_handler != NULL)	// is video attached ?
			{
				cxform cx;
				get_world_cxform(&cx);
				bakeinflash::rgba color = cx.transform(bakeinflash::rgba());

				matrix m;
				get_world_matrix(&m);

				// video_data==NULL means that video is not updated and video_handler will draw the last video frame
				int vw = 0; //m_ns->get_width();
				int vh = 0; //m_ns->get_height();
				Uint8* video_data = m_ns->get_video_data(&vw, &vh);

				rect bounds;
				bounds.m_x_min = 0.0f;
				bounds.m_y_min = 0.0f;
				bounds.m_x_max = PIXELS_TO_TWIPS(m_def->m_width);
				bounds.m_y_max = PIXELS_TO_TWIPS(m_def->m_height);

				if (vw > 00 && vh > 0)
				{
			/*		float xscale = bounds.m_x_max / vw;
					float yscale = bounds.m_y_max / vh;
					float scale = fmin(xscale, yscale);
					float oldw = bounds.m_x_max;
					bounds.m_x_max = bounds.m_x_max / xscale * scale;
					float oldh = bounds.m_y_max;
					bounds.m_y_max = bounds.m_y_max / yscale * scale;

					// move to center
					float adjustx = (oldw - bounds.m_x_max) / 2;
					float adjusty = (oldh - bounds.m_y_max) / 2;
					bounds.m_x_min += adjustx;
					bounds.m_y_min += adjusty;
					bounds.m_x_max += adjustx;
					bounds.m_y_max += adjusty;
					*/
				}

				video_pixel_format::code pixel_format = m_ns->get_pixel_format();
				m_video_handler->display(pixel_format, video_data, vw, vh, &m, &bounds, color);

				m_ns->free_video_data(video_data);
			}
		}
	}

} // end of namespace bakeinflash
