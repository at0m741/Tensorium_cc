#pragma once
#include "cc1/LangOptions.hpp"
#include "cc1/SourceManager.hpp"
#include "lexer/TokenSource.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PPToken {
  enum Kind {
    Identifier,
    Number,
    String,
    Character,
    Header,
    Punct,
    Other,
    Newline
  };
  Kind kind;
  std::string text;
  SourceLoc loc;
  bool leadingSpace = false;
  std::vector<SourceLoc> spellingLocations;
  // A suppressed self-reference stays suppressed during later rescans.
  std::unordered_set<std::string> hideSet = {};
  std::string originalSpelling = {};
  // Builtins in a macro body use the end of the invocation; diagnostics keep loc.
  SourceLoc expansionLoc = {};
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
  struct Macro {
    bool functionLike = false;
    bool variadic = false;
    std::vector<std::string> parameters;
    std::vector<PPToken> replacement;
  };
  std::unordered_map<std::string, Macro> _macros;
  std::string _date;
  std::string _time;
  std::unordered_set<const SourceFile *> _once;
  std::unordered_set<const SourceFile *> _visited;
  std::vector<PPToken> _output;
  SourceLoc _end;
  size_t _cursor = 0;
  size_t _expansions = 0;
  bool _ran = false;

  bool process(const SourceFile &file, unsigned depth, SourceLoc includeLoc);
  bool expand(const std::vector<PPToken> &input, std::vector<PPToken> &output,
              unsigned depth = 0);
  bool define(const std::vector<PPToken> &arguments, SourceLoc loc);
  bool isPredefined(const std::string &name) const;
  bool isDefined(const std::string &name) const;
  PPToken predefined(const PPToken &token) const;
  bool condition(const std::vector<PPToken> &input, SourceLoc loc,
                 bool &result);
  bool include(const SourceFile &file, std::vector<PPToken> arguments,
               SourceLoc loc, unsigned depth);
};
