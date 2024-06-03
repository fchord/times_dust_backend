#include <spdlog/spdlog.h>
#include "frame_devourer.h"

CFrameDevourer::CFrameDevourer()
{
    return;
}

CFrameDevourer::~CFrameDevourer()
{
    return;
}

int CFrameDevourer::Process(CFrameWrapper &frame)
{

    if (frame.av_frame)
    {
        SPDLOG_INFO("frame pts: {}, pkt pts: {}", frame.av_frame->pts, frame.av_frame->pkt_pts);
    }
    return 0;
}