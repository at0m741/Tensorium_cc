#pragma once
#include "Diagnostic.hpp"
#include <memory>
#include <string>
#include <unordered_map>

struct SourceFile {
  std::string filename;
  std::string content;
};

// Files own the filename storage referenced by SourceLoc for the whole parse.
class SourceManager {
public:
  explicit SourceManager(DiagnosticEngine &diagnostics) : _diag(diagnostics) {}
  const SourceFile &add(const std::string &filename, const std::string &content);
  const SourceFile *load(const std::string &filename, SourceLoc includeLoc);

private:
  DiagnosticEngine &_diag;
  std::unordered_map<std::string, std::unique_ptr<SourceFile>> _files;
  static std::string key(const std::string &filename);
};
