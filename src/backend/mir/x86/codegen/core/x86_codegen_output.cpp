#include "x86_codegen_output.hpp"

#include <cctype>

namespace {

bool asm_symbol_char(char ch) {
  const auto uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == '_' || ch == '.' || ch == '$';
}

}  // namespace

namespace c4c::backend::x86::core {

void AssemblyTextBuffer::append_line(std::string_view line) {
  text_.append(line);
  text_.push_back('\n');
}

void AssemblyTextBuffer::append_raw(std::string_view text) {
  text_.append(text);
}

bool asm_text_references_symbol(std::string_view asm_text, std::string_view symbol_name) {
  if (symbol_name.empty()) {
    return false;
  }
  std::size_t search_from = 0;
  while (true) {
    const auto pos = asm_text.find(symbol_name, search_from);
    if (pos == std::string_view::npos) {
      return false;
    }
    const auto end = pos + symbol_name.size();
    const bool prev_is_symbol = pos > 0 && asm_symbol_char(asm_text[pos - 1]);
    const bool next_is_symbol = end < asm_text.size() && asm_symbol_char(asm_text[end]);
    if (!prev_is_symbol && !next_is_symbol) {
      return true;
    }
    search_from = pos + 1;
  }
}

bool asm_text_defines_symbol(std::string_view asm_text, std::string_view symbol_name) {
  if (symbol_name.empty()) {
    return false;
  }
  std::size_t search_from = 0;
  while (true) {
    const auto pos = asm_text.find(symbol_name, search_from);
    if (pos == std::string_view::npos) {
      return false;
    }
    const auto end = pos + symbol_name.size();
    const bool prev_is_symbol = pos > 0 && asm_symbol_char(asm_text[pos - 1]);
    const bool next_is_label = end < asm_text.size() && asm_text[end] == ':';
    if (!prev_is_symbol && next_is_label) {
      return true;
    }
    search_from = pos + 1;
  }
}

}  // namespace c4c::backend::x86::core
