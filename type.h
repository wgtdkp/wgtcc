#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include "mem_pool.h"
#include "scope.h"

#include <cassert>

#include <algorithm>
#include <list>


/********* Type System ***********/
class Scope;
class Token;

class Type;
class VoidType;
class Identifier;
class Object;
class Constant;

class ArithmType;
class DerivedType;
class ArrayType;
class FuncType;
class PointerType;
class StructType;
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

  T_LLONG = 0x2000000,

  /*****function specifier*****/
  F_INLINE = 0x4000000,
  F_NORETURN = 0x8000000,
};


class Type
{
public:
  static const int _intWidth = 4;

  bool operator!=(const Type& other) const = delete;
  bool operator==(const Type& other) const = delete;

  virtual bool Compatible(const Type& other) const {
    return complete_ == other.complete_;
  }

  virtual ~Type() {}

  // For Debugging
  virtual std::string Str() const = 0; 
  virtual int Width() const = 0;
  virtual int Align() const { return Width(); }
  static int MakeAlign(int offset, int align) {
    if ((offset % align) == 0)
      return offset;
    if (offset >= 0)
      return offset + align - (offset % align);
    else
      return offset - align - (offset % align);
  } 

  static Type* MayCast(Type* type);
  int Qual() const { return qual_; }
  void SetQual(int qual) { qual_ = qual; }
  bool Complete() const { return complete_; }
  void SetComplete(bool complete) { complete_ = complete; }

  virtual bool IsScalar() const { return false; }
  virtual bool IsFloat() const { return false; }
  virtual bool IsInteger() const { return false; }

  virtual VoidType* ToVoidType() { return nullptr; }
  virtual const VoidType* ToVoidType() const { return nullptr; }
  virtual ArithmType* ToArithmType() { return nullptr; }  
  virtual const ArithmType* ToArithmType() const { return nullptr; }  
  virtual ArrayType* ToArrayType() { return nullptr; }
  virtual const ArrayType* ToArrayType() const { return nullptr; }
  virtual FuncType* ToFuncType() { return nullptr; }
  virtual const FuncType* ToFuncType() const { return nullptr; }
  virtual PointerType* ToPointerType() { return nullptr; }
  virtual const PointerType* ToPointerType() const { return nullptr; }
  virtual DerivedType* ToDerivedType() { return nullptr; }
  virtual const DerivedType* ToDerivedType() const { return nullptr; }
  virtual StructType* ToStructType() { return nullptr; }
  virtual const StructType* ToStructType() const { return nullptr; }

protected:
  Type(MemPool* pool, bool complete)
      : qual_(0), complete_(complete), pool_(pool) {}

  // C11 6.7.3 [4]: The properties associated with qualified types
  // are meaningful only for expressions that are lvalues.
  // It is convenient to carry qualification within type
  // But it's not used to decide the compatibility of two types.
  mutable int qual_;
  bool complete_;
  MemPool* pool_;
};


class VoidType : public Type
{
  friend class Type;

public:
  static VoidType* New();

  virtual ~VoidType() {}

  virtual VoidType* ToVoidType() { return this; }
  virtual const VoidType* ToVoidType() const { return this; }
  virtual bool Compatible(const Type& other) const {
    // The void Type is compatible to all types,
    // so that a pointer to void type (void*) is
    // always compatible to all pointer types.
    return true;
  }

  virtual int Width() const {
    // Non-standard GNU extension
    return 1;
  }

  virtual std::string Str() const { return "void:1"; }

protected:
  explicit VoidType(MemPool* pool): Type(pool, false) {}
};


class ArithmType : public Type
{
  friend class Type;

public:
  static ArithmType* New(int typeSpec);

  virtual ~ArithmType() {}
  virtual ArithmType* ToArithmType() { return this; }
  virtual const ArithmType* ToArithmType() const { return this; }
  virtual bool Compatible(const Type& other) const {
    // C11 6.2.7 [1]: Two types have compatible type if their types are the same
    return this == &other;
  }

  virtual int Width() const;
  virtual std::string Str() const;
  virtual bool IsScalar() const { return true; }
  virtual bool IsInteger() const { return !IsFloat() && !IsComplex(); }
  virtual bool IsFloat() const {
    return (tag_ & T_FLOAT) || (tag_ & T_DOUBLE);
  }

  bool IsComplex() const { return tag_ & T_COMPLEX; }
  int Tag() const { return tag_; }

protected:
  explicit ArithmType(MemPool* pool, int spec)
    : Type(pool, true), tag_(Spec2Tag(spec)) {}

private:
  static int Spec2Tag(int spec);

  int tag_;
};


class DerivedType : public Type
{
  //friend class Type;
public:
  Type* Derived() {
    return derived_;
  }
  
  void SetDerived(Type* derived) {
    derived_ = derived;
  }
  
  virtual DerivedType* ToDerivedType() {
    return this;
  }
  
  virtual const DerivedType* ToDerivedType() const {
    return this;
  }

protected:
  DerivedType(MemPool* pool, Type* derived)
      : Type(pool, true), derived_(derived) {}

  Type* derived_;
};


class PointerType : public DerivedType
{
  friend class Type;

public:
  static PointerType* New(Type* derived);

  ~PointerType() {}
  virtual PointerType* ToPointerType() { return this; }
  virtual const PointerType* ToPointerType() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return 8; }
  virtual bool IsScalar() const { return true; }
  virtual std::string Str() const {
    return derived_->Str() + "*:" + std::to_string(Width());
  }

protected:
  PointerType(MemPool* pool, Type* derived): DerivedType(pool, derived) {}
};


class ArrayType : public DerivedType
{
  friend class Type;

public:
  static ArrayType* New(int len, Type* eleType);
  virtual ~ArrayType() { /*delete derived_;*/ }

  virtual ArrayType* ToArrayType() { return this; }
  virtual const ArrayType* ToArrayType() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return derived_->Width() * len_; }
  virtual int Align() const { return derived_->Align(); }
  virtual std::string Str() const {
    return derived_->Str() + "[]:" + std::to_string(Width());
  }

  int GetElementOffset(int idx) const { return derived_->Width() * idx; }
  int Len() const { return len_; }
  void SetLen(int len) { len_ = len; }

protected:
  ArrayType(MemPool* pool, int len, Type* derived)
      : DerivedType(pool, derived), len_(len) {
    SetComplete(len_ >= 0);
    SetQual(Q_CONST);
  }

  int len_;
};


class FuncType : public DerivedType
{
  friend class Type;

public:
  typedef std::vector<Type*> TypeList;

public:
  static FuncType* New(Type* derived, int funcSpec,
      bool hasEllipsis, const FuncType::TypeList& params);

  ~FuncType() {}
  
  virtual FuncType* ToFuncType() { return this; }
  virtual const FuncType* ToFuncType() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return 1; }
  virtual std::string Str() const;
  //bool IsInline() const { inlineNoReturn_ & F_INLINE; }
  //bool IsNoReturn() const { return inlineNoReturn_ & F_NORETURN; }
  TypeList& ParamTypes() { return paramTypes_; }
  bool Variadic() const { return variadic_; }

protected:
  //a function does not has the width property
  FuncType(MemPool* pool, Type* derived, int inlineReturn, bool variadic,
           const TypeList& paramTypes)
      : DerivedType(pool, derived), inlineNoReturn_(inlineReturn),
        variadic_(variadic), paramTypes_(paramTypes) {
    SetComplete(false);
  }

private:
  int inlineNoReturn_;
  bool variadic_;
  TypeList paramTypes_;
};


class StructType : public Type
{
  friend class Type;
  
public:
  typedef std::list<Object*> MemberList;
  typedef std::list<Object*>::iterator Iterator;
  
public:
  static StructType* New(bool isStruct, bool hasTag, Scope* parent);
  
  ~StructType() {/*TODO: delete _env ?*/ }
  virtual StructType* ToStructType() { return this; }
  virtual const StructType* ToStructType() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return width_; }
  virtual int Align() const { return align_; }
  virtual std::string Str() const;

  // struct/union
  void AddMember(Object* member);
  void AddBitField(Object* member, int offset);
  bool IsStruct() const { return isStruct_; }
  Object* GetMember(const std::string& member);
  Scope* MemberMap() { return memberMap_; }
  MemberList& Members() { return members_; }
  int Offset() const { return offset_; }
  bool HasTag() { return hasTag_; }
  void MergeAnony(Object* anony);

protected:
  // default is incomplete
  StructType(MemPool* pool, bool isStruct, bool hasTag, Scope* parent);
  
  StructType(const StructType& other);

private:
  void CalcWidth();

  //static std::string AnonymousBitField() {
  //    static int id = 0;
  //    return std::to_string(id++) + "<anonymous>";
  //}

  bool isStruct_;
  bool hasTag_;
  Scope* memberMap_;

  MemberList members_;
  int offset_;
  int width_;
  int align_;
};


ArithmType* MaxType(ArithmType* lhsType, ArithmType* rhsType);

#endif
