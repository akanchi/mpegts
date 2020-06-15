#include "simple_buffer.h"

#include <assert.h>
#include <iterator>

SimpleBuffer::SimpleBuffer(int lSize, char lVal)
        : mPos(0) {
    if (lSize > 0)
        mData = std::vector<char>(lSize, lVal);
}

SimpleBuffer::~SimpleBuffer() {
}

void SimpleBuffer::write_1byte(int8_t lVal) {
    if (mPos < size()) {
        mData[mPos] = lVal;
    } else {
        mData.push_back(lVal);
    }
    mPos++;
}

void SimpleBuffer::write_2bytes(int16_t lVal) {
    char *p = (char *) &lVal;

    for (int i = 1; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_3bytes(int32_t lVal) {
    char *p = (char *) &lVal;

    for (int i = 2; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_4bytes(int32_t lVal) {
    char *p = (char *) &lVal;

    for (int i = 3; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_8bytes(int64_t lVal) {
    char *p = (char *) &lVal;

    for (int i = 7; i >= 0; --i) {
        write_1byte(p[i]);
    }
}

void SimpleBuffer::write_string(std::string lVal) {
    for (const char &rC : lVal) {
        write_1byte(rC);
    }
}

void SimpleBuffer::append(const char *pBytes, int lSize) {
    if (lSize <= 0)
        return;

    mData.insert(mData.end(), pBytes, pBytes + lSize);
}

int8_t SimpleBuffer::read_1byte() {
    assert(require(1));

    int8_t lVal = mData.at(0 + mPos);
    mPos++;

    return lVal;
}

int16_t SimpleBuffer::read_2bytes() {
    assert(require(2));

    int16_t lVal = 0;
    char *p = (char *) &lVal;

    for (int lI = 1; lI >= 0; --lI) {
        p[lI] = mData.at(0 + mPos);
        mPos++;
    }

    return lVal;
}

int32_t SimpleBuffer::read_3bytes() {
    assert(require(3));

    int32_t val = 0;
    char *p = (char *) &val;

    for (int lI = 2; lI >= 0; --lI) {
        p[lI] = mData.at(0 + mPos);
        mPos++;
    }

    return val;
}

int32_t SimpleBuffer::read_4bytes() {
    assert(require(4));

    int32_t lVal = 0;
    char *p = (char *) &lVal;

    for (int lI = 3; lI >= 0; --lI) {
        p[lI] = mData.at(0 + mPos);
        mPos++;
    }

    return lVal;
}

int64_t SimpleBuffer::read_8bytes() {
    assert(require(8));

    int64_t lVal = 0;
    char *p = (char *) &lVal;

    for (int lI = 7; lI >= 0; --lI) {
        p[lI] = mData.at(0 + mPos);
        mPos++;
    }

    return lVal;
}


std::string SimpleBuffer::read_string(int lLen) {
    assert(require(lLen));

    std::string lVal(&mData[0] + mPos, lLen);
    mPos += lLen;

    return lVal;
}

void SimpleBuffer::skip(int lSize) {
    mPos += lSize;
}

bool SimpleBuffer::require(int lRequiredSize) {
    assert(lRequiredSize >= 0);

    return lRequiredSize <= (int) mData.size() - mPos;
}

bool SimpleBuffer::empty() {
    return mPos >= (int) mData.size();
}

int SimpleBuffer::size() {
    return mData.size();
}

int SimpleBuffer::pos() {
    return mPos;
}

char *SimpleBuffer::data() {
    return (size() == 0) ? nullptr : &mData[0];
}

void SimpleBuffer::erase(int lSize) {
    if (lSize <= 0) {
        return;
    }

    mPos = 0;
    if (lSize >= this->size()) {
        mData.clear();
        return;
    }

    mData.erase(mData.begin(), mData.begin() + lSize);
}

void SimpleBuffer::setData(int lPos, char *pData, int lLen) {
    if (!pData)
        return;

    if (lPos + lLen > size()) {
        return;
    }

    for (int lI = 0; lI < lLen; lI++) {
        mData[lPos + lI] = pData[lI];
    }
}

std::string SimpleBuffer::toString() {
    return std::string(mData.begin(), mData.end());
}
