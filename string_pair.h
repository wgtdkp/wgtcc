#ifndef _WGTCC_STRING_PAIR_H_
#define _WGTCC_STRING_PAIR_H_

#include <string>


/*
class StrPair
{
public:
    StrPair(void): _begin(nullptr), _end(nullptr), _needFree(false) {}

    StrPair(char* begin, char* end, bool needFree=false)
            : _begin(begin), _end(end), _needFree(needFree) {}
    
    StrPair(char* begin, size_t len, bool needFree=false)
            : _begin(begin), _end(begin + len), _needFee(needFree) {}

    ~StrPair(void) {
        if (_needFree)
            delete[] _begin;
    }

    StrPair(const StrPair& other) {
        *this = other;
    }

    const StrPair& operator=(const StrPair& other) {
        //_needFree = other._needFree;

        if (other._needFree) {
            this->~StrPair();
            
            size_t len = other._end - other._begin;
            _begin = new char[len];
            memcpy(_begin, other._begin, len);
            _end = _begin + len;
        } else {
            _begin = other._begin;
            _end = other._end;
        }

        _needFree = other._needFree;

        return *this;
    }

    bool operator<(const StrPair& other) const {
        char* p = _begin;
        char* q = other._begin;
        for (; p != _end && q != other._end; p++, q++) {
            if (*p < *q)
                return true;
            else if (*p > *q)
                return false;
        }
        return p == _end;
    }

    size_t Len(void) {
        return _end - _begin;
    }

    // Always deep copy
    operator std::string(void) {
        return std::string(_begin, _end);
    }

    char* _begin;
    char* _end;
    bool _needFree;
};
*/

struct StrPair
{
public:
    StrPair(void): _begin(nullptr), _end(nullptr) {}

    StrPair(char* begin, char* end): _begin(begin), _end(end) {}

    StrPair(char* begin, size_t len): _begin(begin), _end(begin + len) {}

    ~StrPair(void) {}

    StrPair(const StrPair& other) {
        *this = other;
    }

    const StrPair& operator=(const StrPair& other) {
        _begin = other._begin;
        _end = other._end;

        return *this;
    }

    bool operator<(const StrPair& other) const {
        char* p = _begin;
        char* q = other._begin;
        for (; p != _end && q != other._end; p++, q++) {
            if (*p < *q)
                return true;
            else if (*p > *q)
                return false;
        }
        return p == _end;
    }

    size_t Len(void) {
        return _end - _begin;
    }

    // Always deep copy
    operator std::string(void) const {
        return std::string(_begin, _end);
    }

    char* _begin;
    char* _end;
};

#endif

