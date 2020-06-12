#ifndef __SIMPLE_BUFFER_H__
#define __SIMPLE_BUFFER_H__

#include <vector>
#include <string>
#include <stdint.h>

// only support little endian
class SimpleBuffer
{
public:
    SimpleBuffer(int size = 0, char val = 0x00);
    virtual ~SimpleBuffer();

public:
    void write_1byte(int8_t val);
    void write_2bytes(int16_t val);
    void write_3bytes(int32_t val);
    void write_4bytes(int32_t val);
    void write_8bytes(int64_t val);
    void write_string(std::string val);
    void append(const char* bytes, int size);

public:
    int8_t read_1byte();
    int16_t read_2bytes();
    int32_t read_3bytes();
    int32_t read_4bytes();
    int64_t read_8bytes();
    int64_t read_nbytes(int n); // n less than 8
    std::string read_string(int len);

public:
    void skip(int size);
    bool require(int required_size);
    bool empty();
    int size();
    int pos();
    char *data();
    void erase(int size);
    void set_data(int pos, char *data, int len);

public:
    std::string to_string();

private:
    std::vector<char> mData;
    int mPos;
};

#endif /* __SIMPLE_BUFFER_H__ */
