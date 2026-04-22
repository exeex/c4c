#include "x86_codegen_output.hpp"

namespace c4c::backend::x86::core {

void AssemblyTextBuffer::append_line(std::string_view line) {
  text_.append(line);
  text_.push_back('\n');
}

void AssemblyTextBuffer::append_raw(std::string_view text) {
  text_.append(text);
}

}  // namespace c4c::backend::x86::core
