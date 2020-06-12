#include "mpegts_demuxer.h"

#include "simple_buffer.h"
#include "common.h"

MpegTsDemuxer::MpegTsDemuxer()
        : mPmtId(0), mPcrId(0) {

}

MpegTsDemuxer::~MpegTsDemuxer() {
}

int MpegTsDemuxer::decode(SimpleBuffer *pIn, TsFrame *&prOut) {

    bool g = pIn->empty();

    while (!pIn->empty()) {
        int lPos = pIn->pos();
        TsHeader lTsHeader;
        lTsHeader.decode(pIn);

        // found pat & get pmt pid
        if (lTsHeader.mPid == 0 && mPmtId == 0) {
            if (lTsHeader.mAdaptationFieldControl == 0x02 || lTsHeader.mAdaptationFieldControl == 0x03) {
                AdaptationFieldHeader lAdaptionField;
                lAdaptionField.decode(pIn);
                pIn->skip(lAdaptionField.mAdaptationFieldLength > 0 ? (lAdaptionField.mAdaptationFieldLength - 1) : 0);
            }

            if (lTsHeader.mAdaptationFieldControl == 0x01 || lTsHeader.mAdaptationFieldControl == 0x03) {
                if (lTsHeader.mPayloadUnitStartIndicator == 0x01) {
                    uint8_t lPointField = pIn->read_1byte();
                }

                mPatHeader.decode(pIn);
                pIn->read_2bytes();

                mPmtId = pIn->read_2bytes() & 0x1fff;
                mPatIsValid = true;
#ifdef DEBUG
                mPatHeader.print();
#endif
            }
        }

        // found pmt
        if (mTsFrames.empty() && mPmtId != 0 && lTsHeader.mPid == mPmtId) {
            if (lTsHeader.mAdaptationFieldControl == 0x02 || lTsHeader.mAdaptationFieldControl == 0x03) {
                AdaptationFieldHeader lAdaptionField;
                lAdaptionField.decode(pIn);
                pIn->skip(lAdaptionField.mAdaptationFieldLength > 0 ? (lAdaptionField.mAdaptationFieldLength - 1) : 0);
            }

            if (lTsHeader.mPayloadUnitStartIndicator == 0x01) {
                uint8_t lPointField = pIn->read_1byte();

                mPmtHeader.decode(pIn);
                mPcrId = mPmtHeader.mPcrPid;
                for (size_t lI = 0; lI < mPmtHeader.mInfos.size(); lI++) {
                    mTsFrames[mPmtHeader.mInfos[lI]->mElementaryPid] = std::shared_ptr<TsFrame>(
                            new TsFrame(mPmtHeader.mInfos[lI]->mStreamType));
                    mStreamPidMap[mPmtHeader.mInfos[lI]->mStreamType] = mPmtHeader.mInfos[lI]->mElementaryPid;
                }
                mPmtIsValid = true;
#ifdef DEBUG
                mPmtHeader.print();
#endif
            }
        }

        if (mTsFrames.find(lTsHeader.mPid) != mTsFrames.end()) {
            uint8_t lPcrFlag = 0;
            uint64_t lPcr = 0;
            if (lTsHeader.mAdaptationFieldControl == 0x02 || lTsHeader.mAdaptationFieldControl == 0x03) {
                AdaptationFieldHeader lAdaptionField;
                lAdaptionField.decode(pIn);
                int lAdaptFieldLength = lAdaptionField.mAdaptationFieldLength;
                lPcrFlag = lAdaptionField.mPcrFlag;
                if (lAdaptionField.mPcrFlag == 1) {
                    lPcr = readPcr(pIn);
                    lAdaptFieldLength -= 6;
                }
                pIn->skip(lAdaptFieldLength > 0 ? (lAdaptFieldLength - 1) : 0);
            }

            if (lTsHeader.mAdaptationFieldControl == 0x01 || lTsHeader.mAdaptationFieldControl == 0x03) {
                PESHeader lPesHeader;
                if (lTsHeader.mPayloadUnitStartIndicator == 0x01) {
                    if (mTsFrames[lTsHeader.mPid]->mCompleted) {
                        mTsFrames[lTsHeader.mPid]->reset();
                    }

                    if (!mTsFrames[lTsHeader.mPid]->empty()) {
                        mTsFrames[lTsHeader.mPid]->mCompleted = true;
                        mTsFrames[lTsHeader.mPid]->mPid = lTsHeader.mPid;
                        prOut = mTsFrames[lTsHeader.mPid].get();
                        pIn->skip(lPos - pIn->pos());
                        return mTsFrames[lTsHeader.mPid]->mStreamType;
                    }

                    lPesHeader.decode(pIn);
                    mTsFrames[lTsHeader.mPid]->mStreamId = lPesHeader.mStreamId;
                    mTsFrames[lTsHeader.mPid]->mExpectedPesPacketLength = lPesHeader.mPesPacketLength;
                    if (lPesHeader.mPtsDtsFlags == 0x02) {
                        mTsFrames[lTsHeader.mPid]->mPts = mTsFrames[lTsHeader.mPid]->mDts = readPts(pIn);
                    } else if (lPesHeader.mPtsDtsFlags == 0x03) {
                        mTsFrames[lTsHeader.mPid]->mPts = readPts(pIn);
                        mTsFrames[lTsHeader.mPid]->mDts = readPts(pIn);
                    }
                    if (lPesHeader.mPesPacketLength != 0x0000) {
                        if ((lPesHeader.mPesPacketLength - 3 - lPesHeader.mHeaderDataLength) >= 188 ||
                            (lPesHeader.mPesPacketLength - 3 - lPesHeader.mHeaderDataLength) < 0) {
                            mTsFrames[lTsHeader.mPid]->mData->append(pIn->data() + pIn->pos(), 188 - pIn->pos() - lPos);
                        } else {
                            mTsFrames[lTsHeader.mPid]->mData->append(pIn->data() + pIn->pos(),
                                                                     lPesHeader.mPesPacketLength - 3 -
                                                                     lPesHeader.mHeaderDataLength);
                        }

                        break;
                    }
                }

                if (mTsFrames[lTsHeader.mPid]->mExpectedPesPacketLength != 0 &&
                    mTsFrames[lTsHeader.mPid]->mData->size() + 188 - pIn->pos() - lPos >
                    mTsFrames[lTsHeader.mPid]->mExpectedPesPacketLength) {
                    mTsFrames[lTsHeader.mPid]->mData->append(pIn->data() + pIn->pos(),
                                                             mTsFrames[lTsHeader.mPid]->mExpectedPesPacketLength -
                                                             mTsFrames[lTsHeader.mPid]->mData->size());
                } else {
                    mTsFrames[lTsHeader.mPid]->mData->append(pIn->data() + pIn->pos(), 188 - pIn->pos() - lPos);
                }

            }
        } else if (mPcrId != 0 && mPcrId == lTsHeader.mPid) {
            AdaptationFieldHeader lAdaptField;
            lAdaptField.decode(pIn);
            uint64_t lPcr = readPcr(pIn);
        }

        pIn->erase(188);
    }

    return 0;
}
