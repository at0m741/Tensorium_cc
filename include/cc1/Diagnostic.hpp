#pragma once
#include "../../src/lexer/Token.hpp"
#include <cstdio>
#include <string>
#include <vector>

enum class DiagLevel { NOTE, WARNING, ERROR, FATAL };

struct Diag {
  DiagLevel level;
  SourceLoc loc;
  std::string msg;
};

class Diagnostic {
public:
  void report(DiagLevel level, const SourceLoc &loc, const std::string &msg) {
    _diags.push_back({level, loc, msg});

    const char *prefix = nullptr;
    switch (level) {
    case DiagLevel::NOTE:
      prefix = "note";
      break;
    case DiagLevel::WARNING:
      prefix = "warning";
      break;
    case DiagLevel::ERROR:
      prefix = "error";
      break;
    case DiagLevel::FATAL:
      prefix = "fatal";
      break;
    }
    fprintf(stderr, "%s:%u:%u %s: %s\n", loc.filename, loc.line, loc.col,
            prefix, msg.c_str());
  }

  void note(const SourceLoc &l, const std::string &m) {
    report(DiagLevel::NOTE, l, m);
  }
  void warning(const SourceLoc &l, const std::string &m) {
    report(DiagLevel::WARNING, l, m);
  }
  void error(const SourceLoc &l, const std::string &m) {
    report(DiagLevel::ERROR, l, m);
  }
  void fatal(const SourceLoc &l, const std::string &m) {
    report(DiagLevel::FATAL, l, m);
  }

  bool hasErrors() const {
    for (auto &d : _diags)
      if (d.level >= DiagLevel::ERROR)
        return true;
    return false;
  }

  int errorCount() const {
    int n = 0;
    for (auto &d : _diags)
      if (d.level >= DiagLevel::ERROR)
        n++;
    return n;
  }

private:
  std::vector<Diag> _diags;
};
