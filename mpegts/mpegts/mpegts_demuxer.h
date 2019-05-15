#pragma once

#include <stdint.h>
#include <memory>
#include <map>

class SimpleBuffer;
class TsFrame;

class MpegTsDemuxer
{
public:
    MpegTsDemuxer();
    virtual ~MpegTsDemuxer();

public:
    int decode(SimpleBuffer *in, TsFrame *&out);
    // stream, pid
    std::map<uint8_t, int> stream_pid_map;

private:
    // pid, frame
    std::map<int, std::shared_ptr<TsFrame>> _ts_frames;
    int _pmt_id;
    int _pcr_id;
};

