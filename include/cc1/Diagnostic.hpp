#pragma once
#include "SourceLocation.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

enum class DiagnosticLevel { Note, Warning, Error, Fatal };
enum class DiagnosticColor { Auto, Always, Never };

struct Diagnostic {
  DiagnosticLevel level;
  std::string filename;
  uint32_t line;
  uint32_t col;
  std::string message;
  size_t length;
};

class DiagnosticEngine {
public:
  DiagnosticEngine(std::string filename, std::string source,
                   std::ostream &output = std::cerr,
                   DiagnosticColor color = DiagnosticColor::Auto);

  void report(DiagnosticLevel level, SourceLoc loc, const std::string &message,
              size_t length = 1);
  void addSource(const std::string &filename, const std::string &source);
  void error(SourceLoc loc, const std::string &message, size_t length = 1) {
    report(DiagnosticLevel::Error, loc, message, length);
  }
  void warning(SourceLoc loc, const std::string &message, size_t length = 1) {
    report(DiagnosticLevel::Warning, loc, message, length);
  }
  void note(SourceLoc loc, const std::string &message, size_t length = 1) {
    report(DiagnosticLevel::Note, loc, message, length);
  }
  void fatal(SourceLoc loc, const std::string &message, size_t length = 1) {
    report(DiagnosticLevel::Fatal, loc, message, length);
  }

  bool hasErrors() const { return _errorCount != 0; }
  size_t errorCount() const { return _errorCount; }
  const std::vector<Diagnostic> &diagnostics() const { return _diagnostics; }

private:
  void render(const Diagnostic &diagnostic);

  std::string _filename;
  std::ostream &_output;
  bool _useColor;
  size_t _errorCount = 0;
  struct SourceText {
    std::string text;
    std::vector<size_t> lineStarts;
  };
  std::unordered_map<std::string, SourceText> _sources;
  std::vector<Diagnostic> _diagnostics;
};
