#include "mod.hpp"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace c4c::backend::x86::assembler::encoder {

namespace {

EncodeResult encode_mov_imm32_to_eax(const AsmStatement& statement) {
  if (statement.operands.size() != 2 || statement.operands[0].text != "eax") {
    return EncodeResult{
        .error = "Step 3 x86 encoder only supports mov eax, imm32",
    };
  }

  std::size_t parsed = 0;
  std::int64_t imm = 0;
  try {
    imm = std::stoll(statement.operands[1].text, &parsed, 0);
  } catch (const std::exception&) {
    return EncodeResult{
        .error = "Step 3 x86 encoder failed to parse mov imm32 operand",
    };
  }
  if (parsed != statement.operands[1].text.size()) {
    return EncodeResult{
        .error = "Step 3 x86 encoder failed to parse mov imm32 operand",
    };
  }

  const auto value = static_cast<std::int32_t>(imm);
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
  EncodeResult result;
  result.encoded = true;
  result.bytes.push_back(0xB8);
  result.bytes.insert(result.bytes.end(), bytes, bytes + sizeof(value));
  return result;
}

EncodeResult encode_call_rel32(const AsmStatement& statement) {
  if (statement.operands.size() != 1 || statement.operands[0].text.empty()) {
    return EncodeResult{
        .error = "Step 3 x86 encoder only supports call <symbol>",
    };
  }

  EncodeResult result;
  result.encoded = true;
  result.bytes = {0xE8, 0x00, 0x00, 0x00, 0x00};
  result.relocations.push_back(EncodedRelocation{
      .offset = 1,
      .symbol = statement.operands[0].text,
      .reloc_type = 4,
      .addend = -4,
  });
  return result;
}

}  // namespace

EncodeResult encode_instruction(const AsmStatement& statement) {
  if (statement.kind != AsmStatementKind::Instruction) {
    return EncodeResult{
        .error = "Step 3 x86 encoder only accepts instruction statements",
    };
  }

  if (statement.op == "mov") {
    return encode_mov_imm32_to_eax(statement);
  }

  if (statement.op == "call") {
    return encode_call_rel32(statement);
  }

  if (statement.op == "ret") {
    return EncodeResult{
        .encoded = true,
        .bytes = {0xC3},
    };
  }

  return EncodeResult{
      .error = "unsupported x86 instruction for the Step 3 encoder slice: " +
               statement.text,
  };
}

EncodeResult encode_function(const std::vector<AsmStatement>& statements) {
  EncodeResult result;
  for (const auto& statement : statements) {
    if (statement.kind != AsmStatementKind::Instruction) continue;

    auto encoded = encode_instruction(statement);
    if (!encoded.encoded) return encoded;
    const auto byte_offset = static_cast<std::uint64_t>(result.bytes.size());
    for (auto& relocation : encoded.relocations) {
      relocation.offset += byte_offset;
      result.relocations.push_back(relocation);
    }
    result.bytes.insert(result.bytes.end(), encoded.bytes.begin(), encoded.bytes.end());
  }
  result.encoded = true;
  return result;
}

}  // namespace c4c::backend::x86::assembler::encoder
