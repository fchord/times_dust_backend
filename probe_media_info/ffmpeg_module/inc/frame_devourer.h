#pragma once

#include "frame_wrapper.h"
#include "avmodule.h"

class CFrameDevourer : public AvModule
{
public:
    CFrameDevourer();
    ~CFrameDevourer();
    int Process(CFrameWrapper &frame);

private:
};