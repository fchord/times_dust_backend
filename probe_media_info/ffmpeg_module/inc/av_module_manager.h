#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <boost/json.hpp>
#include "demuxer.h"
#include "muxer.h"
#include "avmodule.h"
#include "listener.h"
#include "frame_devourer.h"
#include "filter.h"

class AvModuleManager : public IListener
{
public:
    AvModuleManager(boost::json::object &parameter);
    ~AvModuleManager();
    int Start_1(std::string dst_url, bool video_bypass, bool audio_bypass);
    int Start(std::string dst_url, bool video_bypass, bool audio_bypass);
    int Stop();
    void OnProgressReport(float fProgress);
    void OnFrameReport(void *pFrame);
    void OnErrorReport(string sError);

private:
    boost::json::object parameter_;

    int work();
    std::thread *work_thread_;
    bool quit_ = false;
    std::atomic<uint> packet_count_{0};

    int get_index_from_format_ctx(AVFormatContext *format_ctx, int media_type);
    int get_info_from_format_ctx(AVFormatContext *format_ctx, int index, AVStream **stream, AVCodecContext **codec_ctx);

    std::unordered_map<int, AVCodecContext *> get_codec_ctx_from_format_ctx(AVFormatContext *format_ctx);
    std::unordered_map<int, AVCodecParameters *> get_codec_par_from_format_ctx(AVFormatContext *format_ctx);
    std::unordered_map<int, AVRational> get_time_base_from_format_ctx(AVFormatContext *format_ctx);

    void caculate_target_parameters(AVFormatContext *format_ctx, bool video_force_transcode, bool audio_force_transcode);

    // video target parameters
    bool video_bypass_;
    int tar_width_, tar_height_, tar_frame_rate_, tar_video_bitrate_;
    std::string tar_video_encoder_, video_encoder_name_;
    // audio target parameters
    bool audio_bypass_;
    int tar_sample_rate_, tar_audio_bitrate_;
    AVSampleFormat tar_sample_fmt_;
    std::string tar_audio_encoder_, audio_encoder_name_;

    CDemuxer *demuxer_;
    std::string src_url_{""};

    std::pair<int, AvModule *> video_idx_module_{-1, nullptr};
    std::unordered_map<int, AvModule *> audio_idx_module_{};
    CFrameDevourer *frame_devourer_ = NULL;

    CMuxer *muxer_;
    std::string out_url_{""};

    CFilter *audio_filter_;
};