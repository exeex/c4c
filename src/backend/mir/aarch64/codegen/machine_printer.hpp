#pragma once

#include "instruction.hpp"
#include "mir/printer.hpp"

namespace c4c::backend::aarch64::codegen {

class MachineInstructionPrinter final
    : public mir::TargetInstructionPrinter<InstructionRecord> {
 public:
  [[nodiscard]] mir::TargetInstructionPrintResult print_instruction(
      const mir::MachinePrintContext& context,
      const mir::MachineInstruction<InstructionRecord>& instruction) const override;
};

[[nodiscard]] mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction);

}  // namespace c4c::backend::aarch64::codegen
