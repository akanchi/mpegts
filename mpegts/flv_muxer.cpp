//
// Created by akanchi on 2019-12-31.
//

#include "flv_muxer.h"
#include "ts_packet.h"
#include "simple_buffer.h"
#include "../amf0/amf0/amf0.h"

static const int FLV_TAG_HEADER_SIZE = 11;

class FLVTagType {
public:
    static const uint8_t mAudio = 8;
    static const uint8_t mVideo = 9;
    static const uint8_t mScriptData = 18;
};

class NALU {
public:
    int mType;
    uint32_t mSize;
    char *mpData;
};

inline int getNalu(NALU &rNalu, char *pData, uint32_t lSize, uint32_t lIndex) {
    rNalu.mType = 0;
    int lRet = -1;
    uint32_t lI = lIndex;

    while (lI < lSize) {
        if (pData[lI++] == 0x00
            && pData[lI++] == 0x00
            && pData[lI++] == 0x00
            && pData[lI++] == 0x01) {
            int lPos = lI;

            while (lPos < lSize) {
                if (pData[lPos++] == 0x00
                    && pData[lPos++] == 0x00
                    && pData[lPos++] == 0x00
                    && pData[lPos++] == 0x01) {
                    break;
                }
            }
            uint32_t lNaluSize = 0;
            if (lPos == lSize) {
                lNaluSize = lPos - lI;
                lRet = lPos;
            } else if (lPos > lSize) {
                lNaluSize = lSize - lI;
                lRet = lSize;
            } else {
                lNaluSize = (lPos - 4) - lI;
                lRet = lPos - 4;
            }
            rNalu.mType = pData[lI] & 0x1f;
            rNalu.mSize = lNaluSize;
            rNalu.mpData = &pData[lI];

            return lRet;
        }
    }
    return lRet;
}

FLVMuxer::FLVMuxer()
        : mHasSetStartPts(false), mStartPts(0), mDuration(0) {

}

FLVMuxer::~FLVMuxer() {

}

int FLVMuxer::writeHeader(SimpleBuffer *pSb) {
    pSb->write_1byte('F');   // Signature
    pSb->write_1byte('L');
    pSb->write_1byte('V');
    pSb->write_1byte(0x01);  // version
    pSb->write_1byte(0x05);  // Audio and Video tags are present
    pSb->write_4bytes(
            0x00000009);   // DataOffset, Offset in bytes from start of file to start of body (that is, size of header)
    pSb->write_4bytes(0x00000000);   // PreviousTagSize0, Always 0
    return 0;
}

int FLVMuxer::writeBody(TsFrame *pFrame, SimpleBuffer *pSb) {
    if (pFrame->mStreamType == MpegTsStream::AAC) {
        writeAacTag(pFrame, pSb);
    } else if (pFrame->mStreamType == MpegTsStream::AVC) {
        writeAvcTag(pFrame, pSb);
    }

    calcDuration(pFrame->mPts / 90);

    return 0;
}

int FLVMuxer::writeAacTag(TsFrame *pFrame, SimpleBuffer *pSb) {
    uint32_t lBodySize = 2 + pFrame->mData->size();
    uint32_t lPts = pFrame->mPts / 90;
    pSb->write_1byte(FLVTagType::mAudio);
    pSb->write_3bytes(lBodySize);
    pSb->write_3bytes(lPts);
    pSb->write_1byte((lPts >> 24) & 0xff);
    pSb->write_3bytes(0x00);
    pSb->write_1byte(0xaf);
    pSb->write_1byte(0x01);
    pSb->append(pFrame->mData->data(), pFrame->mData->size());
    pSb->write_4bytes(lBodySize + FLV_TAG_HEADER_SIZE);

    return 0;
}

int FLVMuxer::writeAvcTag(TsFrame *pFrame, SimpleBuffer *pSb) {
    int lIndex = 0;
    uint32_t lPts = pFrame->mPts / 90;
    uint32_t lDts = pFrame->mDts / 90;
    uint32_t lCts = lPts - lDts;
    uint32_t lSize = pFrame->mData->size();
    NALU lNalu;
    do {
        lIndex = getNalu(lNalu, pFrame->mData->data(), lSize, lIndex);
        if (lNalu.mType == 7) {
            mSpsData.erase(mSpsData.size());
            mSpsData.append(lNalu.mpData, lNalu.mSize);
        } else if (lNalu.mType == 8 && mSpsData.size() > 0) {
            mPpsData.erase(mPpsData.size());
            mPpsData.append(lNalu.mpData, lNalu.mSize);

            uint32_t lBodySize = 13 + mSpsData.size() + 3 + mPpsData.size();
            pSb->write_1byte(FLVTagType::mVideo);
            pSb->write_3bytes(lBodySize);
            pSb->write_3bytes(lDts);
            pSb->write_1byte((lDts >> 24) & 0xff);
            pSb->write_3bytes(0x00);

            pSb->write_1byte(0x17);
            pSb->write_4bytes(0x00);
            pSb->write_1byte(0x01);
            pSb->append(mSpsData.data() + 1, 3);
            pSb->write_1byte(0xff);
            pSb->write_1byte(0xe1);
            pSb->write_2bytes(mSpsData.size());
            pSb->append(mSpsData.data(), mSpsData.size());

            pSb->write_1byte(0x01);
            pSb->write_2bytes(mPpsData.size());
            pSb->append(mPpsData.data(), mPpsData.size());

            pSb->write_4bytes(FLV_TAG_HEADER_SIZE + lBodySize);
        } else if ((lNalu.mType == 1 || lNalu.mType == 5) && (mSpsData.size() > 0 && mPpsData.size() > 0)) {
            bool isKeyFrame = lNalu.mType == 0x05;

            uint32_t lBodySize = 9 + lNalu.mSize;
            pSb->write_1byte(FLVTagType::mVideo);
            pSb->write_3bytes(lBodySize);
            pSb->write_3bytes(lDts);
            pSb->write_1byte((lDts >> 24) & 0xff);
            pSb->write_3bytes(0x00);

            pSb->write_1byte(isKeyFrame ? 0x17 : 0x27);
            pSb->write_1byte(0x01);
            pSb->write_3bytes(lCts);

            pSb->write_4bytes(lNalu.mSize);
            pSb->append(lNalu.mpData, lNalu.mSize);

            pSb->write_4bytes(FLV_TAG_HEADER_SIZE + lBodySize);
        }
    } while (lIndex < lSize && lIndex > 0);

    return 0;
}

void FLVMuxer::calcDuration(uint32_t lPts) {
    if (!mHasSetStartPts) {
        mHasSetStartPts = true;
        mStartPts = lPts;
    }

    mDuration = lPts - mStartPts;
}

int FLVMuxer::writeMetadata(SimpleBuffer *pSb, uint32_t lFileSize) {
    SimpleBuffer lBodySb;

    Amf0String lAmfOnMetaData("onMetaData");
    lAmfOnMetaData.write(&lBodySb);

    Amf0EcmaArray lAmfEcmaArray;
    lAmfEcmaArray.put("duration", new Amf0Number(mDuration / 1000.));
    lAmfEcmaArray.put("filesize", new Amf0Number(lFileSize));
    lAmfEcmaArray.write(&lBodySb);

    pSb->write_1byte(FLVTagType::mScriptData);
    pSb->write_3bytes(lBodySb.size());
    pSb->write_3bytes(0);
    pSb->write_1byte(0);
    pSb->write_3bytes(0x00);

    pSb->append(lBodySb.data(), lBodySb.size());

    return 0;
}

