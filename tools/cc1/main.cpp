#include "cc1/Frontend/CompilerInstance.hpp"
#include "cc1/Frontend/CompilerInvocation.hpp"
#include <iostream>
#include <utility>

int main(int argc, char **argv) {
  CompilerInvocation invocation;
  if (!parseCompilerInvocation(argc, argv, invocation, std::cerr))
    return 1;
  CompilerInstance compiler(std::move(invocation), std::cout, std::cerr);
  return compiler.execute() ? 0 : 1;
}
