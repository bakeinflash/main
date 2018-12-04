// bakeinflash_video_impl.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef BAKEINFLASH_MUXER_H
#define BAKEINFLASH_MUXER_H

#include "bakeinflash_impl.h"
#include "bakeinflash_as_classes/as_netstream.h"

#if TU_CONFIG_LINK_TO_FFMPEG == 1

// a wrapper around a single output AVStream
typedef struct OutputStream
{
	AVStream *st;
	// pts of the next frame that will be generated 
	int64_t next_pts;
	int samples_count;
	AVFrame *frame;
	AVFrame *tmp_frame;
	float t;
	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;

	int formatNumber;
	uint8_t* data;
	int audioLength;
	int frame_rate;
	int width;
	int height;
} OutputStream;

namespace bakeinflash
{

	struct muxer : public ref_counted
	{
		muxer();
		virtual ~muxer();

		bool begin_session(const tu_string& fi, int formatNumber, uint8_t* audioPtr, int audioLength, int frameRate, int width, int height);
		void close_session();
		int	write_image(uint8_t* vBuffer, int vBufferSize, uint8_t* aBuffer, int aBufferSize, bool flip);
		void clone();
		bool open_stream(const char* c_url);
		Uint8* decode_video(const AVPacket& pkt);
		bool decode_audio(const AVPacket& pkt, Sint16** data, int* size);

	private:

		int m_formatNumber;
		uint8_t* m_audioPtr;
		int m_audioLength;
		int m_frame_rate;
		int m_width;
		int m_height;

		OutputStream m_video_st;
		OutputStream m_audio_st;
		AVFormatContext* m_oc;
		AVOutputFormat* m_fmt;
		int m_encode_video;
		int m_encode_audio;

		AVFormatContext* m_FormatCtx;

		// video
		AVCodecContext* m_VCodecCtx;
		AVCodecContext* m_aVCodecCtx;	// alpha stream, for embedded
		AVStream* m_video_stream;

		// audio
		AVCodecContext *m_ACodecCtx;
		AVStream* m_audio_stream;
		int m_video_index;
		int m_audio_index;

		SwsContext* m_convert_ctx;

		void packet_rescale_ts(AVPacket *pkt, AVRational tb_src, AVRational tb_dst);
		int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
		void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
		AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
		void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
		AVFrame* get_audio_frame(OutputStream *ost);
		int write_audio_frame(AVFormatContext *oc, OutputStream *ost);
		AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
		void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
		void fill_yuv_image(AVFrame *pict, int frame_index,	 int width, int height);
		AVFrame* get_video_frame(OutputStream *ost);
		int write_video_frame(AVFormatContext *oc, OutputStream *ost);
		void close_stream(AVFormatContext *oc, OutputStream *ost);
		bool init_session(int formatNumber, uint8_t* audioPtr, int audioLength, int frame_rate, int width, int height);

	};

}

// end namespace bakeinflash

#endif

#endif // BAKEINFLASH_MUXER_H
