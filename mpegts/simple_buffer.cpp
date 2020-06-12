#include "simple_buffer.h"

#include <assert.h>
//#include <algorithm>
#include <iterator>

SimpleBuffer::SimpleBuffer(int size, char val)
    : mPos(0)
{
    if (size > 0)
        mData = std::vector<char>(size, val);
}

SimpleBuffer::~SimpleBuffer()
{
}

void SimpleBuffer::write_1byte(int8_t val)
{
    if (mPos < size()) {
        mData[mPos] = val;
    } else {
        mData.push_back(val);
    }
    
    mPos += 1;
}

void SimpleBuffer::write_2bytes(int16_t val)
{
    char *p = (char *)&val;

    for (int i = 1; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_3bytes(int32_t val)
{
    char *p = (char *)&val;

    for (int i = 2; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_4bytes(int32_t val)
{
    char *p = (char *)&val;

    for (int i = 3; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_8bytes(int64_t val)
{
    char *p = (char *)&val;

    for (int i = 7; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_string(std::string val)
{
    for (const char &c : val) {
        write_1byte(c);
    }
}

void SimpleBuffer::append(const char* bytes, int size)
{
    if (size <= 0)
        return;

    mData.insert(mData.end(), bytes, bytes + size);
}

int8_t SimpleBuffer::read_1byte()
{
    assert(require(1));

    int8_t val = mData.at(0 + mPos);
    mPos++;

    return val;
}

int16_t SimpleBuffer::read_2bytes()
{
    assert(require(2));

    int16_t val = 0;
    char *p = (char *)&val;

    for (int i = 1; i >= 0; --i) {
        p[i] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

int32_t SimpleBuffer::read_3bytes()
{
    assert(require(3));

    int32_t val = 0;
    char *p = (char *)&val;

    for (int i = 2; i >= 0; --i) {
        p[i] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

int32_t SimpleBuffer::read_4bytes()
{
    assert(require(4));

    int32_t val = 0;
    char *p = (char *)&val;

    for (int i = 3; i >= 0; --i) {
        p[i] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

int64_t SimpleBuffer::read_8bytes()
{
    assert(require(8));

    int64_t val = 0;
    char *p = (char *)&val;

    for (int i = 7; i >= 0; --i) {
        p[i] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

int64_t SimpleBuffer::read_nbytes(int n)
{
    assert(require(n));

    int64_t val = 0;
    char *p = (char *)&val;
    for (int i = n; i >= 0; --i) {
        p[i] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

std::string SimpleBuffer::read_string(int len)
{
    assert(require(len));

    std::string val(&mData[0] + mPos, len);
    mPos += len;

    return val;
}

void SimpleBuffer::skip(int size)
{
    mPos += size;
}

bool SimpleBuffer::require(int required_size)
{
    assert(required_size >= 0);

    return required_size <= (int)mData.size() - mPos;
}

bool SimpleBuffer::empty()
{
    return mPos >= (int)mData.size();
}

int SimpleBuffer::size()
{
    return mData.size();
}

int SimpleBuffer::pos()
{
    return mPos;
}

char *SimpleBuffer::data()
{
    return (size() == 0) ? nullptr : &mData[0];
}

void SimpleBuffer::erase(int size)
{
    if (size <= 0)
    {
        return;
    }

    mPos = 0;
    if (size >= this->size())
    {
        mData.clear();
        return;
    }

    mData.erase(mData.begin(), mData.begin() + size);
}

void SimpleBuffer::set_data(int pos, char *data, int len)
{
    if (!data)
        return;
    
    if (pos + len > size()) {
        return;
    }

    for (int i = 0; i < len; i++) {
        mData[pos + i] = data[i];
    }
}

std::string SimpleBuffer::to_string()
{
    return std::string(mData.begin(), mData.end());
}
