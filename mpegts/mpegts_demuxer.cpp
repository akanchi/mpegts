#include "mpegts_demuxer.h"

#include "simple_buffer.h"
#include "ts_packet.h"
#include "common.h"

MpegTsDemuxer::MpegTsDemuxer()
    : _pmt_id(0)
    , _pcr_id(0)
{

}

MpegTsDemuxer::~MpegTsDemuxer()
{
}

int MpegTsDemuxer::decode(SimpleBuffer *in, TsFrame *&out)
{
    while (!in->empty()) {
        int pos = in->pos();
        TsHeader ts_header;
        ts_header.decode(in);

        // found pat & get pmt pid
        if (ts_header.pid == 0 && _pmt_id == 0) {
            if (ts_header.adaptation_field_control == 0x02 || ts_header.adaptation_field_control == 0x03) {
                AdaptationFieldHeader adapt_field;
                adapt_field.decode(in);
                in->skip(adapt_field.adaptation_field_length > 0 ? (adapt_field.adaptation_field_length - 1) : 0);
            }

            if (ts_header.adaptation_field_control == 0x01 || ts_header.adaptation_field_control == 0x03) {
                if (ts_header.payload_unit_start_indicator == 0x01) {
                    uint8_t point_field = in->read_1byte();
                }
                PATHeader pat_header;
                pat_header.decode(in);
                in->read_2bytes();
                _pmt_id = in->read_2bytes() & 0x1fff;
                pat_header.print();
            }
        }

        // found pmt
        if (_ts_frames.empty() && _pmt_id != 0 && ts_header.pid == _pmt_id) {
            if (ts_header.adaptation_field_control == 0x02 || ts_header.adaptation_field_control == 0x03) {
                AdaptationFieldHeader adapt_field;
                adapt_field.decode(in);
                in->skip(adapt_field.adaptation_field_length > 0 ? (adapt_field.adaptation_field_length - 1) : 0);
            }

            if (ts_header.payload_unit_start_indicator == 0x01) {
                uint8_t point_field = in->read_1byte();
                PMTHeader pmt_header;
                pmt_header.decode(in);
                _pcr_id = pmt_header.PCR_PID;
                for (size_t i = 0; i < pmt_header.infos.size(); i++) {
                    _ts_frames[pmt_header.infos[i]->elementary_PID] = std::shared_ptr<TsFrame>(new TsFrame(pmt_header.infos[i]->stream_type));
                    stream_pid_map[pmt_header.infos[i]->stream_type] = pmt_header.infos[i]->elementary_PID;
                }
                pmt_header.print();
            }
        }

        if (_ts_frames.find(ts_header.pid) != _ts_frames.end()) {
            uint8_t pcr_flag = 0;
            uint64_t pcr = 0;
            if (ts_header.adaptation_field_control == 0x02 || ts_header.adaptation_field_control == 0x03) {
                AdaptationFieldHeader adapt_field;
                adapt_field.decode(in);
                int adflength = adapt_field.adaptation_field_length;
                pcr_flag = adapt_field.pcr_flag;
                if (adapt_field.pcr_flag == 1) {
                    pcr = read_pcr(in);
                    adflength -= 6;
                }
                in->skip(adflength > 0 ? (adflength - 1) : 0);
            }

            if (ts_header.adaptation_field_control == 0x01 || ts_header.adaptation_field_control == 0x03) {
                PESHeader pes_header;
                if (ts_header.payload_unit_start_indicator == 0x01) {
                    if (_ts_frames[ts_header.pid]->completed) {
                        _ts_frames[ts_header.pid]->reset();
                    }

                    if (!_ts_frames[ts_header.pid]->empty()) {
                        _ts_frames[ts_header.pid]->completed = true;
                        _ts_frames[ts_header.pid]->pid = ts_header.pid;
                        out = _ts_frames[ts_header.pid].get();
                        in->skip(pos - in->pos());
                        return _ts_frames[ts_header.pid]->stream_type;
                    }

                    pes_header.decode(in);
                    _ts_frames[ts_header.pid]->stream_id = pes_header.stream_id;
                    if (pes_header.pts_dts_flags == 0x02) {
                        _ts_frames[ts_header.pid]->pts = _ts_frames[ts_header.pid]->dts = read_pts(in);
                    } else if (pes_header.pts_dts_flags == 0x03) {
                        _ts_frames[ts_header.pid]->pts = read_pts(in);
                        _ts_frames[ts_header.pid]->dts = read_pts(in);
                    }
                    if (pes_header.pes_packet_length != 0x0000) {
                        if ((pes_header.pes_packet_length - 3 - pes_header.header_data_length) >= 188 || (pes_header.pes_packet_length - 3 - pes_header.header_data_length) < 0) {
                            _ts_frames[ts_header.pid]->_data->append(in->data() + in->pos(), 188 - in->pos() - pos);
                        } else {
                            _ts_frames[ts_header.pid]->_data->append(in->data() + in->pos(), pes_header.pes_packet_length - 3 - pes_header.header_data_length);
                        }
                        
                        break;
                    }
                }
                _ts_frames[ts_header.pid]->_data->append(in->data() + in->pos(), 188 - in->pos() - pos);
            }
        } else if (_pcr_id != 0 && _pcr_id == ts_header.pid) {
            AdaptationFieldHeader adapt_field;
            adapt_field.decode(in);
            uint64_t pcr = read_pcr(in);
        }

        in->erase(188);
    }

    return 0;
}
