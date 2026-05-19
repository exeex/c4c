#pragma once

#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstdint>
#include <string>
#include <vector>

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

[[nodiscard]] std::vector<std::string> materialize_integer_constant_lines(
    abi::RegisterReference destination,
    std::uint64_t value,
    unsigned width_bits);

}  // namespace c4c::backend::aarch64::codegen
