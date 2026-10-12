#pragma once
#include "cc1/Lex/Token.hpp"

class TokenSource {
public:
  virtual ~TokenSource() = default;
  virtual Token next() = 0;
};
