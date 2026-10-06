#pragma once
#include "parser/AST.hpp"
#include <string>
#include <unordered_map>

struct Scope {
  std::unordered_map<std::string, Decl *> ordinary;
};
