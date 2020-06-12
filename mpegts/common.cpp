#include "common.h"
#include "simple_buffer.h"

void writePcr(SimpleBuffer *pSb, uint64_t lPcr) {
    pSb->write_1byte((int8_t) (lPcr >> 25));
    pSb->write_1byte((int8_t) (lPcr >> 17));
    pSb->write_1byte((int8_t) (lPcr >> 9));
    pSb->write_1byte((int8_t) (lPcr >> 1));
    pSb->write_1byte((int8_t) (lPcr << 7 | 0x7e));
    pSb->write_1byte(0);
}

void writePts(SimpleBuffer *pSb, uint32_t lFb, uint64_t lPts) {
    uint32_t lVal;

    lVal = lFb << 4 | (((lPts >> 30) & 0x07) << 1) | 1;
    pSb->write_1byte((int8_t) lVal);

    lVal = (((lPts >> 15) & 0x7fff) << 1) | 1;
    pSb->write_2bytes((int16_t) lVal);

    lVal = (((lPts) & 0x7fff) << 1) | 1;
    pSb->write_2bytes((int16_t) lVal);
}

uint64_t readPts(SimpleBuffer *pSb) {
    uint64_t lPts = 0;
    uint32_t lVal = 0;
    lVal = pSb->read_1byte();
    lPts |= ((lVal >> 1) & 0x07) << 30;

    lVal = pSb->read_2bytes();
    lPts |= ((lVal >> 1) & 0x7fff) << 15;

    lVal = pSb->read_2bytes();
    lPts |= ((lVal >> 1) & 0x7fff);

    return lPts;
}

uint64_t readPcr(SimpleBuffer *pSb) {
    uint64_t lPcr = 0;
    uint64_t lVal = pSb->read_1byte();
    lPcr |= (lVal << 25) & 0x1FE000000;

    lVal = pSb->read_1byte();
    lPcr |= (lVal << 17) & 0x1FE0000;

    lVal = pSb->read_1byte();
    lPcr |= (lVal << 9) & 0x1FE00;

    lVal = pSb->read_1byte();
    lPcr |= (lVal << 1) & 0x1FE;

    lVal = pSb->read_1byte();
    lPcr |= ((lVal >> 7) & 0x01);

    pSb->read_1byte();

    return lPcr;
}