#pragma once

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

class CFrameWrapper
{
public:
	CFrameWrapper()
	{
		return;
	}
	CFrameWrapper(AVFrame *p)
	{
		av_frame = p;
		return;
	}
	~CFrameWrapper()
	{
		return;
	}
	AVFrame *av_frame;

private:
};