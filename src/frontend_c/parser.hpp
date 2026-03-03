#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace tinyc2ll::frontend_cxx {

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);
  std::string parse_program_summary() const;

 private:
  std::vector<Token> tokens_;
};

}  // namespace tinyc2ll::frontend_cxx

