#include "PPExpression.hpp"
#include "lexer/Lexer.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {
struct Value {
  uint64_t bits = 0;
  bool unsignedValue = false;
  int64_t signedValue() const { return static_cast<int64_t>(bits); }
};

class Expression {
public:
  Expression(const std::vector<PPToken> &tokens, DiagnosticEngine &diag,
             const LangOptions &options, SourceLoc loc)
      : _tokens(tokens), _diag(diag), _options(options), _loc(loc) {}

  bool evaluate(bool &result) {
    const auto value = conditional(true);
    if (_pos != _tokens.size())
      error("unexpected token in #if expression");
    result = value.bits != 0;
    return !_diag.hasErrors();
  }

private:
  const std::vector<PPToken> &_tokens;
  DiagnosticEngine &_diag;
  const LangOptions &_options;
  SourceLoc _loc;
  size_t _pos = 0;
  unsigned _depth = 0;

  struct Depth {
    unsigned &value;
    explicit Depth(unsigned &depth) : value(depth) { ++value; }
    ~Depth() { --value; }
  };

  void error(const std::string &message, SourceLoc loc = {}) {
    if (!_diag.hasErrors()) {
      if (!loc.filename)
        loc = _pos < _tokens.size() ? _tokens[_pos].loc : _loc;
      _diag.error(loc, message);
    }
  }
  bool match(const char *text) {
    if (_pos == _tokens.size() || _tokens[_pos].text != text)
      return false;
    ++_pos;
    return true;
  }
  static int precedence(const std::string &op) {
    if (op == "||")
      return 1;
    if (op == "&&")
      return 2;
    if (op == "|")
      return 3;
    if (op == "^")
      return 4;
    if (op == "&")
      return 5;
    if (op == "==" || op == "!=")
      return 6;
    if (op == "<" || op == "<=" || op == ">" || op == ">=")
      return 7;
    if (op == "<<" || op == ">>")
      return 8;
    if (op == "+" || op == "-")
      return 9;
    if (op == "*" || op == "/" || op == "%")
      return 10;
    return 0;
  }

  Value number(const PPToken &token) {
    try {
      size_t end = 0;
      const uint64_t value = std::stoull(token.text, &end, 0);
      std::string suffix = token.text.substr(end);
      for (auto &ch : suffix)
        if (ch >= 'A' && ch <= 'Z')
          ch += 'a' - 'A';
      if (suffix != "" && suffix != "u" && suffix != "l" && suffix != "ll" &&
          suffix != "ul" && suffix != "lu" && suffix != "ull" &&
          suffix != "llu") {
        error("expected integer constant in #if expression");
        return {};
      }
      bool isUnsigned = suffix.find('u') != std::string::npos;
      if (!isUnsigned &&
          value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        if (token.text[0] != '0') {
          error("integer constant is out of range in #if expression");
          return {};
        }
        isUnsigned = true;
      }
      return {value, isUnsigned};
    } catch (const std::invalid_argument &) {
      error("expected integer constant in #if expression");
    } catch (const std::out_of_range &) {
      error("integer constant is out of range in #if expression");
    }
    return {};
  }

  Value unary(bool evaluate) {
    Depth guard(_depth);
    if (_depth > 256) {
      error("#if expression nesting limit exceeded");
      return {};
    }
    if (_pos == _tokens.size()) {
      error("expected integer expression after #if");
      return {};
    }
    const auto &token = _tokens[_pos];
    if (token.text == "+" || token.text == "-" || token.text == "!" ||
        token.text == "~") {
      const auto op = token.text;
      ++_pos;
      auto value = unary(evaluate);
      if (op == "!")
        return {evaluate && value.bits == 0 ? uint64_t(1) : uint64_t(0), false};
      if (!evaluate)
        return {0, value.unsignedValue};
      if (op == "-")
        value.bits = uint64_t(0) - value.bits;
      if (op == "~")
        value.bits = ~value.bits;
      return value;
    }
    if (match("(")) {
      auto value = conditional(evaluate);
      if (!match(")"))
        error("expected ')' in #if expression");
      return value;
    }
    if (token.kind == PPToken::Number) {
      const auto value = number(token);
      ++_pos;
      return value;
    }
    if (token.kind == PPToken::Identifier) {
      ++_pos;
      return {};
    }
    if (token.kind == PPToken::Character) {
      Lexer lexer(token.text, token.loc.filename, _diag, _options, token.loc,
                  token.spellingLocations);
      const auto character = lexer.next();
      ++_pos;
      return {static_cast<unsigned char>(character.char_val), false};
    }
    error("expected integer expression after #if");
    ++_pos;
    return {};
  }

  Value apply(const std::string &op, Value left, Value right, bool evaluate,
              SourceLoc loc) {
    const bool logical = op == "&&" || op == "||" || op == "==" || op == "!=" ||
                         op == "<" || op == "<=" || op == ">" || op == ">=";
    const bool shift = op == "<<" || op == ">>";
    const bool isUnsigned =
        shift ? left.unsignedValue : left.unsignedValue || right.unsignedValue;
    if (!evaluate)
      return {0, logical ? false : isUnsigned};
    if (op == "&&")
      return {left.bits != 0 && right.bits != 0 ? uint64_t(1) : uint64_t(0),
              false};
    if (op == "||")
      return {left.bits != 0 || right.bits != 0 ? uint64_t(1) : uint64_t(0),
              false};
    if (op == "==")
      return {left.bits == right.bits ? uint64_t(1) : uint64_t(0), false};
    if (op == "!=")
      return {left.bits != right.bits ? uint64_t(1) : uint64_t(0), false};
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
      const bool less = isUnsigned ? left.bits < right.bits
                                   : left.signedValue() < right.signedValue();
      const bool equal = left.bits == right.bits;
      const bool result = op == "<"    ? less
                          : op == "<=" ? less || equal
                          : op == ">"  ? !less && !equal
                                       : !less;
      return {result ? uint64_t(1) : uint64_t(0), false};
    }
    if (op == "+")
      return {left.bits + right.bits, isUnsigned};
    if (op == "-")
      return {left.bits - right.bits, isUnsigned};
    if (op == "*")
      return {left.bits * right.bits, isUnsigned};
    if (op == "&")
      return {left.bits & right.bits, isUnsigned};
    if (op == "|")
      return {left.bits | right.bits, isUnsigned};
    if (op == "^")
      return {left.bits ^ right.bits, isUnsigned};
    if (shift) {
      if (right.bits >= 64) {
        error("invalid shift count in #if expression", loc);
        return {};
      }
      if (op == "<<")
        return {left.bits << right.bits, isUnsigned};
      return {isUnsigned
                  ? left.bits >> right.bits
                  : static_cast<uint64_t>(left.signedValue() >> right.bits),
              isUnsigned};
    }
    if (right.bits == 0) {
      error("division by zero in #if expression", loc);
      return {};
    }
    if (isUnsigned)
      return {op == "/" ? left.bits / right.bits : left.bits % right.bits,
              true};
    if (left.signedValue() == std::numeric_limits<int64_t>::min() &&
        right.signedValue() == -1) {
      error("integer overflow in #if expression", loc);
      return {};
    }
    return {static_cast<uint64_t>(
                op == "/" ? left.signedValue() / right.signedValue()
                          : left.signedValue() % right.signedValue()),
            false};
  }

  Value binary(int minimum, bool evaluate) {
    auto left = unary(evaluate);
    while (!_diag.hasErrors() && _pos < _tokens.size()) {
      const auto op = _tokens[_pos].text;
      const int prec = precedence(op);
      if (prec < minimum)
        break;
      const auto loc = _tokens[_pos].loc;
      ++_pos;
      const bool rightActive = evaluate && !(op == "&&" && left.bits == 0) &&
                               !(op == "||" && left.bits != 0);
      const auto right = binary(prec + 1, rightActive);
      left = apply(op, left, right, evaluate, loc);
    }
    return left;
  }

  Value conditional(bool evaluate) {
    Depth guard(_depth);
    if (_depth > 256) {
      error("#if expression nesting limit exceeded");
      return {};
    }
    auto value = binary(1, evaluate);
    if (!_diag.hasErrors() && match("?")) {
      const auto then = conditional(evaluate && value.bits != 0);
      if (!match(":"))
        error("expected ':' in #if expression");
      const auto otherwise = conditional(evaluate && value.bits == 0);
      return {value.bits != 0 ? then.bits : otherwise.bits,
              then.unsignedValue || otherwise.unsignedValue};
    }
    return value;
  }
};
} // namespace

bool evaluatePPExpression(const std::vector<PPToken> &tokens,
                          DiagnosticEngine &diagnostics,
                          const LangOptions &options, SourceLoc loc,
                          bool &result) {
  return Expression(tokens, diagnostics, options, loc).evaluate(result);
}
