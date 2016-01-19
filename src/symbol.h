#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <map>
#include <list>
#include <cassert>

/********* Type System ***********/
class Env;
class Type;
class Variable;
class Constant;
class ArithmType;
class DerivedType;
class ArrayType;
class FuncType;
class PointerType;
class StructUnionType;
class EnumType;


enum {
	/*****storage-class-specifiers*****/
	S_TYPEDEF = 0x01,
	S_EXTERN = 0x02,
	S_STATIC = 0x04,
	S_THREAD = 0x08,
	S_AUTO = 0x10,
	S_REGISTER = 0x20,

	/*****type-specifier*****/
	T_SIGNED = 0x40,
	T_UNSIGNED = 0x80,
	T_CHAR = 0x100,
	T_SHORT = 0x200,
	T_INT = 0x400,
	T_LONG = 0x800,
	T_VOID = 0x1000,
	T_FLOAT = 0x2000,
	T_DOUBLE = 0x4000,
	T_BOOL = 0x8000,
	T_COMPLEX = 0x10000,
	T_ATOMIC = 0x20000,
	T_STRUCT_UNION = 0x40000,
	T_ENUM = 0x80000,
	T_TYPEDEF_NAME = 0x100000,

	/*****type-qualifier*****/
	Q_CONST = 0x200000,
	Q_RESTRICT = 0x400000,
	Q_VOLATILE = 0x800000,
	Q_ATOMIC = 0x1000000,

	T_LONG_LONG = 0x2000000,

	/*****function specifier*****/
	F_INLINE = 0x4000000,
	F_NORETURN = 0x8000000,
};


class Type
{
public:
	static const int _machineWord = 4;

	bool operator!=(const Type& other) const { return !(*this == other); }

	virtual bool operator==(const Type& other) const = 0;
	virtual bool Compatible(const Type& ohter) const = 0;
	virtual ~Type(void) {}

	int Width(void) const {return _width;}
	void SetWidth(int width) { _width = width; }
	int Align(void) const {return _align;}
	void SetAlign(int align) { _align = align; }
	int Qual(void) const {return _qual;}
	int SetQual(int qual) {_qual = qual;}
	bool IsComplete(void) const { return _complete; }
	void SetComplete(bool complete) { _complete = complete; }

	bool IsConst(void) const {return _qual & Q_CONST;}
	bool IsScalar(void) const {
		return (nullptr != ToArithmType() 
			|| nullptr != ToPointerType());
	}

	bool IsInteger(void) const;
	bool IsArithm(void) const { return (nullptr != ToArithmType()); }

	virtual VoidType* ToVoidType(void) { return nullptr; }
	virtual const VoidType* ToVoidType(void) const { return nullptr; }
	virtual ArithmType* ToArithmType(void) { return nullptr; }
	virtual const ArithmType* ToArithmType(void) const { return nullptr; }
	virtual ArrayType* ToArrayType(void) {return nullptr;}
	virtual const ArrayType* ToArrayType(void) const {return nullptr;}
	virtual FuncType* ToFuncType(void) {return nullptr;}
	virtual const FuncType* ToFuncType(void) const {return nullptr;}
	virtual PointerType* ToPointerType(void) {return nullptr;}
	virtual const PointerType* ToPointerType(void) const {return nullptr;}
	virtual DerivedType* ToDerivedType(void) { return nullptr; }
	virtual const DerivedType* ToDerivedType(void) const { return nullptr; }
	virtual StructUnionType* ToStructUnionType(void) {return nullptr;}
	virtual const StructUnionType* ToStructUnionType(void) const {return nullptr;}


	virtual EnumType* ToEnumType(void) {
		return nullptr;
	}

	virtual const EnumType* ToEnumType(void) const {
		return nullptr;
	}

	//static IntType* NewIntType();
	static VoidType* NewVoidType(void);
	static ArrayType* NewArrayType(long long len, Type* eleType);
	static FuncType* NewFuncType(Type* derived, int funcSpec, \
		bool hasEllipsis, const std::list<Type*>& params = std::list<Type*>());
	static PointerType* NewPointerType(Type* derived);
	static StructUnionType* NewStructUnionType(bool isStruct);
	//static EnumType* NewEnumType();
	static ArithmType* NewArithmType(int tag);

protected:
	explicit Type(int width, bool complete) 
		: _width(width), _complete(complete) {}

	//the bytes to store object of that type
	int _width;
	int _align;
	int _qual;
	bool _complete;
};


class VoidType : public Type
{
	friend class Type;
public:
	virtual ~VoidType(void) {}
	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;
protected:
	VoidType(void) : Type(0, true) {}
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
		return T_BOOL == _tag;
	}

	bool IsInteger(void) const {
		return (_tag & T_BOOL) || \
			   (_tag & T_SHORT) || \
			   (_tag & T_INT) || \
			   (_tag & T_LONG) || \
			   (_tag & T_LONG_LONG) || \
			   (_tag & T_UNSIGNED);
	}

	bool IsFloat(void) const {
		return (_tag & T_FLOAT) || (_tag & T_DOUBLE);
	}

	bool IsComplex(void) const {
		return _tag & T_COMPLEX;
	}

protected:
	explicit ArithmType(int tag) 
		: _tag(tag), Type(CalcWidth(tag), true) {}
private:
	int _tag;
	static int CalcWidth(int tag);
};


class DerivedType : public Type
{
	//friend class Type;
public:
	Type* Derived(void) { return _derived; }
	const Type* Derived(void) const { return _derived;}
	void SetDerived(Type* derived) { _derived = derived; }
	virtual DerivedType* ToDerivedType(void) { return this; }
	virtual const DerivedType* ToDerivedType(void) const { return this; }

protected:
	DerivedType(Type* derived, int width)
		: _derived(derived), Type(width, true) {}

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
			&& _width == otherArray->_width
			&& *_derived == *otherArray->_derived);
	}

	virtual bool Compactible(const Type& other) const {
		auto otherArray = ToArrayType();
		return (nullptr != otherArray
			&& _width == otherArray->_width
			&& _derived->Compatible(*otherArray->_derived));
	}

protected:
	ArrayType(long long len, Type* derived)
		: PointerType(derived) {
		SetComplete(len > 0);	//如果len < 0,那么此类型不完整
		SetWidth(len > 0 ? len * derived->Width() : 0);
		SetQual(Q_CONST);
		
	}

};


class FuncType : public DerivedType
{
	friend class Type;
public:
	~FuncType(void) {}
	virtual FuncType* ToFuncType(void) { return this; }
	virtual const FuncType* ToFuncType(void) const { return this; }
	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;
	//bool IsInline(void) const { _inlineNoReturn & F_INLINE; }
	//bool IsNoReturn(void) const { return _inlineNoReturn & F_NORETURN; }
protected:
	//a function does not has the width property
	FuncType(Type* derived, int inlineReturn, bool hasEllipsis, const std::list<Type*>& params = std::list<Type*>())
		: DerivedType(_derived, -1), _inlineNoReturn(inlineReturn), _hasEllipsis(hasEllipsis), _params(params) {}

private:
	int _inlineNoReturn;
	bool _hasEllipsis;
	std::list<Type*> _params;
	
};


class StructUnionType : public Type
{
	friend class Type;
public:
	~StructUnionType(void) {/*TODO: delete _env ?*/}
	virtual StructUnionType* ToStructUnionType(void) { return this; }
	virtual const StructUnionType* ToStructUnionType(void) const { return this; }
	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;
	Variable* Find(const char* name);
	const Variable* Find(const char* name) const;
	
	//向struct/union添加成员
	void AddMember(const char* name, Type* type);
	bool IsStruct(void) const { return _isStruct; }
protected:
	//default is incomplete
	explicit StructUnionType(bool isStruct)
		: _mapMember(new Env()), _isStruct(isStruct), Type(0, false) {}
	StructUnionType(const StructUnionType& other);

private:
	static int CalcWidth(const Env* env);

	Env* _mapMember;
	bool _isStruct;
};

/**
class EnumType : public Type
{
	friend class Type;
public:
	virtual ~EnumType(void) {}
	virtual EnumType* ToEnumType(void) { return this; }
	virtual const EnumType* ToEnumType(void) const { return this; }
	virtual bool operator==(const Type& other) const;
	virtual bool Compatible(const Type& other) const;
protected:
	EnumType(void) : Type(_machineWord, false) {}
private:
	
};
*/

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

	Type* FindTypeInCurScope(const char* name);
	const Type* FindTypeInCurScope(const char* name) const;

	Variable* FindVarInCurScope(const char* name);
	const Variable* FindVarInCurScope(const char* name) const;
		 
	Type* FindType(const char* name);
	const Type* FindType(const char* name) const;

	Variable* FindVar(const char* name);
	const Variable* FindVar(const char* name) const;

	Type* InsertType(const char* name, Type* type);
	Variable* InsertVar(const char* name, Type* type);

	bool operator==(const Env& other) const;
	Env* Parent(void) { return _parent; }
	const Env* Parent(void) const { return _parent; }

	Type* FindTagInCurScope(const char* tag);
	Type* InsertTag(const char* tag, Type* type);
	Type* FindTag(const char* tag);

	//enumeration constant 
	//Constant* FindConstantInCurScope(const char* name) {
	//	auto type = FindVarInCurScope(name);
	//	return type->ToConstant();
	//}
	Constant* InsertConstant(const char* name, Constant* constant) {
		//注意enumeration constant 与一般变量处于同一命名空间
		assert(nullptr == FindVarInCurScope(name));
		_symbMap[name] = constant;
		return constant;
	}
	//Constant* FindConstant(const char* name) {
	//	auto type = FindVar(name);
	//	return type->ToConstant();
	//}

private:
	typedef std::map<const char*, Symbol*, StrCmp> SymbMap;
	typedef std::map<const char*, Type*, StrCmp> TagMap;

	Env* _parent;
	int _offset;
	SymbMap _symbMap;
	TagMap _tagMap;
};

#endif
