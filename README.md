# MPEG-TS
A simple C++ implementation of a MPEG-TS Muxer and Demuxer


# Build

Requires cmake version >= **3.10** and **C++14**

**Release:**

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

***Debug:***

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

Outputs the mpegts static library and the example executables 

# Usage

Demuxer ->

```cpp

//Create a input buffer
SimpleBuffer in;
//Create a demuxer
MpegTsDemuxer demuxer;


//Append data to the input buffer 
in.append(packet, 188);

//Demux the data
TsFrame *frame = nullptr;
demuxer.decode(&in, frame);

//If the TsFrame != nullptr there is a demuxed frame availabile
//Example usage of the demuxer can be seen here ->
//https://github.com/Unit-X/ts2efp/blob/master/main.cpp


```

Muxer ->
 
TBD
