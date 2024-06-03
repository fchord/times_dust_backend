#include <spdlog/spdlog.h>
#include "decoder.h"

CDecoder::CDecoder(IListener *listener)
{
    listener_ = listener;
    return;
};

CDecoder::~CDecoder()
{
    return;
};

int CDecoder::Start(AVCodecContext *codec_ctx)
{
    codec_ctx_ = codec_ctx;

    codec_ = avcodec_find_decoder(codec_ctx_->codec_id);

    if (avcodec_open2(codec_ctx_, codec_, NULL) < 0)
    {
        SPDLOG_WARN("avcodec_open2 failed.");
        return NULL;
    }

    return 0;
};

int CDecoder::Stop()
{
    avcodec_close(codec_ctx_);
    return 0;
};

int CDecoder::Process(CPacketWrapper &packet_wp)
{
    int ret = 0, count = 0;
    char err[256];
    AVPacket *packet = packet_wp.av_packet;
    AVFrame *frame = NULL, *frame_filtered = NULL;

    if (packet)
    {
        SPDLOG_INFO("packet stream_index: {}, pts: {}, dts: {}", packet->stream_index, packet->pts, packet->dts);
    }

    ret = avcodec_send_packet(codec_ctx_, packet);
    // SPDLOG_INFO("avcodec_send_packet: {}", ret);

    if (ret < 0)
    {
        // SPDLOG_WARN("avcodec_send_packet failed: {}", ret);
        memset((char *)&err, 0, sizeof(err));
        av_strerror(ret, (char *)&err, sizeof(err));
        // SPDLOG_WARN("avcodec_send_packet failed: {}, err: {}", ret, err);
        return -1;
    }

    if (!frame)
        frame = av_frame_alloc();

    ret = avcodec_receive_frame(codec_ctx_, frame);
    // SPDLOG_INFO("avcodec_receive_frame: {}", ret);
    if (ret < 0)
    {
        // SPDLOG_WARN("avcodec_receive_frame failed: {}", ret);
        memset((char *)&err, 0, sizeof(err));
        av_strerror(ret, (char *)&err, sizeof(err));
        SPDLOG_WARN("avcodec_receive_frame failed: {}, err: {}", ret, err);
        // skipped_frame++;
        return -2;
    }
    else
    {
        if (frame)
        {
            SPDLOG_INFO("frame pts: {}, dts: {}", frame->pts, frame->pkt_dts);
        }
        /* if (frame->pts >= m_i64StartPts) {
            if (m_i64DurationPts > 0 && frame->pts <= m_i64StartPts + m_i64DurationPts)
                bInRange = true;
            else if (m_i64DurationPts <= 0)
                bInRange = true;
            else
                bInRange = false;
        }
        else
            bInRange = false; */
        CFrameWrapper fw(frame);
        receive_module_->Process(fw);
#if 0
        if (true /* bInRange */)
        {
            SPDLOG_INFO("demux count: {}, pts: {}, pkt_pts: {}, picture type: {}", ++count, frame->pts, frame->pkt_pts, frame->pict_type);
            while (1)
            {
                if (quit_)
                    break;
                mtx_dq_frame_.lock();
                if (dq_frame_.size() > deque_max_)
                {
                    mtx_dq_frame_.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                dq_frame_.push_back(CFrameWrapper(frame));
                sort(dq_frame_.begin(), dq_frame_.end(), less_pts);
                /*
                for (iter = dq_frame_.begin(), i = 0; iter != dq_frame_.end(); iter++, i++) {
                LzLogInfo << "" << i << ": " << iter->frame->pts;
                }*/
                mtx_dq_frame_.unlock();
                break;
            }
            frame = NULL;
        }
        else
        {
            // The frame decoded not in range.
            av_frame_free(&frame);
            frame = NULL;
        }
#endif
        av_count_++;
        SPDLOG_INFO("Decoder. pts: {}", frame->pts);
        listener_->OnFrameReport(NULL);
        av_frame_free(&frame);
        frame = NULL;
    }
    return 0;
};

int CDecoder::SetReceiveAvModule(AvModule *receive_module)
{
    if (!receive_module)
    {
        return -1;
    }
    receive_module_ = receive_module;
    return 0;
}