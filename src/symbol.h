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
		QNONE = 0x00,
		QCONST = 0x01,
		QRESTRICT = 0x02,
		QVOLATILE = 0x04,
		QATOMIC = 0x08,
	};

	enum STORAGE {
		SNONE = 0x00,
		STYPEDEF = 0x01,
		SEXTERN = 0x02,
		SSTATIC = 0x04,
		STHREAD_LOCAL = 0x08,
		SAUTO = 0x10,
		SREGISTER = 0x20,
	};

	enum FUNC_SPEC {
		FNONE = 0x00,
		FINLINE = 0x01,
		FONRETURN = 0x02,
	};

	enum TYPE_SPEC {
		SIGNED = 0x00,
		VOID = 0x01,
		CHAR = 0x02,
		SHORT = 0x04,
		INT = 0x08,
		LONG = 0x10,
		FLOAT = 0x20,
		DOUBLE = 0x40,
		UNSIGNED = 0x100,
		BOOL = 0x200,
		COMPLEX = 0x400,
		ATOM = 0x800,
		STRUCT = 0x1000,
		UNION = 0x2000,
		ENUM = 0x4000,
		TYPEDEF = 0x8000,
	};

	enum TYPE {
		TVOID = TYPE_SPEC::VOID, 
		TBOOL = TYPE_SPEC::BOOL, 
		TCHAR = TYPE_SPEC::SIGNED, 
		TUCHAR = TYPE_SPEC::CHAR | TYPE_SPEC::UNSIGNED,
		TSHORT = TYPE_SPEC::SHORT, 
		TUSHORT = TYPE_SPEC::SHORT | TYPE_SPEC::UNSIGNED, 
		TINT = TYPE_SPEC::INT, 
		TUINT = TYPE_SPEC::INT | TYPE_SPEC::UNSIGNED,
		TLONG = TYPE_SPEC::LONG, 
		TULONG = TYPE_SPEC::LONG | TYPE_SPEC::UNSIGNED, 
		TLLONG = TLONG | (1 << 32),
		TULLONG = TULONG | (1 << 32),
		TFLOAT = TYPE_SPEC::FLOAT,
		TDOUBLE = TYPE_SPEC::DOUBLE,
		TLDOUBLE = TYPE_SPEC::LONG | TYPE_SPEC::DOUBLE,
		TFCOMPLEX = TYPE_SPEC::FLOAT | TYPE_SPEC::COMPLEX, 
		TDCOMPLEX = TYPE_SPEC::DOUBLE | TYPE_SPEC::COMPLEX, 
		TLDCOMPLEX = TYPE_SPEC::LONG | TYPE_SPEC::DOUBLE | TYPE_SPEC::COMPLEX,
		SIZE = 18,
	};

	static int QualOfToken(int tokTag) {
		switch (tokTag) {
		case Token::CONST: return QUAL::QCONST;
		case Token::RESTRICT: return QUAL::QRESTRICT;
		case Token::VOLATILE: return QUAL::QVOLATILE;
		case Token::ATOMIC: return QUAL::QATOMIC;
		default: return QUAL::QNONE;
		}
	}

	static int StorageOfToken(int tokTag) {
		switch (tokTag) {
		case Token::TYPEDEF: return STORAGE::STYPEDEF;
		case Token::EXTERN: return STORAGE::SEXTERN;
		case Token::STATIC: return STORAGE::SSTATIC;
		case Token::THREAD_LOCAL: return STORAGE::STHREAD_LOCAL;
		case Token::AUTO: return STORAGE::SAUTO;
		case Token::REGISTER: return STORAGE::SREGISTER;
		default: return STORAGE::SNONE;
		}
	}

	static int FuncSpecOfToken(int tokTag) {
		switch (tokTag) {
		case Token::INLINE: return FUNC_SPEC::FINLINE;
		case Token::NORETURN: return FUNC_SPEC::FONRETURN;
		default: return FUNC_SPEC::FNONE;
		}
	}

	static int TypeSpecOfToken(int tokTag) {
		switch (tokTag) {
		case Token::VOID: return TYPE::TVOID;
			SIGNED = 0x00,
				VOID = 0x01,
				CHAR = 0x02,
				SHORT = 0x04,
				INT = 0x08,
				LONG = 0x10,
				FLOAT = 0x20,
				DOUBLE = 0x40,
				UNSIGNED = 0x100,
				BOOL = 0x200,
				COMPLEX = 0x400,
				ATOM = 0x800,
				STRUCT = 0x1000,
				UNION = 0x2000,
				ENUM = 0x4000,
				TYPEDEF = 0x8000,
		}
	}

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
		return _align;
	}

	int Qual(void) const {
		return _qual;
	}

	int SetQual(int qual) {
		_qual = qual;
	}

	bool IsConst(void) const {
		return _qual & QCONST;
	}

	bool IsScalar(void) const {
		return (nullptr != ToArithmType() 
			|| nullptr != ToPointerType());
	}

	bool IsInteger(void) const;
	bool IsReal(void) const;
	bool IsArithm(void) const {
		return (nullptr != ToArithmType());
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
	explicit Type(int width) : _width(width) {}

	//the bytes to store object of that type
	int _width;
	int _align;
	int _storage;
	int _qual;
};


class ArithmType : public Type
{
	friend class Type;
public:
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
	bool _isInline;
	bool _isNoReturn;
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
