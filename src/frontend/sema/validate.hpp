#pragma once

#include <string>
#include <vector>

#include "ast.hpp"

namespace c4c::sema {

struct Diagnostic {
  int line = 0;
  std::string message;
};

struct ValidateResult {
  bool ok = true;
  std::vector<Diagnostic> diagnostics;
};

ValidateResult validate_program(const Node* root);
void print_diagnostics(const std::vector<Diagnostic>& diags, const std::string& file);

}  // namespace c4c::sema
