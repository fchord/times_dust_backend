#pragma once
#include "encoder.h"

class CAudioEncoder : public CEncoder
{
public:
    CAudioEncoder(IListener *listener, std::string codec_name, int stream_index);
    ~CAudioEncoder();
    int SetParameter(
        AVRational time_base,
        int frame_size = 1024,
        int bit_rate = 128000,
        AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16,
        int sample_rate = 44100,
        int channels = 2,
        int channel_layout = AV_CH_LAYOUT_STEREO,
        int profile = FF_PROFILE_AAC_MAIN);
#if 0        
    std::vector<AVSampleFormat> GetSupportedSampleFormat();
    std::vector<int> GetSupportedSampleRate();
    std::vector<uint64_t> GetSupportedChannelLayout();
#endif
    static int GetSupportedFormat(const std::string &encoder_name, std::vector<AVSampleFormat> &formats, std::vector<int> &sample_rates, std::vector<uint64_t> &channel_layouts);

private:
    void set_codec_context(AVCodecContext *codec_ctx);

    AVRational time_base_{AVRational{1, 90000}};
    int frame_size_{1024};
    int bit_rate_{128000};
    AVSampleFormat sample_fmt_{AV_SAMPLE_FMT_S16};
    int sample_rate_;
    int channels_;
    int channel_layout_;
    int profile_;
};