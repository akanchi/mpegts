#include "mpegts_muxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "crc.h"
#include <string.h>
#include "common.h"

static const uint16_t MPEGTS_NULL_PACKET_PID = 0x1FFF;
static const uint16_t MPEGTS_PAT_PID = 0x00;
static const uint16_t MPEGTS_PMT_PID = 0x100;
static const uint16_t MPEGTS_PCR_PID = 0x110;

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

MpegTsMuxer::MpegTsMuxer() {
}

MpegTsMuxer::~MpegTsMuxer() {
}

void MpegTsMuxer::createPat(SimpleBuffer *pSb, uint16_t lPmtPid, uint8_t lCc) {
    int lPos = pSb->pos();
    TsHeader lTsHeader;
    lTsHeader.mSyncByte = 0x47;
    lTsHeader.mTransportErrorIndicator = 0;
    lTsHeader.mPayloadUnitStartIndicator = 1;
    lTsHeader.mTransportPriority = 0;
    lTsHeader.mPid = MPEGTS_PAT_PID;
    lTsHeader.mTransportScramblingControl = 0;
    lTsHeader.mAdaptationFieldControl = MpegTsAdaptationFieldType::mPayloadOnly;
    lTsHeader.mContinuityCounter = lCc;

    AdaptationFieldHeader lAdaptField;

    PATHeader lPatHeader;
    lPatHeader.mTableId = 0x00;
    lPatHeader.mSectionSyntaxIndicator = 1;
    lPatHeader.mB0 = 0;
    lPatHeader.mReserved0 = 0x3;
    lPatHeader.mTransportStreamId = 0;
    lPatHeader.mReserved1 = 0x3;
    lPatHeader.mVersionNumber = 0;
    lPatHeader.mCurrentNextIndicator = 1;
    lPatHeader.mSectionNumber = 0x0;
    lPatHeader.mLastSectionNumber = 0x0;

    //program_number
    uint16_t lProgramNumber = 0x0001;
    //program_map_PID
    uint16_t lProgramMapPid = 0xe000 | (lPmtPid & 0x1fff);

    unsigned int lSectionLength = 4 + 4 + 5;
    lPatHeader.mSectionLength = lSectionLength & 0x3ff;

    lTsHeader.encode(pSb);
    lAdaptField.encode(pSb);
    lPatHeader.encode(pSb);
    pSb->write_2bytes(lProgramNumber);
    pSb->write_2bytes(lProgramMapPid);

    // crc32
    uint32_t lCrc32 = crc32((uint8_t *) pSb->data() + 5, pSb->pos() - 5);
    pSb->write_4bytes(lCrc32);

    std::string lStuff(188 - (pSb->pos() - lPos), 0xff);
    pSb->write_string(lStuff);
}

void MpegTsMuxer::createPmt(SimpleBuffer *pSb, std::map<uint8_t, int> lStreamPidMap, uint16_t lPmtPid, uint8_t lCc) {
    int lPos = pSb->pos();
    TsHeader lTsHeader;
    lTsHeader.mSyncByte = 0x47;
    lTsHeader.mTransportErrorIndicator = 0;
    lTsHeader.mPayloadUnitStartIndicator = 1;
    lTsHeader.mTransportPriority = 0;
    lTsHeader.mPid = lPmtPid;
    lTsHeader.mTransportScramblingControl = 0;
    lTsHeader.mAdaptationFieldControl = MpegTsAdaptationFieldType::mPayloadOnly;
    lTsHeader.mContinuityCounter = lCc;

    AdaptationFieldHeader lAdaptField;

    PMTHeader lPmtHeader;
    lPmtHeader.mTableId = 0x02;
    lPmtHeader.mSectionSyntaxIndicator = 1;
    lPmtHeader.mB0 = 0;
    lPmtHeader.mReserved0 = 0x3;
    lPmtHeader.mSectionLength = 0;
    lPmtHeader.mProgramNumber = 0x0001;
    lPmtHeader.mReserved1 = 0x3;
    lPmtHeader.mVersionNumber = 0;
    lPmtHeader.mCurrentNextIndicator = 1;
    lPmtHeader.mSectionNumber = 0x00;
    lPmtHeader.mLastSectionNumber = 0x00;
    lPmtHeader.mReserved2 = 0x7;
    lPmtHeader.mReserved3 = 0xf;
    lPmtHeader.mProgramInfoLength = 0;
    for (auto lIt = lStreamPidMap.begin(); lIt != lStreamPidMap.end(); lIt++) {
        lPmtHeader.mInfos.push_back(std::shared_ptr<PMTElementInfo>(new PMTElementInfo(lIt->first, lIt->second)));
        if (lIt->first == MpegTsStream::AVC) {
            lPmtHeader.mPcrPid = lIt->second;
        }
    }

    uint16_t lSectionLength = lPmtHeader.size() - 3 + 4;
    lPmtHeader.mSectionLength = lSectionLength & 0x3ff;

    lTsHeader.encode(pSb);
    lAdaptField.encode(pSb);
    lPmtHeader.encode(pSb);

    // crc32
    uint32_t lCrc32 = crc32((uint8_t *) pSb->data() + lPos + 5, pSb->pos() - lPos - 5);
    pSb->write_4bytes(lCrc32);

    std::string lStuff(188 - (pSb->pos() - lPos), 0xff);
    pSb->write_string(lStuff);
}

void MpegTsMuxer::createPes(TsFrame *pFrame, SimpleBuffer *pSb) {
    bool lFirst = true;
    while (!pFrame->mData->empty()) {
        SimpleBuffer lPacket(188);

        TsHeader lTsHeader;
        lTsHeader.mPid = pFrame->mPid;
        lTsHeader.mAdaptationFieldControl = MpegTsAdaptationFieldType::mPayloadOnly;
        lTsHeader.mContinuityCounter = getCc(pFrame->mStreamType);

        if (lFirst) {
            lTsHeader.mPayloadUnitStartIndicator = 0x01;
            if (pFrame->mStreamType == MpegTsStream::AVC) {
                lTsHeader.mAdaptationFieldControl |= 0x02;
                AdaptationFieldHeader adapt_field_header;
                adapt_field_header.mAdaptationFieldLength = 0x07;
                adapt_field_header.mRandomAccessIndicator = 0x01;
                adapt_field_header.mPcrFlag = 0x01;

                lTsHeader.encode(&lPacket);
                adapt_field_header.encode(&lPacket);
                writePcr(&lPacket, pFrame->mDts);
            } else {
                lTsHeader.encode(&lPacket);
            }

            PESHeader lPesHeader;
            lPesHeader.mPacketStartCode = 0x000001;
            lPesHeader.mStreamId = pFrame->mStreamId;
            lPesHeader.mMarkerBits = 0x02;
            lPesHeader.mOriginalOrCopy = 0x01;

            if (pFrame->mPts != pFrame->mDts) {
                lPesHeader.mPtsDtsFlags = 0x03;
                lPesHeader.mHeaderDataLength = 0x0A;
            } else {
                lPesHeader.mPtsDtsFlags = 0x2;
                lPesHeader.mHeaderDataLength = 0x05;
            }

            uint32_t lPesSize = (lPesHeader.mHeaderDataLength + pFrame->mData->size() + 3);
            lPesHeader.mPesPacketLength = lPesSize > 0xffff ? 0 : lPesSize;
            lPesHeader.encode(&lPacket);

            if (lPesHeader.mPtsDtsFlags == 0x03) {
                writePts(&lPacket, 3, pFrame->mPts);
                writePts(&lPacket, 1, pFrame->mDts);
            } else {
                writePts(&lPacket, 2, pFrame->mPts);
            }
            lFirst = false;
        } else {
            lTsHeader.encode(&lPacket);
        }

        uint32_t lBodySize = lPacket.size() - lPacket.pos();
        uint32_t lInSize = pFrame->mData->size() - pFrame->mData->pos();
        if (lBodySize <=
            lInSize) {    // MpegTsAdaptationFieldType::payload_only or MpegTsAdaptationFieldType::payload_adaption_both for AVC
            lPacket.write_string(pFrame->mData->read_string(lBodySize));
        } else {
            uint16_t lStuffSize = lBodySize - lInSize;
            if (lTsHeader.mAdaptationFieldControl == MpegTsAdaptationFieldType::mAdaptionOnly ||
                lTsHeader.mAdaptationFieldControl == MpegTsAdaptationFieldType::mPayloadAdaptionBoth) {
                char *lpBase = lPacket.data() + 5 + lPacket.data()[4];
                lPacket.setData(lpBase - lPacket.data() + lStuffSize, lpBase, lPacket.data() + lPacket.pos() - lpBase);
                memset(lpBase, 0xff, lStuffSize);
                lPacket.skip(lStuffSize);
                lPacket.data()[4] += lStuffSize;
            } else {
                // adaptationFieldControl |= 0x20 == MpegTsAdaptationFieldType::payload_adaption_both
                lPacket.data()[3] |= 0x20;
                lPacket.setData(188 - 4 - lStuffSize, lPacket.data() + 4, lPacket.pos() - 4);
                lPacket.skip(lStuffSize);
                lPacket.data()[4] = lStuffSize - 1;
                if (lStuffSize >= 2) {
                    lPacket.data()[5] = 0;
                    memset(&(lPacket.data()[6]), 0xff, lStuffSize - 2);
                }
            }
            lPacket.write_string(pFrame->mData->read_string(lInSize));
        }

        pSb->append(lPacket.data(), lPacket.size());
    }
}

void MpegTsMuxer::createPcr(SimpleBuffer *pSb) {
    uint64_t lPcr = 0;
    TsHeader lTsHeader;
    lTsHeader.mSyncByte = 0x47;
    lTsHeader.mTransportErrorIndicator = 0;
    lTsHeader.mPayloadUnitStartIndicator = 0;
    lTsHeader.mTransportPriority = 0;
    lTsHeader.mPid = MPEGTS_PCR_PID;
    lTsHeader.mTransportScramblingControl = 0;
    lTsHeader.mAdaptationFieldControl = MpegTsAdaptationFieldType::mAdaptionOnly;
    lTsHeader.mContinuityCounter = 0;

    AdaptationFieldHeader lAdaptField;
    lAdaptField.mAdaptationFieldLength = 188 - 4 - 1;
    lAdaptField.mSiscontinuityIndicator = 0;
    lAdaptField.mRandomAccessIndicator = 0;
    lAdaptField.mElementaryStreamPriorityIndicator = 0;
    lAdaptField.mPcrFlag = 1;
    lAdaptField.mOpcrFlag = 0;
    lAdaptField.mSplicingPointFlag = 0;
    lAdaptField.mTransportPrivateDataFlag = 0;
    lAdaptField.mAdaptationFieldExtensionFlag = 0;

    char *p = pSb->data();
    lTsHeader.encode(pSb);
    lAdaptField.encode(pSb);
    writePcr(pSb, lPcr);
}

void MpegTsMuxer::createNull(SimpleBuffer *pSb) {
    TsHeader lTsHeader;
    lTsHeader.mSyncByte = 0x47;
    lTsHeader.mTransportErrorIndicator = 0;
    lTsHeader.mPayloadUnitStartIndicator = 0;
    lTsHeader.mTransportPriority = 0;
    lTsHeader.mPid = MPEGTS_NULL_PACKET_PID;
    lTsHeader.mTransportScramblingControl = 0;
    lTsHeader.mAdaptationFieldControl = MpegTsAdaptationFieldType::mPayloadOnly;
    lTsHeader.mContinuityCounter = 0;
    lTsHeader.encode(pSb);
}

void MpegTsMuxer::encode(TsFrame *pFrame, std::map<uint8_t, int> lStreamPidMap, uint16_t lPmtPid, SimpleBuffer *pSb) {
    if (shouldCreatePat()) {
        uint8_t lPatPmtCc = getCc(0);
        createPat(pSb, lPmtPid, lPatPmtCc);
        createPmt(pSb, lStreamPidMap, lPmtPid, lPatPmtCc);
    }

    createPes(pFrame, pSb);
}

uint8_t MpegTsMuxer::getCc(uint32_t lWithPid) {
    if (mPidCcMap.find(lWithPid) != mPidCcMap.end()) {
        mPidCcMap[lWithPid] = (mPidCcMap[lWithPid] + 1) & 0x0F;
        return mPidCcMap[lWithPid];
    }

    mPidCcMap[lWithPid] = 0;
    return 0;
}

bool MpegTsMuxer::shouldCreatePat() {
    bool lRet = false;
    static const int lPatInterval = 20;
    static int lCurrentIndex = 0;
    if (lCurrentIndex % lPatInterval == 0) {
        if (lCurrentIndex > 0) {
            lCurrentIndex = 0;
        }
        lRet = true;
    }

    lCurrentIndex++;

    return lRet;
}
