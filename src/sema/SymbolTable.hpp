#pragma once
#include "Scope.hpp"
#include <vector>

class SymbolTable {
public:
  void clear() { scopes.clear(); }
  void push() { scopes.emplace_back(); }
  void pop() { scopes.pop_back(); }
  Decl *lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->ordinary.find(name);
      if (found != it->ordinary.end())
        return found->second;
    }
    return nullptr;
  }
  Decl *lookupCurrent(const std::string &name) const {
    auto found = scopes.back().ordinary.find(name);
    return found == scopes.back().ordinary.end() ? nullptr : found->second;
  }
  void bind(Decl *decl) { scopes.back().ordinary[decl->name] = decl; }

private:
  std::vector<Scope> scopes;
};
