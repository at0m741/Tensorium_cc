#include "cc1/Frontend/CompilerInstance.hpp"
#include "cc1/Frontend/CompilerInvocation.hpp"
#include <iostream>
#include <string_view>
#include <utility>
#ifdef CC1_HAS_NATIVE_DRIVER
#include "cc1/Driver/Driver.hpp"
#include "cc1/Driver/DriverOptions.hpp"
#endif

int main(int argc, char **argv) {
  const bool internalFrontend = argc > 1 && std::string_view(argv[1]) == "-cc1";
  if (internalFrontend) {
    --argc;
    ++argv;
  }
#ifdef CC1_HAS_NATIVE_DRIVER
  if (!internalFrontend && !cc1::driver::isFrontendInvocation(argc, argv))
    return cc1::driver::runDriver(argc, argv);
#endif
  CompilerInvocation invocation;
  if (!parseCompilerInvocation(argc, argv, invocation, std::cerr))
    return 1;
  CompilerInstance compiler(std::move(invocation), std::cout, std::cerr);
  return compiler.execute() ? 0 : 1;
}
