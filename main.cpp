#include <iostream>
#include <memory>
#include <fstream>
#include "mpegts_muxer.h"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "flv_muxer.h"

std::map<uint16_t, std::ofstream*> file_map;

void write_file(TsFrame *frame)
{
    if (!frame)
        return;

    if (file_map.find(frame->mPid) == file_map.end()) {
        std::string ext = ".unknown";
        if (frame->mStreamType == 0x0f) {
            ext = ".aac";
        } else if (frame->mStreamType == 0x1b) {
            ext = ".264";
        }
        file_map[frame->mPid] = new std::ofstream(std::to_string(frame->mPid) + ext, std::ios::binary);
    }

    file_map[frame->mPid]->write(frame->mData->data(), frame->mData->size());
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        std::cout << "usage: ./mpegts.out in.ts" << std::endl;
        return 0;
    }

    std::string in_file_name = argv[1];
    std::string out_file_name = in_file_name + "out.ts";
    std::string out_flv_fileName = in_file_name + "out.flv";
    if (argc > 2) {
        out_file_name = argv[2];
    }

    std::cout << "input ts: " << in_file_name << std::endl;
    std::cout << "output ts: " << out_file_name << std::endl;

    std::ifstream ifile(in_file_name, std::ios::binary | std::ios::in);
    std::ofstream outts(out_file_name, std::ios::binary);
    std::ofstream outflv(out_flv_fileName, std::ios::binary);

    std::shared_ptr<MpegTsMuxer> muxer(new MpegTsMuxer);
    std::shared_ptr<FLVMuxer> flvMuxer(new FLVMuxer);
    SimpleBuffer flvOutBuffer;
    flvMuxer->writeHeader(&flvOutBuffer);
    flvMuxer->writeMetadata(&flvOutBuffer, 0);

    MpegTsDemuxer demuxer;
    char packet[188] = { 0 };

    SimpleBuffer in;
    SimpleBuffer out;

    while (!ifile.eof()) {
        ifile.read(packet, 188);
        in.append(packet, 188);
        in.skip(-188);

        TsFrame *frame = nullptr;
        demuxer.decode(&in, frame);

        write_file(frame);
        if (frame) {
            frame->mData->skip(0 - frame->mData->pos());
            muxer->encode(frame, demuxer.mStreamPidMap, demuxer.mPmtId, &out);
            flvMuxer->writeBody(frame, &flvOutBuffer);
            outflv.write(flvOutBuffer.data(), flvOutBuffer.size());
            flvOutBuffer.erase(flvOutBuffer.size());
            outts.write(out.data(), out.size());
            out.erase(out.size());
        }
    }

    {
        outflv.seekp(0, std::ios::end);
        uint32_t fileSize = outflv.tellp();
        flvMuxer->writeMetadata(&flvOutBuffer, fileSize);
        outflv.seekp(13, std::ios::beg);
        outflv.write(flvOutBuffer.data(), flvOutBuffer.size());
    }

    ifile.close();
    for (auto it = file_map.begin(); it != file_map.end(); it++) {
        it->second->close();
    }

    return 0;
}
