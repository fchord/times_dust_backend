#pragma once
#include "encoder.h"

class CVideoEncoder : public CEncoder
{
public:
    CVideoEncoder(IListener *listener, std::string codec_name, int stream_index);
    ~CVideoEncoder();
    int SetParameter(
        int width,
        int height,
        int time_base,
        int frame_rate,
        int bit_rate = 1000000,
        int gop_size = 10,
        int max_b_frames = 1,
        std::string preset = "fast");
    std::vector<AVPixelFormat> GetSupportedPixelFormat();
    static int GetSupportedFormat(const std::string &encoder_name, std::vector<AVPixelFormat> &formats);

private:
    void set_codec_context(AVCodecContext *codec_ctx);

    int width_;
    int height_;
    int time_base_;
    int frame_rate_;
    int bit_rate_;
    int gop_size_;
    int max_b_frames_;
    std::string preset_;
};