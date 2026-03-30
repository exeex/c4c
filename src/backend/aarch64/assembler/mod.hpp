#pragma once

#include <cstdint>
#include "parser.hpp"

namespace c4c::backend::aarch64::assembler {

// Marker type for the staged AArch64 assembler contract surface.
struct ContractSurface final {};

struct AssembleRequest {
  std::string asm_text;
  std::string output_path;
};

struct AssembleResult {
  std::string staged_text;
  std::string output_path;
  bool object_emitted = false;
};

// Current staged assembler entry: raw GNU-style assembly text in, placeholder
// object-emission hook out. The implementation is still a stub.
AssembleResult assemble(const AssembleRequest& request);

// Compatibility overload for the pre-existing string-only staging seam.
std::string assemble(const std::string& asm_text, const std::string& output_path);

bool is_branch_reloc_type(std::uint32_t elf_type);

}  // namespace c4c::backend::aarch64::assembler
