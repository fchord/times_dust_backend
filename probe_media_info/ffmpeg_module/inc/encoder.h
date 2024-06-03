#pragma once
#include <vector>
#include <string>
#include "listener.h"
#include "avmodule.h"

class CEncoder : public AvModule
{
public:
    CEncoder(IListener *listener, std::string codec_name, int stream_index);
    virtual ~CEncoder();
    int Start();
    int Stop();
    int SetReceiveAvModule(std::vector<AvModule *> next);
    virtual void set_codec_context(AVCodecContext *codec_ctx);
    AVCodecContext *GetCodecContext();
    int Process(CFrameWrapper &frame);

private:
    IListener *listener_ = NULL;
    std::string codec_name_;
    int stream_index_;
    std::vector<AvModule *> next_;
    AVCodec *codec_ = NULL;
    AVCodecContext *codec_ctx_ = NULL;
};