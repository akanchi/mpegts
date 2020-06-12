#pragma once

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// l local scope

#include <stdint.h>

class SimpleBuffer;

extern void writePcr(SimpleBuffer *pSb, uint64_t lPcr);

extern void writePts(SimpleBuffer *pSb, uint32_t lFb, uint64_t lPts);

extern uint64_t readPts(SimpleBuffer *pSb);

extern uint64_t readPcr(SimpleBuffer *pSb);

