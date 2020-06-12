#include "ts_packet.h"
#include "simple_buffer.h"

#include <iostream>

TsFrame::TsFrame()
        : mCompleted(false), mPid(0), mExpectedPesPacketLength(0) {
    mData.reset(new SimpleBuffer);
}

TsFrame::TsFrame(uint8_t lSt)
        : mStreamType(lSt), mCompleted(false), mPid(0), mExpectedPesPacketLength(0) {
    mData.reset(new SimpleBuffer);
}

bool TsFrame::empty() {
    return mData->size() == 0;
}

void TsFrame::reset() {
    mPid = 0;
    mCompleted = false;
    mExpectedPesPacketLength = 0;
    mData.reset(new SimpleBuffer);
}

TsHeader::TsHeader()
        : mSyncByte(0x47), mTransportErrorIndicator(0), mPayloadUnitStartIndicator(0), mTransportPriority(0), mPid(0),
          mTransportScramblingControl(0), mAdaptationFieldControl(0), mContinuityCounter(0) {
}

TsHeader::~TsHeader() {
}

void TsHeader::encode(SimpleBuffer *sb) {
    sb->write_1byte(mSyncByte);

    uint16_t lB1b2 = mPid & 0x1FFF;
    lB1b2 |= (mTransportPriority << 13) & 0x2000;
    lB1b2 |= (mPayloadUnitStartIndicator << 14) & 0x4000;
    lB1b2 |= (mTransportErrorIndicator << 15) & 0x8000;
    sb->write_2bytes(lB1b2);

    uint8_t lB3 = mContinuityCounter & 0x0F;
    lB3 |= (mAdaptationFieldControl << 4) & 0x30;
    lB3 |= (mTransportScramblingControl << 6) & 0xC0;
    sb->write_1byte(lB3);
}

void TsHeader::decode(SimpleBuffer *pSb) {
    mSyncByte = pSb->read_1byte();

    uint16_t lB1b2 = pSb->read_2bytes();
    mPid = lB1b2 & 0x1FFF;
    mTransportErrorIndicator = (lB1b2 >> 13) & 0x01;
    mPayloadUnitStartIndicator = (lB1b2 >> 14) & 0x01;
    mTransportErrorIndicator = (lB1b2 >> 15) & 0x01;

    uint8_t lB3 = pSb->read_1byte();
    mContinuityCounter = lB3 & 0x0F;
    mAdaptationFieldControl = (lB3 >> 4) & 0x03;
    mTransportScramblingControl = (lB3 >> 6) & 0x03;
}

PATHeader::PATHeader()
        : mTableId(0), mSectionSyntaxIndicator(0), mB0(0), mReserved0(0), mSectionLength(0), mTransportStreamId(0),
          mReserved1(0), mVersionNumber(0), mCurrentNextIndicator(0), mSectionNumber(0), mLastSectionNumber(0) {

}

PATHeader::~PATHeader() {

}

void PATHeader::encode(SimpleBuffer *pSb) {
    pSb->write_1byte(mTableId);

    uint16_t lB1b2 = mSectionLength & 0x0FFF;
    lB1b2 |= (mReserved0 << 12) & 0x3000;
    lB1b2 |= (mB0 << 14) & 0x4000;
    lB1b2 |= (mSectionSyntaxIndicator << 15) & 0x8000;
    pSb->write_2bytes(lB1b2);

    pSb->write_2bytes(mTransportStreamId);

    uint8_t lB5 = mCurrentNextIndicator & 0x01;
    lB5 |= (mVersionNumber << 1) & 0x3E;
    lB5 |= (mReserved1 << 6) & 0xC0;
    pSb->write_1byte(lB5);

    pSb->write_1byte(mSectionNumber);
    pSb->write_1byte(mLastSectionNumber);
}

void PATHeader::decode(SimpleBuffer *pSb) {
    mTableId = pSb->read_1byte();

    uint16_t lB1b2 = pSb->read_2bytes();
    mSectionSyntaxIndicator = (lB1b2 >> 15) & 0x01;
    mB0 = (lB1b2 >> 14) & 0x01;
    mSectionLength = lB1b2 & 0x0FFF;

    mTransportStreamId = pSb->read_2bytes();

    uint8_t lB5 = pSb->read_1byte();
    mReserved1 = (lB5 >> 6) & 0x03;
    mVersionNumber = (lB5 >> 1) & 0x1F;
    mCurrentNextIndicator = lB5 & 0x01;

    mSectionNumber = pSb->read_1byte();

    mLastSectionNumber = pSb->read_1byte();
}

void PATHeader::print() {
    std::cout << "----------PAT information----------" << std::endl;
    std::cout << "table_id: " << std::to_string(mTableId) << std::endl;
    std::cout << "section_syntax_indicator: " << std::to_string(mSectionSyntaxIndicator) << std::endl;
    std::cout << "b0: " << std::to_string(mB0) << std::endl;
    std::cout << "reserved0: " << std::to_string(mReserved0) << std::endl;
    std::cout << "section_length: " << std::to_string(mSectionLength) << std::endl;
    std::cout << "transport_stream_id: " << std::to_string(mTransportStreamId) << std::endl;
    std::cout << "reserved1: " << std::to_string(mReserved1) << std::endl;
    std::cout << "version_number: " << std::to_string(mVersionNumber) << std::endl;
    std::cout << "current_next_indicator: " << std::to_string(mCurrentNextIndicator) << std::endl;
    std::cout << "section_number: " << std::to_string(mSectionNumber) << std::endl;
    std::cout << "last_section_number: " << std::to_string(mLastSectionNumber) << std::endl;
    std::cout << std::endl;
    std::flush(std::cout);
}

PMTElementInfo::PMTElementInfo(uint8_t lSt, uint16_t lPid)
        : mStreamType(lSt), mReserved0(0x7), mElementaryPid(lPid), mReserved1(0xf), mEsInfoLength(0) {

}

PMTElementInfo::PMTElementInfo()
        : PMTElementInfo(0, 0) {

}

PMTElementInfo::~PMTElementInfo() {

}

void PMTElementInfo::encode(SimpleBuffer *pSb) {
    pSb->write_1byte(mStreamType);

    uint16_t lB1b2 = mElementaryPid & 0x1FFF;
    lB1b2 |= (mReserved0 << 13) & 0xE000;
    pSb->write_2bytes(lB1b2);

    int16_t lB3b4 = mEsInfoLength & 0x0FFF;
    lB3b4 |= (mReserved1 << 12) & 0xF000;
    pSb->write_2bytes(lB3b4);

    if (mEsInfoLength > 0) {
        // TODO:
    }
}

void PMTElementInfo::decode(SimpleBuffer *sb) {
    mStreamType = sb->read_1byte();

    uint16_t lB1b2 = sb->read_2bytes();
    mReserved0 = (lB1b2 >> 13) & 0x07;
    mElementaryPid = lB1b2 & 0x1FFF;

    uint16_t lB3b4 = sb->read_2bytes();
    mReserved1 = (lB3b4 >> 12) & 0xF;
    mEsInfoLength = lB3b4 & 0xFFF;

    if (mEsInfoLength > 0) {
        mEsInfo = sb->read_string(mEsInfoLength);
    }
}

uint16_t PMTElementInfo::size() {
    return 5 + mEsInfoLength;
}

void PMTElementInfo::print() {
    std::cout << "**********PMTElement information**********" << std::endl;
    std::cout << "stream_type: " << std::to_string(mStreamType) << std::endl;
    std::cout << "reserved0: " << std::to_string(mReserved0) << std::endl;
    std::cout << "elementary_PID: " << std::to_string(mElementaryPid) << std::endl;
    std::cout << "reserved1: " << std::to_string(mReserved1) << std::endl;
    std::cout << "ES_info_length: " << std::to_string(mEsInfoLength) << std::endl;
    std::cout << "ES_info: " << mEsInfo << std::endl;
    std::flush(std::cout);
}

PMTHeader::PMTHeader()
        : mTableId(0x02), mSectionSyntaxIndicator(0), mB0(0), mReserved0(0), mSectionLength(0), mProgramNumber(0),
          mReserved1(0), mVersionNumber(0), mCurrentNextIndicator(0), mSectionNumber(0), mLastSectionNumber(0),
          mReserved2(0), mPcrPid(0), mReserved3(0), mProgramInfoLength(0) {

}

PMTHeader::~PMTHeader() {

}

void PMTHeader::encode(SimpleBuffer *pSb) {
    pSb->write_1byte(mTableId);

    uint16_t lB1b2 = mSectionLength & 0xFFFF;
    lB1b2 |= (mReserved0 << 12) & 0x3000;
    lB1b2 |= (mB0 << 14) & 0x4000;
    lB1b2 |= (mSectionSyntaxIndicator << 15) & 0x8000;
    pSb->write_2bytes(lB1b2);

    pSb->write_2bytes(mProgramNumber);

    uint8_t lB5 = mCurrentNextIndicator & 0x01;
    lB5 |= (mVersionNumber << 1) & 0x3E;
    lB5 |= (mReserved1 << 6) & 0xC0;
    pSb->write_1byte(lB5);

    pSb->write_1byte(mSectionNumber);
    pSb->write_1byte(mLastSectionNumber);

    uint16_t lB8b9 = mPcrPid & 0x1FFF;
    lB8b9 |= (mReserved2 << 13) & 0xE000;
    pSb->write_2bytes(lB8b9);

    uint16_t lB10b11 = mProgramInfoLength & 0xFFF;
    lB10b11 |= (mReserved3 << 12) & 0xF000;
    pSb->write_2bytes(lB10b11);

    for (int lI = 0; lI < (int) mInfos.size(); lI++) {
        mInfos[lI]->encode(pSb);
    }
}

void PMTHeader::decode(SimpleBuffer *pSb) {
    mTableId = pSb->read_1byte();

    uint16_t lB1b2 = pSb->read_2bytes();
    mSectionSyntaxIndicator = (lB1b2 >> 15) & 0x01;
    mB0 = (lB1b2 >> 14) & 0x01;
    mReserved0 = (lB1b2 >> 12) & 0x03;
    mSectionLength = lB1b2 & 0xFFF;

    mProgramNumber = pSb->read_2bytes();

    uint8_t lB5 = pSb->read_1byte();
    mReserved1 = (lB5 >> 6) & 0x03;
    mVersionNumber = (lB5 >> 1) & 0x1F;
    mCurrentNextIndicator = lB5 & 0x01;

    mSectionNumber = pSb->read_1byte();
    mLastSectionNumber = pSb->read_1byte();

    uint16_t lB8b9 = pSb->read_2bytes();
    mReserved2 = (lB8b9 >> 13) & 0x07;
    mPcrPid = lB8b9 & 0x1FFF;

    uint16_t lB10b11 = pSb->read_2bytes();
    mReserved3 = (lB10b11 >> 12) & 0xF;
    mProgramInfoLength = lB10b11 & 0xFFF;

    if (mProgramInfoLength > 0) {
        pSb->read_string(mProgramInfoLength);
    }

    int lRemainBytes = mSectionLength - 4 - 9 - mProgramInfoLength;
    while (lRemainBytes > 0) {
        std::shared_ptr<PMTElementInfo> element_info(new PMTElementInfo);
        element_info->decode(pSb);
        mInfos.push_back(element_info);
        lRemainBytes -= element_info->size();
    }
}

uint16_t PMTHeader::size() {
    uint16_t lRet = 12;
    for (int lI = 0; lI < (int) mInfos.size(); lI++) {
        lRet += mInfos[lI]->size();
    }

    return lRet;
}

void PMTHeader::print() {
    std::cout << "----------PMT information----------" << std::endl;
    std::cout << "table_id: " << std::to_string(mTableId) << std::endl;
    std::cout << "section_syntax_indicator: " << std::to_string(mSectionSyntaxIndicator) << std::endl;
    std::cout << "b0: " << std::to_string(mB0) << std::endl;
    std::cout << "reserved0: " << std::to_string(mReserved0) << std::endl;
    std::cout << "section_length: " << std::to_string(mSectionLength) << std::endl;
    std::cout << "program_number: " << std::to_string(mProgramNumber) << std::endl;
    std::cout << "reserved1: " << std::to_string(mReserved1) << std::endl;
    std::cout << "version_number: " << std::to_string(mVersionNumber) << std::endl;
    std::cout << "current_next_indicator: " << std::to_string(mCurrentNextIndicator) << std::endl;
    std::cout << "section_number: " << std::to_string(mSectionNumber) << std::endl;
    std::cout << "last_section_number: " << std::to_string(mLastSectionNumber) << std::endl;
    std::cout << "reserved2: " << std::to_string(mReserved2) << std::endl;
    std::cout << "PCR_PID: " << std::to_string(mPcrPid) << std::endl;
    std::cout << "reserved3: " << std::to_string(mReserved3) << std::endl;
    std::cout << "program_info_length: " << std::to_string(mProgramInfoLength) << std::endl;
    for (int lI = 0; lI < (int) mInfos.size(); lI++) {
        mInfos[lI]->print();
    }
    std::cout << std::endl;
    std::flush(std::cout);
}

AdaptationFieldHeader::AdaptationFieldHeader()
        : mAdaptationFieldLength(0), mAdaptationFieldExtensionFlag(0), mTransportPrivateDataFlag(0),
          mSplicingPointFlag(0), mOpcrFlag(0), mPcrFlag(0), mElementaryStreamPriorityIndicator(0),
          mRandomAccessIndicator(0), mSiscontinuityIndicator(0) {

}

AdaptationFieldHeader::~AdaptationFieldHeader() {

}

void AdaptationFieldHeader::encode(SimpleBuffer *pSb) {
    pSb->write_1byte(mAdaptationFieldLength);
    if (mAdaptationFieldLength != 0) {
        uint8_t lVal = mAdaptationFieldExtensionFlag & 0x01;
        lVal |= (mTransportPrivateDataFlag << 1) & 0x02;
        lVal |= (mSplicingPointFlag << 2) & 0x04;
        lVal |= (mOpcrFlag << 3) & 0x08;
        lVal |= (mPcrFlag << 4) & 0x10;
        lVal |= (mElementaryStreamPriorityIndicator << 5) & 0x20;
        lVal |= (mRandomAccessIndicator << 6) & 0x40;
        lVal |= (mSiscontinuityIndicator << 7) & 0x80;
        pSb->write_1byte(lVal);
    }
}

void AdaptationFieldHeader::decode(SimpleBuffer *pAb) {
    mAdaptationFieldLength = pAb->read_1byte();
    if (mAdaptationFieldLength != 0) {
        uint8_t lVal = pAb->read_1byte();
        mAdaptationFieldExtensionFlag = lVal & 0x01;
        mTransportPrivateDataFlag = (lVal >> 1) & 0x01;
        mSplicingPointFlag = (lVal >> 2) & 0x01;
        mOpcrFlag = (lVal >> 3) & 0x01;
        mPcrFlag = (lVal >> 4) & 0x01;
        mElementaryStreamPriorityIndicator = (lVal >> 5) & 0x01;
        mRandomAccessIndicator = (lVal >> 6) & 0x01;
        mSiscontinuityIndicator = (lVal >> 7) & 0x01;
    }
}

PESHeader::PESHeader()
        : mPacketStartCode(0x000001), mStreamId(0), mPesPacketLength(0), mOriginalOrCopy(0), mCopyright(0),
          mDataAlignmentIndicator(0), mPesPriority(0), mPesScramblingControl(0), mMarkerBits(0x02), mPesExtFlag(0),
          mPesCrcFlag(0), mAddCopyInfoFlag(0), mDsmTrickModeFlag(0), mEsRateFlag(0), mEscrFlag(0), mPtsDtsFlags(0),
          mHeaderDataLength(0) {

}

PESHeader::~PESHeader() {

}

void PESHeader::encode(SimpleBuffer *pSb) {
    uint32_t lB0b1b2b3 = (mPacketStartCode << 8) & 0xFFFFFF00;
    lB0b1b2b3 |= mStreamId & 0xFF;
    pSb->write_4bytes(lB0b1b2b3);

    pSb->write_2bytes(mPesPacketLength);

    uint8_t lB6 = mOriginalOrCopy & 0x01;
    lB6 |= (mCopyright << 1) & 0x02;
    lB6 |= (mDataAlignmentIndicator << 2) & 0x04;
    lB6 |= (mPesPriority << 3) & 0x08;
    lB6 |= (mPesScramblingControl << 4) & 0x30;
    lB6 |= (mMarkerBits << 6) & 0xC0;
    pSb->write_1byte(lB6);

    uint8_t lB7 = mPesExtFlag & 0x01;
    lB7 |= (mPesCrcFlag << 1) & 0x02;
    lB7 |= (mAddCopyInfoFlag << 2) & 0x04;
    lB7 |= (mDsmTrickModeFlag << 3) & 0x08;
    lB7 |= (mEsRateFlag << 4) & 0x10;
    lB7 |= (mEscrFlag << 5) & 0x20;
    lB7 |= (mPtsDtsFlags << 6) & 0xC0;
    pSb->write_1byte(lB7);

    pSb->write_1byte(mHeaderDataLength);
}

void PESHeader::decode(SimpleBuffer *pSb) {
    uint32_t lB0b1b2b3 = pSb->read_4bytes();
    mPacketStartCode = (lB0b1b2b3 >> 8) & 0x00FFFFFF;
    mStreamId = (lB0b1b2b3) & 0xFF;

    mPesPacketLength = pSb->read_2bytes();

    uint8_t lB6 = pSb->read_1byte();
    mOriginalOrCopy = lB6 & 0x01;
    mCopyright = (lB6 >> 1) & 0x01;
    mDataAlignmentIndicator = (lB6 >> 2) & 0x01;
    mPesPriority = (lB6 >> 3) & 0x01;
    mPesScramblingControl = (lB6 >> 4) & 0x03;
    mMarkerBits = (lB6 >> 6) & 0x03;

    uint8_t lB7 = pSb->read_1byte();
    mPesExtFlag = lB7 & 0x01;
    mPesCrcFlag = (lB7 >> 1) & 0x01;
    mAddCopyInfoFlag = (lB7 >> 2) & 0x01;
    mDsmTrickModeFlag = (lB7 >> 3) & 0x01;
    mEsRateFlag = (lB7 >> 4) & 0x01;
    mEscrFlag = (lB7 >> 5) & 0x01;
    mPtsDtsFlags = (lB7 >> 6) & 0x03;

    mHeaderDataLength = pSb->read_1byte();
}
