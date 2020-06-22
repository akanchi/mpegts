#pragma once

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// l local scope

#include <stdint.h>

class MpegTsAdaptationFieldType {
public:
    // Reserved for future use by ISO/IEC
    static const uint8_t mReserved = 0x00;
    // No adaptation_field, payload only
    static const uint8_t mPayloadOnly = 0x01;
    // Adaptation_field only, no payload
    static const uint8_t mAdaptionOnly = 0x02;
    // Adaptation_field followed by payload
    static const uint8_t mPayloadAdaptionBoth = 0x03;
};

class SimpleBuffer;

extern void writePcr(SimpleBuffer *pSb, uint64_t lPcr);

extern void writePts(SimpleBuffer *pSb, uint32_t lFb, uint64_t lPts);

extern uint64_t readPts(SimpleBuffer *pSb);

extern uint64_t readPcr(SimpleBuffer *pSb);

