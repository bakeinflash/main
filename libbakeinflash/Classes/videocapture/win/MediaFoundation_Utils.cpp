#include <videocapture/win/MediaFoundation_Utils.h>

namespace ca {

  // Convert the MF format to one we can use.
  int media_foundation_video_format_to_capture_format(GUID guid) {

    if(IsEqualGUID(guid, MFVideoFormat_RGB24))     { return CA_RGB24;   }
    else if(IsEqualGUID(guid, MFVideoFormat_YUY2))   { return CA_YUV420P; } //vv   { return CA_YUYV422;   }
    else if(IsEqualGUID(guid, MFVideoFormat_I420)) { return CA_YUV420P; } 
    else if(IsEqualGUID(guid, MFVideoFormat_MJPG)) { return CA_MJPEG; } 
    else {
      return CA_NONE;
    }
  }
  
  // Convert a MF format to a string.
  #define MEDIAFOUNDATION_CHECK_VIDEOFORMAT(param, val) if (param == val) return #val

  std::string media_foundation_video_format_to_string(const GUID& guid) {
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MAJOR_TYPE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_SUBTYPE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_ALL_SAMPLES_INDEPENDENT);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_FIXED_SIZE_SAMPLES);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_COMPRESSED);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_SAMPLE_SIZE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_WRAPPED_TYPE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_NUM_CHANNELS);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_BITS_PER_SAMPLE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_CHANNEL_MASK);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_FOLDDOWN_MATRIX);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_WMADRC_PEAKREF);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_WMADRC_PEAKTARGET);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_WMADRC_AVGREF);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_WMADRC_AVGTARGET);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AAC_PAYLOAD_TYPE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_FRAME_SIZE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_FRAME_RATE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_FRAME_RATE_RANGE_MAX);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_FRAME_RATE_RANGE_MIN);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_PIXEL_ASPECT_RATIO);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DRM_FLAGS);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_PAD_CONTROL_FLAGS);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_SOURCE_CONTENT_HINT);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_VIDEO_CHROMA_SITING);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_INTERLACE_MODE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_TRANSFER_FUNCTION);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_VIDEO_PRIMARIES);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_YUV_MATRIX);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_VIDEO_LIGHTING);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_VIDEO_NOMINAL_RANGE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_GEOMETRIC_APERTURE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MINIMUM_DISPLAY_APERTURE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_PAN_SCAN_APERTURE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_PAN_SCAN_ENABLED);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AVG_BITRATE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AVG_BIT_ERROR_RATE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MAX_KEYFRAME_SPACING);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DEFAULT_STRIDE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_PALETTE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_USER_DATA);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_AM_FORMAT_TYPE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG_START_TIME_CODE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG2_PROFILE);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG2_LEVEL);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG2_FLAGS);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG_SEQUENCE_HEADER);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_AAUX_SRC_PACK_0);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_AAUX_CTRL_PACK_0);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_AAUX_SRC_PACK_1);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_AAUX_CTRL_PACK_1);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_VAUX_SRC_PACK);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_DV_VAUX_CTRL_PACK);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_ARBITRARY_HEADER);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_ARBITRARY_FORMAT);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_IMAGE_LOSS_TOLERANT); 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_ORIGINAL_4CC); 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);
    
    // Media types
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Audio);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Video);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Protected);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_SAMI);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Script);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Image);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_HTML);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_Binary);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFMediaType_FileTransfer);

    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_AI44);            //     FCC('AI44')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_ARGB32);          //     D3DFMT_A8R8G8B8 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_AYUV);            //     FCC('AYUV')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_DV25);            //     FCC('dv25')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_DV50);            //     FCC('dv50')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_DVH1);            //     FCC('dvh1')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_DVSD);            //     FCC('dvsd')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_DVSL);            //     FCC('dvsl')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_H264);            //     FCC('H264')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_I420);            //     FCC('I420')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_IYUV);            //     FCC('IYUV')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_M4S2);            //     FCC('M4S2')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MJPG);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MP43);            //     FCC('MP43')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MP4S);            //     FCC('MP4S')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MP4V);            //     FCC('MP4V')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MPG1);            //     FCC('MPG1')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MSS1);            //     FCC('MSS1')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_MSS2);            //     FCC('MSS2')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_NV11);            //     FCC('NV11')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_NV12);            //     FCC('NV12')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_P010);            //     FCC('P010')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_P016);            //     FCC('P016')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_P210);            //     FCC('P210')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_P216);            //     FCC('P216')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_RGB24);           //     D3DFMT_R8G8B8 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_RGB32);           //     D3DFMT_X8R8G8B8 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_RGB555);          //     D3DFMT_X1R5G5B5 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_RGB565);          //     D3DFMT_R5G6B5 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_RGB8);
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_UYVY);            //     FCC('UYVY')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_v210);            //     FCC('v210')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_v410);            //     FCC('v410')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_WMV1);            //     FCC('WMV1')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_WMV2);            //     FCC('WMV2')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_WMV3);            //     FCC('WMV3')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_WVC1);            //     FCC('WVC1')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y210);            //     FCC('Y210')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y216);            //     FCC('Y216')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y410);            //     FCC('Y410')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y416);            //     FCC('Y416')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y41P);            
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_Y41T);            
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_YUY2);            //     FCC('YUY2')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_YV12);            //     FCC('YV12')
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFVideoFormat_YVYU);

    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_PCM);              //     WAVE_FORMAT_PCM 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_Float);            //     WAVE_FORMAT_IEEE_FLOAT 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_DTS);              //     WAVE_FORMAT_DTS 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_Dolby_AC3_SPDIF);  //     WAVE_FORMAT_DOLBY_AC3_SPDIF 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_DRM);              //     WAVE_FORMAT_DRM 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_WMAudioV8);        //     WAVE_FORMAT_WMAUDIO2 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_WMAudioV9);        //     WAVE_FORMAT_WMAUDIO3 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_WMAudio_Lossless); //     WAVE_FORMAT_WMAUDIO_LOSSLESS 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_WMASPDIF);         //     WAVE_FORMAT_WMASPDIF 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_MSP1);             //     WAVE_FORMAT_WMAVOICE9 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_MP3);              //     WAVE_FORMAT_MPEGLAYER3 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_MPEG);             //     WAVE_FORMAT_MPEG 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_AAC);              //     WAVE_FORMAT_MPEG_HEAAC 
    MEDIAFOUNDATION_CHECK_VIDEOFORMAT(guid, MFAudioFormat_ADTS);             //     WAVE_FORMAT_MPEG_ADTS_AAC 
    return "UNKNOWN";
  }
} // namespace ca
