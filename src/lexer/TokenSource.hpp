#pragma once
#include "Token.hpp"

class TokenSource {
public:
  virtual ~TokenSource() = default;
  virtual Token next() = 0;
};
