//
// Created by akanchi on 2019-12-31.
//

#include "flv_muxer.h"
#include "ts_packet.h"
#include "simple_buffer.h"
#include "amf0.h"

static const int FLV_TAG_HEADER_SIZE = 11;

class FLVTagType
{
public:
    static const uint8_t Audio = 8;
    static const uint8_t Video = 9;
    static const uint8_t ScriptData = 18;
};

class NALU
{
public:
    int type;
    uint32_t size;
    char *data;
};

inline int get_nalu(NALU &nalu, char *data, uint32_t size, uint32_t index)
{
    nalu.type = 0;
    int ret = -1;
    uint32_t i = index;

    while (i < size) {
        if (data[i++] == 0x00
            && data[i++] == 0x00
            && data[i++] == 0x00
            && data[i++] == 0x01) {
            int pos = i;

            while (pos < size) {
                if (data[pos++] == 0x00
                    && data[pos++] == 0x00
                    && data[pos++] == 0x00
                    && data[pos++] == 0x01) {
                    break;
                }
            }
            uint32_t nalu_size = 0;
            if (pos == size) {
                nalu_size = pos - i;
                ret = pos;
            } else if (pos > size) {
                nalu_size = size - i;
                ret = size;
            } else {
                nalu_size = (pos - 4) - i;
                ret = pos - 4;
            }
            nalu.type = data[i] & 0x1f;
            nalu.size = nalu_size;
            nalu.data = &data[i];

            return ret;
        }
    }
    return ret;
}

FLVMuxer::FLVMuxer()
    : has_set_start_pts(false)
    , start_pts(0)
    , duration(0)
{

}

FLVMuxer::~FLVMuxer()
{

}

int FLVMuxer::write_header(SimpleBuffer *sb)
{
    sb->write_1byte('F');   // Signature
    sb->write_1byte('L');
    sb->write_1byte('V');
    sb->write_1byte(0x01);  // version
    sb->write_1byte(0x05);  // Audio and Video tags are present
    sb->write_4bytes(0x00000009);   // DataOffset, Offset in bytes from start of file to start of body (that is, size of header)
    sb->write_4bytes(0x00000000);   // PreviousTagSize0, Always 0
    return 0;
}

int FLVMuxer::write_body(TsFrame *frame, SimpleBuffer *sb)
{
    if (frame->stream_type == MpegTsStream::AAC) {
        write_aac_tag(frame, sb);
    } else if (frame->stream_type == MpegTsStream::AVC) {
        write_avc_tag(frame, sb);
    }

    calc_duration(frame->pts/90);

    return 0;
}

int FLVMuxer::write_aac_tag(TsFrame *frame, SimpleBuffer *sb)
{
    uint32_t bodySize = 2 + frame->_data->size();
    uint32_t pts = frame->pts / 90;
    sb->write_1byte(FLVTagType::Audio);
    sb->write_3bytes(bodySize);
    sb->write_3bytes(pts);
    sb->write_1byte((pts >> 24) & 0xff);
    sb->write_3bytes(0x00);
    sb->write_1byte(0xaf);
    sb->write_1byte(0x01);
    sb->append(frame->_data->data(), frame->_data->size());
    sb->write_4bytes(bodySize + FLV_TAG_HEADER_SIZE);

    return 0;
}

int FLVMuxer::write_avc_tag(TsFrame *frame, SimpleBuffer *sb)
{
    int index = 0;
    uint32_t pts = frame->pts / 90;
    uint32_t dts = frame->dts / 90;
    uint32_t cts = pts - dts;
    uint32_t size = frame->_data->size();
    NALU nalu;
    do {
        index = get_nalu(nalu, frame->_data->data(), size, index);
        if (nalu.type == 7) {
            spsData.clear();
            spsData.append(nalu.data, nalu.size);
        } else if (nalu.type == 8 && spsData.size() > 0) {
            ppsData.clear();
            ppsData.append(nalu.data, nalu.size);

            uint32_t bodySize = 13 + spsData.size() + 3 + ppsData.size();
            sb->write_1byte(FLVTagType::Video);
            sb->write_3bytes(bodySize);
            sb->write_3bytes(dts);
            sb->write_1byte((dts >> 24) & 0xff);
            sb->write_3bytes(0x00);

            sb->write_1byte(0x17);
            sb->write_4bytes(0x00);
            sb->write_1byte(0x01);
            sb->append(spsData.data() + 1, 3);
            sb->write_1byte(0xff);
            sb->write_1byte(0xe1);
            sb->write_2bytes(spsData.size());
            sb->append(spsData.data(), spsData.size());

            sb->write_1byte(0x01);
            sb->write_2bytes(ppsData.size());
            sb->append(ppsData.data(), ppsData.size());

            sb->write_4bytes(FLV_TAG_HEADER_SIZE + bodySize);
        } else if ((nalu.type == 1 || nalu.type == 5) && (spsData.size() > 0 && ppsData.size() > 0)) {
            bool isKeyFrame = nalu.type == 0x05;

            uint32_t bodySize = 9 + nalu.size;
            sb->write_1byte(FLVTagType::Video);
            sb->write_3bytes(bodySize);
            sb->write_3bytes(dts);
            sb->write_1byte((dts >> 24) & 0xff);
            sb->write_3bytes(0x00);

            sb->write_1byte(isKeyFrame ? 0x17 : 0x27);
            sb->write_1byte(0x01);
            sb->write_3bytes(cts);

            sb->write_4bytes(nalu.size);
            sb->append(nalu.data, nalu.size);

            sb->write_4bytes(FLV_TAG_HEADER_SIZE + bodySize);
        }
    } while (index < size && index > 0);

    return 0;
}

void FLVMuxer::calc_duration(uint32_t pts)
{
    if (!has_set_start_pts) {
        has_set_start_pts = true;
        start_pts = pts;
    }

    duration = pts - start_pts;
}

int FLVMuxer::write_metadata(SimpleBuffer *sb, uint32_t fileSize)
{
    SimpleBuffer bodySb;

    Amf0String amfOnMetaData("onMetaData");
    amfOnMetaData.write(&bodySb);

    Amf0EcmaArray amfEcmaArray;
    amfEcmaArray.put("duration", new Amf0Number(duration / 1000.));
    amfEcmaArray.put("filesize", new Amf0Number(fileSize));
    amfEcmaArray.write(&bodySb);

    sb->write_1byte(FLVTagType::ScriptData);
    sb->write_3bytes(bodySb.size());
    sb->write_3bytes(0);
    sb->write_1byte(0);
    sb->write_3bytes(0x00);

    sb->append(bodySb.data(), bodySb.size());

    return 0;
}

