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

class CPacketWrapper
{
public:
	CPacketWrapper()
	{
		return;
	}
	CPacketWrapper(AVPacket *p)
	{
		av_packet = p;
		return;
	}
	CPacketWrapper(AVPacket *p, AVPictureType t, AVFrame *pF)
	{
		av_packet = p;
		type = t;
		av_frame = pF;
		return;
	}
	~CPacketWrapper()
	{
		return;
	}
	AVPacket *av_packet;
	AVPictureType type;
	AVFrame *av_frame;
};