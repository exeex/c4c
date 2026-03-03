#include "parser.hpp"

#include <sstream>

namespace tinyc2ll::frontend_cxx {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::string Parser::parse_program_summary() const {
  std::ostringstream out;
  out << "Program(tokens=" << tokens_.size() << ")";
  return out.str();
}

}  // namespace tinyc2ll::frontend_cxx

