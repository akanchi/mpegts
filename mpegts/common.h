#ifndef MPEGTS_COMMON_H
#define MPEGTS_COMMON_H

#include <stdint.h>

class SimpleBuffer;

extern void write_pcr(SimpleBuffer *sb, uint64_t pcr);
extern void write_pts(SimpleBuffer *sb, uint32_t fb, uint64_t pts);

extern uint64_t read_pts(SimpleBuffer *sb);
extern uint64_t read_pcr(SimpleBuffer *sb);

#endif //MPEGTS_COMMON_H
