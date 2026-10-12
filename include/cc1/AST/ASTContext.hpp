#pragma once

#include "cc1/AST/AST.hpp"
#include "cc1/AST/Type.hpp"
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// Owns every node and type in a translation unit, including Sema's conversions.
// Pointers between nodes are non-owning and remain valid until this context
// dies.
class ASTContext {
public:
  ASTContext() = default;
  ASTContext(const ASTContext &) = delete;
  ASTContext &operator=(const ASTContext &) = delete;

  TypePool &getTypes() { return types; }

  template <typename NodeTy, typename... Args> NodeTy *create(Args &&...args) {
    static_assert(std::is_base_of_v<Node, NodeTy>, "expected an AST node");
    auto node = std::make_unique<NodeTy>(std::forward<Args>(args)...);
    NodeTy *result = node.get();
    nodes.push_back(std::move(node));
    return result;
  }

private:
  // Nodes are destroyed before the types they reference.
  TypePool types;
  std::vector<std::unique_ptr<Node>> nodes;
};
