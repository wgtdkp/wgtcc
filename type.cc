#include "type.h"

#include "ast.h"
#include "scope.h"
#include "token.h"

#include <cassert>

#include <algorithm>
#include <iostream>


/***************** Type *********************/

static MemPoolImp<VoidType>         voidTypePool;
static MemPoolImp<ArrayType>        arrayTypePool;
static MemPoolImp<FuncType>         funcTypePool;
static MemPoolImp<PointerType>      pointerTypePool;
static MemPoolImp<StructType>  structUnionTypePool;
static MemPoolImp<ArithmType>       arithmTypePool;


bool Type::IsFloat(void) const {
  auto arithmType = ToArithmType();
  return arithmType && arithmType->IsFloat();
}

bool Type::IsInteger(void) const {
  auto arithmType = ToArithmType();
  return arithmType && arithmType->IsInteger();
}

Type* Type::MayCast(Type* type)
{
  auto funcType = type->ToFuncType();
  auto arrayType = type->ToArrayType();
  if (funcType) {
    return PointerType::New(funcType);
  } else if (arrayType) {
    auto ret = PointerType::New(arrayType->Derived());
    ret->SetQual(Q_CONST);
    return ret;
  }
  return type;
}

VoidType* VoidType::New(void)
{
  return new (voidTypePool.Alloc()) VoidType(&voidTypePool);
}

ArithmType* ArithmType::New(int typeSpec) {
  auto tag = ArithmType::Spec2Tag(typeSpec);
  return new (arithmTypePool.Alloc())
      ArithmType(&arithmTypePool, tag);
}

ArrayType* ArrayType::New(int len, Type* eleType)
{
  return new (arrayTypePool.Alloc())
      ArrayType(&arrayTypePool, len, eleType);
}

//static IntType* NewIntType();
FuncType* FuncType::New(Type* derived, int funcSpec,
    bool hasEllipsis, const FuncType::TypeList& params) {
  return new (funcTypePool.Alloc())
      FuncType(&funcTypePool, derived, funcSpec, hasEllipsis, params);
}

PointerType* PointerType::New(Type* derived) {
  return new (pointerTypePool.Alloc())
      PointerType(&pointerTypePool, derived);
}

StructType* StructType::New(
    bool isStruct, bool hasTag, Scope* parent) {
  return new (structUnionTypePool.Alloc())
      StructType(&structUnionTypePool, isStruct, hasTag, parent);
}

/*
static EnumType* Type::NewEnumType() {
  // TODO(wgtdkp):
  assert(false);
  return nullptr;
}
*/

/*************** ArithmType *********************/
int ArithmType::Width(void) const {
  switch (tag_) {
  case T_BOOL:
  case T_CHAR:
  case T_UNSIGNED | T_CHAR:
    return 1;

  case T_SHORT:
  case T_UNSIGNED | T_SHORT:
    return _intWidth >> 1;

  case T_INT:
  case T_UNSIGNED:
  case T_UNSIGNED | T_INT:
    return _intWidth;

  case T_LONG:
  case T_UNSIGNED | T_LONG:
    return _intWidth << 1;

  case T_LONG_LONG:
  case T_UNSIGNED | T_LONG_LONG:
    return _intWidth << 1;

  case T_FLOAT:
    return _intWidth;

  case T_DOUBLE:
    return _intWidth << 1;

  case T_LONG | T_DOUBLE:
    return _intWidth << 2;

  case T_FLOAT | T_COMPLEX:
    return _intWidth << 1;

  case T_DOUBLE | T_COMPLEX:
    return _intWidth << 2;

  case T_LONG | T_DOUBLE | T_COMPLEX:
    return _intWidth << 3;

  default:
    assert(false);
  }

  return _intWidth; // Make compiler happy
}

int ArithmType::Spec2Tag(int spec) {
  spec &= ~T_SIGNED;
  if ((spec & T_SHORT) || (spec & T_LONG)
      || (spec & T_LONG_LONG)) {
    spec &= ~T_INT;
  }
  return spec;
}


std::string ArithmType::Str(void) const
{
  std::string width = std::string(":") + std::to_string(Width());

  switch (tag_) {
  case T_BOOL:
    return "bool" + width;

  case T_CHAR:
    return "char" + width;

  case T_UNSIGNED | T_CHAR:
    return "unsigned char" + width;

  case T_SHORT:
    return "short" + width;

  case T_UNSIGNED | T_SHORT:
    return "unsigned short" + width;

  case T_INT:
    return "int" + width;

  case T_UNSIGNED:
    return "unsigned int" + width;

  case T_LONG:
    return "long" + width;

  case T_UNSIGNED | T_LONG:
    return "unsigned long" + width;

  case T_LONG_LONG:
    return "long long" + width;

  case T_UNSIGNED | T_LONG_LONG:
    return "unsigned long long" + width;

  case T_FLOAT:
    return "float" + width;

  case T_DOUBLE:
    return "double" + width;

  case T_LONG | T_DOUBLE:
    return "long double" + width;

  case T_FLOAT | T_COMPLEX:
    return "float complex" + width;

  case T_DOUBLE | T_COMPLEX:
    return "double complex" + width;

  case T_LONG | T_DOUBLE | T_COMPLEX:
    return "long double complex" + width;

  default:
    assert(false);
  }

  return "error"; // Make compiler happy
}

/*************** PointerType *****************/

bool PointerType::operator==(const Type& other) const {
  auto otherType = other.ToPointerType();
  if (nullptr == otherType)
    return false;

  return *derived_ == *otherType->derived_;
}

bool PointerType::Compatible(const Type& other) const {
  //TODO: compatibility ???
  //if (other.IsInteger())
  //    return true;
  auto otherType = other.ToPointerType();
  if (otherType == nullptr)
    return false;
  if ((otherType->derived_->ToVoidType() && !derived_->ToFuncType())
      || (derived_->ToVoidType() && !otherType->derived_->ToFuncType()))
    return true;
  return derived_->Compatible(*otherType->derived_);
}


/************* FuncType ***************/

bool FuncType::operator==(const Type& other) const
{
  auto otherFunc = other.ToFuncType();
  if (nullptr == otherFunc) return false;
  // TODO: do we need to check the type of return value when deciding 
  // equality of two function types ??
  if (*derived_ != *otherFunc->derived_)
    return false;
  if (paramTypes_.size() != otherFunc->paramTypes_.size())
    return false;

  auto thisIter = paramTypes_.begin();
  auto otherIter = otherFunc->paramTypes_.begin();
  while (thisIter != paramTypes_.end()) {
    if (*(*thisIter) != *(*otherIter))
      return false;

    thisIter++;
    otherIter++;
  }

  return true;
}

bool FuncType::Compatible(const Type& other) const
{
  auto otherFunc = other.ToFuncType();
  //the other type is not an function type
  if (nullptr == otherFunc)
    return false;
  //TODO: do we need to check the type of return value when deciding 
  //compatibility of two function types ??
  if (!derived_->Compatible(*otherFunc->derived_))
    return false;
  if (paramTypes_.size() != otherFunc->paramTypes_.size())
    return false;

  auto thisIter = paramTypes_.begin();
  auto otherIter = otherFunc->paramTypes_.begin();
  while (thisIter != paramTypes_.end()) {
    if (!(*thisIter)->Compatible(*(*otherIter)))
      return false;

    thisIter++;
    otherIter++;
  }

  return true;
}


std::string FuncType::Str(void) const
{
  auto str = derived_->Str() + "(";
  auto iter = paramTypes_.begin();
  for (; iter != paramTypes_.end(); iter++) {
    str += (*iter)->Str() + ", ";
  }
  if (variadic_)
    str += "...";
  else if (paramTypes_.size())
    str.resize(str.size() - 2);

  return str + ")";
}

/*
 * StructType
 */

StructType::StructType(MemPool* pool,
    bool isStruct, bool hasTag, Scope* parent)
    : Type(pool, false), isStruct_(isStruct), hasTag_(hasTag),
      memberMap_(new Scope(parent, S_BLOCK)),
      offset_(0), width_(0), align_(0) {}


Object* StructType::GetMember(const std::string& member) {
  auto ident = memberMap_->FindInCurScope(member);
  if (ident == nullptr)
    return nullptr;
  return ident->ToObject();
}

void StructType::CalcWidth(void)
{
  width_ = 0;
  auto iter = memberMap_->identMap_.begin();
  for (; iter != memberMap_->identMap_.end(); iter++) {
    width_ += iter->second->Type()->Width();
  }
}

bool StructType::operator==(const Type& other) const
{
  auto structUnionType = other.ToStructType();
  if (nullptr == structUnionType)
    return false;

  // Yes, i am doing pointer comparison
  // Two struct type are equal only if they have
  //     identical definition(at the same place).
  // As we do a copy of StructType when setting it's
  //     qualification, we can't compare two struct type
  //     as: this == &other;
  // But, even we do a copy, the two types with different
  //     qualification share the same memberMap_, thus, 
  //     memberMap_ could be the proof of equality.
  return memberMap_ == structUnionType->memberMap_;
}


bool StructType::Compatible(const Type& other) const {
  // TODO: 
  return *this == other;
}


// TODO(wgtdkp): more detailed representation
std::string StructType::Str(void) const
{
  std::string str = isStruct_ ? "struct": "union";
  return str + ":" + std::to_string(width_);
}


void StructType::AddMember(Object* member)
{
  auto offset = MakeAlign(offset_, member->Type()->Align());
  member->SetOffset(offset);
  
  members_.push_back(member);
  memberMap_->Insert(member->Name(), member);

  align_ = std::max(align_, member->Type()->Align());

  if (isStruct_) {
    offset_ = offset + member->Type()->Width();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, member->Type()->Width());
  }
}


void StructType::AddBitField(Object* bitField, int offset)
{
  bitField->SetOffset(offset);
  members_.push_back(bitField);
  //auto name = bitField->Tok() ? bitField->Name(): AnonymousBitField();
  if (bitField->Tok())
    memberMap_->Insert(bitField->Name(), bitField);

  auto bytes = MakeAlign(bitField->BitFieldEnd(), 8) / 8;
  align_ = std::max(align_, bitField->Type()->Align());
  // Does not aligned 
  offset_ = offset + bytes;
  width_ = MakeAlign(offset_, align_);
}


// Move members of Anonymous struct/union to external struct/union
void StructType::MergeAnony(Object* anony)
{   
  auto anonyType = anony->Type()->ToStructType();
  auto offset = MakeAlign(offset_, anonyType->Align());

  // Members in map are never anonymous
  for (auto& kv: *anonyType->memberMap_) {
    auto& name = kv.first;
    auto member = kv.second->ToObject();
    // Every member of anonymous struct/union
    //     are offseted by external struct/union
    member->SetOffset(offset + member->Offset());

    if (GetMember(name)) {
      Error(member, "duplicated member '%s'", name.c_str());
    }
    //members_.push_back(member);
    // Simplify anony struct's member searching
    memberMap_->Insert(name, member);
  }
  anony->SetOffset(offset);
  members_.push_back(anony);

  align_ = std::max(align_, anonyType->Align());
  if (isStruct_) {
    offset_ = offset + anonyType->Width();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, anonyType->Width());
  }
}


ArithmType* MaxType(ArithmType* lhsType, ArithmType* rhsType)
{
  int intWidth = Type::_intWidth;

  if (lhsType->Width() < intWidth && rhsType->Width() < intWidth) {
    return ArithmType::New(T_INT);
  } else if (lhsType->Width() > rhsType->Width()) {
    if (rhsType->IsFloat())
      return rhsType;
    return lhsType;
  } else if (lhsType->Width() < rhsType->Width()) {
    if (lhsType->IsFloat())
      return lhsType;
    return rhsType;
  } else if (lhsType->IsFloat()) {
    return lhsType;
  } else if (rhsType->IsFloat()) {
    return rhsType;
  } else if (lhsType->Tag() & T_UNSIGNED) {
    return lhsType;
  } else {
    return rhsType;
  }
}
