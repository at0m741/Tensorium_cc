#pragma once
#include "Preprocessor.hpp"

bool evaluatePPExpression(const std::vector<PPToken> &tokens,
                          DiagnosticEngine &diagnostics,
                          const LangOptions &options, SourceLoc loc,
                          bool &result);
