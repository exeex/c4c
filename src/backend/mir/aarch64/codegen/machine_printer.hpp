#pragma once

#include "instruction.hpp"
#include "mir/printer.hpp"

#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct MachineAssemblyPrintResult {
  bool ok = false;
  std::string assembly;
  std::string diagnostic;
};

class MachineInstructionPrinter final
    : public mir::TargetInstructionPrinter<InstructionRecord> {
 public:
  [[nodiscard]] mir::TargetInstructionPrintResult print_instruction(
      const mir::MachinePrintContext& context,
      const mir::MachineInstruction<InstructionRecord>& instruction) const override;
};

[[nodiscard]] mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction);
[[nodiscard]] MachineAssemblyPrintResult print_machine_instruction_node(
    const InstructionRecord& instruction);
[[nodiscard]] MachineAssemblyPrintResult print_machine_instruction_nodes(
    const std::vector<InstructionRecord>& instructions);

}  // namespace c4c::backend::aarch64::codegen
