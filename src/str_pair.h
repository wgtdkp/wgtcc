#ifndef _STR_PAIR_H_
#define _STR_PAIR_H_

#include <cassert>
#include <cstring>
#include <xhash>

class StrPair
{
	friend struct std::hash<StrPair>;
public:
	StrPair(const char* begin=0, const char* end=0) :
		_begin(begin), _end(end), _needDelete(false) {}
	
	void Set(const char* begin, const char* end) {
		_begin = const_cast<char*>(begin);
		_end = const_cast<char*>(end);
		_needDelete = false;
	}

	StrPair(const char*str) {
		SetStr(str);
	}

	void SetStr(const char* str) {
		assert(nullptr != str);
		size_t len = strlen(str);
		auto begin = new char[len + 1];
		memcpy(begin, str, len);
		begin[len] = 0;
		_begin = begin;
		_end = begin + len;
		_needDelete = true;
	}

	bool operator==(const StrPair& other) const {
		if (other._end - other._begin != _end - _begin)
			return false;
		return (0 == strncmp(_begin, other._begin, _end - _begin));
	}

	~StrPair(void) {
		if (_needDelete) {
			delete[] _begin;
		}
	}

	void print(void);

private:
	const char* _begin;
	const char* _end;
	bool _needDelete;

	StrPair(const StrPair& other);	// not supported
	void operator=(const StrPair& other);	// not supported
};

/*
namespace std
{

template<>
struct hash<StrPair>
{
public:
	size_t operator()(const StrPair& str) const {
		return std::hash<int>()((int)str._begin)
			^ std::hash<int>()((int)str._end);
	}
};

}
*/


#endif
