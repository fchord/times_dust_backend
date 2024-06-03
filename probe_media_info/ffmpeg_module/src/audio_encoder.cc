#include "audio_encoder.h"

CAudioEncoder::CAudioEncoder(IListener *listener, std::string codec_name, int stream_index) : CEncoder(listener, codec_name, stream_index)
{
    return;
}

CAudioEncoder::~CAudioEncoder()
{
    return;
}

int CAudioEncoder::SetParameter(
    AVRational time_base,
    int frame_size,
    int bit_rate,
    AVSampleFormat sample_fmt,
    int sample_rate,
    int channels,
    int channel_layout,
    int profile)
{
    time_base_ = time_base;
    frame_size_ = frame_size;
    bit_rate_ = bit_rate;
    sample_fmt_ = sample_fmt;
    sample_rate_ = sample_rate;
    channels_ = channels;
    channel_layout_ = channel_layout;
    profile_ = profile;
    return 0;
}

#if 0
std::vector<AVSampleFormat> CAudioEncoder::GetSupportedSampleFormat()
{
    std::vector<AVSampleFormat> formats;
    int i = 0;
    AVCodecContext *codec_ctx = CEncoder::GetCodecContext();
    while (codec_ctx && codec_ctx->codec && codec_ctx->codec->sample_fmts && -1 != codec_ctx->codec->sample_fmts[i])
    {
        formats.push_back(codec_ctx->codec->sample_fmts[i]);
        i++;
    }
    return formats;
}

std::vector<int> CAudioEncoder::GetSupportedSampleRate()
{
    std::vector<int> sample_rates;
    int i = 0;
    AVCodecContext *codec_ctx = CEncoder::GetCodecContext();
    while (codec_ctx && codec_ctx->codec && codec_ctx->codec->supported_samplerates && codec_ctx->codec->supported_samplerates[i])
    {
        sample_rates.push_back(codec_ctx->codec->supported_samplerates[i]);
        i++;
    }
    return sample_rates;
}

std::vector<uint64_t> CAudioEncoder::GetSupportedChannelLayout()
{
    std::vector<uint64_t> channel_layouts;
    int i = 0;
    AVCodecContext *codec_ctx = CEncoder::GetCodecContext();
    while (codec_ctx && codec_ctx->codec && codec_ctx->codec->channel_layouts && codec_ctx->codec->channel_layouts[i])
    {
        channel_layouts.push_back(codec_ctx->codec->channel_layouts[i]);
        i++;
    }
    return channel_layouts;
}
#endif

void CAudioEncoder::set_codec_context(AVCodecContext *codec_ctx)
{
    if (!codec_ctx)
    {
        return;
    }

    /* put sample parameters */
    codec_ctx->bit_rate = bit_rate_ /* 400000 */;

    /* frames per second */
    codec_ctx->time_base = time_base_;

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    // codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->sample_fmt = sample_fmt_;
    codec_ctx->sample_rate = sample_rate_;
    codec_ctx->channels = channels_;
    codec_ctx->channel_layout = channel_layout_;

    codec_ctx->frame_size = frame_size_ /* 1024 */;

    // if (codec_->id == AV_CODEC_ID_H264)
    // av_opt_set(codec_ctx->priv_data, "preset", preset_.c_str() /* "fast" */, 0);

    codec_ctx->profile = profile_;

    return;
}


int CAudioEncoder::GetSupportedFormat(const std::string &encoder_name, std::vector<AVSampleFormat> &formats, std::vector<int> &sample_rates, std::vector<uint64_t> &channel_layouts)
{
    int i = 0;    
    AVCodec *codec = avcodec_find_encoder_by_name(encoder_name.c_str());
    if (!codec)
    {
        //SPDLOG_WARN("Codec {} not found", encoder_name);
        return -1;
    }
    if(codec->type != AVMEDIA_TYPE_AUDIO) 
    {
        return -2;
    }
    i = 0;
    while (codec && codec->sample_fmts && -1 != codec->sample_fmts[i])
    {
        formats.push_back(codec->sample_fmts[i]);
        i++;
    } 
    i = 0;
    while (codec && codec->supported_samplerates && codec->supported_samplerates[i])
    {
        sample_rates.push_back(codec->supported_samplerates[i]);
        i++;
    }
    i = 0;
    while (codec && codec->channel_layouts && codec->channel_layouts[i])
    {
        channel_layouts.push_back(codec->channel_layouts[i]);
        i++;
    }
    return 0;
}