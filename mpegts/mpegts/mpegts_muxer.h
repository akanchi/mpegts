#pragma once

#include <map>
#include <stdint.h>

class SimpleBuffer;
class TsFrame;

class MpegTsMuxer
{
public:
    MpegTsMuxer();
    virtual ~MpegTsMuxer();

public:
    void create_pat(SimpleBuffer *sb, uint8_t cc);
    void create_pmt(SimpleBuffer *sb, std::map<uint8_t, int> stream_pid_map, uint8_t cc);
    void create_pes(TsFrame *frame, SimpleBuffer *sb);
    void create_pcr(SimpleBuffer *sb);
    void create_null(SimpleBuffer *sb);
    void encode(TsFrame *frame, std::map<uint8_t, int> stream_pid_map, SimpleBuffer *sb);

private:
    void write_pcr(SimpleBuffer *sb, uint64_t pcr);
    void write_pts(SimpleBuffer *sb, uint32_t fb, uint64_t pts);
    uint8_t get_cc(uint32_t with_pid);
    bool should_create_pat();

private:
    std::map<uint32_t, uint8_t> _pid_cc_map; 
};

