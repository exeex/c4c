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

}  // namespace c4c::backend::x86::core
