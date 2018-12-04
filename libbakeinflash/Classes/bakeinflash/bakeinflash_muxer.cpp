//#define UINT64_C uint64_t


//#if TU_CONFIG_LINK_TO_FFMPEG == 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bakeinflash_muxer.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}


#define STREAM_DURATION   1.0
#define SCALE_FLAGS SWS_BICUBIC

//#define MAX_SESSION_ID 100
//static mysession* s_session_pool[MAX_SESSION_ID] = { 0 };

namespace bakeinflash
{

#if defined(_WIN32) && TU_CONFIG_LINK_TO_FFMPEG == 1

	muxer::muxer():
		m_formatNumber(0),
		m_audioPtr(NULL),
		m_audioLength(0),
		m_frame_rate(0),
		m_width(0),
		m_height(0),
		m_FormatCtx(NULL),
		m_VCodecCtx(NULL),
		m_aVCodecCtx(NULL),
		m_video_stream(NULL),
		m_ACodecCtx(NULL),
		m_audio_stream(NULL),
		m_video_index(-1),
		m_audio_index(-1)
	{
		av_register_all();
	}

	muxer::~muxer()
	{
		close_session();
	}

	void muxer::packet_rescale_ts(AVPacket *pkt, AVRational tb_src, AVRational tb_dst)
	{
		if (pkt->pts != -1)		// AV_NOPTS_VALUE
		{
			pkt->pts = av_rescale_q(pkt->pts, tb_src, tb_dst);
		}
		if (pkt->dts != -1)		// AV_NOPTS_VALUE
		{
			pkt->dts = av_rescale_q(pkt->dts, tb_src, tb_dst);
		}
		if (pkt->duration > 0)
		{
			pkt->duration = (int) av_rescale_q(pkt->duration, tb_src, tb_dst);
		}
	}


	int muxer::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
	{
		// rescale output packet timestamp values from codec to stream timebase 
		packet_rescale_ts(pkt, *time_base, st->time_base);

		pkt->stream_index = st->index;

		// Write the compressed frame to the media file.
		return av_interleaved_write_frame(fmt_ctx, pkt);
	}

	void muxer::add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
	{
		AVCodecContext *c;
		int i;

		/* find the encoder */
		*codec = avcodec_find_encoder(codec_id);
		if (!(*codec)) 
		{
			fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
			exit(1);
		}

		ost->st = avformat_new_stream(oc, *codec);
		if (!ost->st)
		{
			fprintf(stderr, "Could not allocate stream\n");
			exit(1);
		}

		ost->st->id = oc->nb_streams-1;
		c = ost->st->codec;
		switch ((*codec)->type) 
		{
		case AVMEDIA_TYPE_AUDIO:
			c->sample_fmt  = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
			c->bit_rate    = 64000;
			c->sample_rate = 44100;

			if ((*codec)->supported_samplerates) 
			{
				c->sample_rate = (*codec)->supported_samplerates[0];
				for (i = 0; (*codec)->supported_samplerates[i]; i++)
				{
					if ((*codec)->supported_samplerates[i] == 44100)
					{
						c->sample_rate = 44100;
					}
				}
			}

			c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
			c->channel_layout = AV_CH_LAYOUT_STEREO;
			if ((*codec)->channel_layouts)
			{
				c->channel_layout = (*codec)->channel_layouts[0];
				for (i = 0; (*codec)->channel_layouts[i]; i++)
				{
					if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					{
						c->channel_layout = AV_CH_LAYOUT_STEREO;
					}
				}
			}

			c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
			ost->st->time_base.num = 1;
			ost->st->time_base.den = c->sample_rate;
			break;

		case AVMEDIA_TYPE_VIDEO:
			c->codec_id = codec_id;
			c->bit_rate = 400000;

			// Resolution must be a multiple of two. 
			c->width    = m_width; //352;
			c->height   = m_height; //288;

			// timebase: This is the fundamental unit of time (in seconds) in terms
			// of which frame timestamps are represented. For fixed-fps content,
			// timebase should be 1/framerate and timestamp increments should be identical to 1. 
			ost->st->time_base.den = m_frame_rate;
			ost->st->time_base.num = 1;
			c->time_base       = ost->st->time_base;
			c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt       = AV_PIX_FMT_YUV420P;

			if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
			{
				// just for testing, we also add B frames 
				c->max_b_frames = 2;
			}
			if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
			{
				// Needed to avoid using macroblocks in which some coeffs overflow.
				// This does not happen with normal video, it just happens here as
				// the motion of the chroma plane does not match the luma plane. 
				c->mb_decision = 2;
			}
			break;

		default:
			break;
		}

		// Some formats want stream headers to be separate. 
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		{
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	AVFrame* muxer::alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
	{
	/*	AVFrame *frame = avcodec_alloc_frame();
		if (!frame)
		{
			fprintf(stderr, "Error allocating an audio frame\n");
			exit(1);
		}

		frame->format = sample_fmt;
		frame->channel_layout = channel_layout;
		frame->sample_rate = sample_rate;
		frame->nb_samples = nb_samples;
		if (nb_samples)
		{
			int ret = av_frame_get_buffer(frame, 0);
			uint8_t* buf;
			int bytes = av_get_bytes_per_sample(sample_fmt);
			ret = avcodec_fill_audio_frame(frame, channel_layout, sample_fmt, buf , nb_samples * bytes * channel_layout, 1);
			if (ret < 0) 
			{
				fprintf(stderr, "Error allocating an audio buffer\n");
				exit(1);
			}
		}*/
		return 0; //frame;
	}

	void muxer::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
	{
		AVCodecContext *c;
		int nb_samples;
		int ret;
		AVDictionary *opt = NULL;
		c = ost->st->codec;
		/* open it */
		av_dict_copy(&opt, opt_arg, 0);
		ret = avcodec_open2(c, codec, &opt);
		av_dict_free(&opt);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open audio codec: %d\n", ret);
			exit(1);
		}


		// increment frequency by 110 Hz per second 
		if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
		{
			nb_samples = 10000;
		}
		else
		{
			nb_samples = c->frame_size;
		}

		ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
		ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout, c->sample_rate, nb_samples);

		// create resampler context 
		ost->swr_ctx = swr_alloc();
		if (!ost->swr_ctx)
		{
			fprintf(stderr, "Could not allocate resampler context\n");
			exit(1);
		}

		// set options 
		av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
		av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
	//	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
		av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
		av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
//		av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

		// initialize the resampling context 
		if ((ret = swr_init(ost->swr_ctx)) < 0)
		{
			fprintf(stderr, "Failed to initialize the resampling context\n");
			exit(1);
		}
	}

	// Prepare a 16 bit dummy audio frame of 'frame_size' samples and 'nb_channels' channels.
	AVFrame* muxer::get_audio_frame(OutputStream *ost)
	{

		// check if we want to generate more frames 
		//	AVRational r;
		//	r.den = 1;
		//	r.num = 1;
		//	if (av_compare_ts(ost->next_pts, ost->st->codec->time_base,	STREAM_DURATION, r) >= 0)
		//	{
		//		return NULL;
		//	}

		AVFrame *frame = ost->tmp_frame;
		int j, i;
		int16_t *q = (int16_t*)frame->data[0];
		for (j = 0; j < frame->nb_samples; j++) 
		{
			for (i = 0; i < ost->st->codec->channels; i++)
			{
				if (ost->audioLength-- > 0)
				{
					int16_t* d = (int16_t*) ost->data;
					*q++ = *d;
					ost->data += 2;
				}
				else
				{
					return NULL;
				}
			}
		}

		frame->pts = ost->next_pts;
		ost->next_pts  += frame->nb_samples;
		return frame;
	}

	// encode one audio frame and send it to the muxer
	// return 1 when encoding is finished, 0 otherwise
	int muxer::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
	{
		AVCodecContext *c;
		AVPacket pkt = { 0 }; // data and size must be 0;
		AVFrame *frame;
		int ret;
		int got_packet;
	//	int dst_nb_samples;
		av_init_packet(&pkt);
		c = ost->st->codec;
		frame = get_audio_frame(ost);
		if (frame)
		{
			// convert samples from native format to destination codec format, using the resampler
			// compute destination number of samples
			// dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples, c->sample_rate, c->sample_rate, AV_ROUND_UP);
			//assert(dst_nb_samples == frame->nb_samples);

			// when we pass a frame to the encoder, it may keep a reference to it internally;
			// make sure we do not overwrite it here
			ret = 0; // av_frame_make_writable(ost->frame);
			if (ret < 0)
			{
				exit(1);
			}

			// convert to destination format
			//			ret = swr_convert(ost->swr_ctx,	ost->frame->data, dst_nb_samples,	(const uint8_t **)frame->data, frame->nb_samples);
			if (ret < 0) 
			{
				fprintf(stderr, "Error while converting\n");
				exit(1);
			}
			frame = ost->frame;

			AVRational r;
			r.den=c->sample_rate;
			r.num=1;

			frame->pts = av_rescale_q(ost->samples_count, r, c->time_base);
		//	ost->samples_count += dst_nb_samples;
		}

		ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
		if (ret < 0) 
		{
			fprintf(stderr, "Error encoding audio frame: rc=%d\n", ret);
			exit(1);
		}

		if (got_packet)
		{
			ret = write_frame(oc, &c->time_base, ost->st, &pkt);
			if (ret < 0)
			{
				fprintf(stderr, "Error while writing audio frame: rc=%d\n", ret);
				exit(1);
			}
		}
		return (frame || got_packet) ? 0 : 1;
	}

	// video output 
	AVFrame* muxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
	{
		AVFrame* picture = av_frame_alloc();
		if (!picture)
		{
			return NULL;
		}

		picture->format = pix_fmt;
		picture->width  = width;
		picture->height = height;

		// allocate the buffers for the frame data 
		// int ret = av_frame_get_buffer(picture, 32);
		int ret = avpicture_alloc((AVPicture*) picture, pix_fmt, width, height);
		if (ret < 0) 
		{
			fprintf(stderr, "Could not allocate frame data.\n");
			exit(1);
		}
		return picture;
	}

	void muxer::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
	{
		int ret;
		AVCodecContext *c = ost->st->codec;
		AVDictionary *opt = NULL;
		av_dict_copy(&opt, opt_arg, 0);

		// open the codec 
		ret = avcodec_open2(c, codec, &opt);
		av_dict_free(&opt);
		if (ret < 0) 
		{
			fprintf(stderr, "Could not open video codec: %d\n", ret);
			exit(1);
		}

		// allocate and init a re-usable frame 
		ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
		if (!ost->frame)
		{
			fprintf(stderr, "Could not allocate video frame\n");
			exit(1);
		}

		// If the output format is not YUV420P, then a temporary YUV420P
		// picture is needed too. It is then converted to the required output format.
		ost->tmp_frame = NULL;
		if (c->pix_fmt != AV_PIX_FMT_YUV420P) 
		{
			ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
			if (!ost->tmp_frame)
			{
				fprintf(stderr, "Could not allocate temporary picture\n");
				exit(1);
			}
		}
	}

	// Prepare a dummy image. 
	void muxer::fill_yuv_image(AVFrame *pict, int frame_index,	 int width, int height)
	{
		int x, y, i, ret;

		// when we pass a frame to the encoder, it may keep a reference to it internally;
		// make sure we do not overwrite it here
		ret = 0; // av_frame_make_writable(pict);
		if (ret < 0)
		{
			exit(1);
		}

		i = frame_index;
		// Y 
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
			}
		}

		// Cb and Cr 
		for (y = 0; y < height / 2; y++)
		{
			for (x = 0; x < width / 2; x++)
			{
				pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
				pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
			}
		}


	}

	AVFrame* muxer::get_video_frame(OutputStream *ost)
	{
		//int frame_index = ost->next_pts;
		AVCodecContext* c = ost->st->codec;
		/* check if we want to generate more frames */

		//	AVRational r;
		//	r.den = 1;
		//	r.num = 1;
		//	if (av_compare_ts(ost->next_pts, ost->st->codec->time_base,	STREAM_DURATION, r) >= 0)
		//	{
		//		return NULL;
		//	}

		if (ost->sws_ctx == NULL)
		{
			// rgb ==> yuv
			int swsFlags = SWS_BICUBIC; //SWS_FAST_BILINEAR | SWS_ACCURATE_RND;
			ost->sws_ctx = sws_getContext(ost->width, ost->height, AV_PIX_FMT_RGBA, c->width, c->height, c->pix_fmt, swsFlags, NULL, NULL, NULL);
			if (ost->sws_ctx == NULL) 
			{
				fprintf(stderr,	"Could not initialize the conversion context\n");
				return NULL;
			}
		}

		AVPicture src;
		avpicture_fill(&src, ost->data, AV_PIX_FMT_RGBA, ost->width, ost->height);

		sws_scale(ost->sws_ctx, src.data, src.linesize, 0, ost->height, ost->frame->data, ost->frame->linesize);
		ost->frame->pts = ost->next_pts++;
		return ost->frame;
	}

	// encode one video frame and send it to the muxer
	// return 1 when encoding is finished, 0 otherwise
	int muxer::write_video_frame(AVFormatContext *oc, OutputStream *ost)
	{
		int ret;
		AVCodecContext *c;
		AVFrame *frame;
		int got_packet = 0;
		c = ost->st->codec;
		frame = get_video_frame(ost);
		if (oc->oformat->flags & AVFMT_RAWPICTURE) 
		{
			// a hack to avoid data copy with some raw video muxers 
			AVPacket pkt;
			av_init_packet(&pkt);
			if (!frame)
			{
				return 1;
			}

			pkt.flags        |= AV_PKT_FLAG_KEY;
			pkt.stream_index  = ost->st->index;
			pkt.data          = (uint8_t *)frame;
			pkt.size          = sizeof(AVPicture);
			pkt.pts = pkt.dts = frame->pts;
			packet_rescale_ts(&pkt, c->time_base, ost->st->time_base);
			ret = av_interleaved_write_frame(oc, &pkt);
		} 
		else
		{
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);

			// encode the image 
			ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
			if (ret < 0) 
			{
				//     fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
				exit(1);
			}

			if (got_packet)
			{
				ret = write_frame(oc, &c->time_base, ost->st, &pkt);
			}
			else
			{
				ret = 0;
			}
		}

		if (ret < 0)
		{
			fprintf(stderr, "Error while writing video frame: %d\n", ret);
			exit(1);
		}
		return (frame || got_packet) ? 0 : 1;
	}

	void muxer::close_stream(AVFormatContext *oc, OutputStream *ost)
	{
		avcodec_close(ost->st->codec);
	//	avcodec_free_frame(&ost->frame);
//		avcodec_free_frame(&ost->tmp_frame);
		sws_freeContext(ost->sws_ctx);
		swr_free(&ost->swr_ctx);
	}

	bool muxer::begin_session(const tu_string& filename, int formatNumber, uint8_t* audioPtr, int audioLength, int frame_rate, int width, int height)
	{
		av_register_all();

		if (width > 0 && height > 0 && frame_rate > 0)
		{
			m_formatNumber = formatNumber;
			m_audioPtr = audioPtr;
			m_audioLength = audioLength;
			m_frame_rate = frame_rate;
			m_width = width;
			m_height = height;

			memset(&m_video_st, 0, sizeof(OutputStream));
			memset(&m_audio_st, 0, sizeof(OutputStream));

			m_video_st.width = width;	// hack
			m_video_st.height = height;	// hack

			m_audio_st.audioLength = audioLength;
			m_audio_st.data = audioPtr;

			AVCodec* audio_codec = NULL;
			AVCodec* video_codec = NULL;
			AVDictionary *opt = NULL;
			int ret;

			//	av_dict_set(&opt, argv[2]+1, argv[3], 0);

//			AVCodec *codec;
//			codec = avcodec_find_encoder(AV_CODEC_ID_VP6A);

			// allocate the output media context 
			AVOutputFormat * outFmt = 0; //av_guess_format("vp6", NULL, NULL);
//AVFormatContext *m_oc = NULL;

			avformat_alloc_output_context2(&m_oc, outFmt, NULL, filename.c_str());
			if (!m_oc)
			{
				printf("Could not deduce output format from file extension: using MPEG.\n");
				avformat_alloc_output_context2(&m_oc, NULL, "mpeg", filename.c_str());
			}
			if (!m_oc)
			{
				return false;
			}

			m_fmt = m_oc->oformat;

			// Add the audio and video streams using the default format codecs and initialize the codecs.
			if (m_fmt->video_codec != AV_CODEC_ID_NONE) 
			{
				add_stream(&m_video_st, m_oc, &video_codec, m_fmt->video_codec);
				m_encode_video = 1;
			}

			if (m_fmt->audio_codec != AV_CODEC_ID_NONE)
			{
				add_stream(&m_audio_st, m_oc, &audio_codec, m_fmt->audio_codec);
				m_encode_audio = 1;
			}

			// Now that all the parameters are set, we can open the audio and video codecs and allocate the necessary encode buffers. 
			if (m_video_st.st != NULL)
			{
				open_video(m_oc, video_codec, &m_video_st, opt);
			}

		//	if (audioPtr && m_audio_st.st != NULL)
			{
				open_audio(m_oc, audio_codec, &m_audio_st, opt);
			}
			//av_dump_format(oc, 0, filename, 1);

			// open the output file, if needed 
			if (!(m_fmt->flags & AVFMT_NOFILE)) 
			{
				ret = avio_open(&m_oc->pb, filename, AVIO_FLAG_WRITE);
				if (ret < 0) 
				{
					fprintf(stderr, "Could not open %s, rc=%d\n", filename.c_str(), ret);
					return false;
				}
			}

			// Write the stream header, if any. 
			ret = avformat_write_header(m_oc, &opt);
			if (ret < 0)
			{
				fprintf(stderr, "Error occurred when opening output file: rc=%d\n", ret);
				return false;
			}
			return true;
		}
		return false;
	}


	int muxer::write_image(uint8_t* buf, int vBufferSize, uint8_t* aBuffer, int aBufferSize, bool flip)
	{
		uint8_t* flippedBuf = NULL;
		if (flip)
		{
			flippedBuf = (uint8_t*) malloc(vBufferSize);

			int pitch = m_width * 4;	// rgba
			uint8_t* dst = flippedBuf;
			uint8_t* src = buf + pitch * m_height;
			for (int y = 0; y < m_height; y++)
			{
				src -= pitch;
				memcpy(dst, src, pitch);
				dst += pitch;
			}
		}

		int retval = -1;

		m_encode_video = 1;
		m_encode_audio = m_audioPtr != NULL;

		m_video_st.data = flippedBuf ? flippedBuf : buf;	// set buffer

		while (m_encode_video || m_encode_audio)
		{
			if (m_encode_video)
			{
				m_encode_video = !write_video_frame(m_oc, &m_video_st);
				m_video_st.data = NULL;
				m_encode_video = 0;
			}

			if (m_encode_audio)
			{
				// -1 if ts_a is before ts_b, 1 if ts_a is after ts_b or 0 if they represent the same position
				int va = av_compare_ts(m_video_st.next_pts, m_video_st.st->codec->time_base,	m_audio_st.next_pts, m_audio_st.st->codec->time_base);
				m_encode_audio = !write_audio_frame(m_oc, &m_audio_st);
				if (va < 0)
				{
					m_encode_audio = 0;
				}
			}
		}
		free(flippedBuf);
		return 0;
	}



	void muxer::close_session()
	{

		// Write the trailer, if any. The trailer must be written before you
		// close the CodecContexts open when you wrote the header; otherwise
		// av_write_trailer() may try to use memory that was freed on av_codec_close().
		av_write_trailer(m_oc);

		// Close each codec. 
		if (m_video_st.st != NULL)
		{
			close_stream(m_oc, &m_video_st);
		}
		if (m_audio_st.st != NULL)
		{
			close_stream(m_oc, &m_audio_st);
		}
		if (!(m_fmt->flags & AVFMT_NOFILE))
		{
			// Close the output file.
//			avio_closep(&m_oc->pb);
		}

		// free the stream 
		avformat_free_context(m_oc);
	}


	bool muxer::open_stream(const char* c_url)
	{
		int ret = avformat_open_input(&m_FormatCtx, c_url, NULL, NULL);
		assert(ret == 0);

		ret = avformat_find_stream_info(m_FormatCtx, NULL);
		assert(ret == 0);

		// Find the best video stream

		AVCodec *pvCodec = NULL;
		m_video_index = av_find_best_stream(m_FormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pvCodec, 0);
		assert(m_video_index >= 0 && pvCodec != NULL);

		m_video_stream = m_FormatCtx->streams[m_video_index];

		// Get a pointer to the codec context for the video stream
		m_VCodecCtx = m_FormatCtx->streams[m_video_index]->codec;

		// init the video decoder
		ret = avcodec_open2(m_VCodecCtx, pvCodec, NULL);
		assert(ret == 0);

		AVCodec *paCodec = NULL;
		m_audio_index = av_find_best_stream(m_FormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &paCodec, 0);
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
			assert(pACodec != NULL);

			// Open codec
			assert(avcodec_open2(m_ACodecCtx, pACodec, NULL) >= 0);
		}
		

		int swsFlags = SWS_BICUBIC | SWS_ACCURATE_RND;
		m_convert_ctx = sws_getContext(m_VCodecCtx->width, m_VCodecCtx->height, m_VCodecCtx->pix_fmt,	m_VCodecCtx->width, m_VCodecCtx->height, AV_PIX_FMT_RGBA,	swsFlags, NULL, NULL, NULL);
		assert(m_convert_ctx);

		return true;
	}

		// it is running in decoder thread
	Uint8* muxer::decode_video(const AVPacket& pkt)
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


	bool muxer::decode_audio(const AVPacket& pkt, Sint16** data, int* size)
	{
		int got = 0;
		AVFrame* frame = av_frame_alloc();
		avcodec_decode_audio4(m_ACodecCtx, frame, &got,	&pkt);
		if (got)
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
//				sh->push_stream(m_openal_stream, frame->channels == 2 ? 1 : 0, (Uint8*) buf, size, m_ACodecCtx->sample_rate, frame->channels);
//				free(buf);
			}
			else
			{
//				sh->push_stream(m_openal_stream, m_ACodecCtx->sample_fmt, frame->data[0], frame->linesize[0], m_ACodecCtx->sample_rate, m_ACodecCtx->channels);
			}

			av_free(frame);
			return true;
		}

		av_free(frame);
		return false;
	}



	void muxer::clone()
	{
		bool rrc = open_stream("C:\\bakeinflash\\test.mp4");
		assert(rrc);

		int w = m_VCodecCtx->width;
		int h = m_VCodecCtx->height;
		int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 1);

		tu_string fi = "C:\\bakeinflash\\testx.flv";
		int fps=30;
		rrc = begin_session(fi, 0, 0, 0, fps, w, h);
		assert(rrc);

		AVPacket pkt;
		while (av_read_frame(m_FormatCtx, &pkt) == 0)
		{
			if (pkt.stream_index == m_video_index)
			{
				printf("video\n");

				Uint8* buf = decode_video(pkt);
				if (buf)
				{
					write_image(buf, size, NULL, 0, false);
					av_free(buf);
				}
			}
			else
			if (pkt.stream_index == m_audio_index)
			{
				printf("audio\n");

			//	Sint16* sample;
			//	int size;
			//	decode_audio(pkt, &sample, &size);

			}
			else
			{
				assert(0);
			}
		}

	}




#else

	muxer::muxer():
		m_formatNumber(0),
		m_audioPtr(NULL),
		m_audioLength(0),
		m_frame_rate(0),
		m_width(0),
		m_height(0)
	{
	}

	muxer::~muxer()
	{
	}

	void muxer::packet_rescale_ts(AVPacket *pkt, AVRational tb_src, AVRational tb_dst)
	{
	}


	int muxer::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
	{
		return -1;
	}

	void muxer::add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
	{
	}

	AVFrame* muxer::alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
	{
		return 0; //frame;
	}

	void muxer::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
	{
	}

	// Prepare a 16 bit dummy audio frame of 'frame_size' samples and 'nb_channels' channels.
	AVFrame* muxer::get_audio_frame(OutputStream *ost)
	{
		return 0;
	}

	// encode one audio frame and send it to the muxer
	// return 1 when encoding is finished, 0 otherwise
	int muxer::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
	{
		return 0;
	}

	// video output
//	AVFrame* muxer::alloc_picture(enum PixelFormat pix_fmt, int width, int height)
//	{
//		return 0;
//	}

	void muxer::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
	{
	}

	// Prepare a dummy image.
	void muxer::fill_yuv_image(AVFrame *pict, int frame_index,	 int width, int height)
	{
	}

	AVFrame* muxer::get_video_frame(OutputStream *ost)
	{
		return 0;
	}

	// encode one video frame and send it to the muxer
	// return 1 when encoding is finished, 0 otherwise
	int muxer::write_video_frame(AVFormatContext *oc, OutputStream *ost)
	{
		return 0;
	}

	void muxer::close_stream(AVFormatContext *oc, OutputStream *ost)
	{
	}

	bool muxer::begin_session(const tu_string& filename, int formatNumber, uint8_t* audioPtr, int audioLength, int frame_rate, int width, int height)
	{
		return false;
	}


	int muxer::write_image(uint8_t* buf, int vBufferSize, uint8_t* aBuffer, int aBufferSize, bool flip)
	{
		return 0;
	}

	void muxer::close_session()
	{
	}


#endif



}

#endif


//#endif

