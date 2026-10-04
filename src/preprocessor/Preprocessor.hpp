#pragma once
#include "cc1/LangOptions.hpp"
#include "cc1/SourceManager.hpp"
#include "lexer/TokenSource.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PPToken {
  enum Kind { Identifier, Number, String, Character, Header, Punct, Other, Newline };
  Kind kind;
  std::string text;
  SourceLoc loc;
  bool leadingSpace = false;
  std::vector<SourceLoc> spellingLocations;
};

class Preprocessor : public TokenSource {
public:
  Preprocessor(SourceManager &sources, const SourceFile &mainFile,
               DiagnosticEngine &diagnostics,
               const LangOptions &options = LangOptions::forc99(),
               std::vector<std::string> includeDirs = {});
  bool run();
  Token next() override;
  std::string text();

private:
  SourceManager &_sources;
  const SourceFile &_main;
  DiagnosticEngine &_diag;
  LangOptions _options;
  std::vector<std::string> _includeDirs;
  std::unordered_map<std::string, std::vector<PPToken>> _macros;
  std::unordered_set<const SourceFile *> _once;
  std::unordered_set<const SourceFile *> _visited;
  std::vector<PPToken> _output;
  SourceLoc _end;
  size_t _cursor = 0;
  size_t _expansions = 0;
  bool _ran = false;

  bool process(const SourceFile &file, unsigned depth, SourceLoc includeLoc);
  bool expand(const std::vector<PPToken> &input, std::vector<PPToken> &output,
              std::unordered_set<std::string> &disabled, unsigned depth = 0);
  bool condition(const std::vector<PPToken> &input, SourceLoc loc, bool &result);
  bool include(const SourceFile &file, std::vector<PPToken> arguments,
               SourceLoc loc, unsigned depth);
};
