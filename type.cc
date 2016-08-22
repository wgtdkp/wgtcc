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
static MemPoolImp<StructUnionType>  structUnionTypePool;
static MemPoolImp<ArithmType>       arithmTypePool;


bool Type::IsFloat(void) const {
    auto arithmType = ToArithmType();
    return arithmType && arithmType->IsFloat();
}

bool Type::IsInteger(void) const {
    auto arithmType = ToArithmType();
    return arithmType && arithmType->IsInteger();
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

StructUnionType* StructUnionType::New(
        bool isStruct, bool hasTag, Scope* parent) {
    return new (structUnionTypePool.Alloc())
            StructUnionType(&structUnionTypePool, isStruct, hasTag, parent);
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
    switch (_tag) {
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

    switch (_tag) {
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

    return *_derived == *otherType->_derived;
}

bool PointerType::Compatible(const Type& other) const {
    //TODO: compatibility ???
    //if (other.IsInteger())
    //    return true;
    auto otherType = other.ToPointerType();
    if (otherType == nullptr)
        return false;
    if ((otherType->_derived->ToVoidType() && !_derived->ToFuncType())
            || (_derived->ToVoidType() && !otherType->_derived->ToFuncType()))
        return true;
    return _derived->Compatible(*otherType->_derived);
}


/************* FuncType ***************/

bool FuncType::operator==(const Type& other) const
{
    auto otherFunc = other.ToFuncType();
    if (nullptr == otherFunc) return false;
    // TODO: do we need to check the type of return value when deciding 
    // equality of two function types ??
    if (*_derived != *otherFunc->_derived)
        return false;
    if (_paramTypes.size() != otherFunc->_paramTypes.size())
        return false;

    auto thisIter = _paramTypes.begin();
    auto otherIter = otherFunc->_paramTypes.begin();
    while (thisIter != _paramTypes.end()) {
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
    if (!_derived->Compatible(*otherFunc->_derived))
        return false;
    if (_paramTypes.size() != otherFunc->_paramTypes.size())
        return false;

    auto thisIter = _paramTypes.begin();
    auto otherIter = otherFunc->_paramTypes.begin();
    while (thisIter != _paramTypes.end()) {
        if (!(*thisIter)->Compatible(*(*otherIter)))
            return false;

        thisIter++;
        otherIter++;
    }

    return true;
}


std::string FuncType::Str(void) const
{
    auto str = _derived->Str() + "(";
    auto iter = _paramTypes.begin();
    for (; iter != _paramTypes.end(); iter++) {
        str += (*iter)->Str() + ", ";
    }
    if (_variadic)
        str += "...";
    else if (_paramTypes.size())
        str.resize(str.size() - 2);

    return str + ")";
}

/*
 * StructUnionType
 */

StructUnionType::StructUnionType(MemPool* pool,
        bool isStruct, bool hasTag, Scope* parent)
        : Type(pool, false), _isStruct(isStruct), _hasTag(hasTag),
          _memberMap(new Scope(parent, S_BLOCK)),
          _offset(0), _width(0), _align(0) {}


Object* StructUnionType::GetMember(const std::string& member) {
    auto ident = _memberMap->FindInCurScope(member);
    if (ident == nullptr)
        return nullptr;
    return ident->ToObject();
}

void StructUnionType::CalcWidth(void)
{
    _width = 0;
    auto iter = _memberMap->_identMap.begin();
    for (; iter != _memberMap->_identMap.end(); iter++) {
        _width += iter->second->Type()->Width();
    }
}

bool StructUnionType::operator==(const Type& other) const
{
    auto structUnionType = other.ToStructUnionType();
    if (nullptr == structUnionType)
        return false;

    return *_memberMap == *structUnionType->_memberMap;
}


bool StructUnionType::Compatible(const Type& other) const {
    // TODO: 
    return *this == other;
}


// TODO(wgtdkp): more detailed representation
std::string StructUnionType::Str(void) const
{
    std::string str = _isStruct ? "struct": "union";
    return str + ":" + std::to_string(_width);
}


void StructUnionType::AddMember(const std::string& name, Object* member)
{
    auto offset = MakeAlign(_offset, member->Type()->Align());
    member->SetOffset(offset);
    
    _members.push_back(member);
    _memberMap->Insert(name, member);

    _align = std::max(_align, member->Type()->Align());

    if (_isStruct) {
        _offset = offset + member->Type()->Width();
        _width = MakeAlign(_offset, _align);
    } else {
        assert(_offset == 0);
        _width = std::max(_width, member->Type()->Width());
    }
}


// Move members of Anonymous struct/union to external struct/union
// TODO(wgtdkp):
// Width of struct/union is not the sum of its members
void StructUnionType::MergeAnony(StructUnionType* anonType)
{   
    auto offset = MakeAlign(_offset, anonType->Align());
    for (auto member: anonType->_members) {
        member->SetOffset(offset + member->Offset());

        auto name = member->Name();
        if (GetMember(name)) {
            Error(member->Tok(), "duplicated member '%s'", name.c_str());
        }
        _members.push_back(member);
        _memberMap->Insert(name, member);
    }

    _align = std::max(_align, anonType->Align());
    if (_isStruct) {
        _offset = offset + anonType->Width();
        _width = MakeAlign(_offset, _align);
    } else {
        assert(_offset == 0);
        _width = std::max(_width, anonType->Width());
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