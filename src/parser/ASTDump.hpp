#pragma once
#include <iosfwd>
#include <ostream>

struct Node;

void dumpAST(const Node *node, std::ostream &out, unsigned depth = 0);
