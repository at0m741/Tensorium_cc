#include "cc1/Diagnostic.hpp"
#include "cc1/LangOptions.hpp"
#include "cc1/SourceManager.hpp"
#include "cc1/TargetInfo.hpp"
#include "lexer/Lexer.hpp"
#include "lexer/Token.hpp"
#include "parser/ASTDump.hpp"
#include "parser/Parser.hpp"
#include "preprocessor/Preprocessor.hpp"
#include "sema/Sema.hpp"
#ifdef CC1_ENABLE_MLIR
#include "codegen/MLIRGen.hpp"
#include "llvm/Support/raw_ostream.h"
#endif
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

int main(int argc, char **argv) {
  DiagnosticColor color = DiagnosticColor::Auto;
  bool dumpTokens = false;
  bool dumpAst = false;
  bool dumpSema = false;
  bool preprocessOnly = false;
  bool emitMlir = false;
  std::vector<std::string> includeDirs;
  const char *filename = nullptr;
  bool options = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (options && arg == "--") {
      options = false;
    } else if (options && arg == "-fcolor-diagnostics") {
      color = DiagnosticColor::Always;
    } else if (options && arg == "-fno-color-diagnostics") {
      color = DiagnosticColor::Never;
    } else if (options && arg == "--dump-tokens") {
      dumpTokens = true;
    } else if (options && arg == "--dump-ast") {
      dumpAst = true;
    } else if (options && arg == "--dump-sema") {
      dumpSema = true;
    } else if (options && arg == "--emit-mlir") {
#ifdef CC1_ENABLE_MLIR
      emitMlir = true;
#else
      DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
      diagnostics.error({}, "MLIR support is disabled; rebuild with "
                            "-DCC1_ENABLE_MLIR=ON");
      return 1;
#endif
    } else if (options && arg == "-E") {
      preprocessOnly = true;
    } else if (options && (arg == "-I" || arg.rfind("-I", 0) == 0)) {
      if (arg == "-I") {
        if (i + 1 == argc) {
          DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
          diagnostics.error({}, "expected directory after -I");
          return 1;
        }
        includeDirs.emplace_back(argv[++i]);
      } else {
        includeDirs.push_back(arg.substr(2));
      }
    } else if ((options && !arg.empty() && arg[0] == '-') || filename) {
      DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
      diagnostics.error({}, filename ? "expected a single input file"
                                     : "unknown option '" + arg + "'");
      return 1;
    } else {
      filename = argv[i];
    }
  }
  if (!filename) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "no input file");
    std::cerr << "usage: cc1 "
                 "[-E|--dump-tokens|--dump-ast|--dump-sema|--emit-mlir] [-I "
                 "directory] "
                 "[-fcolor-diagnostics|-fno-color-diagnostics] <file.c>\n";
    return 1;
  }

  if (static_cast<int>(dumpTokens) + static_cast<int>(dumpAst) +
          static_cast<int>(dumpSema) + static_cast<int>(emitMlir) >
      1) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "output options cannot be combined");
    return 1;
  }
  if (preprocessOnly && (dumpTokens || dumpAst || dumpSema || emitMlir)) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "-E cannot be combined with another output option");
    return 1;
  }

  std::ifstream file(filename);
  if (!file) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "cannot open '" + std::string(filename) + "'");
    return 1;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string src = ss.str();
  const LangOptions opts = LangOptions::forc99();
  DiagnosticEngine diagnostics(filename, src, std::cerr, color);
  Lexer lexer(src, filename, diagnostics, opts);
  if (dumpTokens) {
    const auto tokens = lexer.tokenizeAll();
    if (diagnostics.hasErrors())
      return 1;

    for (const auto &tok : tokens) {
      printf("%s:%u:%u\t%s\t'%s'\n", tok.loc.filename, tok.loc.line,
             tok.loc.col, tokenKindName(tok.kind), tok.text.c_str());
    }
    return 0;
  }

  SourceManager sources(diagnostics);
  const auto &mainFile = sources.add(filename, src);
  Preprocessor preprocessor(sources, mainFile, diagnostics, opts, includeDirs);
  if (!preprocessor.run())
    return 1;
  if (preprocessOnly) {
    std::cout << preprocessor.text();
    return 0;
  }

  TargetInfo target{};
#if defined(__aarch64__) || defined(_M_ARM64)
  target = TargetInfo::aarch64();
#elif defined(__x86_64__) || defined(_M_X64)
  target = TargetInfo::x86_64();
#elif defined(__i386__) || defined(_M_IX86)
  target = TargetInfo::i386();
#else
  diagnostics.error({}, "unsupported host architecture");
  return 1;
#endif
  TypePool types;
  Parser parser(preprocessor, types, diagnostics, target, opts);
  std::unique_ptr<TranslationUnit> unit(parser.parse());
  if (diagnostics.hasErrors())
    return 1;

  if (dumpAst) {
    dumpAST(unit.get(), std::cout);
    return 0;
  }

  Sema sema(types, diagnostics, target);
  if (!sema.analyze(*unit))
    return 1;
  if (dumpSema)
    dumpAST(unit.get(), std::cout, 0, true);
#ifdef CC1_ENABLE_MLIR
  if (emitMlir) {
    mlir::MLIRContext context;
    MLIRGen generator(context, diagnostics, target);

    auto module = generator.generate(*unit);
    if (!module)
      return 1;

    module->print(llvm::outs());
    llvm::outs() << '\n';
  }
#endif
  return 0;
}
