#include "cc1/Frontend/CompilerInstance.hpp"
#include "cc1/AST/ASTDump.hpp"
#include "cc1/Lex/Lexer.hpp"
#include "cc1/Lex/Preprocessor.hpp"
#include "cc1/Parse/Parser.hpp"
#include "cc1/Sema/Sema.hpp"
#ifdef CC1_ENABLE_MLIR
#include "cc1/CodeGen/MLIRGen.hpp"
#include "llvm/Support/raw_ostream.h"
#endif
#include <fstream>
#include <ostream>
#include <sstream>
#include <utility>

CompilerInstance::CompilerInstance(CompilerInvocation invocation,
                                   std::ostream &output,
                                   std::ostream &diagnosticOutput)
    : invocation(std::move(invocation)), output(output),
      diagnosticOutput(diagnosticOutput),
      diagnostics(this->invocation.inputFile, "", diagnosticOutput,
                  this->invocation.diagnosticColor),
      sources(diagnostics) {}

bool CompilerInstance::hasMLIRSupport() {
#ifdef CC1_ENABLE_MLIR
  return true;
#else
  return false;
#endif
}

bool CompilerInstance::execute() {
  const char *filename = invocation.inputFile.c_str();
  const bool dumpTokens = invocation.action == FrontendAction::DumpTokens;
  const bool dumpAst = invocation.action == FrontendAction::DumpAST;
  const bool dumpSema = invocation.action == FrontendAction::DumpSema;
  const bool preprocessOnly = invocation.action == FrontendAction::Preprocess;
  const bool emitMlir = invocation.action == FrontendAction::EmitMLIR;
  if (emitMlir && !hasMLIRSupport()) {
    diagnostics.error({}, "MLIR support is disabled; rebuild with "
                          "-DCC1_ENABLE_MLIR=ON");
    return false;
  }
  std::ifstream file(filename);
  if (!file) {
    DiagnosticEngine fileDiagnostics("cc1", "", diagnosticOutput,
                                     invocation.diagnosticColor);
    fileDiagnostics.error({}, "cannot open '" + std::string(filename) + "'");
    return false;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string src = ss.str();
  const LangOptions &opts = invocation.langOptions;
  diagnostics.addSource(filename, src);
  Lexer lexer(src, filename, diagnostics, opts);
  if (dumpTokens) {
    const auto tokens = lexer.tokenizeAll();
    if (diagnostics.hasErrors())
      return false;

    for (const auto &tok : tokens) {
      output << tok.loc.filename << ':' << tok.loc.line << ':' << tok.loc.col
             << '\t' << tokenKindName(tok.kind) << "\t'" << tok.text << "'\n";
    }
    return true;
  }

  const auto &mainFile = sources.add(filename, src);
  Preprocessor preprocessor(sources, mainFile, diagnostics, opts,
                            invocation.includeDirs);
  if (!preprocessor.run())
    return false;
  if (preprocessOnly) {
    output << preprocessor.text();
    return true;
  }

  TargetInfo target{};
  if (invocation.target) {
    target = *invocation.target;
  } else {
#if defined(__aarch64__) || defined(_M_ARM64)
    target = TargetInfo::aarch64();
#elif defined(__x86_64__) || defined(_M_X64)
    target = TargetInfo::x86_64();
#elif defined(__i386__) || defined(_M_IX86)
    target = TargetInfo::i386();
#else
    diagnostics.error({}, "unsupported host architecture");
    return false;
#endif
  }
  Parser parser(preprocessor, ast, diagnostics, target, opts);
  unit = parser.parse();
  if (diagnostics.hasErrors()) {
    unit = nullptr;
    return false;
  }

  if (dumpAst) {
    dumpAST(unit, output);
    return true;
  }

  Sema sema(ast, diagnostics, target);
  if (!sema.analyze(*unit)) {
    unit = nullptr;
    return false;
  }
  if (dumpSema)
    dumpAST(unit, output, 0, true);
#ifdef CC1_ENABLE_MLIR
  if (emitMlir) {
    mlir::MLIRContext context;
    MLIRGen generator(context, diagnostics, target);

    auto module = generator.generate(*unit);
    if (!module)
      return false;

    std::string text;
    llvm::raw_string_ostream stream(text);
    module->print(stream);
    output << stream.str() << '\n';
  }
#endif
  return true;
}
