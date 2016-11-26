#include "type.h"

#include "ast.h"
#include "scope.h"
#include "token.h"

#include <cassert>
#include <algorithm>
#include <iostream>

static MemPoolImp<VoidType>     void_pool;
static MemPoolImp<ArrayType>    array_pool;
static MemPoolImp<FuncType>     func_pool;
static MemPoolImp<PointerType>  pointer_pool;
static MemPoolImp<StructType>   struct_pool;
static MemPoolImp<ArithmType>   arithm_pool;


QualType Type::MayCast(QualType type) {
  auto func_type = type->ToFunc();
  auto array_type = type->ToArray();
  if (func_type) {
    return PointerType::New(func_type);
  } else if (array_type) {
    auto ret = PointerType::New(array_type->derived());
    return QualType(ret, Qualifier::CONST);
  }
  return type;
}


VoidType* VoidType::New() {
  static auto ret = new (void_pool.Alloc()) VoidType(&void_pool);
  return ret;
}


ArithmType* ArithmType::New(int type_spec) {
#define NEW_TYPE(tag)                                           \
  new (arithm_pool.Alloc()) ArithmType(&arithm_pool, tag);

  static auto bool_type    = NEW_TYPE(T_BOOL);
  static auto char_type    = NEW_TYPE(T_CHAR);
  static auto uchar_type   = NEW_TYPE(T_UNSIGNED | T_CHAR);
  static auto short_type   = NEW_TYPE(T_SHORT);
  static auto ushort_type  = NEW_TYPE(T_UNSIGNED | T_SHORT);
  static auto int_type     = NEW_TYPE(T_INT);
  static auto uint_type    = NEW_TYPE(T_UNSIGNED | T_INT);
  static auto long_type    = NEW_TYPE(T_LONG);
  static auto ulong_type   = NEW_TYPE(T_UNSIGNED | T_LONG);
  static auto llong_type   = NEW_TYPE(T_LLONG)
  static auto ullong_type  = NEW_TYPE(T_UNSIGNED | T_LLONG);
  static auto float_type   = NEW_TYPE(T_FLOAT);
  static auto double_type  = NEW_TYPE(T_DOUBLE);
  static auto ldouble_type = NEW_TYPE(T_LONG | T_DOUBLE);
  
  auto tag = ArithmType::Spec2Tag(type_spec);
  switch (tag) {
  case T_BOOL:              return bool_type;
  case T_CHAR:              return char_type;
  case T_UNSIGNED | T_CHAR: return uchar_type;
  case T_SHORT:             return short_type;
  case T_UNSIGNED | T_SHORT:return ushort_type;
  case T_INT:               return int_type;
  case T_UNSIGNED:
  case T_UNSIGNED | T_INT:  return uint_type;
  case T_LONG:              return long_type;
  case T_UNSIGNED | T_LONG: return ulong_type;
  case T_LLONG:             return llong_type;
  case T_UNSIGNED | T_LLONG:return ullong_type;
  case T_FLOAT:             return float_type;
  case T_DOUBLE:            return double_type;
  case T_LONG | T_DOUBLE:   return ldouble_type;
  default: Error("complex not supported yet");
  }
  return nullptr; // Make compiler happy

#undef NEW_TYPE
}


ArrayType* ArrayType::New(int len, QualType ele_type) {
  return new (array_pool.Alloc())
         ArrayType(&array_pool, len, ele_type);
}


ArrayType* ArrayType::New(Expr* expr, QualType ele_type) {
  return new (array_pool.Alloc())
         ArrayType(&array_pool, expr, ele_type);
}


FuncType* FuncType::New(QualType derived,
                        int func_spec,
                        bool variadic,
                        const ParamList& param_list) {
  return new (func_pool.Alloc())
         FuncType(&func_pool, derived, func_spec, variadic, param_list);
}


PointerType* PointerType::New(QualType derived) {
  return new (pointer_pool.Alloc())
         PointerType(&pointer_pool, derived);
}


StructType* StructType::New(bool is_struct,
                              bool has_tag,
                              Scope* parent) {
  return new (struct_pool.Alloc())
         StructType(&struct_pool, is_struct, has_tag, parent);
}


int ArithmType::width() const {
  switch (tag_) {
  case T_BOOL: case T_CHAR: case T_UNSIGNED | T_CHAR:
    return 1;
  case T_SHORT: case T_UNSIGNED | T_SHORT:
    return int_width_ >> 1;
  case T_INT: case T_UNSIGNED: case T_UNSIGNED | T_INT:
    return int_width_;
  case T_LONG: case T_UNSIGNED | T_LONG:
    return int_width_ << 1;
  case T_LLONG: case T_UNSIGNED | T_LLONG:
    return int_width_ << 1;
  case T_FLOAT:
    return int_width_;
  case T_DOUBLE:
    return int_width_ << 1;
  case T_LONG | T_DOUBLE:
    return int_width_ << 1;
  case T_FLOAT | T_COMPLEX:
    return int_width_ << 1;
  case T_DOUBLE | T_COMPLEX:
    return int_width_ << 2;
  case T_LONG | T_DOUBLE | T_COMPLEX:
    return int_width_ << 2;
  default:
    assert(false);
  }

  return int_width_; // Make compiler happy
}


int ArithmType::Rank() const {
  switch (tag_) {
  case T_BOOL: return 0;
  case T_CHAR: case T_UNSIGNED | T_CHAR: return 1;
  case T_SHORT: case T_UNSIGNED | T_SHORT: return 2;
  case T_INT: case T_UNSIGNED: case T_UNSIGNED | T_INT: return 3;
  case T_LONG: case T_UNSIGNED | T_LONG: return 4;
  case T_LLONG: case T_UNSIGNED | T_LLONG: return 5;
  case T_FLOAT: return 6;
  case T_DOUBLE: return 7;
  case T_LONG | T_DOUBLE: return 8;
  default: Error("complex not supported yet");
  }
  return 0;
}


ArithmType* ArithmType::MaxType(ArithmType* lhs, ArithmType* rhs) {
  if (lhs->IsInteger())
    lhs = ArithmType::IntegerPromote(lhs);
  if (rhs->IsInteger())
    rhs = ArithmType::IntegerPromote(rhs);
  auto ret = lhs->Rank() > rhs->Rank() ? lhs: rhs;
  if (lhs->width() == rhs->width() && (lhs->IsUnsigned() || rhs->IsUnsigned()))
    return ArithmType::New(T_UNSIGNED | ret->tag());
  return ret;
}


int ArithmType::Spec2Tag(int spec) {
  spec &= ~T_SIGNED;
  if ((spec & T_SHORT) || (spec & T_LONG)
      || (spec & T_LLONG)) {
    spec &= ~T_INT;
  }
  return spec;
}


std::string ArithmType::Str() const {
  std::string width = ":";// + std::to_string(width());

  switch (tag_) {
  case T_BOOL:
    return "bool" + width;

  case T_CHAR:
    return "char" + width;

  case T_UNSIGNED | T_CHAR:
    return "uint8_t" + width;

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
    return "uint64_t" + width;

  case T_LLONG:
    return "long long" + width;

  case T_UNSIGNED | T_LLONG:
    return "uint64_t long" + width;

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


bool PointerType::Compatible(const Type& other) const {
  // C11 6.7.6.1 [2]: pointer compatibility
  auto otherPointer = other.ToPointer();
  return otherPointer && derived_->Compatible(*otherPointer->derived_);

  // FIXME(wgtdkp): cannot loose compatible constraints
  //return other.IsInteger() ||
  //       (otherPointer && derived_->Compatible(*otherPointer->derived_));
}


bool ArrayType::Compatible(const Type& other) const {
  // C11 6.7.6.2 [6]: For two array type to be compatible,
  // the element types must be compatible, and have same length
  // if both specified.
  auto otherArray = other.ToArray();
  if (!otherArray) return false;
  if (!derived_->Compatible(*otherArray->derived_)) return false;
  // The lengths are both not specified
  if (!complete_ && !otherArray->complete_) return true;
  if (complete_ != otherArray->complete_) return false;
  return len_ == otherArray->len_;
}


bool FuncType::Compatible(const Type& other) const {
  auto other_func = other.ToFunc();
  //the other type is not an function type
  if (!other_func) return false;
  //TODO: do we need to check the type of return value when deciding 
  //compatibility of two function types ??
  if (!derived_->Compatible(*other_func->derived_))
    return false;
  if (param_list_.size() != other_func->param_list_.size())
    return false;

  auto this_iter = param_list_.begin();
  auto other_iter = other_func->param_list_.begin();
  while (this_iter != param_list_.end()) {
    if (!(*this_iter)->type()->Compatible(*(*other_iter)->type()))
      return false;
    ++this_iter;
    ++other_iter;
  }

  return true;
}


std::string FuncType::Str() const {
  auto str = derived_->Str() + "(";
  auto iter = param_list_.begin();
  for (; iter != param_list_.end(); ++iter) {
    str += (*iter)->type()->Str() + ", ";
  }
  if (variadic_)
    str += "...";
  else if (param_list_.size())
    str.resize(str.size() - 2);

  return str + ")";
}


StructType::StructType(MemPool* pool, bool is_struct,
                       bool has_tag, Scope* parent)
    : Type(pool, false),
      is_struct_(is_struct),
      has_tag_(has_tag),
      member_map_(Scope::New(parent, ScopeType::BLOCK)),
      offset_(0),
      width_(0),
      // If a struct type has no member, it gets alignment of 1
      align_(1),
      bitfiled_align_(1) {}


Object* StructType::GetMember(const std::string& member) {
  auto ident = member_map_->FindInCurScope(member);
  if (ident == nullptr)
    return nullptr;
  return ident->ToObject();
}


void StructType::CalcWidth() {
  width_ = 0;
  for (auto iter = member_map_->begin(); iter != member_map_->end(); ++iter) {
    width_ += iter->second->type()->width();
  }
}


bool StructType::Compatible(const Type& other) const {
  return this == &other; // Pointer comparison
}


// TODO(wgtdkp): more detailed representation
std::string StructType::Str() const {
  std::string str = is_struct_ ? "struct": "union";
  return str + ":" + std::to_string(width_);
}


// Remove useless unnamed bitfield members
// They are just for parsing
void StructType::Finalize() {
  for (auto iter = member_list_.begin(); iter != member_list_.end();) {
    if ((*iter)->bitfield_width() && (*iter)->anonymous()) {
      member_list_.erase(iter++);
    } else {
      ++iter;
    }
  }
}


void StructType::AddMember(Object* member) {
  auto offset = MakeAlign(offset_, member->align());
  member->set_offset(offset);
  
  member_list_.push_back(member);
  member_map_->Insert(member->Name(), member);

  align_ = std::max(align_, member->align());
  bitfiled_align_ = std::max(bitfiled_align_, align_);
  
  if (is_struct_) {
    offset_ = offset + member->type()->width();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, member->type()->width());
    width_ = MakeAlign(width_, align_);
  }
}


void StructType::AddBitField(Object* bitfield, int offset) {
  bitfield->set_offset(offset);
  member_list_.push_back(bitfield);
  if (!bitfield->anonymous())
    member_map_->Insert(bitfield->Name(), bitfield);

  auto bytes = MakeAlign(bitfield->bitfield_end(), 8) / 8;
  bitfiled_align_ = std::max(bitfiled_align_, bitfield->align());
  // Does not aligned, default is 1
  if (is_struct_) {
    offset_ = offset + bytes;
    width_ = MakeAlign(offset_, std::max(bitfiled_align_, bitfield->align()));
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, bitfield->type()->width());
  }
}


// Move members of anonymous struct/union to external struct/union
void StructType::MergeAnony(Object* anony) {
  auto anony_type = anony->type()->ToStruct();
  auto offset = MakeAlign(offset_, anony->align());

  // member_list in map are never anonymous
  for (auto& kv: *anony_type->member_map_) {
    auto& name = kv.first;
    auto member = kv.second->ToObject();
    // Every member of anonymous struct/union
    // are offseted by external struct/union
    member->set_offset(offset + member->offset());

    if (GetMember(name)) {
      Error(member, "duplicated member '%s'", name.c_str());
    }
    // Simplify anony struct's member searching
    member_map_->Insert(name, member);
  }
  anony->set_offset(offset);
  member_list_.push_back(anony);

  align_ = std::max(align_, anony->align());
  if (is_struct_) {
    offset_ = offset + anony_type->width();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, anony_type->width());
  }
}
