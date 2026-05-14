#include "machine_printer.hpp"

#include <sstream>
#include <string>
#include <string_view>

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
  return abi::register_name(operand.reg);
}

std::string immediate_name(const ImmediateOperand& operand) {
  return "#" + std::to_string(operand.signed_value);
}

bool is_plain_add_sub_immediate(const ImmediateOperand& operand) {
  return operand.kind == ImmediateKind::SignedInteger && operand.signed_value >= 0 &&
         operand.signed_value <= 4095;
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

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string_view required_auxiliary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_auxiliary_printer_mnemonic(instruction);
}

std::string_view required_branch_mnemonic() {
  return machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Branch);
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

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
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
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return unsupported(bad_header(instruction) + "branch mnemonic is not printable");
    }
    std::ostringstream out;
    out << "    " << mnemonic << " "
        << block_label(branch.target.function_name, branch.target.block_label) << "\n";
    return printed(out.str());
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

  const auto condition_mnemonic = required_primary_mnemonic(instruction);
  const auto branch_mnemonic = required_branch_mnemonic();
  if (condition_mnemonic.empty() || branch_mnemonic.empty()) {
    return unsupported(bad_header(instruction) + "branch mnemonic is not printable");
  }

  std::ostringstream out;
  out << "    " << condition_mnemonic << " " << register_name(*condition) << ", "
      << block_label(targets.true_target.function_name, targets.true_target.block_label) << "\n"
      << "    " << branch_mnemonic << " "
      << block_label(targets.false_target.function_name, targets.false_target.block_label) << "\n";
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

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return unsupported(bad_header(instruction) + "store mnemonic is not printable");
  }

  std::ostringstream out;
  out << "    " << mnemonic << " " << register_name(*value) << ", " << address << "\n";
  return printed(out.str());
}

MachineAssemblyPrintResult print_scalar(const InstructionRecord& instruction,
                                        const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return unsupported(bad_header(instruction) +
                       "scalar node is missing a structured destination register operand");
  }
  if (instruction.opcode != MachineOpcode::Add && instruction.opcode != MachineOpcode::Sub) {
    return unsupported(bad_header(instruction) +
                       "scalar node opcode is outside the printable add/sub subset");
  }
  if (scalar.inputs.size() != 2) {
    return unsupported(bad_header(instruction) +
                       "scalar add/sub node requires exactly two register or immediate operands");
  }

  const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
  const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
  const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
  const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
  const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                               lhs_register != nullptr;
  const bool rhs_is_register = scalar.inputs[1].kind == OperandKind::Register &&
                               rhs_register != nullptr;
  const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                lhs_immediate != nullptr;
  const bool rhs_is_immediate = scalar.inputs[1].kind == OperandKind::Immediate &&
                                rhs_immediate != nullptr;
  if ((!lhs_is_register && !lhs_is_immediate) || (!rhs_is_register && !rhs_is_immediate)) {
    return unsupported(bad_header(instruction) +
                       "scalar add/sub node requires register or immediate operands");
  }
  if ((lhs_is_immediate && !is_plain_add_sub_immediate(*lhs_immediate)) ||
      (rhs_is_immediate && !is_plain_add_sub_immediate(*rhs_immediate))) {
    return unsupported(bad_header(instruction) +
                       "scalar add/sub immediate operand is outside the plain #imm encoding "
                       "range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return unsupported(bad_header(instruction) + "scalar add/sub mnemonic is not printable");
  }

  std::ostringstream out;
  const auto result = register_name(*scalar.result_register);
  if (lhs_is_register && rhs_is_register) {
    out << "    " << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << register_name(*rhs_register) << "\n";
  } else if (lhs_is_register && rhs_is_immediate) {
    out << "    " << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << immediate_name(*rhs_immediate) << "\n";
  } else if (lhs_is_immediate && rhs_is_register && instruction.opcode == MachineOpcode::Add) {
    out << "    " << mnemonic << " " << result << ", " << register_name(*rhs_register) << ", "
        << immediate_name(*lhs_immediate) << "\n";
  } else if (lhs_is_immediate && rhs_is_immediate) {
    const auto move_mnemonic =
        machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Move);
    out << "    " << move_mnemonic << " " << result << ", " << immediate_name(*lhs_immediate)
        << "\n";
    out << "    " << mnemonic << " " << result << ", " << result << ", "
        << immediate_name(*rhs_immediate) << "\n";
  } else {
    return unsupported(bad_header(instruction) +
                       "scalar sub with an immediate lhs and register rhs is not printable");
  }
  return printed(out.str());
}

MachineAssemblyPrintResult print_return(const InstructionRecord& instruction,
                                        const ReturnInstructionRecord& ret) {
  std::ostringstream out;
  if (ret.value.has_value()) {
    const auto* immediate = std::get_if<ImmediateOperand>(&ret.value->payload);
    const auto* reg = std::get_if<RegisterOperand>(&ret.value->payload);
    if (ret.value->kind == OperandKind::Register && reg != nullptr) {
      const auto return_mnemonic = required_primary_mnemonic(instruction);
      if (return_mnemonic.empty()) {
        return unsupported(bad_header(instruction) + "return mnemonic is not printable");
      }
      out << "    " << return_mnemonic << "\n";
      return printed(out.str());
    }
    if (ret.value->kind != OperandKind::Immediate || immediate == nullptr) {
      return unsupported(bad_header(instruction) +
                         "return value is not a printable immediate operand");
    }
    if (immediate->kind != ImmediateKind::SignedInteger ||
        immediate->signed_value < 0 || immediate->signed_value > 65535) {
      return unsupported(bad_header(instruction) +
                         "return immediate is outside the selected printable subset");
    }

    const char* result_register = nullptr;
    switch (ret.value_type) {
      case bir::TypeKind::I1:
      case bir::TypeKind::I8:
      case bir::TypeKind::I16:
      case bir::TypeKind::I32:
        result_register = "w0";
        break;
      case bir::TypeKind::I64:
        result_register = "x0";
        break;
      default:
        return unsupported(bad_header(instruction) +
                           "return type is outside the selected printable subset");
    }
    const auto move_mnemonic = required_auxiliary_mnemonic(instruction);
    if (move_mnemonic.empty()) {
      return unsupported(bad_header(instruction) + "return move mnemonic is not printable");
    }
    out << "    " << move_mnemonic << " " << result_register << ", #"
        << immediate->signed_value << "\n";
  }
  const auto return_mnemonic = required_primary_mnemonic(instruction);
  if (return_mnemonic.empty()) {
    return unsupported(bad_header(instruction) + "return mnemonic is not printable");
  }
  out << "    " << return_mnemonic << "\n";
  return printed(out.str());
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
  if (const auto* scalar = std::get_if<ScalarInstructionRecord>(&instruction.payload)) {
    return print_scalar(instruction, *scalar);
  }
  if (const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload)) {
    return print_return(instruction, *ret);
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
