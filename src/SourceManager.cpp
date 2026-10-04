#include "cc1/SourceManager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

std::string SourceManager::key(const std::string &filename) {
  std::error_code error;
  auto path = std::filesystem::weakly_canonical(filename, error);
  return error ? std::filesystem::path(filename).lexically_normal().string()
               : path.string();
}

const SourceFile &SourceManager::add(const std::string &filename,
                                     const std::string &content) {
  const auto identity = key(filename);
  const auto found = _files.find(identity);
  if (found != _files.end())
    return *found->second;
  auto file = std::make_unique<SourceFile>(SourceFile{filename, content});
  _diag.addSource(filename, content);
  const auto *result = file.get();
  _files.emplace(identity, std::move(file));
  return *result;
}

const SourceFile *SourceManager::load(const std::string &filename,
                                      SourceLoc includeLoc) {
  const auto found = _files.find(key(filename));
  if (found != _files.end())
    return found->second.get();
  std::ifstream input(filename, std::ios::binary);
  if (!input) {
    _diag.error(includeLoc, "cannot open include file '" + filename + "'");
    return nullptr;
  }
  std::ostringstream contents;
  contents << input.rdbuf();
  if (input.bad()) {
    _diag.error(includeLoc, "cannot read include file '" + filename + "'");
    return nullptr;
  }
  return &add(filename, contents.str());
}
