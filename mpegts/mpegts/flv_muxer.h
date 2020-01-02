//
// Created by akanchi on 2019-12-31.
//

#ifndef MPEGTS_FLV_MUXER_H
#define MPEGTS_FLV_MUXER_H

#include "simple_buffer.h"

class TsFrame;

class FLVMuxer
{
public:
    FLVMuxer();
    virtual ~FLVMuxer();

public:
    int write_header(SimpleBuffer *sb);
    int write_body(TsFrame *frame, SimpleBuffer *sb);
    int write_metadata(SimpleBuffer *sb, uint32_t fileSize);

private:
    int write_aac_tag(TsFrame *frame, SimpleBuffer *sb);
    int write_avc_tag(TsFrame *frame, SimpleBuffer *sb);
    void calc_duration(uint32_t pts);

private:
    SimpleBuffer ppsData;
    SimpleBuffer spsData;
    bool has_set_start_pts;
    uint32_t start_pts;
    uint32_t duration;
};


#endif //MPEGTS_FLV_MUXER_H
