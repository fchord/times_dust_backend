#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>
#include "avmodule.h"
#include "listener.h"
#include "decoder.h"

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

using namespace std;

constexpr auto MAXTHREADNUM = 4;

class CDemuxer : public AvModule
{
public:
	CDemuxer(IListener *listener);
	~CDemuxer();
	int Open(std::string media, int start_time, int duration, std::string pic_path, std::string pic_iframe_path, AVFormatContext **format_ctx);
	int Close();
	int SetReceiveAvModule(std::pair<int, AvModule *> video_idx_module, std::unordered_map<int, AvModule *> audio_idx_module);
	int SetReceiveAvModule_1(std::unordered_map<int, AvModule *> idx_module);

	int Process();

private:
	IListener *listener_ = NULL;

	std::string pic_path_, pic_iframe_path_;
	std::string media_;
	int start_time_, duration_;

	AVFormatContext *format_ctx_ = NULL;
	// AVCodecContext *codec_ctx_ = NULL;
	// AVCodec *codec_ = NULL;
	bool quit_ = false;
	// int open();
	// int close();
	// int start();
	// int stop();
	int64_t start_pts_ = 0, duration_pts_ = 0;

	// const int m_iDequeMax = 15;
	// const int m_iDequeMin = 10;

	// Decode
	std::thread *thread_decode_ = NULL;

	bool decode_finish_ = false;
	// queue<AVFrame *> m_qFrame;
	// deque<CFrameWrapper> m_dqFrame;
	// mutex m_mtxFrameQueue;

	// consume frame
	// std::thread *m_pThreadConsume = NULL;
	// int consume();

	// Encode
	/* 	AVCodecContext *pAVCtxJPEG[MAXTHREADNUM];
		AVCodec *m_pAVCodec[MAXTHREADNUM];
		int iEncFinish[MAXTHREADNUM] = { 0 };
		thread::id thread_encoder[MAXTHREADNUM] = { thread::id() };
		std::thread *m_pThreadEncode[MAXTHREADNUM] = { 0 };
		int encode();
		deque<CPacketWrapper> m_dqPacket;
		mutex m_mtxPacketQueue;
		condition_variable m_condition; */

	// Output
	/* 	int m_iPicNum = 0;
		std::thread *m_pThreadOutput = NULL;
		int output();
		int OutputJPEG(AVPacket *pPacketJPEG, enum AVPictureType pict_type, std::string sPath, std::string sIPath); */

	int video_stream_ = -1;

	std::pair<int, AvModule *> video_idx_module_;
	std::unordered_map<int, AvModule *> audio_idx_module_;
	std::unordered_map<int, AvModule *> idx_module_;
};