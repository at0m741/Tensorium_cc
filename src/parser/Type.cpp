#include "Type.hpp"

namespace {
std::string typeName(const Type *type) {
  if (!type)
    return "<null>";

  std::string name;
  static constexpr const char *builtins[] = {
      "void",
      "_Bool",
      "char",
      "signed char",
      "unsigned char",
      "short",
      "unsigned short",
      "int",
      "unsigned int",
      "long",
      "unsigned long",
      "long long",
      "unsigned long long",
      "float",
      "double",
      "long double",
  };
  static_assert(sizeof(builtins) / sizeof(builtins[0]) == Type::LONGDOUBLE + 1);
  if (type->kind <= Type::LONGDOUBLE) {
    name = builtins[type->kind];
  } else {
    switch (type->kind) {
    case Type::POINTER:
      name = typeName(type->pointee) + "*";
      if (type->quals.isConst)
        name += " const";
      if (type->quals.isVolatile)
        name += " volatile";
      return name;
    case Type::ARRAY:
      name = typeName(type->elemType) + "[";
      if (type->arraySize >= 0)
        name += std::to_string(type->arraySize);
      name += "]";
      break;
    case Type::FUNCTION:
      name = typeName(type->retType) + " (";
      for (size_t i = 0; i < type->params.size(); ++i) {
        if (i)
          name += ", ";
        name += typeName(type->params[i]);
      }
      if (type->variadic)
        name += type->params.empty() ? "..." : ", ...";
      name += ")";
      break;
    case Type::STRUCT:
      name = "struct " + type->tag;
      break;
    case Type::UNION:
      name = "union " + type->tag;
      break;
    case Type::ENUM:
      name = "enum " + type->tag;
      break;
    case Type::TYPEDEF:
      name = typeName(type->underlying);
      break;
    default:
      name = "?";
      break;
    }
  }
  if (type->quals.isVolatile)
    name = "volatile " + name;
  if (type->quals.isConst)
    name = "const " + name;
  return name;
}

} // namespace

std::string Type::str() const { return typeName(this); }
