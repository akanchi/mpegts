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

MpegTsMuxer::MpegTsMuxer()
{
}

MpegTsMuxer::~MpegTsMuxer()
{
}

void MpegTsMuxer::create_pat(SimpleBuffer *sb, uint16_t pmt_pid, uint8_t cc)
{
    SimpleBuffer pat_sb;
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
    uint16_t program_map_PID = 0xe000 | (pmt_pid & 0x1fff);

    unsigned int section_length = 4 + 4 + 5;
    pat_header.section_length = section_length & 0x3ff;

    ts_header.encode(&pat_sb);
    adapt_field.encode(&pat_sb);
    pat_header.encode(&pat_sb);
    pat_sb.write_2bytes(program_number);
    pat_sb.write_2bytes(program_map_PID);

    // crc32
    uint32_t crc_32 = crc32((uint8_t *)pat_sb.data() + 5, pat_sb.size() - 5);
    pat_sb.write_4bytes(crc_32);

    std::vector<char> stuff(188 - pat_sb.size(), 0xff);
    pat_sb.append((const char *)stuff.data(), stuff.size());

    sb->append(pat_sb.data(), pat_sb.size());
}

void MpegTsMuxer::create_pmt(SimpleBuffer *sb, std::map<uint8_t, int> stream_pid_map, uint16_t pmt_pid, uint8_t cc)
{
    SimpleBuffer pmt_sb;
    TsHeader ts_header;
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 1;
    ts_header.transport_priority = 0;
    ts_header.pid = pmt_pid;
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

    ts_header.encode(&pmt_sb);
    adapt_field.encode(&pmt_sb);
    pmt_header.encode(&pmt_sb);

    // crc32
    uint32_t crc_32 = crc32((uint8_t *)pmt_sb.data() + 5, pmt_sb.size() - 5);
    pmt_sb.write_4bytes(crc_32);

    std::vector<char> stuff(188 - pmt_sb.size(), 0xff);
    pmt_sb.append((const char *)stuff.data(), stuff.size());

    sb->append(pmt_sb.data(), pmt_sb.size());
}

void MpegTsMuxer::create_pes(TsFrame *frame, SimpleBuffer *sb)
{
    bool first = true;
    while (!frame->_data->empty()) {
        SimpleBuffer packet;

        TsHeader ts_header;
        ts_header.pid = frame->pid;
        ts_header.adaptation_field_control = MpegTsAdaptationFieldType::payload_only;
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

        uint32_t pos = packet.size();
        uint32_t body_size = 188 - pos;
        std::vector<char> body(body_size, 0);
        packet.append((const char *)body.data(), body_size);
        packet.skip(pos);
        uint32_t in_size = frame->_data->size() - frame->_data->pos();
         if (body_size <= in_size) {    // MpegTsAdaptationFieldType::payload_only or MpegTsAdaptationFieldType::payload_adaption_both for AVC
             packet.set_data(pos, frame->_data->data() + frame->_data->pos(), body_size);
             frame->_data->skip(body_size);
         } else {
             uint16_t stuff_size = body_size - in_size;
             if (ts_header.adaptation_field_control == MpegTsAdaptationFieldType::adaption_only || ts_header.adaptation_field_control == MpegTsAdaptationFieldType::payload_adaption_both) {
                 char *base = packet.data() + 5 + packet.data()[4];
                 packet.set_data(base - packet.data() + stuff_size, base, packet.data() + packet.pos() - base);
                 memset(base, 0xff, stuff_size);
                 packet.skip(stuff_size);
                 packet.data()[4] += stuff_size;
             } else {
                 // adaptation_field_control |= 0x20 == MpegTsAdaptationFieldType::payload_adaption_both
                 packet.data()[3] |= 0x20;
                 packet.set_data(188 - 4 - stuff_size, packet.data() + 4, packet.pos() - 4);
                 packet.skip(stuff_size);
                 packet.data()[4] = stuff_size - 1;
                 if (stuff_size >= 2) {
                    packet.data()[5] = 0;
                    memset(&(packet.data()[6]), 0xff, stuff_size - 2);
                 }
             }

             packet.set_data(packet.pos(), frame->_data->data() + frame->_data->pos(), in_size);
             frame->_data->skip(in_size);
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

void MpegTsMuxer::encode(TsFrame *frame, std::map<uint8_t, int> stream_pid_map, uint16_t pmt_pid, SimpleBuffer *sb)
{
    if (should_create_pat()) {
        uint8_t pat_pmt_cc = get_cc(0);
        create_pat(sb, pmt_pid, pat_pmt_cc);
        create_pmt(sb, stream_pid_map, pmt_pid, pat_pmt_cc);
    }

    create_pes(frame, sb);
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
