#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <csignal>
#include <cstdlib>
/* #include <boost/json.hpp>
#include <boost/json/src.hpp> */
#include "av_module_manager.h"
#include "lz_utils.h"
#include "lz_log.h"
#include "my_config.h"

// boost library hpp. Include only once globally.
#include <boost/json/src.hpp>

#include "ffmpeg_probe.h"

// spdlog
#include <spdlog/spdlog.h>
#include "spdlog/cfg/env.h" // for loading levels from the environment variable
#include "spdlog/sinks/basic_file_sink.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

void init_spdlog()
{

    time_t raw_time;
    struct tm *time_info;
    std::stringstream ss;
    std::string str_time;

    time(&raw_time);
    time_info = localtime(&raw_time);
    ss << asctime(time_info);
    str_time = ss.str();
    for (auto &&t : str_time)
    {
        if (t == ' ' || t == '\n')
            t = '_';
        if (t == ':')
            t = '-';
    }

    // Set the default logger to file logger
    auto file_logger = spdlog::basic_logger_mt("basic_logger",
                                               std::string("logs/") + std::string("Spdlog_") + str_time + std::string(".log"));
    spdlog::set_default_logger(file_logger);
    spdlog::set_level(spdlog::level::info);
    // include/spdlog/pattern_formatter-inl.h 定义了pattern flag的含义
    // 或者参考 https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    // %@ %s %g %# %! 一定要用 SPDLOG_TRACE(..), SPDLOG_INFO(...) etc，而不是 spdlog::trace(...)
    spdlog::set_pattern("[%H:%M:%S][%^%l%$][%t][%!][%s:%#] %v");

    SPDLOG_INFO("init_spdlog");
    SPDLOG_INFO("You would get file name and line number.");

    // 使用案例参考 spdlog/example/example.cpp
    // SPDLOG_WARN("Easy padding in numbers like {:08d}", 12);
    // SPDLOG_CRITICAL("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    // SPDLOG_INFO("Support for floats {:03.2f}", 1.23456);
    // SPDLOG_INFO("Positional args are {1} {0}..", "too", "supported");
    // SPDLOG_INFO("{:>8} aligned, {:<8} aligned", "right", "left");
    return;
}

void ffmpeg_spdlog(void *avcl, int level, const char *fmt, va_list vl)
{
#define FFMPEG_SPDLOG
#ifdef FFMPEG_SPDLOG
    const int size = 1024;
    static int flag = av_log_get_flags();
    static char s_cache[size];
    char s[size], out[size];
    if (level > flag)
        return;

    memset(s, 0, size);
    vsnprintf(s, size - 1, fmt, vl);
    /* Maybe the end char is not '\n'. It means the line not finish. */
    if (strlen(s) > 0 && s[strlen(s) - 1] != '\n')
    {
        snprintf(s_cache + strlen(s_cache), size - strlen(s_cache) - 1, s);
        return;
    }

    memset(out, 0, size);
    switch (level)
    {
    case AV_LOG_QUIET:
        snprintf(out, size - 1, "AV_LOG_QUIET. ");
        break;
    case AV_LOG_PANIC:
        snprintf(out, size - 1, "AV_LOG_PANIC. ");
        break;
    case AV_LOG_FATAL:
        snprintf(out, size - 1, "AV_LOG_FATAL. ");
        break;
    case AV_LOG_ERROR:
        snprintf(out, size - 1, "AV_LOG_ERROR. ");
        break;
    case AV_LOG_WARNING:
        snprintf(out, size - 1, "AV_LOG_WARNING. ");
        break;
    case AV_LOG_INFO:
        snprintf(out, size - 1, "AV_LOG_INFO. ");
        break;
    case AV_LOG_VERBOSE:
        snprintf(out, size - 1, "AV_LOG_VERBOSE. ");
        break;
    case AV_LOG_DEBUG:
        snprintf(out, size - 1, "AV_LOG_DEBUG. ");
        break;
    case AV_LOG_TRACE:
        snprintf(out, size - 1, "AV_LOG_TRACE. ");
        break;
    default:
        break;
    }
    if (strlen(s_cache) > 0)
    {
        snprintf(out + strlen(out), size - strlen(out) - 1, s_cache);
        memset(s_cache, 0, size);
    }
    snprintf(out + strlen(out), size - strlen(out) - 1, s);
    if (strlen(out) > 0)
    {
        out[strlen(out) - 1] = '\0';
    }
    // spdlog::log, spdlog::trace 等接口，不打印文件名、行号、函数名
    spdlog::log(spdlog::level::level_enum::off, (out));
#endif
    return;
}

void signalHandler(int signum)
{
    SPDLOG_INFO("Interrupt signal ({}) received.", signum);
    // 清理并关闭
    // 终止程序
    exit(signum);
}

FfmpegProbe::FfmpegProbe()
{
    return ;
}

FfmpegProbe ::~FfmpegProbe()
{
    if (demuxer_) {
        delete demuxer_;
    }
    return;
}

int FfmpegProbe::Open(std::string media_file)
{
    int ret;
    AVFormatContext *pFormatContext = NULL;
    src_url_ = media_file; //parameter_["src_url"].as_string().c_str();
    // out_url_ = dst_url;

    // demuxer
    demuxer_ = new CDemuxer(nullptr);
    ;
    ret = demuxer_->Open(src_url_, 0, 0, "", "", &pFormatContext);
    if (ret < 0)
    {
        SPDLOG_INFO("Demuxer open failed: ", ret);
        open_ret_ = ret;
        return -1;
    }
    open_ret_ = ret;
    bitrate_ = pFormatContext->bit_rate;
    duration_ = pFormatContext->duration;
    SPDLOG_INFO("Bitrate: {}", pFormatContext->bit_rate);
    SPDLOG_INFO("Duration: {}", pFormatContext->duration);

    AVStream *stream;
    AVCodecContext *codec_ctx;
    int video_index = get_index_from_format_ctx(pFormatContext, AVMEDIA_TYPE_VIDEO);
    if (video_index >= 0)
    {
        get_info_from_format_ctx(pFormatContext, video_index, &stream, &codec_ctx);
        video_index_ = video_index;
        video_format_ = avcodec_get_name((AVCodecID)pFormatContext->streams[video_index]->codecpar->codec_id);
        video_bitrate_ = pFormatContext->streams[video_index]->codecpar->bit_rate;
        width_ = stream->codec->width;
        height_ = stream->codec->height;
        AVRational tb = stream->codec->time_base;
    } else {
        SPDLOG_INFO("get_index_from_format_ctx return: {}", ret);
        video_index_ = -1;
        video_format_ = "";
        video_bitrate_ = 0;
        width_ = 0;
        height_ = 0;
    }
    int audio_index = get_index_from_format_ctx(pFormatContext, AVMEDIA_TYPE_AUDIO);
    if (audio_index >= 0)
    {
        get_info_from_format_ctx(pFormatContext, audio_index, &stream, &codec_ctx);
        audio_index_ = audio_index;
        audio_format_ = avcodec_get_name((AVCodecID)pFormatContext->streams[audio_index]->codecpar->codec_id);
        audio_bitrate_ = pFormatContext->streams[audio_index]->codecpar->bit_rate;
    } else {
        audio_index_ = -1;
        audio_format_ = "";
        audio_bitrate_ = 0;
    }

    duration_ = pFormatContext->duration * 1000 / AV_TIME_BASE;
    ;
    SPDLOG_INFO("iformat->name: {}", std::string(pFormatContext->iformat->name));
    SPDLOG_INFO("iformat->long_name: {}", std::string(pFormatContext->iformat->long_name));
    SPDLOG_INFO("iformat->extensions: {}", std::string(pFormatContext->iformat->extensions));
    format_ = std::string(pFormatContext->iformat->name);
    return 0;
}

int FfmpegProbe::get_info_from_format_ctx(
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

int FfmpegProbe::get_index_from_format_ctx(AVFormatContext *format_ctx, int media_type)
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

void init_ffmpeg_probe()
{
    init_spdlog();
    SPDLOG_INFO("Hello FFmpeg!");

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        SPDLOG_WARN("Can't catch the signal.");
    }
    if (signal(SIGKILL, signalHandler) == SIG_ERR)
    {
        SPDLOG_WARN("Can't catch the signal.");
    }

    av_log_set_flags(AV_LOG_TRACE); // AV_LOG_INFO  AV_LOG_DEBUG   AV_LOG_TRACE
    // av_log_set_callback(&ffmpeglog);
    av_log_set_callback(&ffmpeg_spdlog);
    av_register_all();
    avcodec_register_all();
}

void uninit_ffmpeg_probe()
{
    spdlog::shutdown();
}

void* open_media(char *media_file)
{
    SPDLOG_INFO("open media: {}", media_file);
    FfmpegProbe *fm = new FfmpegProbe();
    fm->Open(std::string(media_file));
    return (void*)fm;
}

void close_media(void *fm)
{
    if(fm)
    {
        delete (FfmpegProbe*)fm;
    }
}

const char *getFormat(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe*)fm)->format_.c_str();
    } 
    else 
    {
        return NULL;
    }
}

long long getBitrate(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->bitrate_;
    }
    else
    {
        return 0;
    }
}

long long getDuration(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->duration_;
    }
    else
    {
        return 0;
    }
}

int getWidth(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->width_;
    }
    else
    {
        return 0;
    }
}

int getHeight(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->height_;
    }
    else
    {
        return 0;
    }
}

const char *getVideoFormat(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->video_format_.c_str();
    }
    else
    {
        return NULL;
    }
}

int getVideoIndex(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->video_index_;
    }
    else
    {
        return -1;
    }
}

int getVideoBitrate(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->video_bitrate_;
    }
    else
    {
        return 0;
    }
}

const char *getAudioFormat(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->audio_format_.c_str();
    }
    else
    {
        return NULL;
    }
}

int getAudioIndex(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->audio_index_;
    }
    else
    {
        return -1;
    }
}

int getAudioBitrate(void *fm)
{
    if (fm)
    {
        return ((FfmpegProbe *)fm)->audio_bitrate_;
    }
    else
    {
        return 0;
    }
}