#include <iostream>
#include <memory>
#include <fstream>
#include "mpegts/mpegts_muxer.h"
#include "mpegts/mpegts_demuxer.h"
#include "mpegts/simple_buffer.h"
#include "mpegts/ts_packet.h"


int main(int argc, char *argv[])
{
  std::cout << "TS - demuxlib test " << std::endl;

  std::ifstream ifile("../bars.ts", std::ios::binary | std::ios::in);
  char packet[188] = {0};
  SimpleBuffer in;
  MpegTsDemuxer demuxer;
  while (!ifile.eof()) {
    ifile.read(packet, 188);
    in.append(packet, 188);
    TsFrame *frame = nullptr;
    demuxer.decode(&in, frame);

    if (frame) {

      std::cout << "GOT: " << unsigned(frame->mStreamType);

      if (frame->mStreamType == 0x0f) {
        std::cout << " AAC Frame";
      } else if (frame->mStreamType == 0x1b) {
        std::cout << " H.264 frame";
      }

      std::cout << std::endl;
    }
  }
  return EXIT_SUCCESS;
}
