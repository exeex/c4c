#include "machine_printer.hpp"

#include <sstream>
#include <string>

namespace c4c::backend::aarch64::codegen {

namespace {

MachineAssemblyPrintResult unsupported(std::string diagnostic) {
  return MachineAssemblyPrintResult{.ok = false, .diagnostic = std::move(diagnostic)};
}

MachineAssemblyPrintResult printed(std::string assembly) {
  return MachineAssemblyPrintResult{.ok = true, .assembly = std::move(assembly)};
}

std::string block_label(c4c::FunctionNameId function_name, c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

std::string register_name(const RegisterOperand& operand) {
  return c4c::backend::aarch64::abi::register_name(operand.reg);
}

std::string memory_address(const MemoryOperand& address) {
  std::ostringstream out;
  if (address.base_kind == MemoryBaseKind::FrameSlot) {
    out << "[sp";
  } else if (address.base_kind == MemoryBaseKind::PointerValue && address.base_register.has_value()) {
    out << "[" << register_name(*address.base_register);
  } else {
    return {};
  }
  if (address.byte_offset != 0) {
    out << ", #" << address.byte_offset;
  }
  out << "]";
  return out.str();
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::optional<std::string> validate_selected_machine_node(const InstructionRecord& instruction) {
  if (instruction.surface != RecordSurfaceKind::MachineInstructionNode) {
    return std::string("printer requires surface machine_instruction_node, got ") +
           std::string(record_surface_kind_name(instruction.surface));
  }
  if (instruction.selection.status != MachineNodeSelectionStatus::Selected) {
    std::string diagnostic = "printer requires selected machine node, got ";
    diagnostic += machine_node_selection_status_name(instruction.selection.status);
    if (!instruction.selection.diagnostic.empty()) {
      diagnostic += ": ";
      diagnostic += instruction.selection.diagnostic;
    }
    return diagnostic;
  }
  return std::nullopt;
}

MachineAssemblyPrintResult print_spill_reload(const InstructionRecord& instruction,
                                              const SpillReloadInstructionRecord& spill_reload) {
  if (!spill_reload.scratch.has_value()) {
    return unsupported(bad_header(instruction) + "spill/reload node is missing scratch register");
  }
  if (!spill_reload.stack_offset_bytes.has_value() || !spill_reload.stack_offset_is_prepared_snapshot ||
      !spill_reload.slot_id.has_value()) {
    return unsupported(bad_header(instruction) +
                       "spill/reload node is missing prepared stack-slot offset");
  }
  if (spill_reload.slot.support != MemoryOperandSupportKind::Prepared ||
      spill_reload.slot.base_kind != MemoryBaseKind::FrameSlot ||
      !spill_reload.slot.can_use_base_plus_offset) {
    return unsupported(bad_header(instruction) +
                       "spill/reload node is not a prepared frame-slot address");
  }

  const auto address = memory_address(spill_reload.slot);
  if (address.empty()) {
    return unsupported(bad_header(instruction) + "spill/reload address is not printable");
  }

  const char* mnemonic = nullptr;
  if (spill_reload.pseudo_kind == MachinePseudoKind::SpillToSlot) {
    mnemonic = "str";
  } else if (spill_reload.pseudo_kind == MachinePseudoKind::ReloadFromSlot) {
    mnemonic = "ldr";
  } else {
    return unsupported(bad_header(instruction) + "spill/reload pseudo kind is unsupported");
  }

  std::ostringstream out;
  out << "    " << mnemonic << " " << register_name(*spill_reload.scratch) << ", " << address
      << "\n";
  return printed(out.str());
}

MachineAssemblyPrintResult print_branch(const InstructionRecord& instruction,
                                        const BranchInstructionRecord& branch) {
  if (branch.target.function_name == c4c::kInvalidFunctionName ||
      branch.target.block_label == c4c::kInvalidBlockLabel) {
    return unsupported(bad_header(instruction) + "branch target identity is missing");
  }
  if (!branch.conditional) {
    return printed("    b " + block_label(branch.target.function_name, branch.target.block_label) +
                   "\n");
  }
  if (!branch.target_pair.has_value() || !branch.condition.has_value()) {
    return unsupported(bad_header(instruction) +
                       "conditional branch is missing target pair or condition operand");
  }
  const auto* condition = std::get_if<RegisterOperand>(&branch.condition->payload);
  if (branch.condition->kind != OperandKind::Register || condition == nullptr) {
    return unsupported(bad_header(instruction) +
                       "materialized-bool branch condition is not a register operand");
  }
  if (branch.condition_record.has_value() &&
      branch.condition_record->form != BranchConditionForm::MaterializedBool) {
    return unsupported(bad_header(instruction) +
                       "only materialized-bool conditional branches are printable");
  }

  const auto& targets = *branch.target_pair;
  if (targets.true_target.function_name == c4c::kInvalidFunctionName ||
      targets.true_target.block_label == c4c::kInvalidBlockLabel ||
      targets.false_target.function_name == c4c::kInvalidFunctionName ||
      targets.false_target.block_label == c4c::kInvalidBlockLabel) {
    return unsupported(bad_header(instruction) + "conditional branch target identity is missing");
  }

  std::ostringstream out;
  out << "    cbnz " << register_name(*condition) << ", "
      << block_label(targets.true_target.function_name, targets.true_target.block_label) << "\n"
      << "    b " << block_label(targets.false_target.function_name, targets.false_target.block_label)
      << "\n";
  return printed(out.str());
}

MachineAssemblyPrintResult print_memory(const InstructionRecord& instruction,
                                        const MemoryInstructionRecord& memory) {
  if (memory.memory_kind == MemoryInstructionKind::Load) {
    return unsupported(bad_header(instruction) +
                       "load node is missing a structured destination register operand");
  }
  if (!memory.value.has_value()) {
    return unsupported(bad_header(instruction) + "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind != OperandKind::Register || value == nullptr) {
    return unsupported(bad_header(instruction) + "store value is not a register operand");
  }
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return unsupported(bad_header(instruction) + "store address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return unsupported(bad_header(instruction) + "store address is not printable");
  }

  std::ostringstream out;
  out << "    str " << register_name(*value) << ", " << address << "\n";
  return printed(out.str());
}

MachineAssemblyPrintResult print_scalar(const InstructionRecord& instruction) {
  return unsupported(bad_header(instruction) +
                     "scalar node is missing a structured destination register operand");
}

}  // namespace

MachineAssemblyPrintResult print_machine_instruction_node(const InstructionRecord& instruction) {
  if (const auto invalid = validate_selected_machine_node(instruction); invalid.has_value()) {
    return unsupported(*invalid);
  }

  if (const auto* spill_reload =
          std::get_if<SpillReloadInstructionRecord>(&instruction.payload)) {
    return print_spill_reload(instruction, *spill_reload);
  }
  if (const auto* branch = std::get_if<BranchInstructionRecord>(&instruction.payload)) {
    return print_branch(instruction, *branch);
  }
  if (const auto* memory = std::get_if<MemoryInstructionRecord>(&instruction.payload)) {
    return print_memory(instruction, *memory);
  }
  if (std::get_if<ScalarInstructionRecord>(&instruction.payload) != nullptr) {
    return print_scalar(instruction);
  }
  return unsupported(bad_header(instruction) + "instruction family is not in the printable subset");
}

MachineAssemblyPrintResult print_machine_instruction_nodes(
    const std::vector<InstructionRecord>& instructions) {
  std::ostringstream out;
  for (std::size_t index = 0; index < instructions.size(); ++index) {
    const auto result = print_machine_instruction_node(instructions[index]);
    if (!result.ok) {
      return unsupported("instruction " + std::to_string(index) + ": " + result.diagnostic);
    }
    out << result.assembly;
  }
  return printed(out.str());
}

}  // namespace c4c::backend::aarch64::codegen
