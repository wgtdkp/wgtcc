#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <map>
#include <list>
#include <cassert>

/********* Type System ***********/
class Env;
class Type;
class Variable;
class ArithmType;
class ArrayType;
class FuncType;
class PointerType;
class StructUnionType;
class UnionType;
class EnumType;


class Type
{
public:
	static const int _machineWord = 4;

	enum QUAL{
		NONE = 0x00,
		CONST = 0x01,
		RESTRICT = 0x02,
		VOLATILE = 0x04,
		ATOMIC = 0x08,
	};

	bool operator!=(const Type& other) const {
		return !(*this == other);
	}

	virtual bool operator==(const Type& other) const = 0;
	virtual bool Compatible(const Type& ohter) const = 0;
	virtual ~Type(void) {}

	int Width(void) const {
		return _width;
	}

	int Align(void) const {
		//TODO: return the aligned width
		return _width;
	}

	int Qual(void) const {
		return _qual;
	}

	int SetQual(unsigned char qual) {
		_qual = qual;
	}

	bool IsConst(void) const {
		return _qual & CONST;
	}

	virtual ArithmType* ToArithmType(void) {
		return nullptr;
	}

	virtual const ArithmType* ToArithmType(void) const {
		return nullptr;
	}

	virtual ArrayType* ToArrayType(void) {
		return nullptr;
	}

	virtual const ArrayType* ToArrayType(void) const {
		return nullptr;
	}

	virtual FuncType* ToFuncType(void) {
		return nullptr;
	}

	virtual const FuncType* ToFuncType(void) const {
		return nullptr;
	}

	virtual PointerType* ToPointerType(void) {
		return nullptr;
	}

	virtual const PointerType* ToPointerType(void) const {
		return nullptr;
	}

	virtual StructUnionType* ToStructUnionType(void) {
		return nullptr;
	}

	virtual const StructUnionType* ToStructUnionType(void) const {
		return nullptr;
	}

	virtual EnumType* ToEnumType(void) {
		return nullptr;
	}

	virtual const EnumType* ToEnumType(void) const {
		return nullptr;
	}

	//static IntType* NewIntType();
	static FuncType* NewFuncType(Type* derived, const std::list<Type*>& params);
	static PointerType* NewPointerType(Type* derived);
	static StructUnionType* NewStructUnionType(Env* env);
	static EnumType* NewEnumType();
	
	static ArithmType* NewArithmType(int tag);

protected:
	Type(int width) : _width(width) {}

	//the bytes to store object of that type
	int _width;
	unsigned char _qual;
};


class ArithmType : public Type
{
	friend class Type;
public:
	enum {
		TBOOL = 0, TCHAR, TUCHAR,
		TSHORT, TUSHORT, TINT, TUINT,
		TLONG, TULONG, TLLONG, TULLONG,
		TFLOAT, TDOUBLE, TLDOUBLE,
		TFCOMPLEX, TDCOMPLEX, TLDCOMPLEX, SIZE
	};

	virtual ~ArithmType(void) {}

	virtual ArithmType* ToArithmType(void) {
		return this;
	}

	virtual const ArithmType* ToArithmType(void) const {
		return this;
	}

	virtual bool operator==(const Type& other) const {
		auto ArithmType = other.ToArithmType();
		return (nullptr != ArithmType && _tag == ArithmType->_tag);
	}

	virtual bool Compatible(const Type& other) const {
		//TODO: 
		return false;
	}

	bool IsBool(void) const {
		return TBOOL == _tag;
	}

	bool IsInteger(void) const {
		return (TBOOL <= _tag) && (_tag <= TULLONG);
	}

	bool IsReal(void) const {
		return (TFLOAT <= _tag) && (_tag <= TLDOUBLE);
	}

	bool IsComplex(void) const {
		return (TFCOMPLEX <= _tag) && (_tag <= TLDCOMPLEX);
	}

protected:
	ArithmType(int tag) : _tag(tag), Type(CalcWidth(tag)) {
		assert(TBOOL <= tag && tag <= TLDCOMPLEX);
	}

private:
	int _tag;
	static int CalcWidth(int tag);
};


class DerivedType : public Type
{
	//friend class Type;
public:
	Type* Derived(void) {
		return _derived;
	}

	const Type* Derived(void) const {
		return _derived;
	}

protected:
	DerivedType(Type* derived, int width)
		: _derived(derived), Type(width) {}

	Type* _derived;
};


class PointerType : public DerivedType
{
	friend class Type;
public:
	~PointerType(void) {}

	virtual PointerType* ToPointerType(void) {
		return this;
	}

	virtual const PointerType* ToPointerType(void) const {
		return this;
	}

	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;

protected:
	PointerType(Type* derived)
		: DerivedType(derived, _machineWord) {}

private:

};


class ArrayType : public PointerType
{
	friend class Type;
public:
	virtual ~ArrayType(void) {
		delete _derived;
	}

	virtual ArrayType* ToArrayType(void) {
		return this;
	}

	virtual const ArrayType* ToArrayType(void) const {
		return this;
	}

	virtual PointerType* ToPointerType(void) {
		return this;
	}

	virtual const PointerType* ToPointerType(void) const {
		return this;
	}

	virtual bool operator==(const Type& other) const {
		auto otherArray = ToArrayType();
		return (nullptr != otherArray
			&& _len == otherArray->_len
			&& *_derived == *otherArray->_derived);
	}

	virtual bool Compactible(const Type& other) const {
		auto otherArray = ToArrayType();
		return (nullptr != otherArray
			&& _len == otherArray->_len
			&& _derived->Compatible(*otherArray->_derived));
	}

protected:
	ArrayType(size_t len, Type* derived)
		: _len(len), PointerType(derived) {
		//TODO: calc the _width
	}

private:
	size_t _len;
};


class FuncType : public DerivedType
{
	friend class Type;
public:
	~FuncType(void) {}

	virtual FuncType* ToFuncType(void) {
		return this;
	}

	virtual const FuncType* ToFuncType(void) const {
		return this;
	}

	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;

protected:
	//a function does not has the width property
	FuncType(Type* derived, const std::list<Type*>& params)
		: DerivedType(_derived, -1), _params(params) {}

private:
	std::list<Type*> _params;
};


class StructUnionType : public Type
{
	friend class Type;
public:
	~StructUnionType(void) {
		//TODO: delete _env ?
	}

	virtual StructUnionType* ToStructUnionType(void) {
		return this;
	}

	virtual const StructUnionType* ToStructUnionType(void) const {
		return this;
	}

	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;

	Variable* Find(const char* name);
	const Variable* Find(const char* name) const;

protected:
	explicit StructUnionType(Env* env)
		: _env(env), Type(CalcWidth(env)) {}

private:
	static int CalcWidth(const Env* env);

	Env* _env;
	//member map
};


class EnumType : public Type
{
private:
	//member map
};




typedef Variable Symbol;

struct StrCmp
{
	bool operator()(const char* lhs, const char* rhs) const {
		return strcmp(lhs, rhs) < 0;
	}
};


class Env
{
	friend class StructUnionType;
public:
	explicit Env(Env* parent=nullptr) 
		: _parent(parent), _offset(0) {}

	Symbol* Find(const char* name);
	const Symbol* Find(const char* name) const;

	Type* FindType(const char* name);
	const Type* FindType(const char* name) const;

	Variable* FindVar(const char* name);
	const Variable* FindVar(const char* name) const;

	void InsertType(const char* name, Type* type);
	void InsertVar(const char* name, Type* type);

	bool operator==(const Env& other) const;

private:
	Env* _parent;
	int _offset;

	std::map<const char*, Symbol*, StrCmp> _mapSymb;
};

#endif
