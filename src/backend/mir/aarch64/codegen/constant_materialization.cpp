#include "constant_materialization.hpp"

#include <sstream>

namespace c4c::backend::aarch64::codegen {

std::vector<std::string> materialize_integer_constant_lines(
    abi::RegisterReference destination,
    std::uint64_t value,
    unsigned width_bits) {
  std::vector<std::string> lines;
  const std::string destination_name = abi::register_name(destination);
  const unsigned chunks = width_bits == 64U ? 4U : 2U;

  bool emitted_base = false;
  for (unsigned chunk = 0; chunk < chunks; ++chunk) {
    const auto halfword =
        static_cast<unsigned>((value >> (chunk * 16U)) & 0xffffU);
    if (halfword == 0U) {
      continue;
    }

    std::ostringstream line;
    line << (emitted_base ? "movk " : "movz ") << destination_name
         << ", #" << halfword;
    if (chunk != 0U) {
      line << ", lsl #" << (chunk * 16U);
    }
    lines.push_back(line.str());
    emitted_base = true;
  }

  if (!emitted_base) {
    lines.push_back("movz " + destination_name + ", #0");
  }
  return lines;
}

}  // namespace c4c::backend::aarch64::codegen
