#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include "mem_pool.h"
#include "scope.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <list>

class Scope;
class Token;

class Expr;
class Object;

class Type;
class QualType;
class VoidType;
class ArithmType;
class DerivedType;
class ArrayType;
class FuncType;
class PointerType;
class StructType;
class EnumType;

enum {
  // Storage class specifiers
  S_TYPEDEF = 0x01,
  S_EXTERN = 0x02,
  S_STATIC = 0x04,
  S_THREAD = 0x08,
  S_AUTO = 0x10,
  S_REGISTER = 0x20,

  // Type specifier
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
  //T_ATOMIC = 0x20000,
  T_STRUCT_UNION = 0x40000,
  T_ENUM = 0x80000,
  T_TYPEDEF_NAME = 0x100000,

  T_LLONG = 0x2000000,

  // Function specifier
  F_INLINE = 0x4000000,
  F_NORETURN = 0x8000000,
};


struct Qualifier {
  enum {
    CONST = 0x01,
    RESTRICT = 0x02,
    VOLATILE = 0x04,
    MASK = CONST | RESTRICT | VOLATILE
  };
};


class QualType {
public:
  QualType(Type* ptr, int quals=0x00)
      : ptr_(reinterpret_cast<intptr_t>(ptr)) {
    assert((quals & ~Qualifier::MASK) == 0);
    ptr_ |= quals;
  } 

  operator bool() const { return !IsNull(); }
  bool IsNull() const { return ptr() == nullptr; }
  const Type* ptr() const {
    return reinterpret_cast<const Type*>(ptr_ & ~Qualifier::MASK);
  }
  Type* ptr() {
    return reinterpret_cast<Type*>(ptr_ & ~Qualifier::MASK);
  }
  Type& operator*() { return *ptr(); }
  const Type& operator*() const { return *ptr(); }
  Type* operator->() { return ptr(); }
  const Type* operator->() const { return ptr(); }

  // Indicate whether the specified types are identical(exclude qualifiers).
  friend bool operator==(QualType lhs, QualType rhs) {
    return lhs.operator->() == rhs.operator->();
  }
  friend bool operator!=(QualType lhs, QualType rhs) {
    return !(lhs == rhs);
  }

  int Qual() const { return ptr_ & 0x03; }
  bool IsConstQualified() const { return ptr_ & Qualifier::CONST; }
  bool IsRestrictQualified() const { return ptr_ & Qualifier::RESTRICT; }
  bool IsVolatileQualified() const { return ptr_ & Qualifier::VOLATILE; }

private:
  intptr_t ptr_;
};


class Type {
public:
  static const int int_width_ = 4;
  static const int machineWidth_ = 8;

  bool operator!=(const Type& other) const = delete;
  bool operator==(const Type& other) const = delete;

  virtual bool Compatible(const Type& other) const {
    return complete_ == other.complete_;
  }

  virtual ~Type() {}

  // For Debugging
  virtual std::string Str() const = 0; 
  virtual int width() const = 0;
  virtual int align() const { return width(); }
  static int MakeAlign(int offset, int align) {
    if ((offset % align) == 0)
      return offset;
    if (offset >= 0)
      return offset + align - (offset % align);
    else
      return offset - align - (offset % align);
  } 

  static QualType MayCast(QualType type);
  bool complete() const { return complete_; }
  void set_complete(bool complete) const { complete_ = complete; }

  bool IsReal() const { return IsInteger() || IsFloat(); };  
  virtual bool IsScalar() const { return false; }
  virtual bool IsFloat() const { return false; }
  virtual bool IsInteger() const { return false; }
  virtual bool IsBool() const { return false; }
  virtual bool IsVoidPointer() const { return false; }
  virtual bool IsUnsigned() const { return false; }

  virtual VoidType*           ToVoid() { return nullptr; }
  virtual const VoidType*     ToVoid() const { return nullptr; }  
  virtual ArithmType*         ToArithm() { return nullptr; }
  virtual const ArithmType*   ToArithm() const { return nullptr; }
  virtual ArrayType*          ToArray() { return nullptr; }
  virtual const ArrayType*    ToArray() const { return nullptr; }
  virtual FuncType*           ToFunc() { return nullptr; }
  virtual const FuncType*     ToFunc() const { return nullptr; }
  virtual PointerType*        ToPointer() { return nullptr; }
  virtual const PointerType*  ToPointer() const { return nullptr; }
  virtual DerivedType*        ToDerived() { return nullptr; }
  virtual const DerivedType*  ToDerived() const { return nullptr; }
  virtual StructType*         ToStruct() { return nullptr; }
  virtual const StructType*   ToStruct() const { return nullptr; }

protected:
  Type(MemPool* pool, bool complete)
      : complete_(complete), pool_(pool) {}

  mutable bool complete_;
  MemPool* pool_;
};


class VoidType : public Type {
public:
  static VoidType* New();
  virtual ~VoidType() {}
  virtual VoidType* ToVoid() { return this; }
  virtual const VoidType* ToVoid() const { return this; }
  virtual bool Compatible(QualType other) const { return other->ToVoid(); }
  virtual int width() const {
    // Non-standard GNU extension
    return 1;
  }
  virtual std::string Str() const { return "void:1"; }

protected:
  explicit VoidType(MemPool* pool): Type(pool, false) {}
};


class ArithmType : public Type {
public:
  static ArithmType* New(int type_spec);

  virtual ~ArithmType() {}
  virtual ArithmType* ToArithm() { return this; }
  virtual const ArithmType* ToArithm() const { return this; }
  virtual bool Compatible(const Type& other) const {
    // C11 6.2.7 [1]: Two types have compatible type if their types are the same
    // But i would to loose this constraints: integer and pointer are compatible
    if (IsInteger() && other.ToPointer())
      return other.Compatible(*this);
    return this == &other;
  }

  virtual int width() const;
  virtual std::string Str() const;
  virtual bool IsScalar() const { return true; }
  virtual bool IsInteger() const { return !IsFloat() && !IsComplex(); }
  virtual bool IsUnsigned() const { return tag_ & T_UNSIGNED; }
  virtual bool IsFloat() const {
    return (tag_ & T_FLOAT) || (tag_ & T_DOUBLE);
  }
  virtual bool IsBool() const { return tag_ & T_BOOL; }
  bool IsComplex() const { return tag_ & T_COMPLEX; }
  int tag() const { return tag_; }
  int Rank() const;
  static ArithmType* IntegerPromote(ArithmType* type) {
    assert(type->IsInteger());
    if (type->Rank() < ArithmType::New(T_INT)->Rank())
      return ArithmType::New(T_INT);
    return type;
  }
  static ArithmType* MaxType(ArithmType* lhs_type,
                                   ArithmType* rhs_type);

protected:
  explicit ArithmType(MemPool* pool, int spec)
    : Type(pool, true), tag_(Spec2Tag(spec)) {}

private:
  static int Spec2Tag(int spec);

  int tag_;
};


class DerivedType : public Type {
public:
  QualType derived() const { return derived_; }  
  void set_derived(QualType derived) { derived_ = derived; }
  virtual DerivedType* ToDerived() { return this; }
  virtual const DerivedType* ToDerived() const { return this; }

protected:
  DerivedType(MemPool* pool, QualType derived)
      : Type(pool, true), derived_(derived) {}

  QualType derived_;
};


class PointerType : public DerivedType {
public:
  static PointerType* New(QualType derived);
  ~PointerType() {}
  virtual PointerType* ToPointer() { return this; }
  virtual const PointerType* ToPointer() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int width() const { return 8; }
  virtual bool IsScalar() const { return true; }
  virtual bool IsVoidPointer() const { return derived_->ToVoid(); }
  virtual std::string Str() const {
    return derived_->Str() + "*:" + std::to_string(width());
  }

protected:
  PointerType(MemPool* pool, QualType derived): DerivedType(pool, derived) {}
};


class ArrayType : public DerivedType {
public:
  static ArrayType* New(int len, QualType ele_type);
  static ArrayType* New(Expr* expr, QualType ele_type);
  virtual ~ArrayType() { /*delete derived_;*/ }

  virtual ArrayType* ToArray() { return this; }
  virtual const ArrayType* ToArray() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int width() const {
    return complete() ? (derived_->width() * len_): 0;
  }
  virtual int align() const { return derived_->align(); }
  virtual std::string Str() const {
    return derived_->Str() + "[]:" + std::to_string(width());
  }

  int GetElementOffset(int idx) const { return derived_->width() * idx; }
  int len() const { return len_; }
  void set_len(int len) { len_ = len; }
  bool Variadic() const { return len_expr_ != nullptr; }

protected:
  ArrayType(MemPool* pool, Expr* len_expr, QualType derived)
      : DerivedType(pool, derived),
        len_expr_(len_expr), len_(0) {
    set_complete(false);
    //SetQual(QualType::CONST);
  }
  
  ArrayType(MemPool* pool, int len, QualType derived)
      : DerivedType(pool, derived),
        len_expr_(nullptr), len_(len) {
    set_complete(len_ >= 0);
    //SetQual(QualType::CONST);
  }
  const Expr* len_expr_;
  int len_;
};


class FuncType : public DerivedType {
public:
  typedef std::vector<Object*> ParamList;

public:
  static FuncType* New(QualType derived,
                       int func_spec,
                       bool variadic,
                       const ParamList& param_list);
  ~FuncType() {}
  virtual FuncType* ToFunc() { return this; }
  virtual const FuncType* ToFunc() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int width() const { return 1; }
  virtual std::string Str() const;
  const ParamList& param_list() const { return param_list_; }
  void set_param_list(const ParamList& param_list) {
    param_list_ = param_list;
  }
  bool Variadic() const { return variadic_; }

protected:
  FuncType(MemPool* pool, QualType derived, int inline_noreturn,
           bool variadic, const ParamList& param_list)
      : DerivedType(pool, derived), inline_noreturn_(inline_noreturn),
        variadic_(variadic), param_list_(param_list) {
    set_complete(false);
  }

private:
  int inline_noreturn_;
  bool variadic_;
  ParamList param_list_;
};


class StructType : public Type {
public:
  using MemberList = std::list<Object*>;
  using Iterator   = std::list<Object*>::iterator;
  
public:
  static StructType* New(bool is_struct,
                         bool has_tag,
                         Scope* parent);
  ~StructType() {}
  virtual StructType* ToStruct() { return this; }
  virtual const StructType* ToStruct() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int width() const { return width_; }
  virtual int align() const { return align_; }
  virtual std::string Str() const;

  // struct/union
  void AddMember(Object* member);
  void AddBitField(Object* member, int offset);
  bool IsStruct() const { return is_struct_; }
  Object* GetMember(const std::string& member);
  Scope* member_map() { return member_map_; }
  const Scope* member_map() const { return member_map_; }
  MemberList& member_list() { return member_list_; }
  const MemberList& member_list() const { return member_list_; }
  int offset() const { return offset_; }
  bool HasTag() const { return has_tag_; }
  void MergeAnony(Object* anony);
  void Finalize();
  
protected:
  // default is incomplete
  StructType(MemPool* pool, bool is_struct, bool has_tag, Scope* parent);
  
  StructType(const StructType& other);

private:
  void CalcWidth();

  bool is_struct_;
  bool has_tag_;
  Scope* member_map_;

  MemberList member_list_;
  int offset_;
  int width_;
  int align_;
  int bitfiled_align_;
};

/*
// Not used yet
class EnumType: public Type {
public:
  static EnumTypePtr New(bool complete=false, int quals);

  virtual ~EnumType() {}
  virtual bool IsInteger() const { return true; }

  virtual ArithmType* ToArithm() {
    assert(false);
    return ArithmType::New(T_INT);
  }

  virtual EnumTypePtr ToEnum() { return this; }
  virtual bool Compatible(const Type& other) const {
    // As enum is always casted to INT, there is no chance for this call    
    assert(false);
    return (other.ToEnum() || other.IsInteger()) && Type::Compatible(other);    
  }

  virtual int width() const {
    return ArithmType::New(T_INT)->width();
  }

  virtual std::string Str() const { return "enum:4"; }

protected:
  explicit EnumType(MemPool* pool, bool complete): Type(pool, complete) {}
};
*/

#endif
