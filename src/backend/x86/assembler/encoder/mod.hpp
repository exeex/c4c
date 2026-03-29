#pragma once

#include "../parser.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::x86::assembler::encoder {

struct EncodeResult {
  bool encoded = false;
  std::vector<std::uint8_t> bytes;
  std::string error;
};

EncodeResult encode_instruction(const AsmStatement& statement);
EncodeResult encode_function(const std::vector<AsmStatement>& statements);

}  // namespace c4c::backend::x86::assembler::encoder
