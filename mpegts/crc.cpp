#include "crc.h"

// @see http://www.stmc.edu.hk/~vincent/ffmpeg_0.4.9-pre1/libavformat/mpegtsenc.c
uint32_t crc32(const uint8_t *pData, int lLen) {
    int lI;
    uint32_t lCrc = 0xffffffff;

    for (lI = 0; lI < lLen; lI++)
        lCrc = (lCrc << 8) ^ crcTable[((lCrc >> 24) ^ *pData++) & 0xff];

    return lCrc;
}
