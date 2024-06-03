#pragma once
#include <string>
#include <unordered_map>
#include "avmodule.h"
#include "listener.h"

typedef struct MuxerParameter
{
    AVCodecParameters *codec_par;
    AVCodecContext *codec_ctx;
    AVRational stream_time_base;
} MuxerParameter;

class CMuxer : public AvModule
{
public:
    CMuxer(IListener *listener);
    ~CMuxer();
    int Start(std::unordered_map<int, MuxerParameter> muxer_parameter, std::string out_url);
    int Stop();
    int Process(CPacketWrapper &packet);

private:
    IListener *listener_ = NULL;
    std::string out_url_;
    AVFormatContext *ofmt_ctx = NULL;
    AVOutputFormat *ofmt_ = NULL;
    AVStream *video_stream_ = NULL;
    AVStream *audio_stream_ = NULL;
    AVRational src_video_time_base_, src_audio_time_base_;

    std::unordered_map<int, MuxerParameter> muxer_parameter_;
    std::unordered_map<int, AVStream *> muxer_stream_;

    void create_av_stream(std::pair<const int, MuxerParameter> &para, AVMediaType type);
};