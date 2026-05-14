#pragma once

#include "mir.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::mir {

struct MachinePrintContext {
  std::size_t function_index = 0;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
};

struct MachinePrintResult {
  bool ok = false;
  std::string assembly;
  std::string diagnostic;
};

struct TargetInstructionPrintResult {
  bool ok = false;
  std::vector<std::string> instruction_lines;
  std::string diagnostic;
};

[[nodiscard]] MachinePrintResult machine_printed(std::string assembly);
[[nodiscard]] MachinePrintResult machine_print_unsupported(std::string diagnostic);
[[nodiscard]] TargetInstructionPrintResult target_instruction_printed(std::string instruction_line);
[[nodiscard]] TargetInstructionPrintResult target_instruction_lines_printed(
    std::vector<std::string> instruction_lines);
[[nodiscard]] TargetInstructionPrintResult target_instruction_unsupported(
    std::string diagnostic);
[[nodiscard]] std::string describe_print_context(const MachinePrintContext& context);

template <typename TargetInstruction>
class TargetInstructionPrinter {
 public:
  virtual ~TargetInstructionPrinter() = default;

  [[nodiscard]] virtual TargetInstructionPrintResult print_instruction(
      const MachinePrintContext& context,
      const MachineInstruction<TargetInstruction>& instruction) const = 0;
};

template <typename TargetInstruction>
[[nodiscard]] MachinePrintResult print_machine_function(
    const MachineFunction<TargetInstruction>& function,
    const TargetInstructionPrinter<TargetInstruction>& target_printer,
    std::size_t function_index = 0) {
  std::string assembly;
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.instructions.size();
         ++instruction_index) {
      const MachinePrintContext context{
          .function_index = function_index,
          .block_index = block_index,
          .instruction_index = instruction_index,
          .function_name = function.function_name,
          .block_label = block.block_label,
      };
      const auto printed =
          target_printer.print_instruction(context, block.instructions[instruction_index]);
      if (!printed.ok) {
        return machine_print_unsupported("target instruction spelling failed at " +
                                         describe_print_context(context) + ": " +
                                         printed.diagnostic);
      }
      if (printed.instruction_lines.empty()) {
        return machine_print_unsupported("target instruction spelling failed at " +
                                         describe_print_context(context) +
                                         ": target produced no instruction line payloads");
      }
      for (const auto& line : printed.instruction_lines) {
        if (line.find('\n') != std::string::npos || line.find('\r') != std::string::npos) {
          return machine_print_unsupported(
              "target instruction spelling failed at " + describe_print_context(context) +
              ": target line payload must not contain newline characters");
        }
        if (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
          return machine_print_unsupported(
              "target instruction spelling failed at " + describe_print_context(context) +
              ": target line payload must not include indentation");
        }
        assembly += "    ";
        assembly += line;
        assembly += "\n";
      }
    }
  }
  return machine_printed(std::move(assembly));
}

template <typename TargetInstruction>
[[nodiscard]] MachinePrintResult print_machine_module(
    const MachineModule<TargetInstruction>& module,
    const TargetInstructionPrinter<TargetInstruction>& target_printer) {
  std::string assembly;
  for (std::size_t function_index = 0; function_index < module.functions.size();
       ++function_index) {
    const auto printed =
        print_machine_function(module.functions[function_index], target_printer, function_index);
    if (!printed.ok) {
      return printed;
    }
    assembly += printed.assembly;
  }
  return machine_printed(std::move(assembly));
}

}  // namespace c4c::backend::mir
