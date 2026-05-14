#pragma once

#include "instruction.hpp"

#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct MachineAssemblyPrintResult {
  bool ok = false;
  std::string assembly;
  std::string diagnostic;
};

[[nodiscard]] MachineAssemblyPrintResult print_machine_instruction_node(
    const InstructionRecord& instruction);
[[nodiscard]] MachineAssemblyPrintResult print_machine_instruction_nodes(
    const std::vector<InstructionRecord>& instructions);

}  // namespace c4c::backend::aarch64::codegen
