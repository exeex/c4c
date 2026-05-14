#include "printer.hpp"

#include <string>
#include <utility>

namespace c4c::backend::mir {

MachinePrintResult machine_printed(std::string assembly) {
  return MachinePrintResult{.ok = true, .assembly = std::move(assembly)};
}

MachinePrintResult machine_print_unsupported(std::string diagnostic) {
  return MachinePrintResult{.ok = false, .diagnostic = std::move(diagnostic)};
}

TargetInstructionPrintResult target_instruction_printed(std::string instruction_line) {
  std::vector<std::string> instruction_lines;
  instruction_lines.push_back(std::move(instruction_line));
  return target_instruction_lines_printed(std::move(instruction_lines));
}

TargetInstructionPrintResult target_instruction_lines_printed(
    std::vector<std::string> instruction_lines) {
  return TargetInstructionPrintResult{
      .ok = true,
      .instruction_lines = std::move(instruction_lines),
  };
}

TargetInstructionPrintResult target_instruction_unsupported(std::string diagnostic) {
  return TargetInstructionPrintResult{.ok = false, .diagnostic = std::move(diagnostic)};
}

std::string describe_print_context(const MachinePrintContext& context) {
  return "function " + std::to_string(context.function_index) + " block " +
         std::to_string(context.block_index) + " instruction " +
         std::to_string(context.instruction_index);
}

}  // namespace c4c::backend::mir
