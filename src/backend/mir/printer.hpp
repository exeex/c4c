#pragma once

#include "mir.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::mir {

struct AssemblyPrintResult {
  bool ok = false;
  std::string assembly;
  std::string diagnostic;
};

[[nodiscard]] AssemblyPrintResult print_success(std::string assembly);
[[nodiscard]] AssemblyPrintResult print_failure(std::string diagnostic);

struct AssemblyPrinterOptions {
  bool emit_text_section = true;
  bool emit_gnu_stack_note = true;
  bool emit_empty_block_labels = false;
  std::string_view instruction_indent = "    ";
};

template <typename TargetInstruction>
struct TargetAssemblyPrinter {
  std::function<AssemblyPrintResult(c4c::FunctionNameId)> print_function_label;
  std::function<AssemblyPrintResult(c4c::FunctionNameId, c4c::BlockLabelId, std::size_t)>
      print_block_label;
  std::function<AssemblyPrintResult(const TargetInstruction&)> print_instruction;
};

namespace detail {

void append_instruction_lines(std::string& assembly,
                              std::string_view unindented_text,
                              std::string_view indent);

}  // namespace detail

template <typename TargetInstruction>
[[nodiscard]] AssemblyPrintResult print_functions(
    const std::vector<Function<MachineNode<TargetInstruction>>>& functions,
    const TargetAssemblyPrinter<TargetInstruction>& target_printer,
    const AssemblyPrinterOptions& options = {}) {
  if (!target_printer.print_function_label) {
    return print_failure("common MIR printer requires a function-label callback");
  }
  if (!target_printer.print_block_label) {
    return print_failure("common MIR printer requires a block-label callback");
  }
  if (!target_printer.print_instruction) {
    return print_failure("common MIR printer requires an instruction callback");
  }

  std::string assembly;
  bool printed_any_function = false;
  if (options.emit_text_section) {
    assembly += "    .text\n";
  }

  for (std::size_t function_index = 0; function_index < functions.size(); ++function_index) {
    const auto& function = functions[function_index];
    if (empty(function)) {
      continue;
    }

    const auto function_label = target_printer.print_function_label(function.name);
    if (!function_label.ok) {
      return print_failure("function " + std::to_string(function_index) +
                           " label: " + function_label.diagnostic);
    }
    if (function_label.assembly.empty()) {
      return print_failure("function " + std::to_string(function_index) +
                           " label: target returned an empty label");
    }

    printed_any_function = true;
    assembly += "    .globl " + function_label.assembly + "\n";
    assembly += "    .type " + function_label.assembly + ", %function\n";
    assembly += function_label.assembly + ":\n";

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      if (block.instructions.empty() && !options.emit_empty_block_labels) {
        continue;
      }
      if (block_index != 0) {
        const auto block_label =
            target_printer.print_block_label(function.name, block.label, block.index);
        if (!block_label.ok) {
          return print_failure("function " + std::to_string(function_index) + " block " +
                               std::to_string(block_index) +
                               " label: " + block_label.diagnostic);
        }
        if (block_label.assembly.empty()) {
          return print_failure("function " + std::to_string(function_index) + " block " +
                               std::to_string(block_index) +
                               " label: target returned an empty label");
        }
        assembly += block_label.assembly + ":\n";
      }

      for (std::size_t instruction_index = 0; instruction_index < block.instructions.size();
           ++instruction_index) {
        const auto printed =
            target_printer.print_instruction(block.instructions[instruction_index].target);
        if (!printed.ok) {
          return print_failure("function " + std::to_string(function_index) + " block " +
                               std::to_string(block_index) + " instruction " +
                               std::to_string(instruction_index) + ": " + printed.diagnostic);
        }
        detail::append_instruction_lines(assembly, printed.assembly, options.instruction_indent);
      }
    }

    assembly += "    .size " + function_label.assembly + ", .-" + function_label.assembly + "\n";
  }

  if (!printed_any_function) {
    assembly.clear();
    return print_success(std::move(assembly));
  }

  if (options.emit_gnu_stack_note) {
    assembly += "    .section .note.GNU-stack,\"\",@progbits\n";
  }
  return print_success(std::move(assembly));
}

template <typename TargetInstruction>
[[nodiscard]] AssemblyPrintResult print_function(
    const Function<MachineNode<TargetInstruction>>& function,
    const TargetAssemblyPrinter<TargetInstruction>& target_printer,
    const AssemblyPrinterOptions& options = {}) {
  return print_functions(std::vector<Function<MachineNode<TargetInstruction>>>{function},
                         target_printer,
                         options);
}

}  // namespace c4c::backend::mir
