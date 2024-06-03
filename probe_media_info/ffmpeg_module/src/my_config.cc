#include <iostream>
#include <string>
#include <sstream>
#include <spdlog/spdlog.h>
#include "my_config.h"

CMyConfig *CMyConfig::instance = nullptr;

CMyConfig *CMyConfig::GetInstance()
{
    if (!CMyConfig::instance)
    {
        CMyConfig::instance = new CMyConfig();
    }
    return CMyConfig::instance;
}

int CMyConfig::ParseInputParameter(std::string input_parameter, boost::json::object &transcode_parameter)
{
    std::stringstream ss;
    boost::json::error_code ec;
    boost::json::value config = boost::json::parse(input_parameter, ec);
    if (ec)
    {
        SPDLOG_ERROR("Parsing failed: {}", ec.message());
        return -1;
    }

    SPDLOG_INFO("config kind: {}", config.kind());
    ss.str("");
    ss.clear();
    ss << config;
    SPDLOG_INFO("config: {}", ss.str());
    if (config.get_object().find("src_url") != config.get_object().end())
    {
        SPDLOG_INFO("config.src_url: {}", config.get_object()["src_url"].as_string().c_str());
        if (config.get_object()["src_url"].is_null())
        {
            return -2;
        }
        transcode_parameter["src_url"] = config.get_object()["src_url"];
        // SPDLOG_INFO("src_url kind: {}", transcode_parameter["src_url"].kind());
    }
    if (config.get_object().find("dst_parameter") != config.get_object().end() && config.get_object()["dst_parameter"].is_object())
    {
        boost::json::value dst = config.get_object()["dst_parameter"];
        SPDLOG_INFO("config.get_object dst_parameter kind: {}", config.get_object()["dst_parameter"].kind());
        ss.str("");
        ss.clear();
        ss << config.get_object()["dst_parameter"];
        SPDLOG_INFO("config.get_object dst_parameter: {}", ss.str().c_str());
        transcode_parameter["dst_parameter"] = {}; // 先构造一个object对象，否则kind是null. 或者用boost::json::object()初始化
        // SPDLOG_INFO("transcode_parameter dst_parameter kind: {}", transcode_parameter["dst_parameter"].kind());

        //// mux_format
        SPDLOG_INFO("config.dst_parameter.mux_format: {}", dst.get_object()["mux_format"].as_string().c_str());
        if ("mp4" == dst.get_object()["mux_format"] || "mpegts" == dst.get_object()["mux_format"] || "hls" == dst.get_object()["mux_format"] || "dash" == dst.get_object()["mux_format"])
        {
            transcode_parameter["dst_parameter"].as_object()["mux_format"] = dst.get_object()["mux_format"];
        }
        else
        {
            SPDLOG_ERROR("mux_format error: {}", dst.get_object()["mux_format"].as_string().c_str());
            return -3;
        }

        //// video_format
        SPDLOG_INFO("config.dst_parameter.video_format: {}", dst.get_object()["video_format"].as_string().c_str());
        if ("h264" == dst.get_object()["video_format"])
        {
            transcode_parameter["dst_parameter"].as_object()["video_format"] = dst.get_object()["video_format"];
        }
        else
        {
            SPDLOG_ERROR("video_format error: {}", dst.get_object()["video_format"].as_string().c_str());
            return -4;
        }

        //// audio_format
        SPDLOG_INFO("config.dst_parameter.audio_format: {}", dst.get_object()["audio_format"].as_string().c_str());
        if ("aac" == dst.get_object()["audio_format"])
        {
            transcode_parameter["dst_parameter"].as_object()["audio_format"] = dst.get_object()["audio_format"];
        }
        else
        {
            SPDLOG_ERROR("audio_format error: {}", dst.get_object()["audio_format"].as_string().c_str());
            return -5;
        }

        //// parameter
        if (dst.get_object().find("parameter") != dst.get_object().end())
        {
            boost::json::value para = dst.get_object()["parameter"];
            int size = 0;
            if (para.is_array())
            {
                size = para.as_array().size();
            }
            else if (para.is_object())
            {
                size = 1;
            }
            else
            {
                return -6;
            }

            SPDLOG_INFO("size: {}", size);
            if (1 == size)
            {
                transcode_parameter["dst_parameter"].as_object()["parameter"] = boost::json::object();
            }
            else
            {
                transcode_parameter["dst_parameter"].as_object()["parameter"] = boost::json::array();
            }
            SPDLOG_INFO("transcode_parameter dst_parameter parameter kind: {}", transcode_parameter["dst_parameter"].as_object()["parameter"].kind());

            boost::json::object temp_obj;
            if (para.is_array())
            {
                int num = 0;
                for (auto it = para.as_array().begin(); it != para.as_array().end(); it++, num++)
                {
                    SPDLOG_INFO("config.dst_parameter.parameter[{}].resolution: {}", num, it->get_object()["resolution"].as_int64());
                    temp_obj["resolution"] = it->get_object()["resolution"];

                    SPDLOG_INFO("config.dst_parameter.parameter[{}].frame_rate: {} {}", num, it->get_object()["frame_rate"].as_int64());
                    temp_obj["frame_rate"] = it->get_object()["frame_rate"];

                    SPDLOG_INFO("config.dst_parameter.parameter[{}].video_bitrate: {}", num, it->get_object()["video_bitrate"].as_int64());
                    temp_obj["video_bitrate"] = it->get_object()["video_bitrate"];

                    SPDLOG_INFO("config.dst_parameter.parameter[{}].audio_bitrate: {}", num, it->get_object()["audio_bitrate"].as_int64());
                    temp_obj["audio_bitrate"] = it->get_object()["audio_bitrate"];

                    if (size > 1)
                    {
                        transcode_parameter["dst_parameter"].as_object()["parameter"].as_array().push_back(temp_obj);
                    }
                    else
                    {
                        transcode_parameter["dst_parameter"].as_object()["parameter"] = temp_obj;
                    }
                    temp_obj = {};
                }
            }
            else if (para.is_object())
            {
                ss.str("");
                ss.clear();
                ss << para.get_object();
                SPDLOG_INFO("config.dst_parameter.parameter: ", ss.str());

                // SPDLOG_INFO("config.dst_parameter.parameter.resolution: {}", para.get_object()["resolution"].to_string());
                temp_obj["resolution"] = para.get_object()["resolution"];

                // SPDLOG_INFO("config.dst_parameter.parameter.frame_rate: {}", para.get_object()["frame_rate"].to_string());
                temp_obj["frame_rate"] = para.get_object()["frame_rate"];

                // SPDLOG_INFO("config.dst_parameter.parameter.video_bitrate: {}", para.get_object()["video_bitrate"].to_string());
                temp_obj["video_bitrate"] = para.get_object()["video_bitrate"];

                // SPDLOG_INFO("config.dst_parameter.parameter.audio_bitrate: {}", para.get_object()["audio_bitrate"].to_string());
                temp_obj["audio_bitrate"] = para.get_object()["audio_bitrate"];

                transcode_parameter["dst_parameter"].as_object()["parameter"] = temp_obj;
                temp_obj = {};
            }
        }
    }
    // SPDLOG_INFO("transcode_parameter: {}", boost::json::to_string(boost::json::value(transcode_parameter)));
    ss.str("");
    ss.clear();
    ss << transcode_parameter;
    SPDLOG_INFO("transcode_parameter: {}", ss.str());

    return 0;
}

// const std::string CMyConfig::config_file = "my_transcode.xml";

CMyConfig::CMyConfig()
{

    return;
}

CMyConfig::~CMyConfig()
{

    return;
}
