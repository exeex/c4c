#pragma once

#include "../parser.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::x86::assembler::encoder {

struct EncodedRelocation {
  std::uint64_t offset = 0;
  std::string symbol;
  std::uint32_t reloc_type = 0;
  std::int64_t addend = 0;
};

struct EncodeResult {
  bool encoded = false;
  std::vector<std::uint8_t> bytes;
  std::vector<EncodedRelocation> relocations;
  std::string error;
};

EncodeResult encode_instruction(const AsmStatement& statement);
EncodeResult encode_function(const std::vector<AsmStatement>& statements);

}  // namespace c4c::backend::x86::assembler::encoder
