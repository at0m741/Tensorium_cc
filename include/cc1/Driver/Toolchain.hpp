#pragma once

#include "cc1/Driver/DriverOptions.hpp"
#include <iosfwd>
#include <optional>
#include <string>

namespace cc1::driver {
struct Toolchain {
  std::string cc1, optimizer, translator, clang;
};
std::optional<Toolchain> discoverToolchain(const DriverOptions &options,
                                           const std::string &executable,
                                           std::ostream &errors);
} // namespace cc1::driver
