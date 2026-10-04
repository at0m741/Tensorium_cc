#include "cc1/Diagnostic.hpp"
#include <algorithm>
#include <cstdlib>
#include <utility>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
bool useColor(std::ostream &output, DiagnosticColor mode) {
  if (mode != DiagnosticColor::Auto)
    return mode == DiagnosticColor::Always;

  const char *noColor = std::getenv("NO_COLOR");
  const char *term = std::getenv("TERM");
  if ((noColor && *noColor) || (term && std::string(term) == "dumb"))
    return false;

#ifdef _WIN32
  // ANSI escape support is not guaranteed for Windows console streams.
  return false;
#else
  int fd = -1;
  if (&output == &std::cerr)
    fd = 2;
  else if (&output == &std::cout)
    fd = 1;
  return fd >= 0 && isatty(fd);
#endif
}

const char *levelName(DiagnosticLevel level) {
  switch (level) {
  case DiagnosticLevel::Error: return "error";
  case DiagnosticLevel::Fatal: return "fatal error";
  case DiagnosticLevel::Warning: return "warning";
  case DiagnosticLevel::Note: return "note";
  }
  return "error";
}

const char *levelColor(DiagnosticLevel level) {
  switch (level) {
  case DiagnosticLevel::Error: return "\033[1;31m";
  case DiagnosticLevel::Fatal: return "\033[1;31m";
  case DiagnosticLevel::Warning: return "\033[1;35m";
  case DiagnosticLevel::Note: return "\033[1;36m";
  }
  return "\033[1;31m";
}
} // namespace

DiagnosticEngine::DiagnosticEngine(std::string filename, std::string source,
                                   std::ostream &output, DiagnosticColor color)
    : _filename(std::move(filename)), _output(output),
      _useColor(useColor(output, color)) {
  addSource(_filename, source);
}

void DiagnosticEngine::addSource(const std::string &filename,
                                  const std::string &source) {
  SourceText entry{source, {0}};
  for (size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '\r') {
      if (i + 1 < source.size() && source[i + 1] == '\n')
        ++i;
      entry.lineStarts.push_back(i + 1);
    } else if (source[i] == '\n') {
      entry.lineStarts.push_back(i + 1);
    }
  }
  _sources[filename] = std::move(entry);
}

void DiagnosticEngine::report(DiagnosticLevel level, SourceLoc loc,
                              const std::string &message, size_t length) {
  _diagnostics.push_back({level, loc.filename ? loc.filename : _filename,
                          loc.line, loc.col, message, std::max(size_t(1), length)});
  if (level == DiagnosticLevel::Error || level == DiagnosticLevel::Fatal)
    ++_errorCount;
  render(_diagnostics.back());
}

void DiagnosticEngine::render(const Diagnostic &diagnostic) {
  const char *bold = _useColor ? "\033[1m" : "";
  const char *reset = _useColor ? "\033[0m" : "";
  _output << bold << diagnostic.filename;
  if (diagnostic.line != 0) {
    _output << ':' << diagnostic.line;
    if (diagnostic.col != 0)
      _output << ':' << diagnostic.col;
  }
  _output << ": " << reset;
  if (_useColor)
    _output << levelColor(diagnostic.level);
  _output << levelName(diagnostic.level) << ": " << reset
          << bold << diagnostic.message << reset << '\n';

  const auto entry = _sources.find(diagnostic.filename);
  if (entry == _sources.end() || diagnostic.line == 0 ||
      diagnostic.line > entry->second.lineStarts.size() || diagnostic.col == 0)
    return;

  const auto &source = entry->second.text;
  const size_t start = entry->second.lineStarts[diagnostic.line - 1];
  size_t end = source.find_first_of("\r\n", start);
  if (end == std::string::npos)
    end = source.size();
  const std::string line = source.substr(start, end - start);
  const size_t column = std::min(size_t(diagnostic.col - 1), line.size());
  const size_t span = std::min(diagnostic.length, line.size() - column);

  // Source columns count bytes; tabs expand to eight-column stops for display.
  std::string displayed;
  size_t caret = 0;
  size_t highlightEnd = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    if (i == column)
      caret = displayed.size();
    const unsigned char c = static_cast<unsigned char>(line[i]);
    if (c == '\t')
      displayed.append(8 - displayed.size() % 8, ' ');
    else
      displayed += c < 32 || c == 127 ? '?' : static_cast<char>(c);
    if (i + 1 == column + span)
      highlightEnd = displayed.size();
  }
  if (column == line.size())
    caret = displayed.size();
  const size_t width = span == 0 ? 1 : highlightEnd - caret;
  const std::string number = std::to_string(diagnostic.line);
  const size_t gutter = std::max(size_t(5), number.size());
  _output << std::string(gutter - number.size(), ' ') << number
          << " | " << displayed << '\n'
          << std::string(gutter, ' ') << " | " << std::string(caret, ' ');
  if (_useColor)
    _output << "\033[1;32m";
  _output << '^' << std::string(width - 1, '~') << reset << '\n';
}
