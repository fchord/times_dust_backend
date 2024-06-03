#pragma once
#include <stdexcept>
#include <memory>
#include "avmodule.h"
#include "frame_wrapper.h"

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

#if 0
typedef struct FilteringContext {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
} FilteringContext;
#endif

class CFilter : public AvModule
{
public:
    CFilter();
    ~CFilter();
    int Start(
        AVRational src_time_base,
        int src_sample_rate,
        AVSampleFormat src_sample_fmt,
        int src_channel_layout,
        AVRational dst_time_base,
        int dst_sample_rate,
        AVSampleFormat dst_sample_fmt,
        int dst_channel_layout,
        const std::string &user_filters);
    int Stop();
    int SetReceiveAvModule(AvModule *next_module);
    int Process(CFrameWrapper &frame);
    int Command(const std::string &filter_name, const std::string &cmd, const std::string &args);

private:
    AvModule *next_module_;

    AVRational src_time_base_;
    int src_sample_rate_;
    AVSampleFormat src_sample_fmt_;
    int src_channel_layout_;
    AVRational dst_time_base_;
    long dst_sample_rate_;
    AVSampleFormat dst_sample_fmt_;
    long dst_channel_layout_;
    std::string user_filters_;

    AVFilterGraph *graph_;
    AVFilterContext *buffersrc_ctx_, *buffersink_ctx_;

    int init_filter_graph();
    int uninit_filter_graph();
    // FilteringContext *filter_ctx_;
    long acc_nb_samples_ = 0;
};

class CVFilter : public AvModule
{
public:
    CVFilter();
    ~CVFilter();
    int Start(
        AVRational src_time_base,
        int src_width, int src_height,
        AVPixelFormat src_pix_fmt,
        AVRational dst_time_base,
        AVPixelFormat dst_pix_fmt,
        const std::string &user_filters);
    int Stop();
    int SetReceiveAvModule(AvModule *next_module);
    int Process(CFrameWrapper &frame);
    int Command(const std::string &filter_name, const std::string &cmd, const std::string &args);

private:
    AvModule *next_module_;

    AVRational src_time_base_;
    int src_width_, src_height_;
    AVPixelFormat src_pix_fmt_; // AVSampleFormat src_sample_fmt_;
    // int src_channel_layout_;
    AVRational dst_time_base_;
    // int dst_width_, dst_height_;
    AVPixelFormat dst_pix_fmt_; // AVSampleFormat dst_sample_fmt_;
    // long dst_channel_layout_;
    std::string user_filters_;

    AVFilterGraph *graph_;
    AVFilterContext *buffersrc_ctx_, *buffersink_ctx_;

    int init_filter_graph();
    int uninit_filter_graph();
    // FilteringContext *filter_ctx_;
    long acc_frame_number_ = 0;

    int private_process(AVFrame *f);
};