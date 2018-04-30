#include "mpegts_muxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "crc.h"
#include <string.h>

static const uint16_t MPEGTS_NULL_PACKET_PID = 0x1FFF;
static const uint16_t MPEGTS_PAT_PID = 0x00;
static const uint16_t MPEGTS_PMT_PID = 0x100;
static const uint16_t MPEGTS_PCR_PID = 0x110;
static const uint16_t MPEGTS_AUDIO_AAC_PID = 0X200;
static const uint16_t MPEGTS_VIDEO_AVC_PID = 0x400;

class MpegTsAdaptationFieldType
{
public:
    // Reserved for future use by ISO/IEC
    static const uint8_t reserved = 0x00;
    // No adaptation_field, payload only
    static const uint8_t payload_only = 0x01;
    // Adaptation_field only, no payload
    static const uint8_t adaption_only = 0x02;
    // Adaptation_field followed by payload
    static const uint8_t payload_adaption_both = 0x03;
};

class MpegTsStream
{
public:
    static const uint8_t AAC = 0x0f;
    static const uint8_t AVC = 0x1b;
};

MpegTsMuxer::MpegTsMuxer()
{
}

MpegTsMuxer::~MpegTsMuxer()
{
}

void MpegTsMuxer::create_pat(SimpleBuffer *sb, uint8_t cc)
{
    int pos = sb->pos();
    TsHeader ts_header;
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 1;
    ts_header.transport_priority = 0;
    ts_header.pid = MPEGTS_PAT_PID;
    ts_header.transport_scrambling_control = 0;
    ts_header.adaptation_field_control = MpegTsAdaptationFieldType::payload_only;
    ts_header.continuity_counter = cc;

    AdaptationFieldHeader adapt_field;

    PATHeader pat_header;
    pat_header.table_id = 0x00;
    pat_header.section_syntax_indicator = 1;
    pat_header.b0 = 0;
    pat_header.reserved0 = 0x3;
    pat_header.transport_stream_id = 0;
    pat_header.reserved1 = 0x3;
    pat_header.version_number = 0;
    pat_header.current_next_indicator = 1;
    pat_header.section_number = 0x0;
    pat_header.last_section_number = 0x0;

    //program_number
    uint16_t program_number = 0x0001;
    //program_map_PID
    uint16_t program_map_PID = 0xe000 | (MPEGTS_PMT_PID & 0x1fff);

    unsigned int section_length = 4 + 4 + 5;
    pat_header.section_length = section_length & 0x3ff;

    ts_header.encode(sb);
    adapt_field.encode(sb);
    pat_header.encode(sb);
    sb->write_2bytes(program_number);
    sb->write_2bytes(program_map_PID);

    // crc32
    uint32_t crc_32 = crc32((uint8_t *)sb->data() + 5, sb->pos() - 5);
    sb->write_4bytes(crc_32);

    std::string stuff(188 - (sb->pos() - pos), 0xff);
    sb->write_string(stuff);
}

void MpegTsMuxer::create_pmt(SimpleBuffer *sb, std::map<uint8_t, int> stream_pid_map, uint8_t cc)
{
    int pos = sb->pos();
    TsHeader ts_header;
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 1;
    ts_header.transport_priority = 0;
    ts_header.pid = MPEGTS_PMT_PID;
    ts_header.transport_scrambling_control = 0;
    ts_header.adaptation_field_control = MpegTsAdaptationFieldType::payload_only;
    ts_header.continuity_counter = cc;

    AdaptationFieldHeader adapt_field;

    PMTHeader pmt_header;
    pmt_header.table_id = 0x02;
    pmt_header.section_syntax_indicator = 1;
    pmt_header.b0 = 0;
    pmt_header.reserved0 = 0x3;
    pmt_header.section_length = 0;
    pmt_header.program_number = 0x0001;
    pmt_header.reserved1 = 0x3;
    pmt_header.version_number = 0;
    pmt_header.current_next_indicator = 1;
    pmt_header.section_number = 0x00;
    pmt_header.last_section_number = 0x00;
    pmt_header.reserved2 = 0x7;
    pmt_header.reserved3 = 0xf;
    pmt_header.program_info_length = 0;
    for (auto it = stream_pid_map.begin(); it != stream_pid_map.end(); it++) {
        pmt_header.infos.push_back(std::shared_ptr<PMTElementInfo>(new PMTElementInfo(it->first, it->second)));
        if (it->first == MpegTsStream::AVC) {
            pmt_header.PCR_PID = it->second;
        }
    }

    uint16_t section_length = pmt_header.size() - 3 + 4;
    pmt_header.section_length = section_length & 0x3ff;

    ts_header.encode(sb);
    adapt_field.encode(sb);
    pmt_header.encode(sb);

    // crc32
    uint32_t crc_32 = crc32((uint8_t *)sb->data() + pos + 5, sb->pos() - pos - 5);
    sb->write_4bytes(crc_32);

    std::string stuff(188 - (sb->pos() - pos), 0xff);
    sb->write_string(stuff);
}

void MpegTsMuxer::create_pes(TsFrame *frame, SimpleBuffer *sb)
{
    bool first = true;
    while (!frame->_data->empty()) {
        SimpleBuffer packet(188);
        char *pdata = packet.data();

        TsHeader ts_header;
        ts_header.pid = frame->pid;
        ts_header.adaptation_field_control = 0x01;
        ts_header.continuity_counter = get_cc(frame->stream_type);

        if (first) {
            ts_header.payload_unit_start_indicator = 0x01;
            if (frame->stream_type == MpegTsStream::AVC) {
                ts_header.adaptation_field_control |= 0x02;
                AdaptationFieldHeader adapt_field_header;
                adapt_field_header.adaptation_field_length = 0x07;
                adapt_field_header.random_access_indicator = 0x01;
                adapt_field_header.pcr_flag = 0x01;

                ts_header.encode(&packet);
                adapt_field_header.encode(&packet);
                write_pcr(&packet, frame->dts);
            } else {
                ts_header.encode(&packet);
            }

            PESHeader pes_header;
            pes_header.packet_start_code = 0x000001;
            pes_header.stream_id = frame->stream_id;
            pes_header.marker_bits = 0x02;
            pes_header.original_or_copy = 0x01;

           if (frame->pts != frame->dts) {
               pes_header.pts_dts_flags = 0x03;
               pes_header.header_data_length = 0x0A;
           } else {
               pes_header.pts_dts_flags = 0x2;
               pes_header.header_data_length = 0x05;
           }

            uint32_t pes_size = (pes_header.header_data_length + frame->_data->size() + 3);
            pes_header.pes_packet_length = pes_size > 0xffff ? 0 : pes_size;
            pes_header.encode(&packet);

           if (pes_header.pts_dts_flags == 0x03) {
               write_pts(&packet, 3, frame->pts);
               write_pts(&packet, 1, frame->dts);
           } else {
               write_pts(&packet, 2, frame->pts);
           }
            first = false;
        } else {
            ts_header.encode(&packet);
        }

        uint32_t body_size = packet.size() - packet.pos();
        uint32_t in_size = frame->_data->size() - frame->_data->pos();
         if (body_size <= in_size) {
             packet.write_string(frame->_data->read_string(body_size));
         } else {
             uint16_t stuff_size = body_size - in_size;
             if (ts_header.adaptation_field_control == MpegTsAdaptationFieldType::adaption_only || ts_header.adaptation_field_control == MpegTsAdaptationFieldType::payload_adaption_both) {
                 char *base = packet.data() + 5 + packet.data()[4];
                 packet.set_data(base - packet.data() + stuff_size, base, packet.data() + packet.pos() - base);
                 memset(base, 0xff, stuff_size);
                 packet.skip(stuff_size);
                 packet.data()[4] += stuff_size;
             } else {
                 packet.data()[3] |= 0x20;
                 packet.set_data(188 - 4 - stuff_size, packet.data() + 4, packet.pos() - 4);
                 packet.skip(stuff_size);
                 packet.data()[4] = stuff_size - 1;
                 if (stuff_size >= 2) {
                    packet.data()[5] = 0;
                    memset(&(packet.data()[6]), 0xff, stuff_size - 2);
                 }
             }
             packet.write_string(frame->_data->read_string(in_size));
         }

        sb->append(packet.data(), packet.size());
    }
}

void MpegTsMuxer::create_pcr(SimpleBuffer *sb)
{
    uint64_t pcr = 0;
    TsHeader ts_header;
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 0;
    ts_header.transport_priority = 0;
    ts_header.pid = MPEGTS_PCR_PID;
    ts_header.transport_scrambling_control = 0;
    ts_header.adaptation_field_control = MpegTsAdaptationFieldType::adaption_only;
    ts_header.continuity_counter = 0;

    AdaptationFieldHeader adapt_field;
    adapt_field.adaptation_field_length = 188 - 4 - 1;
    adapt_field.discontinuity_indicator = 0;
    adapt_field.random_access_indicator = 0;
    adapt_field.elementary_stream_priority_indicator = 0;
    adapt_field.pcr_flag = 1;
    adapt_field.opcr_flag = 0;
    adapt_field.splicing_point_flag = 0;
    adapt_field.transport_private_data_flag = 0;
    adapt_field.adaptation_field_extension_flag = 0;

    char *p = sb->data();
    ts_header.encode(sb);
    adapt_field.encode(sb);
    write_pcr(sb, pcr);
}

void MpegTsMuxer::create_null(SimpleBuffer *sb)
{
    TsHeader ts_header;
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 0;
    ts_header.transport_priority = 0;
    ts_header.pid = MPEGTS_NULL_PACKET_PID;
    ts_header.transport_scrambling_control = 0;
    ts_header.adaptation_field_control = MpegTsAdaptationFieldType::payload_only;
    ts_header.continuity_counter = 0;
    ts_header.encode(sb);
}

void MpegTsMuxer::encode(TsFrame *frame, std::map<uint8_t, int> stream_pid_map, SimpleBuffer *sb)
{
    if (should_create_pat()) {
        uint8_t pat_pmt_cc = get_cc(0);
        create_pat(sb, pat_pmt_cc);
        create_pmt(sb, stream_pid_map, pat_pmt_cc);
    }

    create_pes(frame, sb);
}

void MpegTsMuxer::write_pcr(SimpleBuffer *sb, uint64_t pcr)
{
    sb->write_1byte((int8_t)(pcr >> 25));
    sb->write_1byte((int8_t)(pcr >> 17));
    sb->write_1byte((int8_t)(pcr >> 9));
    sb->write_1byte((int8_t)(pcr >> 1));
    sb->write_1byte((int8_t)(pcr << 7 | 0x7e));
    sb->write_1byte(0);
}

void MpegTsMuxer::write_pts(SimpleBuffer *sb, uint32_t fb, uint64_t pts)
{
    uint32_t val;

    val = fb << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    sb->write_1byte((int8_t)val);

    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    sb->write_2bytes((int16_t)val);

    val = (((pts) & 0x7fff) << 1) | 1;
    sb->write_2bytes((int16_t)val);
}

uint8_t MpegTsMuxer::get_cc(uint32_t with_pid)
{
    if (_pid_cc_map.find(with_pid) != _pid_cc_map.end()) {
        _pid_cc_map[with_pid] = (_pid_cc_map[with_pid] + 1) & 0x0F;
        return _pid_cc_map[with_pid];
    }

    _pid_cc_map[with_pid] = 0;
    return 0;
}

bool MpegTsMuxer::should_create_pat()
{
    bool ret = false;
    static const int pat_interval = 20;
    static int current_index = 0;
    if (current_index % pat_interval == 0) {
        if (current_index > 0) {
            current_index = 0;
        }
        ret = true;
    }

    current_index++;

    return ret;
}
