#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::x86::core {

class AssemblyTextBuffer {
 public:
  void append_line(std::string_view line);
  void append_raw(std::string_view text);

  [[nodiscard]] const std::string& text() const { return text_; }
  [[nodiscard]] std::string take_text() { return std::move(text_); }

 private:
  std::string text_;
};

[[nodiscard]] bool asm_text_references_symbol(std::string_view asm_text,
                                              std::string_view symbol_name);
[[nodiscard]] bool asm_text_defines_symbol(std::string_view asm_text,
                                           std::string_view symbol_name);

}  // namespace c4c::backend::x86::core
