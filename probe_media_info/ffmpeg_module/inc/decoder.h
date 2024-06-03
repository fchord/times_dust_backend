#pragma once
#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include "avmodule.h"
#include "listener.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

class CDecoder : public AvModule
{
public:
	CDecoder(IListener *listener);
	~CDecoder();
	int Start(AVCodecContext *codec_ctx);
	int Stop();
	int SetReceiveAvModule(AvModule *receive_module);
	int Process(CPacketWrapper &packet);

private:
	IListener *listener_ = NULL;
	AVCodecContext *codec_ctx_ = NULL;
	AVCodec *codec_ = NULL;
	bool quit_ = false;

	const int deque_max_ = 15;
	const int deque_min_ = 10;

	deque<CFrameWrapper> dq_frame_;
	mutex mtx_dq_frame_;

	AvModule *receive_module_ = nullptr;
};