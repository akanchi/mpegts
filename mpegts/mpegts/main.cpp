#include <iostream>
#include <memory>
#include <fstream>
#include "mpegts_muxer.h"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"

std::ofstream outts("out.ts", std::ios::binary);

std::map<uint16_t, std::ofstream*> file_map;

void write_file(TsFrame *frame)
{
    if (!frame)
        return;

    if (file_map.find(frame->pid) == file_map.end()) {
        std::string ext = ".unknown";
        if (frame->stream_type == 0x0f) {
            ext = ".aac";
        } else if (frame->stream_type == 0x1b) {
            ext = ".264";
        }
        file_map[frame->pid] = new std::ofstream(std::to_string(frame->pid) + ext, std::ios::binary);
    }

    file_map[frame->pid]->write(frame->_data->data(), frame->_data->size());
}

int main()
{
    std::shared_ptr<MpegTsMuxer> muxer(new MpegTsMuxer);

    MpegTsDemuxer demuxer;
    char packet[188] = { 0 };
    std::ifstream ifile("./resource/test1.ts", std::ios::binary | std::ios::in);
    std::cout << ifile.is_open() << std::endl;

    SimpleBuffer in;
    SimpleBuffer out;

    while (!ifile.eof()) {
        ifile.read(packet, 188);
        in.append(packet, 188);

        TsFrame *frame = nullptr;
        demuxer.decode(&in, frame);

        write_file(frame);
        if (frame) {
            muxer->encode(frame, demuxer.stream_pid_map, &out);
            outts.write(out.data(), out.size());
            out.erase(out.size());
        }
    }

    ifile.close();
    for (auto it = file_map.begin(); it != file_map.end(); it++) {
        it->second->close();
    }

    return 0;
}
