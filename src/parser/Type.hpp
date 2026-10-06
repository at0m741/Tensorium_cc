#pragma once
#include <string>
#include <vector>

struct TargetInfo;

struct Qualifiers {
  bool isConst = false;
  bool isVolatile = false;

  bool operator==(const Qualifiers &o) const {
    return isConst == o.isConst && isVolatile == o.isVolatile;
  }
};

struct Type {
  enum Kind {
    VOID,
    BOOL,
    CHAR,
    SCHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    LONGLONG,
    ULONGLONG,
    FLOAT,
    DOUBLE,
    LONGDOUBLE,
    POINTER,
    ARRAY,
    FUNCTION,
    STRUCT,
    UNION,
    ENUM,
    TYPEDEF,
  } kind;

  Qualifiers quals;

  Type *pointee = nullptr;
  Type *elemType = nullptr;
  int arraySize = -1;
  Type *retType = nullptr;
  std::vector<Type *> params;
  bool variadic = false;
  bool hasPrototype = true;

  struct Field {
    std::string name;
    Type *type = nullptr;
    int offset = 0;
    int bitWidth = -1;
  };
  std::vector<Field> fields;
  std::string tag;
  bool defined = false;

  Type *underlying = nullptr;

  Type() : kind(VOID) {}
  explicit Type(Kind k) : kind(k) {}

  static Type *makePointer(Type *pointee) {
    Type *t = new Type(POINTER);
    t->pointee = pointee;
    return t;
  }
  static Type *makeArray(Type *elem, int size = -1) {
    Type *t = new Type(ARRAY);
    t->elemType = elem;
    t->arraySize = size;
    return t;
  }
  static Type *makeFunction(Type *ret, std::vector<Type *> params,
                            bool variadic = false) {
    Type *t = new Type(FUNCTION);
    t->retType = ret;
    t->params = std::move(params);
    t->variadic = variadic;
    return t;
  }

  bool isVoid() const { return kind == VOID; }
  bool isPointer() const { return kind == POINTER; }
  bool isArray() const { return kind == ARRAY; }
  bool isFunction() const { return kind == FUNCTION; }
  bool isStruct() const { return kind == STRUCT; }
  bool isUnion() const { return kind == UNION; }
  bool isEnum() const { return kind == ENUM; }

  bool isIntegral() const {
    return (kind >= BOOL && kind <= ULONGLONG) || kind == ENUM;
  }
  bool isFloating() const {
    return kind == FLOAT || kind == DOUBLE || kind == LONGDOUBLE;
  }
  bool isArithmetic() const { return isIntegral() || isFloating(); }
  bool isScalar() const { return isArithmetic() || isPointer(); }

  bool isSigned() const {
    return kind == CHAR || kind == SCHAR || kind == SHORT || kind == INT ||
           kind == LONG || kind == LONGLONG;
  }
  bool isUnsigned() const {
    return kind == UCHAR || kind == USHORT || kind == UINT || kind == ULONG ||
           kind == ULONGLONG;
  }

  bool isComplete() const {
    if (kind == VOID || kind == FUNCTION)
      return false;
    if (kind == ARRAY)
      return arraySize > 0 && elemType && elemType->isComplete();
    if ((kind == STRUCT || kind == UNION || kind == ENUM) && !defined)
      return false;
    return true;
  }

  int sizeOf(const TargetInfo &ti) const;
  int alignOf(const TargetInfo &ti) const;
  std::string llvmRepr(const TargetInfo &ti) const;
  std::string str() const;
};

struct TypePool {
  std::vector<Type *> _all;

  TypePool() = default;
  TypePool(const TypePool &) = delete;
  TypePool &operator=(const TypePool &) = delete;

  ~TypePool() {
    for (auto *t : _all)
      delete t;
  }

  Type *make(Type::Kind k) {
    Type *t = new Type(k);
    _all.push_back(t);
    return t;
  }

  Type *voidTy() { return make(Type::VOID); }
  Type *charTy() { return make(Type::CHAR); }
  Type *intTy() { return make(Type::INT); }
  Type *longTy() { return make(Type::LONG); }
  Type *uintTy() { return make(Type::UINT); }
  Type *doubleTy() { return make(Type::DOUBLE); }
  Type *floatTy() { return make(Type::FLOAT); }
  Type *shortTy() { return make(Type::SHORT); }
  Type *ulongTy() { return make(Type::ULONG); }
  Type *ptrTo(Type *t) {
    Type *ptr = make(Type::POINTER);
    ptr->pointee = t;
    return ptr;
  }
  Type *arrayOf(Type *elem, int size = -1) {
    Type *array = make(Type::ARRAY);
    array->elemType = elem;
    array->arraySize = size;
    return array;
  }
  Type *function(Type *ret, std::vector<Type *> params, bool variadic = false) {
    Type *fn = make(Type::FUNCTION);
    fn->retType = ret;
    fn->params = std::move(params);
    fn->variadic = variadic;
    return fn;
  }
  Type *qualified(Type *base, Qualifiers quals) {
    Type *copy = make(base->kind);
    *copy = *base;
    copy->quals = quals;
    if (base->isArray() && base->elemType) {
      Qualifiers elementQuals = base->elemType->quals;
      elementQuals.isConst = elementQuals.isConst || quals.isConst;
      elementQuals.isVolatile = elementQuals.isVolatile || quals.isVolatile;
      copy->elemType = qualified(base->elemType, elementQuals);
    }
    return copy;
  }
};
