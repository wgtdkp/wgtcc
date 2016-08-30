#ifndef _WGTCC_STRING_PAIR_H_
#define _WGTCC_STRING_PAIR_H_

#include <string>


/*
class StrPair
{
public:
  StrPair(void): begin_(nullptr), end_(nullptr), _needFree(false) {}

  StrPair(char* begin, char* end, bool needFree=false)
      : begin_(begin), end_(end), _needFree(needFree) {}
  
  StrPair(char* begin, size_t len, bool needFree=false)
      : begin_(begin), end_(begin + len), _needFee(needFree) {}

  ~StrPair(void) {
    if (_needFree)
      delete[] begin_;
  }

  StrPair(const StrPair& other) {
    *this = other;
  }

  const StrPair& operator=(const StrPair& other) {
    //_needFree = other._needFree;

    if (other._needFree) {
      this->~StrPair();
      
      size_t len = other.end_ - other.begin_;
      begin_ = new char[len];
      memcpy(begin_, other.begin_, len);
      end_ = begin_ + len;
    } else {
      begin_ = other.begin_;
      end_ = other.end_;
    }

    _needFree = other._needFree;

    return *this;
  }

  bool operator<(const StrPair& other) const {
    char* p = begin_;
    char* q = other.begin_;
    for (; p != end_ && q != other.end_; p++, q++) {
      if (*p < *q)
        return true;
      else if (*p > *q)
        return false;
    }
    return p == end_;
  }

  size_t Len(void) {
    return end_ - begin_;
  }

  // Always deep copy
  operator std::string(void) {
    return std::string(begin_, end_);
  }

  char* begin_;
  char* end_;
  bool _needFree;
};
*/

struct StrPair
{
public:
  StrPair(void): begin_(nullptr), end_(nullptr) {}

  StrPair(char* begin, char* end): begin_(begin), end_(end) {}

  StrPair(char* begin, size_t len): begin_(begin), end_(begin + len) {}

  ~StrPair(void) {}

  StrPair(const StrPair& other) {
    *this = other;
  }

  const StrPair& operator=(const StrPair& other) {
    begin_ = other.begin_;
    end_ = other.end_;

    return *this;
  }

  bool operator<(const StrPair& other) const {
    char* p = begin_;
    char* q = other.begin_;
    for (; p != end_ && q != other.end_; p++, q++) {
      if (*p < *q)
        return true;
      else if (*p > *q)
        return false;
    }
    return p == end_;
  }

  size_t Len(void) {
    return end_ - begin_;
  }

  // Always deep copy
  operator std::string(void) const {
    return std::string(begin_, end_);
  }

  char* begin_;
  char* end_;
};

#endif

