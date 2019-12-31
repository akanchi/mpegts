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

private:
    int write_aac_tag(TsFrame *frame, SimpleBuffer *sb);
    int write_avc_tag(TsFrame *frame, SimpleBuffer *sb);

private:
    SimpleBuffer ppsData;
    SimpleBuffer spsData;
};


#endif //MPEGTS_FLV_MUXER_H
