#pragma once

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// l local scope

#include <map>
#include <stdint.h>

class SimpleBuffer;

class TsFrame;

class MpegTsMuxer {
public:
    MpegTsMuxer();

    virtual ~MpegTsMuxer();

public:
    void createPat(SimpleBuffer *pSb, uint16_t lPmtPid, uint8_t lCc);

    void createPmt(SimpleBuffer *pSb, std::map<uint8_t, int> lStreamPidMap, uint16_t lPmtPid, uint8_t lCc);

    void createPes(TsFrame *pFrame, SimpleBuffer *pSb);

    void createPcr(SimpleBuffer *pSb);

    void createNull(SimpleBuffer *pSb);

    void encode(TsFrame *pFrame, std::map<uint8_t, int> lStreamPidMap, uint16_t lPmtPid, SimpleBuffer *pSb);

private:
    uint8_t getCc(uint32_t lWithPid);

    bool shouldCreatePat();

private:
    std::map<uint32_t, uint8_t> mPidCcMap;
};

