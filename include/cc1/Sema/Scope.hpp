#pragma once
#include "cc1/AST/AST.hpp"
#include <string>
#include <unordered_map>

struct Scope {
  std::unordered_map<std::string, Decl *> ordinary;
};
