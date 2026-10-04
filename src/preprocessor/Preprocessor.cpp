#include "Preprocessor.hpp"
#include "PPExpression.hpp"
#include "lexer/Lexer.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
bool alpha(char ch) { return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_'; }
bool digit(char ch) { return std::isdigit(static_cast<unsigned char>(ch)); }
bool alnum(char ch) { return alpha(ch) || digit(ch); }

struct Input {
  std::string text;
  std::vector<SourceLoc> locations;
  SourceLoc end;
};

Input normalize(const SourceFile &file) {
  Input result;
  SourceLoc loc{file.filename.c_str(), 1, 1};
  const auto &source = file.content;
  for (size_t i = 0; i < source.size();) {
    if (source[i] == '\\' && i + 1 < source.size() &&
        (source[i + 1] == '\n' || source[i + 1] == '\r')) {
      i += 2;
      if (source[i - 1] == '\r' && i < source.size() && source[i] == '\n')
        ++i;
      ++loc.line;
      loc.col = 1;
      continue;
    }
    result.locations.push_back(loc);
    if (source[i] == '\n' || source[i] == '\r') {
      if (source[i] == '\r' && i + 1 < source.size() && source[i + 1] == '\n')
        ++i;
      result.text += '\n';
      ++loc.line;
      loc.col = 1;
    } else {
      result.text += source[i];
      ++loc.col;
    }
    ++i;
  }
  result.end = loc;
  return result;
}

std::vector<PPToken> tokenize(const Input &input, DiagnosticEngine &diag,
                               const LangOptions &options) {
  std::vector<PPToken> tokens;
  const auto &s = input.text;
  bool space = false;
  size_t lineStart = 0;
  for (size_t i = 0; i < s.size();) {
    if (s[i] == '\n') {
      tokens.push_back({PPToken::Newline, "\n", input.locations[i], false, {}});
      lineStart = tokens.size();
      space = false;
      ++i;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(s[i]))) {
      space = true;
      ++i;
      continue;
    }
    if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '*') {
      const auto start = input.locations[i];
      i += 2;
      while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/'))
        ++i;
      if (i + 1 >= s.size()) {
        diag.error(start, "unterminated block comment", 2);
        return {};
      }
      i += 2;
      space = true;
      continue;
    }
    if (options.allowLineComments && s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/') {
      while (i < s.size() && s[i] != '\n')
        ++i;
      space = true;
      continue;
    }

    const size_t start = i;
    PPToken::Kind kind = PPToken::Punct;
    // Literal header names have different quoting and comment rules from C strings.
    const bool header = tokens.size() == lineStart + 2 &&
                        tokens[lineStart].text == "#" &&
                        tokens[lineStart + 1].text == "include";
    if (header && (s[i] == '<' || s[i] == '"')) {
      const char close = s[i++] == '<' ? '>' : '"';
      while (i < s.size() && s[i] != close && s[i] != '\n')
        ++i;
      if (i == s.size() || s[i] != close) {
        diag.error(input.locations[start], "unterminated include filename");
        return {};
      }
      ++i;
      kind = PPToken::Header;
    } else if (alpha(s[i])) {
      while (i < s.size() && alnum(s[i]))
        ++i;
      kind = PPToken::Identifier;
    } else if (digit(s[i]) || (s[i] == '.' && i + 1 < s.size() && digit(s[i + 1]))) {
      ++i;
      while (i < s.size()) {
        if (alnum(s[i]) || s[i] == '.') {
          ++i;
        } else if ((s[i] == '+' || s[i] == '-') &&
                   (s[i - 1] == 'e' || s[i - 1] == 'E' || s[i - 1] == 'p' || s[i - 1] == 'P')) {
          ++i;
        } else {
          break;
        }
      }
      kind = PPToken::Number;
    } else if (s[i] == '"' || s[i] == '\'') {
      const char quote = s[i++];
      while (i < s.size() && s[i] != quote && s[i] != '\n') {
        if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] != '\n')
          ++i;
        ++i;
      }
      if (i == s.size() || s[i] != quote) {
        diag.error(input.locations[start], quote == '"' ? "unterminated string literal"
                                                      : "unterminated character literal");
        return {};
      }
      ++i;
      kind = quote == '"' ? PPToken::String : PPToken::Character;
    } else {
      static constexpr const char *punctuators[] = {
          "%:%:", "<<=", ">>=", "...", "##", "++", "--", "->", "&&", "||",
          "<=", ">=", "==", "!=", "<<", ">>", "+=", "-=", "*=", "/=", "%=",
          "&=", "|=", "^=", "<%", "%>", "<:", ":>", "%:",
      };
      bool found = false;
      for (const char *punct : punctuators) {
        const std::string spelling = punct;
        if (s.compare(i, spelling.size(), spelling) == 0) {
          i += spelling.size();
          found = true;
          break;
        }
      }
      if (!found) {
        const std::string single = "[](){}.&*+-~!/%<>^|?:;=,#";
        if (single.find(s[i]) == std::string::npos)
          kind = PPToken::Other;
        ++i;
      }
    }
    std::string spelling = s.substr(start, i - start);
    if (kind == PPToken::Punct) {
      if (spelling == "%:") spelling = "#";
      else if (spelling == "%:%:") spelling = "##";
      else if (spelling == "<%") spelling = "{";
      else if (spelling == "%>") spelling = "}";
      else if (spelling == "<:") spelling = "[";
      else if (spelling == ":>") spelling = "]";
    }
    PPToken token{kind, std::move(spelling), input.locations[start], space, {}};
    if (kind == PPToken::String || kind == PPToken::Character)
      token.spellingLocations.assign(input.locations.begin() + start, input.locations.begin() + i);
    tokens.push_back(std::move(token));
    space = false;
  }
  return tokens;
}

struct Conditional {
  bool parentActive;
  bool active;
  bool taken;
  bool seenElse;
  SourceLoc loc;
};
} // namespace

Preprocessor::Preprocessor(SourceManager &sources, const SourceFile &mainFile,
                           DiagnosticEngine &diagnostics, const LangOptions &options,
                           std::vector<std::string> includeDirs)
    : _sources(sources), _main(mainFile), _diag(diagnostics), _options(options),
      _includeDirs(std::move(includeDirs)), _end{mainFile.filename.c_str(), 1, 1} {}

bool Preprocessor::expand(const std::vector<PPToken> &input,
                          std::vector<PPToken> &output,
                          std::unordered_set<std::string> &disabled, unsigned depth) {
  for (const auto &token : input) {
    const auto macro = _macros.find(token.text);
    if (token.kind != PPToken::Identifier || macro == _macros.end() || disabled.count(token.text)) {
      output.push_back(token);
      continue;
    }
    if (depth >= 128 || ++_expansions > 100000) {
      _diag.error(token.loc, "macro expansion limit exceeded");
      return false;
    }
    auto replacement = macro->second;
    for (auto &part : replacement) {
      part.loc = token.loc;
      part.spellingLocations.clear();
    }
    disabled.insert(token.text);
    const bool success = expand(replacement, output, disabled, depth + 1);
    disabled.erase(token.text);
    if (!success)
      return false;
  }
  return true;
}

bool Preprocessor::condition(const std::vector<PPToken> &input,
                             SourceLoc loc, bool &result) {
  std::vector<PPToken> prepared;
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i].kind != PPToken::Identifier || input[i].text != "defined") {
      prepared.push_back(input[i]);
      continue;
    }
    const auto at = input[i].loc;
    const bool parenthesized = i + 1 < input.size() && input[i + 1].text == "(";
    if (parenthesized)
      ++i;
    if (++i == input.size() || input[i].kind != PPToken::Identifier) {
      _diag.error(at, "expected macro name after defined");
      return false;
    }
    const bool exists = _macros.count(input[i].text) != 0;
    if (parenthesized && (++i == input.size() || input[i].text != ")")) {
      _diag.error(at, "expected ')' after defined");
      return false;
    }
    prepared.push_back({PPToken::Number, exists ? "1" : "0", at, false, {}});
  }
  std::vector<PPToken> expanded;
  std::unordered_set<std::string> disabled;
  return expand(prepared, expanded, disabled) &&
         evaluatePPExpression(expanded, _diag, _options, loc, result);
}

bool Preprocessor::include(const SourceFile &file, std::vector<PPToken> arguments,
                           SourceLoc loc, unsigned depth) {
  if (!arguments.empty() && arguments.front().kind != PPToken::Header &&
      arguments.front().kind != PPToken::String && arguments.front().text != "<") {
    std::vector<PPToken> expanded;
    std::unordered_set<std::string> disabled;
    if (!expand(arguments, expanded, disabled))
      return false;
    arguments = std::move(expanded);
  }
  std::string header;
  bool quoted = false;
  if (arguments.size() == 1 &&
      (arguments[0].kind == PPToken::Header || arguments[0].kind == PPToken::String)) {
    quoted = arguments[0].text.front() == '"';
    header = arguments[0].text.substr(1, arguments[0].text.size() - 2);
  } else if (arguments.size() >= 3 && arguments.front().text == "<" && arguments.back().text == ">") {
    for (size_t i = 1; i + 1 < arguments.size(); ++i) {
      if (i > 1 && arguments[i].leadingSpace)
        header += ' ';
      header += arguments[i].text;
    }
  } else {
    _diag.error(loc, "expected a quoted or angle-bracket filename after #include");
    return false;
  }
  if (header.empty()) {
    _diag.error(loc, "empty include filename");
    return false;
  }
  std::vector<std::filesystem::path> directories;
  if (quoted)
    directories.push_back(std::filesystem::path(file.filename).parent_path());
  for (const auto &directory : _includeDirs)
    directories.emplace_back(directory);
  for (const auto &directory : directories) {
    const auto path = (directory / header).lexically_normal();
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error))
      continue;
    const auto *included = _sources.load(path.string(), loc);
    return included && process(*included, depth + 1, loc);
  }
  _diag.error(loc, "include file '" + header + "' not found");
  return false;
}

bool Preprocessor::process(const SourceFile &file, unsigned depth, SourceLoc includeLoc) {
  if (_once.count(&file) && _visited.count(&file))
    return true;
  if (depth >= 64) {
    _diag.error(includeLoc, "include nesting limit exceeded");
    return false;
  }
  _visited.insert(&file);
  const auto input = normalize(file);
  if (&file == &_main)
    _end = input.end;
  const auto tokens = tokenize(input, _diag, _options);
  if (_diag.hasErrors())
    return false;
  std::vector<Conditional> stack;
  for (size_t cursor = 0; cursor < tokens.size();) {
    const size_t begin = cursor;
    while (cursor < tokens.size() && tokens[cursor].kind != PPToken::Newline)
      ++cursor;
    const std::vector<PPToken> line(tokens.begin() + begin, tokens.begin() + cursor);
    if (cursor < tokens.size())
      ++cursor;
    if (line.empty())
      continue;
    const bool active = stack.empty() || stack.back().active;
    if (line[0].text != "#") {
      if (active) {
        std::unordered_set<std::string> disabled;
        if (!expand(line, _output, disabled))
          return false;
      }
      continue;
    }
    if (line.size() == 1)
      continue;
    const auto loc = line[0].loc;
    const auto &directive = line[1].text;
    std::vector<PPToken> args(line.begin() + 2, line.end());
    if (directive == "if" || directive == "ifdef" || directive == "ifndef") {
      bool selected = false;
      if (directive != "if") {
        if (args.size() != 1 || args[0].kind != PPToken::Identifier) {
          _diag.error(loc, "expected one macro name after #" + directive);
          return false;
        }
        selected = _macros.count(args[0].text) != 0;
        if (directive == "ifndef")
          selected = !selected;
      } else if (active && !condition(args, loc, selected)) {
        return false;
      }
      stack.push_back({active, active && selected, active && selected, false, loc});
    } else if (directive == "elif" || directive == "else" || directive == "endif") {
      if (stack.empty()) {
        _diag.error(loc, "#" + directive + " without matching #if");
        return false;
      }
      auto &frame = stack.back();
      if (directive == "elif") {
        if (frame.seenElse) {
          _diag.error(loc, "#elif after #else");
          return false;
        }
        bool selected = false;
        if (frame.parentActive && !frame.taken && !condition(args, loc, selected))
          return false;
        frame.active = frame.parentActive && !frame.taken && selected;
        frame.taken = frame.taken || frame.active;
      } else {
        if (!args.empty()) {
          _diag.error(loc, "unexpected tokens after #" + directive);
          return false;
        }
        if (directive == "endif") {
          stack.pop_back();
        } else {
          if (frame.seenElse) {
            _diag.error(loc, "duplicate #else");
            return false;
          }
          frame.seenElse = true;
          frame.active = frame.parentActive && !frame.taken;
          frame.taken = true;
        }
      }
    } else if (!active) {
      continue;
    } else if (directive == "define") {
      if (args.empty() || args[0].kind != PPToken::Identifier || args[0].text == "defined") {
        _diag.error(loc, "expected macro name after #define");
        return false;
      }
      const auto name = args[0].text;
      if (args.size() > 1 && args[1].text == "(" && !args[1].leadingSpace) {
        _diag.error(loc, "function-like macros are not supported yet");
        return false;
      }
      std::vector<PPToken> replacement(args.begin() + 1, args.end());
      for (const auto &token : replacement) {
        if (token.text == "#" || token.text == "##") {
          _diag.error(token.loc, "macro '#' and '##' operators are not supported yet");
          return false;
        }
      }
      const auto old = _macros.find(name);
      if (old != _macros.end()) {
        bool same = old->second.size() == replacement.size();
        for (size_t i = 0; same && i < replacement.size(); ++i)
          same = old->second[i].text == replacement[i].text &&
                 (!i || old->second[i].leadingSpace == replacement[i].leadingSpace);
        if (!same) {
          _diag.error(args[0].loc, "incompatible redefinition of macro '" + name + "'");
          return false;
        }
      }
      _macros[name] = std::move(replacement);
    } else if (directive == "undef") {
      if (args.size() != 1 || args[0].kind != PPToken::Identifier) {
        _diag.error(loc, "expected one macro name after #undef");
        return false;
      }
      _macros.erase(args[0].text);
    } else if (directive == "include") {
      if (!include(file, std::move(args), loc, depth))
        return false;
    } else if (directive == "pragma" && args.size() == 1 && args[0].text == "once") {
      _once.insert(&file);
    } else if (directive == "error") {
      std::string message;
      for (const auto &arg : args) {
        if (!message.empty()) message += ' ';
        message += arg.text;
      }
      _diag.error(loc, message.empty() ? "#error" : "#error " + message);
      return false;
    } else {
      _diag.error(loc, "unsupported preprocessing directive '#" + directive + "'");
      return false;
    }
  }
  if (!stack.empty()) {
    _diag.error(stack.back().loc, "unterminated conditional directive");
    return false;
  }
  return true;
}

bool Preprocessor::run() {
  if (!_ran) {
    _ran = true;
    if (!process(_main, 0, _end))
      _output.clear();
  }
  return !_diag.hasErrors();
}

Token Preprocessor::next() {
  if (!run())
    return Token(TokenKind::END_OF_FILE, "", _end);
  if (_cursor == _output.size())
    return Token(TokenKind::END_OF_FILE, "", _end);
  const auto &raw = _output[_cursor++];
  Lexer lexer(raw.text, raw.loc.filename, _diag, _options, raw.loc, raw.spellingLocations);
  Token token = lexer.next();
  if (token.isError())
    return token;
  if (!lexer.next().isEof()) {
    _diag.error(raw.loc, "invalid preprocessing token '" + raw.text + "'", raw.text.size());
    return Token(TokenKind::ERROR, "", raw.loc);
  }
  return token;
}

std::string Preprocessor::text() {
  if (!run())
    return {};
  std::string result;
  SourceLoc previous{};
  for (const auto &token : _output) {
    if (!result.empty())
      result += previous.filename != token.loc.filename || previous.line != token.loc.line ? '\n' : ' ';
    result += token.text;
    previous = token.loc;
  }
  if (!result.empty())
    result += '\n';
  return result;
}
