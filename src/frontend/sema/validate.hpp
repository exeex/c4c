#pragma once

#include <string>
#include <vector>

#include "ast.hpp"

namespace c4c::sema {

struct Diagnostic {
  const char* file = nullptr;
  int line = 0;
  int column = 1;
  std::string message;
};

struct ValidateResult {
  bool ok = true;
  std::vector<Diagnostic> diagnostics;
};

ValidateResult validate_program(const Node* root);
void print_diagnostics(const std::vector<Diagnostic>& diags, const std::string& file);

}  // namespace c4c::sema
