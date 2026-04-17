#pragma once

#include <string>

namespace c4c::backend::riscv::assembler {

// Marker type for the staged RV64 assembler contract surface.
struct ContractSurface final {};

struct AssembleRequest {
  std::string asm_text;
  std::string output_path;
};

struct AssembleResult {
  std::string staged_text;
  std::string output_path;
  bool object_emitted = false;
  std::string error;
};

AssembleResult assemble(const AssembleRequest& request);
std::string assemble(const std::string& asm_text, const std::string& output_path);

}  // namespace c4c::backend::riscv::assembler
