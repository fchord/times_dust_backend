#pragma once
#include <vector>
#include "frame_wrapper.h"
#include "packet_wrapper.h"

using namespace std;

class AvModule
{
public:
    AvModule();
    ~AvModule();
    virtual void SetNextAvModule(AvModule *m) { return; };
    virtual int Process() { return 0; };
    virtual int Process(CFrameWrapper &frame) { return 0; };
    virtual int Process(CPacketWrapper &packet) { return 0; };

protected:
    uint av_count_ = 0;

private:
    // std::vector<AvModule*> next_;
};