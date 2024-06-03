#include <spdlog/spdlog.h>
#include "encoder.h"

CEncoder::CEncoder(IListener *listener, std::string codec_name, int stream_index) : listener_(listener), codec_name_(codec_name), stream_index_(stream_index)
{
    return;
};

CEncoder::~CEncoder()
{
    return;
}

int CEncoder::Start()
{
    const char *filename /* , *codec_name = "libx264" */;
    int ret = 0;

    SPDLOG_INFO("codec name: {}", codec_name_);
    codec_ = avcodec_find_encoder_by_name(codec_name_.c_str());
    if (!codec_)
    {
        SPDLOG_WARN("Codec {} not found", codec_name_);
        return -1;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_)
    {
        SPDLOG_WARN("{}, Could not allocate video codec context", codec_name_);
        return -2;
    }

    set_codec_context(codec_ctx_);

#if 0
    /* put sample parameters */
    codec_ctx_->bit_rate = 400000;
    /* resolution must be a multiple of two */
    codec_ctx_->width = 838;
    codec_ctx_->height = 472;
    /* frames per second */
    codec_ctx_->time_base = (AVRational){1, 90000};
    codec_ctx_->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codec_ctx_->gop_size = 10;
    codec_ctx_->max_b_frames = 1;
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_->id == AV_CODEC_ID_H264)
        av_opt_set(codec_ctx_->priv_data, "preset", "fast", 0);
#endif

    /* open it */
    ret = avcodec_open2(codec_ctx_, codec_, NULL);
    if (ret < 0)
    {
        // fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        SPDLOG_WARN("{}, Could not open codec:", codec_name_); // << av_err2str(ret);
        return -3;
    }

    SPDLOG_INFO("{}, codec type: {}", codec_name_, codec_ctx_->codec_type);

    /* flush the encoder */
    // encode(codec_ctx_, NULL, pkt, f);

    // av_frame_free(&frame);
    // av_packet_free(&pkt);

    return 0;
}

void CEncoder::set_codec_context(AVCodecContext *codec_ctx)
{
    return;
}

AVCodecContext *CEncoder::GetCodecContext()
{
    return codec_ctx_;
}

int CEncoder::Stop()
{
    avcodec_close(codec_ctx_);
    avcodec_free_context(&codec_ctx_);
    return 0;
}

int CEncoder::SetReceiveAvModule(std::vector<AvModule *> next)
{
    next_ = next;
    return 0;
}

int CEncoder::Process(CFrameWrapper &frame)
{
    int ret;
    AVPacket *pkt = NULL;
    AVFrame *pFrame = frame.av_frame;
    pkt = av_packet_alloc();
    if (!pkt)
    {
        SPDLOG_WARN("{}, av_packet_alloc failed.", codec_name_);
        return -1;
    }

    /*     frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "Could not allocate video frame");
            return -2;
        }
        frame->format = codec_ctx_->pix_fmt;
        frame->width  = codec_ctx_->width;
        frame->height = codec_ctx_->height; */

    /*     ret = av_frame_get_buffer(pFrame, 32);
        if (ret < 0) {
            //fprintf(stderr, "Could not allocate the video frame data\n");
            SPDLOG_WARN("Could not allocate the video frame data");
            return -1;
        } */

    /* encode the image */
    // encode(codec_ctx_, frame, pkt, f);

    /* send the frame to the encoder */
    /*     if (pFrame)
            printf("Send frame %3"PRId64"\n", pFrame->pts); */
    SPDLOG_INFO("{}, avcodec_send_frame pts: {}", codec_name_, pFrame->pts);
    ret = avcodec_send_frame(codec_ctx_, pFrame);
    if (ret < 0)
    {
        // fprintf(stderr, "Error sending a frame for encoding\n");
        char err_buf[128];
        memset(err_buf, 0, sizeof(err_buf));
        av_strerror(ret, err_buf, sizeof(err_buf));
        SPDLOG_WARN("{}, Error sending a frame for encoding. {}, err string: {}", codec_name_, ret, err_buf);
        if (codec_name_ == "aac" && 0)
        {
            SPDLOG_WARN("{}", codec_name_);
            if (pFrame && 0)
            {
                SPDLOG_WARN("frame====>>: ");
                SPDLOG_WARN("channels: {}", pFrame->channels);
                SPDLOG_WARN("channel_layout: {}", pFrame->channel_layout);
                SPDLOG_WARN("nb_samples: {}", pFrame->nb_samples);
                SPDLOG_WARN("format: {}", pFrame->format);
                SPDLOG_WARN("pts: {}", pFrame->pts);
                SPDLOG_WARN("pkt_dts: {}", pFrame->pkt_dts);
                SPDLOG_WARN("sample_rate: {}", pFrame->sample_rate);
                SPDLOG_WARN("linesize[0]: {}", pFrame->linesize[0]);
                SPDLOG_WARN("linesize[1]: {}", pFrame->linesize[1]);
                SPDLOG_WARN("linesize[2]: {}", pFrame->linesize[2]);
                SPDLOG_WARN("codec_ctx====>>: ");
                SPDLOG_WARN("bit_rate: {}", codec_ctx_->bit_rate);
            }
        }
        return -2;
    }
    else
    {
        SPDLOG_INFO("{}, sended a frame successfully. {}", codec_name_, ret);
    }

    ret = avcodec_receive_packet(codec_ctx_, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        char err_buf[128];
        memset(err_buf, 0, sizeof(err_buf));
        av_strerror(ret, err_buf, sizeof(err_buf));
        SPDLOG_WARN("{}, avcodec_receive_packet: {}, {}", codec_name_, ret, err_buf);
        return -3;
    }
    else if (ret < 0)
    {
        // fprintf(stderr, "Error during encoding\n");
        char err_buf[128];
        memset(err_buf, 0, sizeof(err_buf));
        av_strerror(ret, err_buf, sizeof(err_buf));
        SPDLOG_WARN("{}, Error during encoding: {}, {}", codec_name_, ret, err_buf);
        return -4;
    }
    pkt->stream_index = stream_index_;
    SPDLOG_INFO("{}, avcodec_receive_packet ret: {}, stream_index: {}, pts: {}, dts: {}, duration: {}, size: {}", codec_name_, ret, pkt->stream_index, pkt->pts, pkt->dts, pkt->duration, pkt->size);
    CPacketWrapper pw(pkt);
    for (auto &a : next_)
    {
        a->Process(pw);
    }
    if (pkt)
    {
        av_packet_free(&pkt);
    }
    return 0;
};