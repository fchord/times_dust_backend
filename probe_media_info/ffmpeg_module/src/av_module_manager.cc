#include <cstdio>
#include <unistd.h>
#include <unordered_map>
#include <boost/json.hpp>
#include <spdlog/spdlog.h>
#include "av_module_manager.h"
#include "lz_log.h"
#include "lz_utils.h"
#include "decoder.h"
#include "video_encoder.h"
#include "audio_encoder.h"

extern "C"
{
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
}

void get_aenc_format()
{
    std::string audio_encoder{"aac"};
    std::vector<AVSampleFormat> formats;
    std::vector<int> sample_rates;
    std::vector<uint64_t> channel_layouts;
    int ret = CAudioEncoder::GetSupportedFormat(audio_encoder, formats, sample_rates, channel_layouts);
    SPDLOG_INFO("Get {} supported format. ret: {}", audio_encoder, ret);
    for (auto &a : formats)
    {
        SPDLOG_INFO("{}", av_get_sample_fmt_name(a));
    }
    for (auto &a : sample_rates)
    {
        SPDLOG_INFO("{}", a);
    }
    for(auto &a : channel_layouts)
    {
        char buf[64] = {};
        av_get_channel_layout_string(buf, sizeof(buf), 0, a);
        SPDLOG_INFO("{}", buf);
    }
    return ;
}

void get_venc_format()
{
    std::string video_encoder{"libx264"};
    std::vector<AVPixelFormat> formats;
    int ret = CVideoEncoder::GetSupportedFormat(video_encoder, formats);
    SPDLOG_INFO("Get {} supported format. ret: {}", video_encoder, ret);
    for (auto &a : formats)
    {
        SPDLOG_INFO("{}", av_get_pix_fmt_name(a));
    }
    return ;
}

AvModuleManager::AvModuleManager(boost::json::object &parameter)
{
    SPDLOG_INFO("AvModuleManager");
    parameter_ = parameter;

    get_venc_format();
    get_aenc_format();

    return;
};

AvModuleManager::~AvModuleManager()
{
    return;
};

int AvModuleManager::Start_1(std::string dst_url, bool video_bypass, bool audio_bypass)
{

    SPDLOG_INFO("AvModuleManager Start_1");
    int ret;
    AVFormatContext *pFormatContext = NULL;
    src_url_ = parameter_["src_url"].as_string().c_str();
    out_url_ = dst_url;

    // demuxer
    demuxer_ = new CDemuxer(this);
    ;
    ret = demuxer_->Open(src_url_, 0, 0, "", "", &pFormatContext);
    if (ret < 0)
    {
        SPDLOG_INFO("Demuxer open failed: ", ret);
        return -1;
    }
/*     caculate_target_parameters(pFormatContext, true, true);

    std::unordered_map<int, AvModule *> idx_module;
    std::unordered_map<int, MuxerParameter> muxer_parameter;

    AVStream *stream;
    AVCodecContext *codec_ctx; */

    // video decoder
/*     int video_index = get_index_from_format_ctx(pFormatContext, AVMEDIA_TYPE_VIDEO);
    get_info_from_format_ctx(pFormatContext, video_index, &stream, &codec_ctx);
    CDecoder *video_decoder = new CDecoder(this); */
    // std::pair<int, AVCodecContext*> video_idx_ctx = get_video_codec_ctx_from_format_ctx(pFormatContext);
/*     video_decoder->Start(codec_ctx);
    idx_module[video_index] = video_decoder; */

    // video filter
/*     CVFilter *video_filter = new CVFilter();
    video_decoder->SetReceiveAvModule(video_filter);
    char args[128] = {0};
    snprintf(args, sizeof(args), "scale=w=%d:h=%d[v1];[v1]fps=fps=10[v2];[v2]fade=in:0:30", tar_width_, tar_height_);
    video_filter->Start(stream->time_base, codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, stream->time_base,
                        codec_ctx->pix_fmt, std::string(args)); */
    // format=pix_fmts=yuv420p|yuv444p|yuv410p

    // video encoder
/*     CVideoEncoder *video_encoder = new CVideoEncoder(this, video_encoder_name_, video_index);
    video_encoder->SetParameter(tar_width_, tar_height_, 90000, tar_frame_rate_, tar_video_bitrate_);
    video_encoder->Start();
    std::vector<AVPixelFormat> venc_supported_fmts = video_encoder->GetSupportedPixelFormat();
    for (auto a : venc_supported_fmts)
    {
        SPDLOG_INFO("video encoder supported pix fmt: {}", av_get_pix_fmt_name(a));
    }

    video_filter->SetReceiveAvModule(video_encoder);
    muxer_parameter[video_index].codec_ctx = video_encoder->GetCodecContext();
    muxer_parameter[video_index].stream_time_base = stream->time_base; */

    // audio decoder
/*     int audio_index = get_index_from_format_ctx(pFormatContext, AVMEDIA_TYPE_AUDIO);
    get_info_from_format_ctx(pFormatContext, audio_index, &stream, &codec_ctx);
    CDecoder *audio_decoder = new CDecoder(this);
    // std::pair<int, AVCodecContext*> audio_idx_ctx = get_audio_codec_ctx_from_format_ctx(pFormatContext);
    audio_decoder->Start(codec_ctx);
    idx_module[audio_index] = audio_decoder;

    // audio filter
    // CAudioFilter *audio_filter = new CAudioFilter();
    CFilter *audio_filter = new CFilter();
    audio_filter_ = audio_filter;
    audio_decoder->SetReceiveAvModule(audio_filter);
    audio_filter->Start(stream->time_base, codec_ctx->sample_rate, codec_ctx->sample_fmt, codec_ctx->channel_layout,
                        AVRational{1, tar_sample_rate_}, tar_sample_rate_, tar_sample_fmt_, codec_ctx->channel_layout,
                        std::string("volume=volume=0.95[a1];[a1]asetnsamples=n=1024:p=1")); */

    /* CAudioFilterChain* audio_filter_chain = new CAudioFilterChain();
    audio_filter_chain->AddFilters({"aformat=sample_fmts=s16:channel_layouts=mono", "atempo=1.0"});
    audio_decoder->SetReceiveAvModule(audio_filter_chain); */

    // audio encoder
/*     CAudioEncoder *audio_encoder = new CAudioEncoder(this, audio_encoder_name_, audio_index);
    audio_encoder->SetParameter(AVRational{1, tar_sample_rate_}, 1024, 128000, tar_sample_fmt_, tar_sample_rate_, 2, AV_CH_LAYOUT_STEREO, FF_PROFILE_AAC_MAIN);
    audio_encoder->Start(); */
    

/*     audio_filter->SetReceiveAvModule(audio_encoder);
    // audio_filter_chain->SetReceiveAvModule(audio_encoder);
    muxer_parameter[audio_index].codec_ctx = audio_encoder->GetCodecContext();
    muxer_parameter[audio_index].stream_time_base = AVRational{1, tar_sample_rate_};

    demuxer_->SetReceiveAvModule_1(idx_module); */

    // muxer
/*     muxer_ = new CMuxer(this);
    muxer_->Start(muxer_parameter, out_url_);
    video_encoder->SetReceiveAvModule(std::vector<AvModule *>{muxer_});
    audio_encoder->SetReceiveAvModule(std::vector<AvModule *>{muxer_}); */

    // work_thread_ = new std::thread(std::mem_fn(&AvModuleManager::work), this);

    return 0;
}

int AvModuleManager::Start(std::string dst_url, bool video_bypass, bool audio_bypass)
{
#if 0
    AVFormatContext* pFormatContext = NULL;
    AVCodecContext *pVideoCodecContext = NULL, *pAudioCodecContext = NULL;
    //int video_stream_idx = -1, audio_stream_idx = -1;
    int ret;

    //src_url_ = src_url;
    src_url_ = parameter_["src_url"].as_string().c_str();
    //LzLogInfo << parameter_["src_url"];
    out_url_ = dst_url;

    LzLogInfo << "AvModuleManager Start";

    std::pair<int, AVCodecContext*> video_idx_ctx = {-1, NULL};
    std::unordered_map<int, AVCodecContext*> audio_idx_ctx;
    std::pair<int, AVCodecParameters*> video_idx_codecpar = {-1, NULL};
    std::unordered_map<int, AVCodecParameters*> audio_idx_codecpar;
    std::pair<int, AVRational> video_idx_time_base;
    std::unordered_map<int, AVRational> audio_idx_time_base;
    //std::string src_url = "/home/liuzhi/Videos/liuyanghe.mp4";

    CDemuxer* demuxer = new CDemuxer(this);
    ret = demuxer->Open(src_url_, 0, 0, "", "", &pFormatContext);
    if (ret < 0) {
        LzLogInfo << "Demuxer open failed: " << ret;
        return -1;
    }
    demuxer_ = demuxer;

    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && -1 == video_idx_ctx.first) {
            video_idx_ctx.first = i;
            video_idx_ctx.second = pFormatContext->streams[i]->codec;
            video_idx_codecpar.first = i;
            video_idx_codecpar.second = pFormatContext->streams[i]->codecpar;
            video_idx_time_base.first = i;
            video_idx_time_base.second = pFormatContext->streams[i]->time_base;
            LzLogInfo << "video stream idx: " << i;
        }
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_idx_ctx[i] = pFormatContext->streams[i]->codec;
            audio_idx_codecpar[i] = pFormatContext->streams[i]->codecpar;
            audio_idx_time_base[i] = pFormatContext->streams[i]->time_base;
            LzLogInfo << "audio stream idx: " << i;
        }
    }

    if(!video_bypass) {
        CDecoder* video_decoder = new CDecoder(this);
        //video_decoder->SetReceiveAvModule(frame_devourer_);
        video_decoder->Start(video_idx_ctx.second);
        video_idx_module_.first = video_idx_ctx.first;
        video_idx_module_.second = video_decoder;

        //CEncoder* video_encoder = new CEncoder(this);
        //video_encoder->Start();
    }

    if(!audio_bypass) {
        for(auto& a : audio_idx_ctx) {
            CDecoder* audio_decoder = new CDecoder(this);
            //audio_decoder->SetReceiveAvModule(frame_devourer_);
            audio_decoder->Start(a.second);
            audio_idx_module_[a.first] = audio_decoder;
        }

    }

    

//#define BY_PASS
#ifndef BY_PASS
    frame_devourer_ = new CFrameDevourer();
    
    CDecoder* video_decoder = new CDecoder(this);
    video_decoder->SetReceiveAvModule(frame_devourer_);
    video_decoder->Start(video_idx_ctx.second);
    video_idx_module_.first = video_idx_ctx.first;
    video_idx_module_.second = video_decoder;

    for(auto& a : audio_idx_ctx) {
        CDecoder* audio_decoder = new CDecoder(this);
        audio_decoder->SetReceiveAvModule(frame_devourer_);
        audio_decoder->Start(a.second);
        audio_idx_module_[a.first] = audio_decoder;
    }
#else 
    muxer_ = new CMuxer(this);
    //std::vector<AVCodecParameters*> codec_codecpar;//
    //std::vector<AVRational> src_time_base;// 
    std::unordered_map<int, MuxerParameter> muxer_parameter;

    muxer_parameter[video_idx_codecpar.first].codec_par = video_idx_codecpar.second;
    muxer_parameter[video_idx_time_base.first].stream_time_base = video_idx_time_base.second;
    video_idx_module_.first = video_idx_ctx.first;
    video_idx_module_.second = muxer_;

    for(auto& a : audio_idx_codecpar) {
        muxer_parameter[a.first].codec_par = a.second;
        audio_idx_module_[a.first] = muxer_;
    }
    for(auto& a : audio_idx_time_base) {
        muxer_parameter[a.first].stream_time_base = a.second;
    }

    /* codec_codecpar.push_back(video_idx_codecpar.second);
    src_time_base.push_back(video_idx_time_base.second);
    video_idx_module_.first = video_idx_ctx.first;
    video_idx_module_.second = muxer_;

    for(auto& a : audio_idx_codecpar) {
        codec_codecpar.push_back(a.second);
        audio_idx_module_[a.first] = muxer_;
    }
    for(auto& a : audio_idx_time_base) {
        src_time_base.push_back(a.second);
    } */
    muxer_->Start(muxer_parameter, out_url_);
#endif
    demuxer->SetReceiveAvModule(video_idx_module_, audio_idx_module_);

    work_thread_ = new std::thread(std::mem_fn(&AvModuleManager::work), this);
#endif
    return 0;
};

int AvModuleManager::Stop()
{
    quit_ = true;
    if (work_thread_ && work_thread_->joinable())
    {
        work_thread_->join();
    }
#ifndef BY_PASS
    /* if(frame_devourer_) {
        delete frame_devourer_;
        frame_devourer_ = nullptr;
    } */
    /* if(video_idx_module_.second) {
        delete video_idx_module_.second;
        video_idx_module_.second = nullptr;
    }
    video_idx_module_.first = -1;
    for(auto& a : audio_idx_module_) {
        delete a.second;
        a.second = nullptr;
    }
    audio_idx_module_.clear(); */
#else
    if (muxer_)
    {
        muxer_->Stop();
        delete muxer_;
        muxer_ = nullptr;
    }
#endif

    if (demuxer_)
    {
        delete demuxer_;
        demuxer_ = nullptr;
    }
    if (muxer_)
    {
        muxer_->Stop();
        delete muxer_;
        muxer_ = nullptr;
    }
    return 0;
};

void AvModuleManager::OnProgressReport(float fProgress)
{
    return;
}

void AvModuleManager::OnFrameReport(void *pFrame)
{
    packet_count_++;
    /* if(packet_count_ >= 100) {
        quit_ = true;
    } */
    return;
}

void AvModuleManager::OnErrorReport(string sError)
{
    return;
}

int AvModuleManager::work()
{
    int ret, count = 0;
    while (!quit_)
    {
        ret = demuxer_->Process();
        count++;
        // LzLogInfo << "work thread. count: " << count << ", packet_count_: " << packet_count_;
        /* if(count - packet_count_ > 100) {
            quit_ = true;
        } */
#if 0        
        if(count % 100 == 0) {
            if((count / 100) % 2 == 0) {
                audio_filter_->Command(std::string("volume"), std::string("volume"), std::string("0.9"));
            } else {
                audio_filter_->Command(std::string("volume"), std::string("volume"), std::string("0.1"));
            }
        }
#endif

#if 0
        if(count <= 1000) {
            float value = (float)count / 1000;
            char args[16] = {0};
            memset(args, 0, sizeof(args));
            snprintf(args, sizeof(args), "%1.2f", value);
            audio_filter_->Command(std::string("volume"), std::string("volume"), std::string(args));

        }
#endif
    }
    // LzLogInfo << "work thread quit. packet_count_: " << packet_count_;
    return ret;
}

int AvModuleManager::get_index_from_format_ctx(AVFormatContext *format_ctx, int media_type)
{
    for (int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == media_type)
        {
            return i;
        }
    }
    return -1;
}

int AvModuleManager::get_info_from_format_ctx(
    AVFormatContext *format_ctx,
    int index,
    AVStream **stream,
    AVCodecContext **codec_ctx)
{
    std::unordered_map<int, AVCodecContext *> codec_ctx_map;
    std::pair<int, AVCodecContext *> codec_idx_ctx;
    AVCodecContext *video_ctx = NULL;
    if (index >= 0 && index < format_ctx->nb_streams)
    {
        if (stream)
        {
            *stream = format_ctx->streams[index];
        }
        if (codec_ctx)
        {
            *codec_ctx = format_ctx->streams[index]->codec;
        }
        return index;
    }
    else
    {
        return -1;
    }
}

/* std::pair<int, AVCodecContext*> AvModuleManager::get_codec_ctx_from_format_ctx(AVFormatContext* format_ctx, int media_type) {
    std::pair<int, AVCodecContext*> codec_idx_ctx;
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == media_type) {
            video_ctx = format_ctx->streams[i]->codec;
            codec_idx_ctx.first = i;
            codec_idx_ctx.second = format_ctx->streams[i]->codec;
            break;
        }
    }
    return codec_idx_ctx;
} */

/* std::pair<int, AVCodecContext*> AvModuleManager::get_video_codec_ctx_from_format_ctx(AVFormatContext* format_ctx) {
    std::unordered_map<int, AVCodecContext*> codec_ctx_map;
    std::pair<int, AVCodecContext*> codec_idx_ctx;
    AVCodecContext* video_ctx = NULL;
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_ctx = format_ctx->streams[i]->codec;
            codec_idx_ctx.first = i;
            codec_idx_ctx.second = format_ctx->streams[i]->codec;
            break;
        }
    }
    return codec_idx_ctx;
}

std::pair<int, AVCodecContext*> AvModuleManager::get_audio_codec_ctx_from_format_ctx(AVFormatContext* format_ctx) {
    std::unordered_map<int, AVCodecContext*> codec_ctx_map;
    std::pair<int, AVCodecContext*> codec_idx_ctx;
    AVCodecContext* audio_ctx = NULL;
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_ctx = format_ctx->streams[i]->codec;
            codec_idx_ctx.first = i;
            codec_idx_ctx.second = format_ctx->streams[i]->codec;
            break;
        }
    }
    return codec_idx_ctx;
} */

std::unordered_map<int, AVCodecContext *> AvModuleManager::get_codec_ctx_from_format_ctx(AVFormatContext *format_ctx)
{
    std::unordered_map<int, AVCodecContext *> codec_ctx_map;
    bool get_video_stream = false;
    for (int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && !get_video_stream)
        {
            get_video_stream = true;
            codec_ctx_map[i] = format_ctx->streams[i]->codec;
        }
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            codec_ctx_map[i] = format_ctx->streams[i]->codec;
        }
    }
    return codec_ctx_map;
}

std::unordered_map<int, AVCodecParameters *> AvModuleManager::get_codec_par_from_format_ctx(AVFormatContext *format_ctx)
{
    std::unordered_map<int, AVCodecParameters *> codec_par_map;
    bool get_video_stream = false;
    for (int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && !get_video_stream)
        {
            get_video_stream = true;
            codec_par_map[i] = format_ctx->streams[i]->codecpar;
        }
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            codec_par_map[i] = format_ctx->streams[i]->codecpar;
        }
    }
    return codec_par_map;
}

std::unordered_map<int, AVRational> AvModuleManager::get_time_base_from_format_ctx(AVFormatContext *format_ctx)
{
    std::unordered_map<int, AVRational> time_base_map;
    bool get_video_stream = false;
    for (int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && !get_video_stream)
        {
            get_video_stream = true;
            time_base_map[i] = format_ctx->streams[i]->time_base;
        }
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            time_base_map[i] = format_ctx->streams[i]->time_base;
        }
    }
    return time_base_map;
}

void AvModuleManager::caculate_target_parameters(AVFormatContext *format_ctx, bool video_force_transcode, bool audio_force_transcode)
{
    AVStream *stream;
    AVCodecContext *codec_ctx;

    // VIDEO
    int video_index = get_index_from_format_ctx(format_ctx, AVMEDIA_TYPE_VIDEO);
    bool video_allow_bypass = true;
    get_info_from_format_ctx(format_ctx, video_index, &stream, &codec_ctx);

    int src_width = stream->codec->width;
    int src_height = stream->codec->height;
    int short_side = parameter_["dst_parameter"].as_object()["parameter"].as_object()["resolution"].as_int64();
    if (src_width >= src_height && src_height != short_side)
    {
        video_allow_bypass = false;
    }
    if (src_width < src_height && src_width != short_side)
    {
        video_allow_bypass = false;
    }
    if (src_width > src_height)
    {
        tar_height_ = short_side;
        tar_width_ = src_width * tar_height_ / src_height;
    }
    else
    {
        tar_width_ = short_side;
        tar_height_ = src_height * tar_width_ / src_width;
    }
    if (tar_width_ % 2)
    {
        tar_width_++;
    }
    if (tar_height_ % 2)
    {
        tar_height_++;
    }

    int src_frame_rate = stream->codec->framerate.num;
    tar_frame_rate_ = parameter_["dst_parameter"].as_object()["parameter"].as_object()["frame_rate"].as_int64();
    if (tar_frame_rate_ != 0 && tar_frame_rate_ != src_frame_rate)
    {
        video_allow_bypass = false;
    }
    if (tar_frame_rate_ == 0)
    {
        tar_frame_rate_ = src_frame_rate;
    }

    int src_bit_rate = stream->codec->bit_rate;
    tar_video_bitrate_ = parameter_["dst_parameter"].as_object()["parameter"].as_object()["video_bitrate"].as_int64();
    if (!tar_video_bitrate_ && src_bit_rate)
        tar_video_bitrate_ = src_bit_rate;
    else if (!tar_video_bitrate_)
        tar_video_bitrate_ = 1000000;
    if (src_bit_rate && abs(tar_video_bitrate_ - src_bit_rate) / (float)src_bit_rate > 0.05)
    {
        video_allow_bypass = false;
    }
    else if (!src_bit_rate)
    {
        video_allow_bypass = false;
    }

    AVCodecID src_video_codec_id = stream->codecpar->codec_id; // AV_CODEC_ID_H264;
    std::string src_video_format = avcodec_get_name(src_video_codec_id);
    tar_video_encoder_ = parameter_["dst_parameter"].as_object()["video_format"].as_string().c_str();
    if (!((tar_video_encoder_ == "h264" || tar_video_encoder_ == "h265") && tar_video_encoder_ == src_video_format))
    {
        video_allow_bypass = false;
    }
    if ("h264" == tar_video_encoder_)
    {
        video_encoder_name_ = "libx264";
    }
    else if ("h265" == tar_video_encoder_)
    {
        video_encoder_name_ = "libx265";
    }
    else
    {
        video_encoder_name_ = "libx264";
    }
    if (!video_force_transcode && video_allow_bypass)
    {
        video_bypass_ = true;
    }
    else
    {
        video_bypass_ = false;
    }

    // AUDIO
    int audio_index = get_index_from_format_ctx(format_ctx, AVMEDIA_TYPE_AUDIO);
    bool audio_allow_bypass = true;
    get_info_from_format_ctx(format_ctx, audio_index, &stream, &codec_ctx);
    /*
            int time_base,
            int frame_size = 1024,
            int bit_rate = 128000,
            AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16,
            int sample_rate = 44100,
            int channels = 2,
            int channel_layout = AV_CH_LAYOUT_STEREO,
            int profile = FF_PROFILE_AAC_MAIN
       */

    int src_frame_size, src_channels, src_channel_layout;

    AVCodecID src_audio_codec_id = stream->codecpar->codec_id;
    std::string src_audio_format = avcodec_get_name(src_audio_codec_id);
    tar_audio_encoder_ = parameter_["dst_parameter"].as_object()["audio_format"].as_string().c_str();
    tar_audio_encoder_ = "aac"; // 目前只支持aac
    if (tar_audio_encoder_ != "aac" || src_audio_format != "aac")
    {
        audio_allow_bypass = false;
    }

    int src_audio_bit_rate = stream->codec->bit_rate;
    tar_audio_bitrate_ = parameter_["dst_parameter"].as_object()["parameter"].as_object()["audio_bitrate"].as_int64();
    if (tar_audio_bitrate_ < 10 * 1000)
    {
        tar_audio_bitrate_ = 10 * 1000;
    }
    if (src_audio_bit_rate && abs(tar_audio_bitrate_ - src_audio_bit_rate) / src_audio_bit_rate > 0.05)
    {
        audio_allow_bypass = false;
    }

    int src_sample_rate = stream->codec->sample_rate;
    tar_sample_rate_ = 24000; // src_sample_rate;

    AVSampleFormat src_sample_fmt = stream->codec->sample_fmt;
    if (src_sample_fmt)
    {
        // aac 支持什么sample_fmt？
    }
    tar_sample_fmt_ = AV_SAMPLE_FMT_FLTP; //; //AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16

    audio_encoder_name_ = "aac"; //"aac";

    return;
}