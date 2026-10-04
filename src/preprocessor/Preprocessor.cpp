#include "Preprocessor.hpp"
#include "PPExpression.hpp"
#include "lexer/Lexer.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>

namespace {
bool alpha(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}
bool digit(char ch) { return std::isdigit(static_cast<unsigned char>(ch)); }
bool alnum(char ch) { return alpha(ch) || digit(ch); }

std::string quote(const std::string &text) {
  std::string result = "\"";
  for (char ch : text) {
    if (ch == '\n' || ch == '\r' || ch == '\t') {
      result += ch == '\n' ? "\\n" : ch == '\r' ? "\\r" : "\\t";
      continue;
    }
    if (ch == '\\' || ch == '"')
      result += '\\';
    result += ch;
  }
  return result + '"';
}

std::string stringify(const std::vector<PPToken> &argument) {
  std::string result = "\"";
  for (size_t i = 0; i < argument.size(); ++i) {
    const auto &part = argument[i];
    if (i && part.leadingSpace)
      result += ' ';
    const bool literal = part.kind == PPToken::String ||
                         part.kind == PPToken::Character;
    const auto &spelling = part.originalSpelling.empty()
                               ? part.text : part.originalSpelling;
    for (char ch : spelling) {
      if (literal && (ch == '\\' || ch == '"'))
        result += '\\';
      result += ch;
    }
  }
  return result + '"';
}

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
    if (options.allowLineComments && s[i] == '/' && i + 1 < s.size() &&
        s[i + 1] == '/') {
      while (i < s.size() && s[i] != '\n')
        ++i;
      space = true;
      continue;
    }

    const size_t start = i;
    PPToken::Kind kind = PPToken::Punct;
    // Literal header names have different quoting and comment rules from C
    // strings.
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
    } else if (digit(s[i]) ||
               (s[i] == '.' && i + 1 < s.size() && digit(s[i + 1]))) {
      ++i;
      while (i < s.size()) {
        if (alnum(s[i]) || s[i] == '.') {
          ++i;
        } else if ((s[i] == '+' || s[i] == '-') &&
                   (s[i - 1] == 'e' || s[i - 1] == 'E' || s[i - 1] == 'p' ||
                    s[i - 1] == 'P')) {
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
        diag.error(input.locations[start],
                   quote == '"' ? "unterminated string literal"
                                : "unterminated character literal");
        return {};
      }
      ++i;
      kind = quote == '"' ? PPToken::String : PPToken::Character;
    } else {
      static constexpr const char *punctuators[] = {
          "%:%:", "<<=", ">>=", "...", "##", "++", "--", "->", "&&", "||",
          "<=",   ">=",  "==",  "!=",  "<<", ">>", "+=", "-=", "*=", "/=",
          "%=",   "&=",  "|=",  "^=",  "<%", "%>", "<:", ":>", "%:",
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
    const auto original = spelling;
    if (kind == PPToken::Punct) {
      if (spelling == "%:")
        spelling = "#";
      else if (spelling == "%:%:")
        spelling = "##";
      else if (spelling == "<%")
        spelling = "{";
      else if (spelling == "%>")
        spelling = "}";
      else if (spelling == "<:")
        spelling = "[";
      else if (spelling == ":>")
        spelling = "]";
    }
    PPToken token{kind, std::move(spelling), input.locations[start], space, {}};
    if (token.text != original)
      token.originalSpelling = original;
    if (kind == PPToken::String || kind == PPToken::Character)
      token.spellingLocations.assign(input.locations.begin() + start,
                                     input.locations.begin() + i);
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
                           DiagnosticEngine &diagnostics,
                           const LangOptions &options,
                           std::vector<std::string> includeDirs)
    : _sources(sources), _main(mainFile), _diag(diagnostics), _options(options),
      _includeDirs(std::move(includeDirs)), _end{mainFile.filename.c_str(), 1, 1} {
  const auto now = std::time(nullptr);
  const auto *local = std::localtime(&now);
  _date = "??? ?? ????";
  _time = "??:??:??";
  if (local) {
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s %2d %04d", months[local->tm_mon],
                  local->tm_mday, local->tm_year + 1900);
    _date = buffer;
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", local->tm_hour,
                  local->tm_min, local->tm_sec);
    _time = buffer;
  }
}

bool Preprocessor::isPredefined(const std::string &name) const {
  return name == "__FILE__" || name == "__LINE__" || name == "__DATE__" ||
         name == "__TIME__" || name == "__STDC__" || name == "__STDC_HOSTED__" ||
         (name == "__STDC_VERSION__" && _options.std != LangOptions::C89);
}

bool Preprocessor::isDefined(const std::string &name) const {
  return isPredefined(name) || _macros.count(name);
}

PPToken Preprocessor::predefined(const PPToken &token) const {
  PPToken result = token;
  const auto context = token.expansionLoc.filename ? token.expansionLoc : token.loc;
  result.kind = PPToken::Number;
  result.spellingLocations.clear();
  result.originalSpelling.clear();
  if (token.text == "__FILE__") {
    result.kind = PPToken::String;
    result.text = quote(context.filename ? context.filename : "");
  } else if (token.text == "__LINE__") {
    result.text = std::to_string(context.line);
  } else if (token.text == "__DATE__" || token.text == "__TIME__") {
    result.kind = PPToken::String;
    result.text = quote(token.text == "__DATE__" ? _date : _time);
  } else if (token.text == "__STDC_HOSTED__") {
    result.text = "0";
  } else if (token.text == "__STDC_VERSION__") {
    switch (_options.std) {
    case LangOptions::C11:
      result.text = "201112L";
      break;
    case LangOptions::C17:
      result.text = "201710L";
      break;
    default:
      result.text = "199901L";
      break;
    }
  } else {
    result.text = "1";
  }
  return result;
}

bool Preprocessor::expand(const std::vector<PPToken> &input,
                          std::vector<PPToken> &output, unsigned depth) {
  if (depth >= 128) {
    _diag.error(input.empty() ? _end : input.front().loc,
                "macro expansion limit exceeded");
    return false;
  }
  // Replacement tokens and the remaining input must share the same scan:
  // an object-like alias can produce a function-like macro's name.
  std::deque<PPToken> pending(input.begin(), input.end());
  while (!pending.empty()) {
    PPToken token = std::move(pending.front());
    pending.pop_front();
    if (token.kind != PPToken::Identifier || token.hideSet.count(token.text)) {
      output.push_back(std::move(token));
      continue;
    }
    if (isPredefined(token.text)) {
      if (++_expansions > 100000) {
        _diag.error(token.loc, "macro expansion limit exceeded");
        return false;
      }
      output.push_back(predefined(token));
      continue;
    }
    const auto found = _macros.find(token.text);
    if (found == _macros.end()) {
      output.push_back(std::move(token));
      continue;
    }
    const auto &macro = found->second;
    if (macro.functionLike && (pending.empty() || pending.front().text != "(")) {
      output.push_back(std::move(token));
      continue;
    }
    if (token.hideSet.size() >= 128 || ++_expansions > 100000) {
      _diag.error(token.loc, "macro expansion limit exceeded");
      return false;
    }

    std::vector<std::vector<PPToken>> arguments;
    auto hide = token.hideSet;
    auto context = token.expansionLoc.filename ? token.expansionLoc : token.loc;
    if (macro.functionLike) {
      pending.pop_front(); // opening parenthesis
      arguments.emplace_back();
      std::vector<PPToken> commas;
      size_t nesting = 0;
      bool closed = false;
      while (!pending.empty()) {
        auto part = std::move(pending.front());
        pending.pop_front();
        if (part.text == ")" && nesting == 0) {
          context = part.expansionLoc.filename ? part.expansionLoc : part.loc;
          // A function invocation's hide set is the intersection at its ends.
          for (auto it = hide.begin(); it != hide.end();) {
            if (!part.hideSet.count(*it))
              it = hide.erase(it);
            else
              ++it;
          }
          closed = true;
          break;
        }
        if (part.text == "," && nesting == 0) {
          commas.push_back(part);
          arguments.emplace_back();
          continue;
        }
        if (part.text == "(")
          ++nesting;
        else if (part.text == ")")
          --nesting;
        arguments.back().push_back(std::move(part));
      }
      if (!closed) {
        _diag.error(token.loc, "unterminated invocation of macro '" + token.text + "'");
        return false;
      }
      if (macro.parameters.empty() && arguments.size() == 1 && arguments[0].empty())
        arguments.clear();
      const size_t required = macro.parameters.size();
      if ((!macro.variadic && arguments.size() != required) ||
          (macro.variadic && arguments.size() < required)) {
        _diag.error(token.loc, "wrong number of arguments to macro '" + token.text + "'");
        return false;
      }
      if (macro.variadic) {
        auto &variable = arguments[required - 1];
        for (size_t i = required; i < arguments.size(); ++i) {
          variable.push_back(commas[i - 1]);
          variable.insert(variable.end(), arguments[i].begin(), arguments[i].end());
        }
        arguments.resize(required);
      }
    }
    hide.insert(token.text);

    std::vector<std::vector<PPToken>> expanded(arguments.size());
    std::vector<bool> prepared(arguments.size(), false);
    auto parameter = [&](const PPToken &part) -> size_t {
      if (part.kind != PPToken::Identifier)
        return macro.parameters.size();
      const auto at = std::find(macro.parameters.begin(),
                                macro.parameters.end(), part.text);
      return static_cast<size_t>(at - macro.parameters.begin());
    };
    struct Part {
      PPToken token;
      bool paste = false;
      bool empty = false;
    };
    std::vector<Part> substituted;
    for (size_t i = 0; i < macro.replacement.size(); ++i) {
      auto part = macro.replacement[i];
      part.loc = token.loc;
      part.expansionLoc = context;
      part.spellingLocations.clear();
      if (macro.functionLike && part.text == "#") {
        const auto index = parameter(macro.replacement[++i]);
        part.kind = PPToken::String;
        part.text = stringify(arguments[index]);
        part.originalSpelling.clear();
        substituted.push_back({std::move(part)});
        continue;
      }
      if (part.text == "##") {
        substituted.push_back({std::move(part), true});
        continue;
      }
      const size_t index = parameter(part);
      if (!macro.functionLike || index == macro.parameters.size()) {
        substituted.push_back({std::move(part)});
        continue;
      }
      const bool pasted = (i && macro.replacement[i - 1].text == "##") ||
          (i + 1 < macro.replacement.size() && macro.replacement[i + 1].text == "##");
      if (!pasted && !prepared[index]) {
        // Prescan before hiding the current macro, so F(F(1)) expands both calls.
        if (!expand(arguments[index], expanded[index], depth + 1))
          return false;
        prepared[index] = true;
      }
      const auto &value = pasted ? arguments[index] : expanded[index];
      if (value.empty()) {
        if (pasted)
          substituted.push_back({std::move(part), false, true});
        continue;
      }
      for (size_t j = 0; j < value.size(); ++j) {
        auto argument = value[j];
        if (!j)
          argument.leadingSpace = part.leadingSpace;
        substituted.push_back({std::move(argument)});
      }
    }

    std::vector<Part> pasted;
    for (size_t i = 0; i < substituted.size(); ++i) {
      auto part = std::move(substituted[i]);
      if (!part.paste) {
        pasted.push_back(std::move(part));
        continue;
      }
      auto left = std::move(pasted.back());
      pasted.pop_back();
      auto right = std::move(substituted[++i]);
      if (left.empty || right.empty) {
        if (left.empty) {
          right.token.leadingSpace = left.token.leadingSpace;
          pasted.push_back(std::move(right));
        } else {
          pasted.push_back(std::move(left));
        }
        continue;
      }
      const auto &lhs = left.token.originalSpelling.empty()
                            ? left.token.text : left.token.originalSpelling;
      const auto &rhs = right.token.originalSpelling.empty()
                            ? right.token.text : right.token.originalSpelling;
      const auto spelling = lhs + rhs;
      // Comments were removed before expansion; pasting cannot create one.
      std::vector<PPToken> joined;
      if (spelling != "/*" && spelling != "//") {
        Input fragment{spelling,
                       std::vector<SourceLoc>(spelling.size(), token.loc), token.loc};
        joined = tokenize(fragment, _diag, _options);
      }
      if (_diag.hasErrors())
        return false;
      if (joined.size() != 1 || joined[0].kind == PPToken::Other ||
          joined[0].kind == PPToken::Newline) {
        _diag.error(token.loc,
                    "token paste does not form one preprocessing token: '" +
                        spelling + "'");
        return false;
      }
      auto combined = std::move(joined[0]);
      combined.expansionLoc = context;
      combined.leadingSpace = left.token.leadingSpace;
      combined.spellingLocations.clear();
      for (const auto &name : left.token.hideSet)
        if (right.token.hideSet.count(name))
          combined.hideSet.insert(name);
      pasted.push_back({std::move(combined)});
    }
    std::vector<PPToken> replacement;
    for (auto &part : pasted) {
      if (part.empty)
        continue; // placemarkers disappear before rescan
      part.token.hideSet.insert(hide.begin(), hide.end());
      replacement.push_back(std::move(part.token));
    }
    if (!replacement.empty())
      replacement.front().leadingSpace = token.leadingSpace;
    else if (!pending.empty())
      pending.front().leadingSpace |= token.leadingSpace;
    for (auto it = replacement.rbegin(); it != replacement.rend(); ++it)
      pending.push_front(std::move(*it));
  }
  return true;
}

bool Preprocessor::define(const std::vector<PPToken> &args, SourceLoc loc) {
  if (args.empty() || args[0].kind != PPToken::Identifier ||
      args[0].text == "defined" || args[0].text == "__VA_ARGS__") {
    _diag.error(loc, "expected macro name after #define");
    return false;
  }
  const auto &name = args[0].text;
  if (isPredefined(name)) {
    _diag.error(args[0].loc, "cannot redefine predefined macro '" + name + "'");
    return false;
  }
  Macro macro;
  size_t cursor = 1;
  if (args.size() > 1 && args[1].text == "(" && !args[1].leadingSpace) {
    macro.functionLike = true;
    cursor = 2;
    if (cursor < args.size() && args[cursor].text != ")") {
      while (cursor < args.size()) {
        const auto &part = args[cursor++];
        if (part.text == "...") {
          if (_options.std == LangOptions::C89) {
            _diag.error(part.loc, "variadic macros require C99 or later");
            return false;
          }
          macro.variadic = true;
          macro.parameters.push_back("__VA_ARGS__");
          break;
        }
        if (part.kind != PPToken::Identifier || part.text == "__VA_ARGS__") {
          _diag.error(part.loc, "expected macro parameter name");
          return false;
        }
        if (std::find(macro.parameters.begin(), macro.parameters.end(), part.text) !=
            macro.parameters.end()) {
          _diag.error(part.loc, "duplicate macro parameter '" + part.text + "'");
          return false;
        }
        macro.parameters.push_back(part.text);
        if (cursor == args.size() || args[cursor].text != ",")
          break;
        ++cursor;
        if (cursor < args.size() && args[cursor].text == ")") {
          _diag.error(args[cursor].loc, "expected macro parameter name");
          return false;
        }
      }
    }
    if (cursor == args.size() || args[cursor].text != ")") {
      _diag.error(args[0].loc, "expected ')' after macro parameters");
      return false;
    }
    ++cursor;
  }
  macro.replacement.assign(args.begin() + cursor, args.end());
  for (size_t i = 0; i < macro.replacement.size(); ++i) {
    const auto &part = macro.replacement[i];
    if (part.text == "##" && (!i || i + 1 == macro.replacement.size() ||
                             macro.replacement[i - 1].text == "##")) {
      _diag.error(part.loc, "'##' requires a token on each side");
      return false;
    }
    if (macro.functionLike && part.text == "#" &&
        (i + 1 == macro.replacement.size() ||
         std::find(macro.parameters.begin(), macro.parameters.end(),
                   macro.replacement[i + 1].text) == macro.parameters.end())) {
      _diag.error(part.loc, "'#' must be followed by a macro parameter");
      return false;
    }
    if (part.text == "__VA_ARGS__" && !macro.variadic) {
      _diag.error(part.loc, "__VA_ARGS__ is only valid in a variadic macro");
      return false;
    }
  }
  const auto old = _macros.find(name);
  if (old != _macros.end()) {
    const auto &previous = old->second;
    bool same = previous.functionLike == macro.functionLike &&
                previous.variadic == macro.variadic &&
                previous.parameters == macro.parameters &&
                previous.replacement.size() == macro.replacement.size();
    for (size_t i = 0; same && i < macro.replacement.size(); ++i)
      same = previous.replacement[i].text == macro.replacement[i].text &&
             (!i || previous.replacement[i].leadingSpace ==
                        macro.replacement[i].leadingSpace);
    if (!same) {
      _diag.error(args[0].loc, "incompatible redefinition of macro '" + name + "'");
      return false;
    }
  }
  _macros[name] = std::move(macro);
  return true;
}

bool Preprocessor::condition(const std::vector<PPToken> &input, SourceLoc loc,
                             bool &result) {
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
    const bool exists = isDefined(input[i].text);
    if (parenthesized && (++i == input.size() || input[i].text != ")")) {
      _diag.error(at, "expected ')' after defined");
      return false;
    }
    prepared.push_back({PPToken::Number, exists ? "1" : "0", at, false, {}});
  }
  std::vector<PPToken> expanded;
  return expand(prepared, expanded) &&
         evaluatePPExpression(expanded, _diag, _options, loc, result);
}

bool Preprocessor::include(const SourceFile &file,
                           std::vector<PPToken> arguments, SourceLoc loc,
                           unsigned depth) {
  if (!arguments.empty() && arguments.front().kind != PPToken::Header &&
      arguments.front().kind != PPToken::String &&
      arguments.front().text != "<") {
    std::vector<PPToken> expanded;
    if (!expand(arguments, expanded))
      return false;
    arguments = std::move(expanded);
  }
  std::string header;
  bool quoted = false;
  if (arguments.size() == 1 && (arguments[0].kind == PPToken::Header ||
                                arguments[0].kind == PPToken::String)) {
    quoted = arguments[0].text.front() == '"';
    header = arguments[0].text.substr(1, arguments[0].text.size() - 2);
  } else if (arguments.size() >= 3 && arguments.front().text == "<" &&
             arguments.back().text == ">") {
    for (size_t i = 1; i + 1 < arguments.size(); ++i) {
      if (i > 1 && arguments[i].leadingSpace)
        header += ' ';
      header += arguments[i].text;
    }
  } else {
    _diag.error(loc,
                "expected a quoted or angle-bracket filename after #include");
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

bool Preprocessor::process(const SourceFile &file, unsigned depth,
                           SourceLoc includeLoc) {
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
  std::vector<PPToken> pending;
  auto flush = [&]() {
    const bool success = expand(pending, _output);
    pending.clear();
    return success;
  };
  for (size_t cursor = 0; cursor < tokens.size();) {
    const size_t begin = cursor;
    while (cursor < tokens.size() && tokens[cursor].kind != PPToken::Newline)
      ++cursor;
    std::vector<PPToken> line(tokens.begin() + begin, tokens.begin() + cursor);
    if (cursor < tokens.size())
      ++cursor;
    if (line.empty())
      continue;
    const bool active = stack.empty() || stack.back().active;
    if (line[0].text != "#") {
      if (active) {
        // Ordinary newlines are whitespace inside a macro invocation.
        line.front().leadingSpace = true;
        pending.insert(pending.end(), line.begin(), line.end());
      }
      continue;
    }
    // Expand before a directive changes macro definitions or include order.
    if (!flush())
      return false;
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
        selected = isDefined(args[0].text);
        if (directive == "ifndef")
          selected = !selected;
      } else if (active && !condition(args, loc, selected)) {
        return false;
      }
      stack.push_back(
          {active, active && selected, active && selected, false, loc});
    } else if (directive == "elif" || directive == "else" ||
               directive == "endif") {
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
        if (frame.parentActive && !frame.taken &&
            !condition(args, loc, selected))
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
      if (!define(args, loc))
        return false;
    } else if (directive == "undef") {
      if (args.size() != 1 || args[0].kind != PPToken::Identifier) {
        _diag.error(loc, "expected one macro name after #undef");
        return false;
      }
      if (isPredefined(args[0].text)) {
        _diag.error(args[0].loc, "cannot undefine predefined macro '" + args[0].text + "'");
        return false;
      }
      _macros.erase(args[0].text);
    } else if (directive == "include") {
      if (!include(file, std::move(args), loc, depth))
        return false;
    } else if (directive == "pragma" && args.size() == 1 &&
               args[0].text == "once") {
      _once.insert(&file);
    } else if (directive == "error") {
      std::string message;
      for (const auto &arg : args) {
        if (!message.empty())
          message += ' ';
        message += arg.text;
      }
      _diag.error(loc, message.empty() ? "#error" : "#error " + message);
      return false;
    } else {
      _diag.error(loc,
                  "unsupported preprocessing directive '#" + directive + "'");
      return false;
    }
  }
  if (!flush())
    return false;
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
  auto convert = [&](const PPToken &raw) {
    Lexer lexer(raw.text, raw.loc.filename, _diag, _options, raw.loc,
                raw.spellingLocations);
    Token token = lexer.next();
    if (!token.isError() && !lexer.next().isEof()) {
      _diag.error(raw.loc, "invalid preprocessing token '" + raw.text + "'",
                  raw.text.size());
      return Token(TokenKind::ERROR, "", raw.loc);
    }
    return token;
  };
  Token token = convert(_output[_cursor++]);
  // Phase 6 follows expansion: decode each literal before joining its bytes.
  while (token.kind == TokenKind::STRING_LIT && _cursor < _output.size() &&
         _output[_cursor].kind == PPToken::String) {
    const auto part = convert(_output[_cursor++]);
    if (part.isError())
      return part;
    token.text += part.text;
    token.str_val += part.str_val;
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
      result += previous.filename != token.loc.filename ||
                        previous.line != token.loc.line
                    ? '\n'
                    : ' ';
    result += token.text;
    previous = token.loc;
  }
  if (!result.empty())
    result += '\n';
  return result;
}
