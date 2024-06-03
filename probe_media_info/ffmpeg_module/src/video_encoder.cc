#include "video_encoder.h"

CVideoEncoder::CVideoEncoder(IListener *listener, std::string codec_name, int stream_index) : CEncoder(listener, codec_name, stream_index)
{
    return;
}

CVideoEncoder::~CVideoEncoder()
{
    return;
}

int CVideoEncoder::SetParameter(
    int width,
    int height,
    int time_base,
    int frame_rate,
    int bit_rate,
    int gop_size,
    int max_b_frames,
    std::string preset)
{
    width_ = width;
    height_ = height;
    time_base_ = time_base;
    frame_rate_ = frame_rate;
    bit_rate_ = bit_rate;
    gop_size_ = gop_size;
    max_b_frames_ = max_b_frames;
    preset_ = preset;
    return 0;
}

void CVideoEncoder::set_codec_context(AVCodecContext *codec_ctx)
{
    if (!codec_ctx)
    {
        return;
    }

    /* put sample parameters */
    codec_ctx->bit_rate = bit_rate_ /* 400000 */;
    /* resolution must be a multiple of two */
    codec_ctx->width = width_;   // 838;
    codec_ctx->height = height_; // 472;
    /* frames per second */
    codec_ctx->time_base = (AVRational){1, time_base_ /* 90000 */};
    codec_ctx->framerate = (AVRational){frame_rate_ /* 25 */, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codec_ctx->gop_size = gop_size_ /* 10 */;
    codec_ctx->max_b_frames = max_b_frames_ /* 1 */;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // AV_PIX_FMT_YUV420P;

    // if (codec_->id == AV_CODEC_ID_H264)
    av_opt_set(codec_ctx->priv_data, "preset", preset_.c_str() /* "fast" */, 0);

    return;
}

std::vector<AVPixelFormat> CVideoEncoder::GetSupportedPixelFormat()
{
    std::vector<AVPixelFormat> formats;
    int i = 0;
    AVCodecContext *codec_ctx = CEncoder::GetCodecContext();
    while (codec_ctx && codec_ctx->codec && codec_ctx->codec->pix_fmts && -1 != codec_ctx->codec->pix_fmts[i])
    {
        formats.push_back(codec_ctx->codec->pix_fmts[i]);
        i++;
    }
    return formats;
}

int CVideoEncoder::GetSupportedFormat(const std::string &encoder_name, std::vector<AVPixelFormat> &formats)
{
    int i = 0;    
    AVCodec *codec = avcodec_find_encoder_by_name(encoder_name.c_str());
    if (!codec)
    {
        //SPDLOG_WARN("Codec {} not found", encoder_name);
        return -1;
    }
    if(codec->type != AVMEDIA_TYPE_VIDEO) 
    {
        return -2;
    }
    i = 0;
    while (codec && codec->pix_fmts && -1 != codec->pix_fmts[i])
    {
        formats.push_back(codec->pix_fmts[i]);
        i++;
    } 
    return 0;
}