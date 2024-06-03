#include <spdlog/spdlog.h>
#include "filter.h"

extern "C"
{
#include <libavutil/pixdesc.h>
}

CFilter::CFilter()
{
    return;
}

CFilter::~CFilter()
{
    return;
}

int CFilter::Start(AVRational src_time_base, int src_sample_rate,
                   AVSampleFormat src_sample_fmt, int src_channel_layout,
                   AVRational dst_time_base, int dst_sample_rate,
                   AVSampleFormat dst_sample_fmt, int dst_channel_layout,
                   const std::string &user_filters)
{

    int err, nb_frames, i;

    src_time_base_ = src_time_base;
    src_sample_rate_ = src_sample_rate;
    src_sample_fmt_ = src_sample_fmt;
    src_channel_layout_ = src_channel_layout;
    dst_time_base_ = dst_time_base;
    dst_sample_rate_ = dst_sample_rate;
    dst_sample_fmt_ = dst_sample_fmt;
    dst_channel_layout_ = dst_channel_layout;
    user_filters_ = user_filters;

    SPDLOG_INFO("src_time_base_: {}/{}", src_time_base_.num, src_time_base_.den);
    SPDLOG_INFO("src_sample_rate_: {}", src_sample_rate_);
    SPDLOG_INFO("src_sample_fmt_: {}", src_sample_fmt_);
    SPDLOG_INFO("src_channel_layout_: {}", src_channel_layout_);

    err = init_filter_graph();
    if (err < 0)
    {
        SPDLOG_INFO("Unable to init filter graph: {}", err);
        return -1;
    }
    return 0;
}

int CFilter::Stop()
{
    uninit_filter_graph();
    return 0;
}

int CFilter::SetReceiveAvModule(AvModule *next_module)
{
    next_module_ = next_module;
    return 0;
}

int CFilter::Process(CFrameWrapper &fw)
{
    uint8_t errstr[1024];
    float duration;
    int err, nb_frames, i, ret;
    AVFrame *frame = NULL, *frame_out = NULL;

    if (!fw.av_frame)
    {
        return 0;
    }
    frame = fw.av_frame;

#if 0
    if(frame && 0) {
        static int64_t last_in_pts = 0;
        SPDLOG_INFO("frame pts: {}, diff pts: {}", frame->pts, frame->pts - last_in_pts);
        last_in_pts = frame->pts;
        SPDLOG_INFO("frame nb_samples: {}", frame->nb_samples);
    }
#endif

    av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
#if 0
    if(filter_ctx_ && !filter_ctx_->buffersrc_ctx){
        av_log(NULL, AV_LOG_ERROR, "buffersrc_ctx is null\n");
        return -1;
    }
#else
    if (!buffersrc_ctx_)
    {
        av_log(NULL, AV_LOG_ERROR, "buffersrc_ctx is null\n");
        return -1;
    }

#endif
    ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, frame, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return ret;
    }

    /* pull filtered frames from the filtergraph */
    while (1)
    {
        if (!frame_out)
        {
            frame_out = av_frame_alloc();
        }
        if (!frame_out)
        {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
        ret = av_buffersink_get_frame(buffersink_ctx_, frame_out);
        if (ret < 0)
        {
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            av_frame_free(&frame_out);
            break;
        }
        if (frame_out)
        {
            frame_out->pts = av_rescale_q(acc_nb_samples_, (AVRational){1, dst_sample_rate_}, dst_time_base_);
            acc_nb_samples_ += frame_out->nb_samples;
            frame_out->pts;
#if 0
            static int64_t last_pts = 0;
            SPDLOG_INFO("frame_out pts: {}, diff pts: {}", frame_out->pts, frame_out->pts - last_pts);
            last_pts = frame_out->pts;
            SPDLOG_INFO("frame_out nb_samples: {}", frame_out->nb_samples) ;
#endif
        }
        CFrameWrapper dst_fw(frame_out);
        next_module_->Process(dst_fw);
    }
    return 0;
}

int CFilter::init_filter_graph()
{
    unsigned int i = 0;
    int ret = 0;
    char args[512] = {0}, args_in[512] = {0};
    const char *filter_spec = NULL;
    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterContext *user_ctx = NULL;

    av_log(NULL, AV_LOG_INFO, "init_filter_graph\n");

    graph_ = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph_)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    memset(args, 0, sizeof(args));
    // snprintf(args, sizeof(args), "volume=volume=0.2[a1];[a1]asetnsamples=n=1024:p=1[out]");
    snprintf(args, sizeof(args), user_filters_.c_str());
    filter_spec = args;

    av_log(NULL, AV_LOG_INFO, "init_filter\n");

    buffersrc = avfilter_get_by_name("abuffer");
    buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersrc || !buffersink)
    {
        av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    memset(args_in, 0, sizeof(args_in));
    snprintf(args_in, sizeof(args_in),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             src_time_base_.num, src_time_base_.den, src_sample_rate_,
             av_get_sample_fmt_name(src_sample_fmt_),
             (uint64_t)src_channel_layout_);

    ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in",
                                       args_in, NULL, graph_);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out",
                                       NULL, NULL, graph_);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_bin(buffersink_ctx_, "sample_fmts",
                         (uint8_t *)&dst_sample_fmt_, sizeof(dst_sample_fmt_),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
        goto end;
    }
    ret = av_opt_set_bin(buffersink_ctx_, "channel_layouts",
                         (uint8_t *)&dst_channel_layout_,
                         sizeof(dst_channel_layout_), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
        goto end;
    }
    ret = av_opt_set_bin(buffersink_ctx_, "sample_rates",
                         (uint8_t *)&dst_sample_rate_, sizeof(dst_sample_rate_),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
        goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx_;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx_;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if (!outputs->name || !inputs->name)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(graph_, filter_spec,
                                        &inputs, &outputs, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_parse_ptr: %d\n", ret);
        goto end;
    }

    if ((ret = avfilter_graph_config(graph_, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_config: %d\n", ret);
        goto end;
    }
#if 0    
    if(user_ctx = avfilter_graph_get_filter(graph_, "a1")) {
        SPDLOG_INFO("user_ctx name: {}", user_ctx->name);
        
    } else {
        SPDLOG_INFO("user_ctx is null.");
    }
#endif
#if 1
    for (int n = 0; n < graph_->nb_filters; n++)
    {
        SPDLOG_INFO("graph filter_ctx_ name: {}, filter name: {}", graph_->filters[n]->name, graph_->filters[n]->filter->name);
    }
#endif

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

int CFilter::uninit_filter_graph()
{
    avfilter_graph_free(&graph_);
    buffersrc_ctx_ = NULL;
    buffersink_ctx_ = NULL;

    acc_nb_samples_ = 0;
}

int CFilter::Command(const std::string &filter_name, const std::string &cmd,
                     const std::string &args)
{
    SPDLOG_INFO("Command. filter_name: {}, cmd: {}, args: {}", filter_name, cmd, args);
    for (int n = 0; n < graph_->nb_filters; n++)
    {
        if (std::string(graph_->filters[n]->filter->name) == filter_name)
        {
            graph_->filters[n]->filter->process_command(graph_->filters[n], cmd.c_str(), args.c_str(),
                                                        NULL, 0, 0);
        }
    }
    return 0;
}

/********* Video Filter *********/

CVFilter::CVFilter()
{
    return;
}

CVFilter::~CVFilter()
{
    return;
}

int CVFilter::Start(
    AVRational src_time_base,
    int src_width, int src_height,
    AVPixelFormat src_pix_fmt,
    AVRational dst_time_base,
    AVPixelFormat dst_pix_fmt,
    const std::string &user_filters)
{

    int err, nb_frames, i;

    src_time_base_ = src_time_base;
    src_width_ = src_width;
    src_height_ = src_height;
    src_pix_fmt_ = src_pix_fmt;

    dst_time_base_ = dst_time_base;
    dst_pix_fmt_ = dst_pix_fmt;

    user_filters_ = user_filters;

    SPDLOG_INFO("src_time_base_: {}/{}", src_time_base_.num, src_time_base_.den);
    SPDLOG_INFO("src_width_: {}", src_width_);
    SPDLOG_INFO("src_height_: {}", src_height_);
    SPDLOG_INFO("src_pix_fmt_: {}", src_pix_fmt_);

    SPDLOG_INFO("dst_time_base_: {}/{}", dst_time_base_.num, dst_time_base_.den);
    SPDLOG_INFO("dst_pix_fmt_: {}", dst_pix_fmt_);

    err = init_filter_graph();
    if (err < 0)
    {
        SPDLOG_INFO("Unable to init filter graph: {}", err);
        return -1;
    }
    return 0;
}

int CVFilter::Process(CFrameWrapper &fw)
{
    uint8_t errstr[1024];
    float duration;
    int err, nb_frames, i, ret;
    AVFrame *frame = NULL, *frame_out = NULL;

    if (!fw.av_frame)
    {
        return 0;
    }
    frame = fw.av_frame;
    private_process(frame);

#if 0
    if(frame && 1) {
        static int64_t last_in_pts = 0;
        SPDLOG_INFO("frame pts: {}, diff pts: {}" << frame->pts, frame->pts - last_in_pts);
        last_in_pts = frame->pts;
    }
#endif

    av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
#if 0
    if(filter_ctx_ && !filter_ctx_->buffersrc_ctx){
        av_log(NULL, AV_LOG_ERROR, "buffersrc_ctx is null\n");
        return -1;
    }
#else
    if (!buffersrc_ctx_)
    {
        av_log(NULL, AV_LOG_ERROR, "buffersrc_ctx is null\n");
        return -1;
    }

#endif
    /*
        flags
        AV_BUFFERSRC_FLAG_KEEP_REF: 如果设置了此flag，接口内将新构造一个AVFrame copy，并复制原AVFrame的所有成员，除了buf里的数据，
            成员 AVBufferRef *buf[8] 结构体也复制过来，data指针不变，引用refcount加1。filter将对copy进行处理。如果没有设置此flag，则
            直接用传入的frame处理。结束时调用av_frame_free析构掉该AVFrame。

    */
    ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return ret;
    }

    /* pull filtered frames from the filtergraph */
    while (1)
    {
        if (!frame_out)
        {
            frame_out = av_frame_alloc();
        }
        if (!frame_out)
        {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
        // ret = av_buffersink_get_frame(filter_ctx_->buffersink_ctx, frame_out);
        ret = av_buffersink_get_frame(buffersink_ctx_, frame_out);
        if (ret < 0)
        {
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            av_frame_free(&frame_out);
            break;
        }
        if (frame_out)
        {
            // frame_out->pts = av_rescale_q(frame->pts, src_time_base_, dst_time_base_);
            frame_out->pts = frame->pts;
            // acc_nb_samples_ += frame_out->nb_samples;
            // frame_out->pts;
#if 0
            static int64_t last_pts = 0;
            SPDLOG_INFO("frame_out pts: {}, diff pts: {}, frame->pts: {}", frame_out->pts, frame_out->pts - last_pts, frame->pts);
            last_pts = frame_out->pts;
            SPDLOG_INFO("frame_out nb_samples: {}", frame_out->nb_samples);
#endif
        }
        CFrameWrapper dst_fw(frame_out);
        next_module_->Process(dst_fw);
    }
    return 0;
}

int CVFilter::init_filter_graph()
{
    unsigned int i = 0;
    int ret = 0;
    char args[512] = {0}, args_in[512] = {0}, args_out[512] = {0};
    const char *filter_spec = NULL;
    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    av_log(NULL, AV_LOG_INFO, "init_filter_graph\n");

    graph_ = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph_)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    memset(args, 0, sizeof(args));
    snprintf(args, sizeof(args), user_filters_.c_str());
    filter_spec = args;

    av_log(NULL, AV_LOG_INFO, "init_filter\n");

    buffersrc = avfilter_get_by_name("buffer");
    buffersink = avfilter_get_by_name("buffersink");
    if (!buffersrc || !buffersink)
    {
        av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    memset(args_in, 0, sizeof(args_in));
    snprintf(args_in, sizeof(args_in),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             src_width_, src_height_, src_pix_fmt_,
             src_time_base_.num, src_time_base_.den,
             1, 1);
    ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in",
                                       args_in, NULL, graph_);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    memset(args_out, 0, sizeof(args_out));
    snprintf(args_out, sizeof(args_out), "pix_fmts=%s", av_get_pix_fmt_name(dst_pix_fmt_)); // AVPixelFormat; av_get_pix_fmt_name

    ret = avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out",
                                       NULL, NULL, graph_);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx_;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx_;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if (!outputs->name || !inputs->name)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(graph_, filter_spec,
                                        &inputs, &outputs, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_parse_ptr: %d\n", ret);
        goto end;
    }

    if ((ret = avfilter_graph_config(graph_, NULL)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_config: %d\n", ret);
        goto end;
    }

#if 1
    for (int n = 0; n < graph_->nb_filters; n++)
    {
        SPDLOG_INFO("graph filter_ctx_ name: {}, filter name: {}", graph_->filters[n]->name, graph_->filters[n]->filter->name);
    }
#endif

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

int CVFilter::Stop()
{
    uninit_filter_graph();
    return 0;
}

int CVFilter::SetReceiveAvModule(AvModule *next_module)
{
    next_module_ = next_module;
    return 0;
}

int CVFilter::uninit_filter_graph()
{
    avfilter_graph_free(&graph_);
    buffersrc_ctx_ = NULL;
    buffersink_ctx_ = NULL;
    return 0;
}

int CVFilter::Command(const std::string &filter_name, const std::string &cmd,
                      const std::string &args)
{
    SPDLOG_INFO("Command. filter_name: {}, cmd: {}, args: {}", filter_name, cmd, args);
    for (int n = 0; n < graph_->nb_filters; n++)
    {
        // SPDLOG_INFO() << "graph filter_ctx_ name: " << graph_->filters[n]->name
        //     << ", filter name: " << graph_->filters[n]->filter->name;
        if (std::string(graph_->filters[n]->filter->name) == filter_name)
        {
            graph_->filters[n]->filter->process_command(graph_->filters[n], cmd.c_str(), args.c_str(),
                                                        NULL, 0, 0);
        }
    }
    return 0;
}

int CVFilter::private_process(AVFrame *f)
{
    if(!f) {
        return -1;
    }
    if (f->format == AV_PIX_FMT_YUV420P) {

        for(int i = 0; i < f->height - 1; i++){
            int delta = 0;
            for(int j = 0; j < f->width; j++){
                delta += abs((int)f->data[0][(i+1)*f->linesize[0] + j] - (int)f->data[0][i*f->linesize[0] + j]);
            }
            if (delta / f->width > 32) {
                SPDLOG_INFO("border line: {}, delta: {}", i, delta);
                // Y
                for(int j = 0; j < f->width; j++) {
                    f->data[0][i*f->linesize[0] + j] = 49;
                }
                if(i%2 == 0)              
                {
                    for(int j = 0; j < f->width; j++) {
                        f->data[0][(i+1)*f->linesize[0] + j] = 49;
                    }
                    i++;
                } else {
                    for(int j = 0; j < f->width; j++) {
                        f->data[0][(i-1)*f->linesize[0] + j] = 49;
                    }                
                }
                // U
                for(int j = 0; j < f->width / 2; j++) {
                    f->data[1][i/2*f->linesize[1] + j] = 109;
                }
                // V
                for(int j = 0; j < f->width / 2; j++) {
                    f->data[2][i/2*f->linesize[2] + j] = 184;
                }
            }
        }
            

    }

}