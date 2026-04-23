#include "core.hpp"

namespace c4c::backend::x86::core {

void Text::append_line(std::string_view line) {
  text_ += line;
  text_ += "\n";
}

void Text::append_raw(std::string_view text) {
  text_ += text;
}

bool references_symbol(std::string_view asm_text, std::string_view symbol_name) {
  return !symbol_name.empty() && asm_text.find(symbol_name) != std::string_view::npos;
}

bool defines_symbol(std::string_view asm_text, std::string_view symbol_name) {
  if (symbol_name.empty()) {
    return false;
  }
  const std::string label = std::string(symbol_name) + ":";
  return asm_text.find(label) != std::string_view::npos;
}

}  // namespace c4c::backend::x86::core
