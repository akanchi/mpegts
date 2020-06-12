#pragma once

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// l local scope

#include <vector>
#include <string>
#include <stdint.h>

// only support little endian
class SimpleBuffer {
public:
    SimpleBuffer(int lSize = 0, char lVal = 0x00);

    virtual ~SimpleBuffer();

    void write_1byte(int8_t lVal);

    void write_2bytes(int16_t lVal);

    void write_3bytes(int32_t lVal);

    void write_4bytes(int32_t lVal);

    void write_8bytes(int64_t lVal);

    void write_string(std::string lVal);

    void append(const char *pBytes, int lSize);

    int8_t read_1byte();

    int16_t read_2bytes();

    int32_t read_3bytes();

    int32_t read_4bytes();

    int64_t read_8bytes();

    int64_t read_nbytes(int lN); // n less than 8
    std::string read_string(int lLen);

    void skip(int lSize);

    bool require(int lRequiredSize);

    bool empty();

    int size();

    int pos();

    char *data();

    void erase(int lSize);

    void setData(int lPos, char *pData, int lLen);

    std::string toString();

private:
    std::vector<char> mData;
    int mPos;
};

