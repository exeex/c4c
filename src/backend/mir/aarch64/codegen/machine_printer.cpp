#include "machine_printer.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
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

std::string prepared_value_home_name(const prepare::PreparedValueHome& home) {
  std::ostringstream out;
  out << "value#" << home.value_id << ":"
      << prepare::prepared_value_home_kind_name(home.kind);
  if (home.register_name.has_value()) {
    out << ":" << *home.register_name;
  }
  if (home.slot_id.has_value()) {
    out << ":slot#" << *home.slot_id;
  }
  if (home.offset_bytes.has_value()) {
    out << ":offset+" << *home.offset_bytes;
  }
  return out.str();
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

mir::TargetInstructionPrintResult print_spill_reload(
    const InstructionRecord& instruction,
    const SpillReloadInstructionRecord& spill_reload) {
  if (!spill_reload.scratch.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing scratch register");
  }
  if (!spill_reload.stack_offset_bytes.has_value() || !spill_reload.stack_offset_is_prepared_snapshot ||
      !spill_reload.slot_id.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing prepared stack-slot offset");
  }
  if (spill_reload.slot.support != MemoryOperandSupportKind::Prepared ||
      spill_reload.slot.base_kind != MemoryBaseKind::FrameSlot ||
      !spill_reload.slot.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is not a prepared frame-slot address");
  }

  const auto address = memory_address(spill_reload.slot);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload address is not printable");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload pseudo kind is unsupported");
  }

  std::ostringstream out;
  out << mnemonic << " " << register_name(*spill_reload.scratch) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_branch(const InstructionRecord& instruction,
                                               const BranchInstructionRecord& branch) {
  if (branch.target.function_name == c4c::kInvalidFunctionName ||
      branch.target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) + "branch target identity is missing");
  }
  if (!branch.conditional) {
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
    }
    std::ostringstream out;
    out << mnemonic << " " << block_label(branch.target.function_name, branch.target.block_label);
    return target_printed({out.str()});
  }
  if (!branch.target_pair.has_value() || !branch.condition.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch is missing target pair or condition operand");
  }
  const auto* condition = std::get_if<RegisterOperand>(&branch.condition->payload);
  if (branch.condition->kind != OperandKind::Register || condition == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "materialized-bool branch condition is not a register operand");
  }
  if (branch.condition_record.has_value() &&
      branch.condition_record->form != BranchConditionForm::MaterializedBool) {
    return target_unsupported(bad_header(instruction) +
                              "only materialized-bool conditional branches are printable");
  }

  const auto& targets = *branch.target_pair;
  if (targets.true_target.function_name == c4c::kInvalidFunctionName ||
      targets.true_target.block_label == c4c::kInvalidBlockLabel ||
      targets.false_target.function_name == c4c::kInvalidFunctionName ||
      targets.false_target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch target identity is missing");
  }

  const auto condition_mnemonic = required_primary_mnemonic(instruction);
  const auto branch_mnemonic = required_branch_mnemonic();
  if (condition_mnemonic.empty() || branch_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
  }

  std::ostringstream condition_line;
  condition_line << condition_mnemonic << " " << register_name(*condition) << ", "
                 << block_label(targets.true_target.function_name,
                                targets.true_target.block_label);
  std::ostringstream branch_line;
  branch_line << branch_mnemonic << " "
              << block_label(targets.false_target.function_name,
                             targets.false_target.block_label);
  return target_printed({condition_line.str(), branch_line.str()});
}

mir::TargetInstructionPrintResult print_memory(const InstructionRecord& instruction,
                                               const MemoryInstructionRecord& memory) {
  if (memory.memory_kind == MemoryInstructionKind::Load) {
    return target_unsupported(bad_header(instruction) +
                              "load node is missing a structured destination register operand");
  }
  if (!memory.value.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind != OperandKind::Register || value == nullptr) {
    return target_unsupported(bad_header(instruction) + "store value is not a register operand");
  }
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "store address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) + "store address is not printable");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "store mnemonic is not printable");
  }

  std::ostringstream out;
  out << mnemonic << " " << register_name(*value) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call(const InstructionRecord& instruction,
                                             const CallInstructionRecord& call) {
  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaStart}) {
    if (!call.variadic_va_start.has_value() || call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "va_start node is missing structured prepared va_start provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "va_start mnemonic is not printable");
    }

    const auto& va_start = *call.variadic_va_start;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_start.destination_va_list)
          << " named_gp=" << va_start.named_gp_register_count
          << " named_fp=" << va_start.named_fp_register_count
          << " va_list_size=" << va_start.va_list_size_bytes
          << " va_list_align=" << va_start.va_list_align_bytes
          << " scratch_registers=" << va_start.scratch_register_count
          << " scratch_stack=" << va_start.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.rsa slot#" << va_start.register_save_area_slot_id
          << " stack+" << va_start.register_save_area_stack_offset_bytes
          << " size=" << va_start.register_save_area_size_bytes
          << " align=" << va_start.register_save_area_align_bytes
          << " gp_offset=" << va_start.register_save_area_gp_offset_bytes
          << " fp_offset=" << va_start.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_start.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_start.register_save_area_fp_slot_size_bytes
          << " saved_gp=" << va_start.saved_gp_register_count
          << " saved_fp=" << va_start.saved_fp_register_count;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.initial_offsets gp=" << va_start.initial_gp_offset_bytes
          << " fp=" << va_start.initial_fp_offset_bytes
          << " overflow_slot#" << va_start.overflow_area_base_slot_id
          << " overflow_stack+" << va_start.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_start.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_start.va_list_fields) {
      std::ostringstream out;
      out << "va.start.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " offset=" << field.offset_bytes
          << " size=" << field.size_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArg}) {
    if (!call.variadic_scalar_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar va_arg node is missing structured prepared access-plan provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_scalar_va_arg;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_scalar_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " result=" << prepared_value_home_name(va_arg.result_home)
          << " value_size=" << va_arg.value_size_bytes
          << " value_align=" << va_arg.value_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " slot_size=" << va_arg.source_slot_size_bytes
          << " rsa_slot#" << va_arg.register_save_area_slot_id
          << " rsa_stack+" << va_arg.register_save_area_stack_offset_bytes
          << " rsa_size=" << va_arg.register_save_area_size_bytes
          << " rsa_align=" << va_arg.register_save_area_align_bytes
          << " gp_base=" << va_arg.register_save_area_gp_offset_bytes
          << " fp_base=" << va_arg.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_arg.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_arg.register_save_area_fp_slot_size_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.overflow_source_field)
          << " overflow_field_offset=" << va_arg.overflow_source_field_offset_bytes
          << " overflow_stride=" << va_arg.overflow_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArgAggregate}) {
    if (!call.variadic_aggregate_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "aggregate va_arg node is missing structured prepared access-plan provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "aggregate va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_aggregate_va_arg;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_aggregate_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " destination_payload="
          << prepared_value_home_name(va_arg.destination_payload_home)
          << " payload_size=" << va_arg.payload_size_bytes
          << " payload_align=" << va_arg.payload_align_bytes
          << " copy_size=" << va_arg.copy_size_bytes
          << " copy_align=" << va_arg.copy_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.aggregate.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " payload_offset=" << va_arg.source_payload_offset_bytes
          << " slot_size=" << va_arg.source_slot_size_bytes
          << " rsa_slot#" << va_arg.register_save_area_slot_id
          << " rsa_stack+" << va_arg.register_save_area_stack_offset_bytes
          << " rsa_size=" << va_arg.register_save_area_size_bytes
          << " rsa_align=" << va_arg.register_save_area_align_bytes
          << " gp_base=" << va_arg.register_save_area_gp_offset_bytes
          << " fp_base=" << va_arg.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_arg.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_arg.register_save_area_fp_slot_size_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.aggregate.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "call mnemonic is not printable");
  }
  if (call.is_indirect) {
    if (!call.indirect_callee.has_value() ||
        !call.prepared_indirect_callee.has_value() || call.source_call == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call node is missing prepared callee provenance");
    }
    const auto* callee = std::get_if<RegisterOperand>(&call.indirect_callee->payload);
    if (call.indirect_callee->kind != OperandKind::Register || callee == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call callee is not a register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*callee);
    return target_printed({out.str()});
  }
  if (!call.direct_callee.has_value() || call.direct_callee_label.empty() ||
      call.source_call == nullptr || !call.wrapper_kind.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "direct call node is missing prepared callee provenance");
  }

  std::ostringstream out;
  out << mnemonic << " " << call.direct_callee_label;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_frame(const InstructionRecord& instruction,
                                              const FrameInstructionRecord& frame) {
  if (frame.source_frame == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame provenance");
  }
  if (frame.function_name == c4c::kInvalidFunctionName) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing function identity");
  }
  if (frame.frame_alignment_bytes == 0) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame alignment");
  }
  if (frame.has_dynamic_stack) {
    return target_unsupported(bad_header(instruction) +
                              "dynamic-stack frame node is outside the printable subset");
  }
  if (!frame.saved_callee_registers.empty() || frame.callee_save.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "callee-save frame node is outside the printable subset");
  }

  if (frame.frame_kind != FrameInstructionKind::PrologueSetup &&
      frame.frame_kind != FrameInstructionKind::EpilogueTeardown) {
    return target_unsupported(bad_header(instruction) +
                              "frame node kind is outside the printable subset");
  }
  if (frame.frame_size_bytes == 0) {
    return target_printed({});
  }
  if (frame.frame_size_bytes > 4095) {
    return target_unsupported(bad_header(instruction) +
                              "frame adjustment immediate is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "frame mnemonic is not printable");
  }

  std::ostringstream out;
  out << mnemonic << " sp, sp, #" << frame.frame_size_bytes;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move) {
  if (move.source_bundle == nullptr || move.source_move == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move node is missing prepared move provenance");
  }
  if (!move.source_register.has_value() || !move.destination_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary move node requires prepared register source and destination");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move mnemonic is not printable");
  }
  std::ostringstream out;
  out << mnemonic << " " << register_name(*move.destination_register) << ", "
      << register_name(*move.source_register);
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding) {
  if (binding.source_bundle == nullptr || binding.source_binding == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary ABI binding node is missing prepared binding provenance");
  }
  return target_unsupported(
      bad_header(instruction) +
      "call-boundary ABI binding node requires later AArch64 move lowering");
}

mir::TargetInstructionPrintResult print_scalar(const InstructionRecord& instruction,
                                               const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar node is missing a structured destination register operand");
  }
  if (instruction.opcode != MachineOpcode::Add && instruction.opcode != MachineOpcode::Sub) {
    return target_unsupported(bad_header(instruction) +
                              "scalar node opcode is outside the printable add/sub subset");
  }
  if (scalar.inputs.size() != 2) {
    return target_unsupported(
        bad_header(instruction) +
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
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub node requires register or immediate operands");
  }
  if ((lhs_is_immediate && !is_plain_add_sub_immediate(*lhs_immediate)) ||
      (rhs_is_immediate && !is_plain_add_sub_immediate(*rhs_immediate))) {
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub immediate operand is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub mnemonic is not printable");
  }

  std::vector<std::string> lines;
  const auto result = register_name(*scalar.result_register);
  if (lhs_is_register && rhs_is_register) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << register_name(*rhs_register);
    lines.push_back(out.str());
  } else if (lhs_is_register && rhs_is_immediate) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << immediate_name(*rhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_register && instruction.opcode == MachineOpcode::Add) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*rhs_register) << ", "
        << immediate_name(*lhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_immediate) {
    const auto move_mnemonic =
        machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Move);
    std::ostringstream move_line;
    move_line << move_mnemonic << " " << result << ", " << immediate_name(*lhs_immediate);
    lines.push_back(move_line.str());
    std::ostringstream add_line;
    add_line << mnemonic << " " << result << ", " << result << ", "
             << immediate_name(*rhs_immediate);
    lines.push_back(add_line.str());
  } else {
    return target_unsupported(
        bad_header(instruction) +
        "scalar sub with an immediate lhs and register rhs is not printable");
  }
  return target_printed(std::move(lines));
}

mir::TargetInstructionPrintResult print_return(const InstructionRecord& instruction,
                                               const ReturnInstructionRecord& ret) {
  std::vector<std::string> lines;
  if (ret.value.has_value()) {
    const auto* immediate = std::get_if<ImmediateOperand>(&ret.value->payload);
    const auto* reg = std::get_if<RegisterOperand>(&ret.value->payload);
    if (ret.value->kind == OperandKind::Register && reg != nullptr) {
      const auto return_mnemonic = required_primary_mnemonic(instruction);
      if (return_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
      }
      return target_printed({std::string(return_mnemonic)});
    }
    if (ret.value->kind != OperandKind::Immediate || immediate == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "return value is not a printable immediate operand");
    }
    if (immediate->kind != ImmediateKind::SignedInteger ||
        immediate->signed_value < 0 || immediate->signed_value > 65535) {
      return target_unsupported(bad_header(instruction) +
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
        return target_unsupported(bad_header(instruction) +
                                  "return type is outside the selected printable subset");
    }
    const auto move_mnemonic = required_auxiliary_mnemonic(instruction);
    if (move_mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "return move mnemonic is not printable");
    }
    std::ostringstream move_line;
    move_line << move_mnemonic << " " << result_register << ", #" << immediate->signed_value;
    lines.push_back(move_line.str());
  }
  const auto return_mnemonic = required_primary_mnemonic(instruction);
  if (return_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
  }
  lines.emplace_back(return_mnemonic);
  return target_printed(std::move(lines));
}

}  // namespace

mir::TargetInstructionPrintResult MachineInstructionPrinter::print_instruction(
    const mir::MachinePrintContext&,
    const mir::MachineInstruction<InstructionRecord>& instruction) const {
  return print_machine_instruction_line_payloads(instruction.target);
}

mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction) {
  if (const auto invalid = validate_selected_machine_node(instruction); invalid.has_value()) {
    return target_unsupported(*invalid);
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
  if (const auto* frame = std::get_if<FrameInstructionRecord>(&instruction.payload)) {
    return print_frame(instruction, *frame);
  }
  if (const auto* move =
          std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_move(instruction, *move);
  }
  if (const auto* binding =
          std::get_if<CallBoundaryAbiBindingInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_abi_binding(instruction, *binding);
  }
  if (const auto* call = std::get_if<CallInstructionRecord>(&instruction.payload)) {
    return print_call(instruction, *call);
  }
  if (const auto* scalar = std::get_if<ScalarInstructionRecord>(&instruction.payload)) {
    return print_scalar(instruction, *scalar);
  }
  if (const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload)) {
    return print_return(instruction, *ret);
  }
  return target_unsupported(bad_header(instruction) +
                            "instruction family is not in the printable subset");
}

}  // namespace c4c::backend::aarch64::codegen
