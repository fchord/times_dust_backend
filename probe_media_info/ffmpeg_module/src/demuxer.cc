#include <spdlog/spdlog.h>
#include "demuxer.h"

// #define FFMPEG_LOG

bool less_pts(const CFrameWrapper &p1, const CFrameWrapper &p2)
{
	return p1.av_frame->pts < p2.av_frame->pts;
}

bool less_pts_pkg(const CPacketWrapper &p1, const CPacketWrapper &p2)
{
	return p1.av_packet->pts < p2.av_packet->pts;
}

enum AVPixelFormat pix_fmt_map(enum AVPixelFormat input_fmt)
{
	enum AVPixelFormat output_fmt = AV_PIX_FMT_YUV420P;
	switch (input_fmt)
	{
	case AV_PIX_FMT_YUV411P:
	case AV_PIX_FMT_UYYVYY411:
		output_fmt = AV_PIX_FMT_YUV411P;
		break;
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVA420P:
	case AV_PIX_FMT_YUV420P16LE:
	case AV_PIX_FMT_YUV420P16BE:
	case AV_PIX_FMT_YUV420P9BE:
	case AV_PIX_FMT_YUV420P9LE:
	case AV_PIX_FMT_YUV420P10BE:
	case AV_PIX_FMT_YUV420P10LE:
	case AV_PIX_FMT_YUVA420P9BE:
	case AV_PIX_FMT_YUVA420P9LE:
	case AV_PIX_FMT_YUVA420P10BE:
	case AV_PIX_FMT_YUVA420P10LE:
	case AV_PIX_FMT_YUVA420P16BE:
	case AV_PIX_FMT_YUVA420P16LE:
	case AV_PIX_FMT_YUV420P12BE:
	case AV_PIX_FMT_YUV420P12LE:
	case AV_PIX_FMT_YUV420P14BE:
	case AV_PIX_FMT_YUV420P14LE:
#if 0
	case AV_PIX_FMT_YUV420P9:
	case AV_PIX_FMT_YUV420P10:
	case AV_PIX_FMT_YUV420P12:
	case AV_PIX_FMT_YUV420P14:
	case AV_PIX_FMT_YUV420P16:
	case AV_PIX_FMT_YUVA420P9:
	case AV_PIX_FMT_YUVA420P10:
	case AV_PIX_FMT_YUVA420P16:
#endif
		output_fmt = AV_PIX_FMT_YUV420P;
		break;
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_YUV422P:
	case AV_PIX_FMT_UYVY422:
	case AV_PIX_FMT_YUV422P16LE:
	case AV_PIX_FMT_YUV422P16BE:
	case AV_PIX_FMT_YUV422P10BE:
	case AV_PIX_FMT_YUV422P10LE:
	case AV_PIX_FMT_YUV422P9BE:
	case AV_PIX_FMT_YUV422P9LE:
	case AV_PIX_FMT_YUVA422P:
	case AV_PIX_FMT_YUVA422P9BE:
	case AV_PIX_FMT_YUVA422P9LE:
	case AV_PIX_FMT_YUVA422P10BE:
	case AV_PIX_FMT_YUVA422P10LE:
	case AV_PIX_FMT_YUVA422P16BE:
	case AV_PIX_FMT_YUVA422P16LE:
	case AV_PIX_FMT_YVYU422:
	case AV_PIX_FMT_YUV422P12BE:
	case AV_PIX_FMT_YUV422P12LE:
	case AV_PIX_FMT_YUV422P14BE:
	case AV_PIX_FMT_YUV422P14LE:
#if 0
	case AV_PIX_FMT_YUV422P9:
	case AV_PIX_FMT_YUV422P10:
	case AV_PIX_FMT_YUV422P12:
	case AV_PIX_FMT_YUV422P14:
	case AV_PIX_FMT_YUV422P16:
	case AV_PIX_FMT_YUVA422P9:
	case AV_PIX_FMT_YUVA422P10:
	case AV_PIX_FMT_YUVA422P16:
#endif
		output_fmt = AV_PIX_FMT_YUV422P;
		break;
	case AV_PIX_FMT_YUV440P:
	case AV_PIX_FMT_YUV440P10LE:
	case AV_PIX_FMT_YUV440P10BE:
	case AV_PIX_FMT_YUV440P12LE:
	case AV_PIX_FMT_YUV440P12BE:
#if 0
	case AV_PIX_FMT_YUV440P10:
	case AV_PIX_FMT_YUV440P12:
#endif
		output_fmt = AV_PIX_FMT_YUV440P;
		break;
	case AV_PIX_FMT_YUV444P:
	case AV_PIX_FMT_YUV444P16LE:
	case AV_PIX_FMT_YUV444P16BE:
	case AV_PIX_FMT_RGB444LE:
	case AV_PIX_FMT_RGB444BE:
	case AV_PIX_FMT_BGR444LE:
	case AV_PIX_FMT_BGR444BE:
	case AV_PIX_FMT_YUV444P9BE:
	case AV_PIX_FMT_YUV444P9LE:
	case AV_PIX_FMT_YUV444P10BE:
	case AV_PIX_FMT_YUV444P10LE:
	case AV_PIX_FMT_YUVA444P:
	case AV_PIX_FMT_YUVA444P9BE:
	case AV_PIX_FMT_YUVA444P9LE:
	case AV_PIX_FMT_YUVA444P10BE:
	case AV_PIX_FMT_YUVA444P10LE:
	case AV_PIX_FMT_YUVA444P16BE:
	case AV_PIX_FMT_YUVA444P16LE:
	case AV_PIX_FMT_YUV444P12BE:
	case AV_PIX_FMT_YUV444P12LE:
	case AV_PIX_FMT_YUV444P14BE:
	case AV_PIX_FMT_YUV444P14LE:
#if 0			
	case AV_PIX_FMT_RGB444:
	case AV_PIX_FMT_BGR444:
	case AV_PIX_FMT_YUV444P9:
	case AV_PIX_FMT_YUV444P10:
	case AV_PIX_FMT_YUV444P12:
	case AV_PIX_FMT_YUV444P14:
	case AV_PIX_FMT_YUV444P16:
	case AV_PIX_FMT_YUVA444P9:
	case AV_PIX_FMT_YUVA444P10:
	case AV_PIX_FMT_YUVA444P16:
#endif
		output_fmt = AV_PIX_FMT_YUV444P;
		break;
	default:
		printf("[%s, %d] Not support pix_fmt: %d\n", __FUNCTION__, __LINE__, input_fmt);
		output_fmt = AV_PIX_FMT_YUV420P;
		break;
	}
	return output_fmt;
}
#if 0
void ffmpegUtilsLogcallback(void *avcl, int level, const char *fmt, va_list vl)
{
#ifdef FFMPEG_LOG
	char s[1024];
	memset(s, 0, 1024);
	vsnprintf(s, 1024, fmt, vl);
	if (level <= av_log_get_flags())
		switch (level)
		{
		case -8:
			printf("AV_LOG_QUIET. ");
			vprintf(fmt, vl);
			break;
		case 0:
			printf("AV_LOG_PANIC. ");
			vprintf(fmt, vl);
			break;
		case 8:
			printf("AV_LOG_FATAL. ");
			vprintf(fmt, vl);
			break;
		case 16:
			printf("AV_LOG_ERROR. ");
			vprintf(fmt, vl);
			break;
		case 24:
			printf("AV_LOG_WARNING. ");
			vprintf(fmt, vl);
			break;
		case 32:
			printf("AV_LOG_INFO. ");
			vprintf(fmt, vl);
			break;
		case 40:
			printf("AV_LOG_VERBOSE. ");
			vprintf(fmt, vl);
			break;
		case 48:
			printf("AV_LOG_DEBUG. ");
			vprintf(fmt, vl);
			break;
		case 56:
			printf("AV_LOG_TRACE. ");
			vprintf(fmt, vl);
			break;
		default:
			break;
		}
	SPDLOG_DEBUG("level[{}]. {}", level, s);
#endif
	return;
}
#endif

CDemuxer::CDemuxer(IListener *listener)
{
	listener_ = listener;
	return;
}

CDemuxer::~CDemuxer()
{
	listener_ = NULL;
	return;
}

int CDemuxer::Open(
	string media,
	int start_time,
	int duration,
	string pic_path,
	string pic_iframe_path,
	AVFormatContext **format_ctx)
{
	media_ = media;
	start_time_ = start_time;
	duration_ = duration;
	pic_path_ = pic_path;
	pic_iframe_path_ = pic_iframe_path;
	SPDLOG_DEBUG("{}, start time: {}, duration: {}", media_, start_time_, duration_);

	int ret, i;

	quit_ = false;
	SPDLOG_DEBUG("avformat_open_input start");

	ret = avformat_open_input(&format_ctx_, media_.c_str(), NULL, NULL);
	if (ret < 0)
	{
		SPDLOG_WARN("avformat_open_input: {}", ret);
		return ret;
	}
	SPDLOG_DEBUG("avformat_open_input finish");

	if (format_ctx)
	{
		*format_ctx = format_ctx_;
	}

	ret = avformat_find_stream_info(format_ctx_, NULL);
	if (ret < 0)
	{
		SPDLOG_WARN("avformat_find_stream_info: {}", ret);
		return ret;
	}
	SPDLOG_DEBUG("avformat_find_stream_info finish");

	av_dump_format(format_ctx_, 0, media_.c_str(), 0);
	SPDLOG_DEBUG("av_dump_format finish");

	/* for (i = 0; i < format_ctx_->nb_streams; i++)
	{
		if (format_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_ = i;
			SPDLOG_INFO("video_stream_: {}", video_stream_);
			break;
		}
	} */

	// thread_decode_ = new std::thread(std::mem_fn(&CDemuxer::Process), this);
	// m_pThreadConsume = new std::thread(std::mem_fn(&CDemuxer::consume), this);
	/*
	for (i = 0; i < MAXTHREADNUM; i++)
	{
		m_pThreadEncode[i] = new std::thread(std::mem_fn(&CDemuxer::encode), this);
		thread_encoder[i] = m_pThreadEncode[i]->get_id();
	}
	m_condition.notify_all();

	m_pThreadOutput = new std::thread(std::mem_fn(&CDemuxer::output), this);
	*/
	return ret;
}

int CDemuxer::Close()
{
	int i;
	quit_ = true;
	/*
	if (m_pThreadOutput && m_pThreadOutput->joinable()) {
		m_pThreadOutput->join();
		m_pThreadOutput = NULL;
	}
	for (i = 0; i < MAXTHREADNUM; i++)
	{
		if (m_pThreadEncode[i] && m_pThreadEncode[i]->joinable()) {

			m_pThreadEncode[i]->join();
			m_pThreadEncode[i] = NULL;
			thread_encoder[i] = thread::id();
		}
	}
	*/
	/* if (thread_decode_ && thread_decode_->joinable()) {
		thread_decode_->join();
		thread_decode_ = NULL;
	} */
	/* 	if (m_pThreadConsume && m_pThreadConsume->joinable()) {
			m_pThreadConsume->join();
			m_pThreadConsume = NULL;
		} */
	if (format_ctx_)
	{
		avformat_close_input(&format_ctx_);
		format_ctx_ = NULL;
	}
	// m_dqFrame.clear();
	// m_dqPacket.clear();
	// decode_finish_ = false;
	/* for (i = 0; i < MAXTHREADNUM; i++)
		iEncFinish[i] = 0;
	m_iPicNum = 0; */
	return 0;
}

/* int CDemuxer::start()
{
	return 0;
}

int CDemuxer::stop()
{
	return 0;
} */

int CDemuxer::Process()
{
	int i, j, ret, count, flag = 0;
	bool in_range, get_first_pts = false;
	int64_t first_pts = -1, media_duration;
	static int skipped_frame = 0;
	char err[256];

	/* 	codec_ctx_ = format_ctx_->streams[video_stream_]->codec;
		codec_ctx_->thread_count = 4;

		codec_ = avcodec_find_decoder(codec_ctx_->codec_id);

		if (avcodec_open2(codec_ctx_, codec_, NULL) < 0)
		{
			SPDLOG_WARN("avcodec_open2 failed.");
			return NULL;
		}
		SPDLOG_INFO("codec_ctx_ delay: {}", codec_ctx_->delay); */

	AVPacket *packet = NULL, *packet_null = NULL;
	// AVFrame *pFrame = NULL, *pFrameFiltered = NULL;
	// deque<CFrameWrapper>::iterator iter;
	int frame_finished;
	// AVRational rational;
	count = 0;

	// rational = format_ctx_->streams[video_stream_]->time_base;
	media_duration = format_ctx_->duration;
	// LzLogInfo << "media_duration: " << media_duration;

	packet = av_packet_alloc();
	av_init_packet(packet);
	ret = av_read_frame(format_ctx_, packet);
	// LzLogInfo << "av_read_frame: " << ret;
	if (ret < 0)
	{
		av_packet_free(&packet);
		packet = NULL;
	}
	// listener_->OnFrameReport(NULL);
	av_count_++;

	CPacketWrapper pw(packet);
	/* 	if(packet) {
			if (packet->stream_index == video_idx_module_.first && video_idx_module_.second) {
				video_idx_module_.second->Process(pw);
			} else {
				for(auto& a : audio_idx_module_) {
					if(packet->stream_index == a.first) {
						a.second->Process(pw);
						break;
					}
				}
			}
		} else {
			// Send null packet for flushing frames in decoders.
			if(video_idx_module_.second) {
				video_idx_module_.second->Process(pw);
			}
			for(auto& a : audio_idx_module_) {
				a.second->Process(pw);
			}
		} */
	if (packet)
	{
		for (auto &a : idx_module_)
		{
			if (packet->stream_index == a.first && a.second)
			{
				a.second->Process(pw);
				break;
			}
		}
	}
	else
	{
		// Send null packet for flushing frames in decoders.
		for (auto &a : idx_module_)
		{
			if (a.second)
			{
				a.second->Process(pw);
			}
		}
	}

	av_packet_free(&packet);
	return 0;

#if 0
	while (1) {
		packet = av_packet_alloc();
		av_init_packet(packet);
		ret = av_read_frame(format_ctx_, packet);
		if (ret < 0) {
			av_packet_free(&packet);
			break;
		}
		if (packet->stream_index == video_stream_)
		{
			if (!get_first_pts) {
				first_pts = packet->pts;
				SPDLOG_INFO("first_pts: {}", first_pts);
				if (start_time_ > 0) {
					start_pts_ = first_pts + start_time_ * rational.den / rational.num;
					ret = av_seek_frame(format_ctx_, video_stream_, start_pts_, AVSEEK_FLAG_BACKWARD);
					SPDLOG_INFO("av_seek_frame ret:{}", ret);
				}
				else {
					start_pts_ = first_pts;
				}
				SPDLOG_INFO("start_pts_: {}", start_pts_);

				if (duration_ > 0) {
					duration_pts_ = duration_ * rational.den / rational.num;
					if (duration_pts_ > media_duration - start_pts_)
						duration_pts_ = media_duration - start_pts_;
				}
				else {
					// duration_pts_ == 0 means unlimited duration.
					duration_pts_ = media_duration - start_pts_;
				}
				SPDLOG_INFO("duration_pts_: {}", duration_pts_);
				get_first_pts = true;

				// start_time_ > 0 meas ffmpeg has seeked. The first packet is needed to drop, and demux again.
				if (start_time_ > 0) {
					av_packet_free(&packet);
					continue;
				}
			}

			ret = avcodec_send_packet(codec_ctx_, packet);
			SPDLOG_INFO("avcodec_send_packet: {}", ret);
			if (ret < 0) {
				//LzLogFail << "avcodec_send_packet failed: " << ret;
				memset((char *)&err, 0, sizeof(err));
				av_strerror(ret, (char *)&err, sizeof(err));
				SPDLOG_WARN("avcodec_send_packet failed: {}", err);
			}
			do {
				if (NULL == pFrame)
					pFrame = av_frame_alloc();

				ret = avcodec_receive_frame(codec_ctx_, pFrame);
				SPDLOG_INFO("avcodec_receive_frame: {}", ret);
				if (ret < 0) {
					//LzLogFail << "avcodec_receive_frame failed: " << ret;
					memset((char *)&err, 0, sizeof(err));
					av_strerror(ret, (char *)&err, sizeof(err));
					SPDLOG_WARN("avcodec_receive_frame failed: {}", err);
					// skipped_frame++;
				}
				else
				{
					if (pFrame->pts >= start_pts_) {
						if (duration_pts_ > 0 && pFrame->pts <= start_pts_ + duration_pts_)
							in_range = true;
						else if (duration_pts_ <= 0)
							in_range = true;
						else
							in_range = false;
					}
					else
						in_range = false;

					if (in_range) {
						SPDLOG_INFO("demux count: {}, pts: {}, pkt_pts: {}, picture type: {}", ++count, pFrame->pts, pFrame->pkt_pts, pFrame->pict_type);
						while (1) {
							if (quit_)
								break;
							m_mtxFrameQueue.lock();
							if (m_dqFrame.size() > m_iDequeMax) {
								m_mtxFrameQueue.unlock();
								std::this_thread::sleep_for(std::chrono::milliseconds(10));
								continue;
							}
							m_dqFrame.push_back(CFrameWrapper(pFrame));
							sort(m_dqFrame.begin(), m_dqFrame.end(), less_pts);
							/*
							for (iter = m_dqFrame.begin(), i = 0; iter != m_dqFrame.end(); iter++, i++) {
								SPDLOG_INFO("{}: {}", i, iter->pFrame->pts);
							}*/
							m_mtxFrameQueue.unlock();
							break;
						}
						pFrame = NULL;
						

					}
					else {
						// The frame decoded not in range.
						av_frame_free(&pFrame);
						pFrame = NULL;
					}
				}

				if (quit_)
					break;
			} while (ret >= 0);
		}
		av_packet_free(&packet);
		if (quit_)
			break;
	}

	if (!quit_)
	{
		// Flush frames buffered in decoder

		packet_null = av_packet_alloc();
		av_init_packet(packet_null);
		packet_null->data = NULL;
		packet_null->size = 0;
		// Send a null packet signing end of the stream to flush frames.
		ret = avcodec_send_packet(codec_ctx_, packet_null);
		if (ret < 0) {
			SPDLOG_WARN("avcodec_send_packet failed: {}", ret);
		}
		do
		{
			if (NULL == pFrame)
				pFrame = av_frame_alloc();
			ret = avcodec_receive_frame(codec_ctx_, pFrame); // AVERROR(EAGAIN); AVERROR(EINVAL);
			if (ret < 0) {
				SPDLOG_WARN("avcodec_receive_frame failed: {}", ret);
				break;
			}
			else {
				if (pFrame->pts >= start_pts_) {
					if (duration_pts_ > 0 && pFrame->pts <= start_pts_ + duration_pts_)
						in_range = true;
					else if (duration_pts_ <= 0)
						in_range = true;
					else
						in_range = false;
				}
				else
					in_range = false;

				if (in_range) {
					SPDLOG_INFO("demux count: {}, pts: {}, pkt_pts: {}", ++count, pFrame->pts, pFrame->pkt_pts);
					while (1) {
						m_mtxFrameQueue.lock();
						if (m_dqFrame.size() > m_iDequeMax) {
							m_mtxFrameQueue.unlock();
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							continue;
						}
						m_dqFrame.push_back(CFrameWrapper(pFrame));
						sort(m_dqFrame.begin(), m_dqFrame.end(), less_pts);
						/*
						for (iter = m_dqFrame.begin(), j = 0; iter != m_dqFrame.end(); iter++, j++) {
							SPDLOG_WARN("{}: {}", j, iter->pFrame->pts);
						}*/
						m_mtxFrameQueue.unlock();
						break;
					}
					pFrame = NULL;
				}
			}
		} while (1);
		av_packet_free(&packet_null);
		packet_null = NULL;
	}

	decode_finish_ = true;
	SPDLOG_INFO("decode decode_finish_: {}", decode_finish_);

	avcodec_close(codec_ctx_);

	SPDLOG_INFO("decode finish.");
	return 0;
#endif
}

#if 0
int CDemuxer::consume()
{
	AVFrame *pFrame = NULL;
	CFrameWrapper node = NULL;

	while (1)
	{
		if (quit_) {
			while (!m_dqFrame.empty()) {
				m_mtxFrameQueue.lock();
				node = m_dqFrame.front();
				m_dqFrame.pop_front();
				pFrame = node.pFrame;
				av_frame_free(&pFrame);
				m_mtxFrameQueue.unlock();
			}
			break;
		}
		if (m_dqFrame.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		m_mtxFrameQueue.lock();
		node = m_dqFrame.front();
		m_dqFrame.pop_front();
		pFrame = node.pFrame;
		if (listener_)
			listener_->OnFrameReport(pFrame);
		//av_frame_free(&pFrame);
		m_mtxFrameQueue.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return 0;
}
#endif

#if 0
int CDemuxer::encode()
{
	int threadId, got_JPEG_output, ret, count = 0;
	AVCodecID codec_id = AV_CODEC_ID_MJPEG;
	bool bRet;
	enum AVPictureType pict_type;
	long long enc_time = 0;
	AVRational r1;
	AVPacket *pPacketJPEG;
	AVFrame *pFrame = NULL;
	CFrameWrapper node = NULL;
	thread::id encTid;
	mutex m;

	{
		unique_lock<mutex> lock(m);
		m_condition.wait(lock);
	}

	encTid = std::this_thread::get_id();
	for (threadId = 0; threadId < MAXTHREADNUM; threadId++)
	{
		if (m_pThreadEncode[threadId] && encTid == m_pThreadEncode[threadId]->get_id())
			break;
	}
	if (MAXTHREADNUM == threadId)
		return 0;

	while (1)
	{
		m_mtxFrameQueue.lock();
		bRet = m_dqFrame.empty();
		m_mtxFrameQueue.unlock();
		if (false == bRet)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	pAVCtxJPEG[threadId] = avcodec_alloc_context3(NULL);
	//pf->pAVCtxJPEG[threadId]->thread_count = 4;
	switch (pix_fmt_map(codec_ctx_->pix_fmt))
	{
	case AV_PIX_FMT_YUV411P:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ411P;
		break;
	case AV_PIX_FMT_YUV420P:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ420P;
		break;
	case AV_PIX_FMT_YUV422P:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ422P;
		break;
	case AV_PIX_FMT_YUV440P:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ440P;
		break;
	case AV_PIX_FMT_YUV444P:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ444P;
		break;
	default:
		pAVCtxJPEG[threadId]->pix_fmt = AV_PIX_FMT_YUVJ420P;
		break;
	}
	pAVCtxJPEG[threadId]->codec_type = AVMEDIA_TYPE_VIDEO;
	pAVCtxJPEG[threadId]->width = codec_ctx_->width;
	pAVCtxJPEG[threadId]->height = codec_ctx_->height;
	r1.den = 1;
	r1.num = 25;
	pAVCtxJPEG[threadId]->time_base = r1;
	pAVCtxJPEG[threadId]->bit_rate = 400000;
	m_pAVCodec[threadId] = avcodec_find_encoder(codec_id);
	if (avcodec_open2(pAVCtxJPEG[threadId], m_pAVCodec[threadId], NULL) < 0)
	{
		return 0;
	}
	while (1)
	{
		if (m_dqFrame.empty() && decode_finish_) {
			break;
		}
		do {
			if (quit_)
				break;
			m_mtxFrameQueue.lock();
			if (m_dqFrame.empty()) {
				m_mtxFrameQueue.unlock();
				if (decode_finish_) {
					pFrame = NULL;
					break;
				}
				else {
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
			}
			if (!decode_finish_ && m_dqFrame.size() < m_iDequeMin) {
				m_mtxFrameQueue.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			node = m_dqFrame.front();
			m_dqFrame.pop_front();
			pFrame = node.pFrame;
			SPDLOG_INFO("encode get pFrame: {}", pFrame->pts);
			m_mtxFrameQueue.unlock();
			break;
		} while (1);

		if (quit_)
			break;

		if (NULL == pFrame)
			break;
		pPacketJPEG = av_packet_alloc();
		av_init_packet(pPacketJPEG);
		pPacketJPEG->data = NULL; // packet data will be allocated by the encoder
		pPacketJPEG->size = 0;

		ret = avcodec_send_frame(pAVCtxJPEG[threadId], pFrame); // AVERROR_EOF;
		if (ret < 0) {
			SPDLOG_WARN("avcodec_send_frame failed: {}", ret);
		}
		ret = avcodec_receive_packet(pAVCtxJPEG[threadId], pPacketJPEG);
		if (ret < 0) {
			SPDLOG_WARN("avcodec_receive_packet failed: {}", ret);
			av_packet_free(&pPacketJPEG);
			av_frame_free(&pFrame);
		}
		else {
			pPacketJPEG->pts = pFrame->pts;
			SPDLOG_INFO("encode ret: {}, pts: {}", ret, pPacketJPEG->pts);
			do {
				m_mtxPacketQueue.lock();
				if (m_dqPacket.size() >= m_iDequeMax)
				{
					m_mtxPacketQueue.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				else {
					m_dqPacket.push_back(CPacketWrapper(pPacketJPEG, pFrame->pict_type, pFrame));
					pPacketJPEG = NULL;
					m_mtxPacketQueue.unlock();
					break;
				}
			} while (1);
		}
		// av_frame_free(&pFrame);
		pFrame = NULL;
	}
	iEncFinish[threadId] = 1;
	SPDLOG_INFO("encode finish. threadId: {}", threadId);
	return 0;

}
#endif

#if 0
int CDemuxer::output()
{
	int ret, flag, i, count = 0;
	float fProgress;
	long long sav_time = 0;
	CPacketWrapper PktWrapper;
	enum AVPictureType jpeg_pict_type = AV_PICTURE_TYPE_NONE;

	while (1)
	{
		if (quit_)
			break;
		/* Have all enc threads quited ? */
		flag = 1;
		for (i = 0; i < MAXTHREADNUM; i++)
		{
			if (0 == iEncFinish[i])
				flag = 0;
		}
		m_mtxPacketQueue.lock();
		if (flag && m_dqPacket.empty())
		{
			m_mtxPacketQueue.unlock();
			break;
		}
		if (!flag && m_dqPacket.size() < m_iDequeMin)
		{
			m_mtxPacketQueue.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		sort(m_dqPacket.begin(), m_dqPacket.end(), less_pts_pkg);
		PktWrapper = m_dqPacket.front();
		m_dqPacket.pop_front();
		m_mtxPacketQueue.unlock();
		SPDLOG_INFO("output count: {}, size: {}", ++count, PktWrapper.packet->size);
		if (PktWrapper.packet->size <= 0)
			continue;
		if (NULL == PktWrapper.packet)
			continue;
		OutputJPEG(PktWrapper.packet, PktWrapper.type, pic_path_, pic_iframe_path_);
		listener_->OnFrameReport(PktWrapper.pFrame);
		fProgress = (float)(PktWrapper.packet->pts - start_pts_) / duration_pts_;
		if (0 == m_dqFrame.size() && 0 == m_dqPacket.size())
			listener_->OnProgressReport(1.0);
		else
			listener_->OnProgressReport(fProgress);

		av_frame_free(&PktWrapper.pFrame);
		av_packet_free(&PktWrapper.packet);
	}
	SPDLOG_INFO("output finish.");
	listener_->OnProgressReport(1.0);
	return 0;
}

int CDemuxer::OutputJPEG(AVPacket *pPacketJPEG, enum AVPictureType pict_type, string sPath, string sIPath)
{
	FILE *fp = NULL;
	char sPathName[1024] = {0};
	char sJPEGName[1024] = {0};

	memset(sPathName, 0, 1024);
	memset(sJPEGName, 0, 1024);
	strcat(sPathName, (const char *)sPath.c_str());
	strcat(sPathName, "/");
	sprintf(sJPEGName, "F%d_%llu", m_iPicNum, pPacketJPEG->pts);
	if (AV_PICTURE_TYPE_NONE == pict_type)
	{
		strcat(sJPEGName, "_Unknow");
	}
	else if (AV_PICTURE_TYPE_I == pict_type)
	{
		strcat(sJPEGName, "_I");
	}
	else if (AV_PICTURE_TYPE_P == pict_type)
	{
		strcat(sJPEGName, "_P");
	}
	else if (AV_PICTURE_TYPE_B == pict_type)
	{
		strcat(sJPEGName, "_B");
	}
	else if (AV_PICTURE_TYPE_S == pict_type)
	{
		strcat(sJPEGName, "_S");
	}
	else if (AV_PICTURE_TYPE_SI == pict_type)
	{
		strcat(sJPEGName, "_SI");
	}
	else if (AV_PICTURE_TYPE_SP == pict_type)
	{
		strcat(sJPEGName, "_SP");
	}
	else if (AV_PICTURE_TYPE_BI == pict_type)
	{
		strcat(sJPEGName, "_BI");
	}
	strcat(sJPEGName, ".jpg");
	strcat(sPathName, sJPEGName);

	/*ofstream fout;
	fout.open(sPathName);
	fout << pPacketJPEG->data;
	fout.flush();
	fout.close();*/
	fp = fopen(sPathName, "wb");
	fwrite(pPacketJPEG->data, 1, pPacketJPEG->size, fp);
	fflush(fp);
	fclose(fp);

	if (AV_PICTURE_TYPE_I == pict_type || AV_PICTURE_TYPE_SI == pict_type)
	{
		memset(sPathName, 0, 1024);
		memset(sJPEGName, 0, 1024);
		strcat(sPathName, (const char *)sIPath.c_str());
		strcat(sPathName, "/");
		sprintf(sJPEGName, "F%d_%llu", m_iPicNum, pPacketJPEG->pts);
		if (AV_PICTURE_TYPE_I == pict_type)
		{
			strcat(sJPEGName, "_I");
		}
		else if (AV_PICTURE_TYPE_SI == pict_type)
		{
			strcat(sJPEGName, "_SI");
		}
		strcat(sJPEGName, ".jpg");
		strcat(sPathName, sJPEGName);
		/*fout.open(sPathName);
		fout << pPacketJPEG->data;
		fout.flush();
		fout.close();*/

		fp = fopen(sPathName, "wb");
		fwrite(pPacketJPEG->data, 1, pPacketJPEG->size, fp);
		fflush(fp);
		fclose(fp);
	}
	m_iPicNum++;

	return 0;
}
#endif

int CDemuxer::SetReceiveAvModule(std::pair<int, AvModule *> video_idx_module, std::unordered_map<int, AvModule *> audio_idx_module)
{
	if (!video_idx_module.second)
	{
		return -1;
	}
	for (auto &a : audio_idx_module)
	{
		if (!a.second)
		{
			return -2;
		}
	}
	video_idx_module_ = video_idx_module;
	audio_idx_module_ = audio_idx_module;
	return 0;
}

int CDemuxer::SetReceiveAvModule_1(std::unordered_map<int, AvModule *> idx_module)
{
	idx_module_ = idx_module;
	return 0;
}