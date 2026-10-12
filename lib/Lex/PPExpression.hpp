#pragma once
#include "cc1/Lex/Preprocessor.hpp"

bool evaluatePPExpression(const std::vector<PPToken> &tokens,
                          DiagnosticEngine &diagnostics,
                          const LangOptions &options, SourceLoc loc,
                          bool &result);
