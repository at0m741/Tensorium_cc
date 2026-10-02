#pragma once
#include <cstdint>

struct SourceLoc {
  const char *filename = nullptr;
  uint32_t line = 0;
  uint32_t col = 0;
};
