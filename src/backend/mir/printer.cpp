#include "printer.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::mir {

AssemblyPrintResult print_success(std::string assembly) {
  return AssemblyPrintResult{.ok = true, .assembly = std::move(assembly)};
}

AssemblyPrintResult print_failure(std::string diagnostic) {
  return AssemblyPrintResult{.ok = false, .diagnostic = std::move(diagnostic)};
}

namespace detail {

void append_instruction_lines(std::string& assembly,
                              std::string_view unindented_text,
                              std::string_view indent) {
  std::size_t line_start = 0;
  while (line_start < unindented_text.size()) {
    const std::size_t line_end = unindented_text.find('\n', line_start);
    const auto line = line_end == std::string_view::npos
                          ? unindented_text.substr(line_start)
                          : unindented_text.substr(line_start, line_end - line_start);
    if (!line.empty()) {
      assembly.append(indent.data(), indent.size());
      assembly.append(line.data(), line.size());
    }
    assembly.push_back('\n');
    if (line_end == std::string_view::npos) {
      break;
    }
    line_start = line_end + 1;
  }
}

}  // namespace detail

}  // namespace c4c::backend::mir
