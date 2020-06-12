#pragma once

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// l local scope

#include "ts_packet.h"
#include <stdint.h>
#include <memory>
#include <map>

class SimpleBuffer;

class TsFrame;

class MpegTsDemuxer {
public:
    MpegTsDemuxer();

    virtual ~MpegTsDemuxer();

public:
    int decode(SimpleBuffer *pIn, TsFrame *&prOut);

    // stream, pid
    std::map<uint8_t, int> mStreamPidMap;
    int mPmtId;

    // PAT
    PATHeader mPatHeader;
    bool mPatIsValid = false;

    // PMT
    PMTHeader mPmtHeader;
    bool mPmtIsValid = false;

private:
    // pid, frame
    std::map<int, std::shared_ptr<TsFrame>> mTsFrames;
    int mPcrId;
};

