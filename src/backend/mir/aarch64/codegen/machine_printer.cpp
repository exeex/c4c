#include "machine_printer.hpp"

#include <cstdint>
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

std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> register_name_with_view(const RegisterOperand& operand,
                                                   abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<std::string> fp_register_name_with_view(const RegisterOperand& operand,
                                                      abi::RegisterView view) {
  if (!abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> floating_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::string_view floating_alu_mnemonic(ScalarAluOperationKind operation) {
  switch (operation) {
    case ScalarAluOperationKind::Add:
      return "fadd";
    case ScalarAluOperationKind::Sub:
      return "fsub";
    case ScalarAluOperationKind::Mul:
      return "fmul";
    case ScalarAluOperationKind::Div:
      return "fdiv";
    case ScalarAluOperationKind::And:
    case ScalarAluOperationKind::Or:
    case ScalarAluOperationKind::Xor:
    case ScalarAluOperationKind::Deferred:
      return {};
  }
  return {};
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

bool complete_preserved_value_route(
    const prepare::PreparedCallPreservedValue& preserved) {
  switch (preserved.route) {
    case prepare::PreparedCallPreservationRoute::Unknown:
      return false;
    case prepare::PreparedCallPreservationRoute::CalleeSavedRegister:
      return preserved.register_name.has_value() &&
             preserved.register_bank.has_value() &&
             !preserved.occupied_register_names.empty() &&
             preserved.register_placement.has_value() &&
             preserved.callee_saved_save_index.has_value();
    case prepare::PreparedCallPreservationRoute::StackSlot:
      return preserved.slot_id.has_value() &&
             preserved.stack_offset_bytes.has_value() &&
             preserved.spill_slot_placement.has_value();
  }
  return false;
}

bool has_complete_live_preservation(
    const prepare::PreparedI128RuntimeHelper::LivePreservationPolicy& policy) {
  if (!policy.evaluated || !policy.caller_saved_clobbers_modeled ||
      !policy.no_additional_live_preservation_required) {
    return false;
  }
  for (const auto& preserved : policy.preserved_values) {
    if (!complete_preserved_value_route(preserved)) {
      return false;
    }
  }
  return true;
}

bool has_complete_selected_call_ownership(
    const prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy& ownership) {
  return ownership.owns_terminal_call &&
         ownership.has_callee_identity &&
         ownership.has_resource_policy &&
         ownership.has_clobber_policy &&
         ownership.has_abi_bindings &&
         ownership.has_marshaling &&
         ownership.has_live_preservation;
}

bool is_decimal_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool is_printable_x_register_name(std::string_view name) {
  if (name.size() < 2 || name.front() != 'x') {
    return false;
  }
  unsigned index = 0;
  for (std::size_t pos = 1; pos < name.size(); ++pos) {
    if (!is_decimal_digit(name[pos])) {
      return false;
    }
    index = index * 10U + static_cast<unsigned>(name[pos] - '0');
  }
  return index <= 30U;
}

std::optional<std::string> validate_i128_helper_move(
    const prepare::PreparedI128RuntimeHelper::MarshalingMove& move,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction,
    prepare::PreparedMovePhase phase) {
  if (move.direction != direction || move.phase != phase ||
      move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return std::string{"i128 helper boundary has unsupported marshal/unmarshal move shape"};
  }
  if (move.carrier_lane.width_bytes != 8 ||
      !move.carrier_lane.register_name.has_value() ||
      !is_printable_x_register_name(*move.carrier_lane.register_name)) {
    return std::string{"i128 helper boundary requires register-backed carrier lane moves"};
  }
  if (move.abi_register.width_bytes != 8 ||
      move.abi_register.register_bank != prepare::PreparedRegisterBank::Gpr ||
      move.abi_register.register_class != prepare::PreparedRegisterClass::General ||
      !is_printable_x_register_name(move.abi_register.register_name)) {
    return std::string{"i128 helper boundary requires GPR ABI register bindings"};
  }
  if (move.carrier_lane.value_id != move.abi_register.value_id ||
      move.carrier_lane.value_name != move.abi_register.value_name ||
      move.carrier_lane.role != move.abi_register.role ||
      move.carrier_lane.lane_index != move.abi_register.lane_index ||
      move.carrier_lane.width_bytes != move.abi_register.width_bytes) {
    return std::string{"i128 helper boundary marshal/unmarshal lane facts mismatch"};
  }
  return std::nullopt;
}

std::optional<std::string> append_i128_helper_move_line(
    std::vector<std::string>& lines,
    const std::optional<prepare::PreparedI128RuntimeHelper::MarshalingMove>& move,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction,
    prepare::PreparedMovePhase phase) {
  if (!move.has_value()) {
    return std::string{"i128 helper boundary is missing structured marshal/unmarshal moves"};
  }
  if (const auto invalid = validate_i128_helper_move(*move, direction, phase);
      invalid.has_value()) {
    return invalid;
  }

  std::ostringstream line;
  if (direction ==
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument) {
    line << "mov " << move->abi_register.register_name << ", "
         << *move->carrier_lane.register_name;
  } else {
    line << "mov " << *move->carrier_lane.register_name << ", "
         << move->abi_register.register_name;
  }
  lines.push_back(line.str());
  return std::nullopt;
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

std::optional<std::string> lane_register_name(const I128LaneTransportRecord& lane) {
  if (!lane.reg.has_value()) {
    return std::nullopt;
  }
  return register_name(*lane.reg);
}

std::optional<std::string> pair_low_register_name(const I128PairOperandRecord& operand) {
  return lane_register_name(operand.low_lane);
}

std::optional<std::string> pair_high_register_name(const I128PairOperandRecord& operand) {
  return lane_register_name(operand.high_lane);
}

std::string relocation_operand(std::string_view label, std::int64_t byte_offset) {
  std::string operand(label);
  if (byte_offset > 0) {
    operand += "+";
    operand += std::to_string(byte_offset);
  } else if (byte_offset < 0) {
    operand += std::to_string(byte_offset);
  }
  return operand;
}

std::string prefixed_relocation_operand(std::string_view prefix, std::string_view label) {
  std::string operand(prefix);
  operand += label;
  return operand;
}

std::string tls_relocation_prefix(prepare::PreparedTlsRelocationKind kind) {
  switch (kind) {
    case prepare::PreparedTlsRelocationKind::Aarch64TprelHi12:
      return ":tprel_hi12:";
    case prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc:
      return ":tprel_lo12_nc:";
    case prepare::PreparedTlsRelocationKind::None:
      return {};
  }
  return {};
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

mir::TargetInstructionPrintResult print_address_materialization(
    const InstructionRecord& instruction,
    const AddressMaterializationRecord& address) {
  if (!address.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "address materialization node is missing result register");
  }

  std::string_view label;
  switch (address.kind) {
    case AddressMaterializationKind::DirectPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "direct address materialization is missing symbol label");
      }
      break;
    case AddressMaterializationKind::GotPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "GOT address materialization is missing symbol label");
      }
      if (address.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
        return target_unsupported(bad_header(instruction) +
                                  "GOT address materialization is missing GOT-required policy");
      }
      break;
    case AddressMaterializationKind::StringConstant:
      label = address.text_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "string address materialization is missing text label");
      }
      break;
    case AddressMaterializationKind::LabelPageLow12:
      label = address.target_label_name;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "label address materialization is missing target label text");
      }
      break;
    case AddressMaterializationKind::TlsRelative:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "TLS address materialization is missing symbol label");
      }
      if (address.tls_model !=
              prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
          address.tls_thread_pointer_register !=
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0) {
        return target_unsupported(bad_header(instruction) +
                                  "TLS address materialization is missing local-exec TLS facts");
      }
      if (address.tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
          address.tls_low_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
        return target_unsupported(
            bad_header(instruction) +
            "TLS address materialization is missing thread-pointer-relative relocations");
      }
      break;
    case AddressMaterializationKind::DeferredUnsupported:
      return target_unsupported(bad_header(instruction) +
                                "address materialization printer path is deferred for " +
                                std::string(prepare::prepared_address_materialization_kind_name(
                                    address.prepared_kind)));
  }

  const std::string result = register_name(*address.result_register);
  if (address.kind == AddressMaterializationKind::GotPageLow12) {
    std::vector<std::string> lines{
        "adrp " + result + ", " + prefixed_relocation_operand(":got:", label),
        "ldr " + result + ", [" + result + ", " +
            prefixed_relocation_operand(":got_lo12:", label) + "]",
    };
    if (address.byte_offset != 0) {
      lines.push_back("add " + result + ", " + result + ", #" +
                      std::to_string(address.byte_offset));
    }
    return target_printed(std::move(lines));
  }
  if (address.kind == AddressMaterializationKind::TlsRelative) {
    std::vector<std::string> lines{
        "mrs " + result + ", tpidr_el0",
        "add " + result + ", " + result + ", " +
            prefixed_relocation_operand(tls_relocation_prefix(address.tls_high_relocation), label),
        "add " + result + ", " + result + ", " +
            prefixed_relocation_operand(tls_relocation_prefix(address.tls_low_relocation), label),
    };
    if (address.byte_offset != 0) {
      lines.push_back("add " + result + ", " + result + ", #" +
                      std::to_string(address.byte_offset));
    }
    return target_printed(std::move(lines));
  }

  const std::string reloc = relocation_operand(label, address.byte_offset);
  return target_printed({
      "adrp " + result + ", " + reloc,
      "add " + result + ", " + result + ", :lo12:" + reloc,
  });
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
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "memory address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) + "memory address is not printable");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "memory mnemonic is not printable");
  }

  if (memory.memory_kind == MemoryInstructionKind::Load) {
    if (!memory.result_register.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "load node is missing a structured destination register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*memory.result_register) << ", " << address;
    return target_printed({out.str()});
  }
  if (!memory.value.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind != OperandKind::Register || value == nullptr) {
    return target_unsupported(bad_header(instruction) + "store value is not a register operand");
  }
  std::ostringstream out;
  out << mnemonic << " " << register_name(*value) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_i128_transport(
    const InstructionRecord& instruction,
    const I128TransportRecord& transport) {
  const auto low = lane_register_name(transport.low_lane);
  const auto high = lane_register_name(transport.high_lane);
  if (!low.has_value() || !high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 transport node is missing structured low/high registers");
  }
  if (transport.transport_kind == I128TransportKind::CarrierSnapshot) {
    std::ostringstream out;
    out << "// i128 carrier " << *low << ", " << *high;
    return target_printed({out.str()});
  }
  if (!transport.memory.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 memory transport is missing structured memory operand");
  }
  const auto address = memory_address(*transport.memory);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 memory transport address is not printable");
  }
  std::ostringstream out;
  if (transport.transport_kind == I128TransportKind::LoadFromMemory) {
    out << "ldp " << *low << ", " << *high << ", " << address;
    return target_printed({out.str()});
  }
  if (transport.transport_kind == I128TransportKind::StoreToMemory) {
    out << "stp " << *low << ", " << *high << ", " << address;
    return target_printed({out.str()});
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 transport kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_pair_operation(
    const InstructionRecord& instruction,
    const I128PairOperationRecord& pair) {
  const auto result_low = pair_low_register_name(pair.result);
  const auto result_high = pair_high_register_name(pair.result);
  const auto lhs_low = pair_low_register_name(pair.lhs);
  const auto lhs_high = pair_high_register_name(pair.lhs);
  const auto rhs_low = pair_low_register_name(pair.rhs);
  const auto rhs_high = pair_high_register_name(pair.rhs);
  if (!result_low.has_value() || !result_high.has_value() ||
      !lhs_low.has_value() || !lhs_high.has_value() ||
      !rhs_low.has_value() || !rhs_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 pair node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  auto emit_independent = [&](std::string_view mnemonic) {
    std::ostringstream low;
    low << mnemonic << " " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
    lines.push_back(low.str());
    std::ostringstream high;
    high << mnemonic << " " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
    lines.push_back(high.str());
  };

  switch (pair.operation) {
    case I128PairOperationKind::Add: {
      std::ostringstream low;
      low << "adds " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "adc " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128PairOperationKind::Sub: {
      std::ostringstream low;
      low << "subs " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "sbc " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128PairOperationKind::And:
      emit_independent("and");
      return target_printed(std::move(lines));
    case I128PairOperationKind::Or:
      emit_independent("orr");
      return target_printed(std::move(lines));
    case I128PairOperationKind::Xor:
      emit_independent("eor");
      return target_printed(std::move(lines));
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 pair operation is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_shift(const InstructionRecord& instruction,
                                                   const I128ShiftRecord& shift) {
  if (shift.count_kind != I128ShiftCountKind::Immediate) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift printer currently requires an immediate shift count");
  }
  const auto* immediate = std::get_if<ImmediateOperand>(&shift.shift_count.payload);
  if (shift.shift_count.kind != OperandKind::Immediate || immediate == nullptr ||
      immediate->signed_value < 0 || immediate->signed_value >= 64) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift immediate is outside the printable 0..63 subset");
  }
  const auto amount = immediate->signed_value;
  const auto result_low = pair_low_register_name(shift.result);
  const auto result_high = pair_high_register_name(shift.result);
  const auto source_low = pair_low_register_name(shift.source);
  const auto source_high = pair_high_register_name(shift.source);
  if (!result_low.has_value() || !result_high.has_value() ||
      !source_low.has_value() || !source_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  auto emit_mov_pair = [&]() {
    std::ostringstream low;
    low << "mov " << *result_low << ", " << *source_low;
    lines.push_back(low.str());
    std::ostringstream high;
    high << "mov " << *result_high << ", " << *source_high;
    lines.push_back(high.str());
  };
  if (amount == 0) {
    emit_mov_pair();
    return target_printed(std::move(lines));
  }

  switch (shift.shift_kind) {
    case I128ShiftKind::Left: {
      std::ostringstream low;
      low << "lsl " << *result_low << ", " << *source_low << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "extr " << *result_high << ", " << *source_high << ", " << *source_low
           << ", #" << (64 - amount);
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128ShiftKind::LogicalRight: {
      std::ostringstream low;
      low << "extr " << *result_low << ", " << *source_low << ", " << *source_high
          << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "lsr " << *result_high << ", " << *source_high << ", #" << amount;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128ShiftKind::ArithmeticRight: {
      std::ostringstream low;
      low << "extr " << *result_low << ", " << *source_low << ", " << *source_high
          << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "asr " << *result_high << ", " << *source_high << ", #" << amount;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 shift kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_compare(const InstructionRecord& instruction,
                                                     const I128CompareRecord& compare) {
  if (!compare.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 compare node is missing structured result register");
  }
  const auto result = register_name(*compare.result_register);
  const auto lhs_low = pair_low_register_name(compare.lhs);
  const auto lhs_high = pair_high_register_name(compare.lhs);
  const auto rhs_low = pair_low_register_name(compare.rhs);
  const auto rhs_high = pair_high_register_name(compare.rhs);
  if (!lhs_low.has_value() || !lhs_high.has_value() ||
      !rhs_low.has_value() || !rhs_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 compare node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  switch (compare.predicate) {
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne: {
      const auto condition = compare.predicate == bir::BinaryOpcode::Eq ? "eq" : "ne";
      {
        std::ostringstream high;
        high << "cmp " << *lhs_high << ", " << *rhs_high;
        lines.push_back(high.str());
      }
      {
        std::ostringstream low;
        low << "ccmp " << *lhs_low << ", " << *rhs_low << ", #0, eq";
        lines.push_back(low.str());
      }
      {
        std::ostringstream cset;
        cset << "cset " << result << ", " << condition;
        lines.push_back(cset.str());
      }
      return target_printed(std::move(lines));
    }
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
    default:
      return target_unsupported(
          bad_header(instruction) +
          "i128 compare printer supports equality and relational predicates only");
  }

  const auto signed_high =
      compare.predicate == bir::BinaryOpcode::Slt ||
      compare.predicate == bir::BinaryOpcode::Sle ||
      compare.predicate == bir::BinaryOpcode::Sgt ||
      compare.predicate == bir::BinaryOpcode::Sge;
  const auto greater_predicate =
      compare.predicate == bir::BinaryOpcode::Sgt ||
      compare.predicate == bir::BinaryOpcode::Sge ||
      compare.predicate == bir::BinaryOpcode::Ugt ||
      compare.predicate == bir::BinaryOpcode::Uge;
  const auto inclusive_predicate =
      compare.predicate == bir::BinaryOpcode::Sle ||
      compare.predicate == bir::BinaryOpcode::Sge ||
      compare.predicate == bir::BinaryOpcode::Ule ||
      compare.predicate == bir::BinaryOpcode::Uge;
  const auto high_true = greater_predicate ? "gt" : "lt";
  const auto high_false = greater_predicate ? "lt" : "gt";
  const auto unsigned_high_true = greater_predicate ? "hi" : "lo";
  const auto unsigned_high_false = greater_predicate ? "lo" : "hi";
  const auto low_true =
      greater_predicate ? (inclusive_predicate ? "hs" : "hi")
                        : (inclusive_predicate ? "ls" : "lo");
  const auto true_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                          std::to_string(compare.block_label) + "_" +
                          std::to_string(compare.instruction_index) + "_true";
  const auto false_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                           std::to_string(compare.block_label) + "_" +
                           std::to_string(compare.instruction_index) + "_false";
  const auto done_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                          std::to_string(compare.block_label) + "_" +
                          std::to_string(compare.instruction_index) + "_done";
  {
    std::ostringstream high;
    high << "cmp " << *lhs_high << ", " << *rhs_high;
    lines.push_back(high.str());
  }
  {
    std::ostringstream high_true_branch;
    high_true_branch << "b." << (signed_high ? high_true : unsigned_high_true) << " "
                     << true_label;
    lines.push_back(high_true_branch.str());
  }
  {
    std::ostringstream high_false_branch;
    high_false_branch << "b." << (signed_high ? high_false : unsigned_high_false) << " "
                      << false_label;
    lines.push_back(high_false_branch.str());
  }
  {
    std::ostringstream low;
    low << "cmp " << *lhs_low << ", " << *rhs_low;
    lines.push_back(low.str());
  }
  {
    std::ostringstream low_true_branch;
    low_true_branch << "b." << low_true << " " << true_label;
    lines.push_back(low_true_branch.str());
  }
  {
    std::ostringstream fallthrough;
    fallthrough << "b " << false_label;
    lines.push_back(fallthrough.str());
  }
  lines.push_back(true_label + ":");
  {
    std::ostringstream set_true;
    set_true << "mov " << result << ", #1";
    lines.push_back(set_true.str());
  }
  {
    std::ostringstream done;
    done << "b " << done_label;
    lines.push_back(done.str());
  }
  lines.push_back(false_label + ":");
  {
    std::ostringstream set_false;
    set_false << "mov " << result << ", #0";
    lines.push_back(set_false.str());
  }
  lines.push_back(done_label + ":");
  return target_printed(std::move(lines));
}

mir::TargetInstructionPrintResult print_i128_runtime_helper(
    const InstructionRecord& instruction,
    const I128RuntimeHelperBoundaryRecord& helper) {
  if (helper.source_helper == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing prepared helper provenance");
  }
  if (helper.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem ||
      helper.callee_name.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing div/rem callee facts");
  }
  if (helper.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary requires direct low/high result ownership");
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity ||
      helper.clobbered_registers.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing resource or clobber policy");
  }
  if (helper.abi_policy.transition !=
          prepare::PreparedI128RuntimeHelperAbiTransition::
              DirectRegisterPairArgumentsAndResult ||
      helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.argument_count != 2 ||
      helper.abi_policy.lanes_per_argument != 2 ||
      helper.abi_policy.result_lane_count != 2 ||
      helper.abi_policy.lane_width_bytes != 8) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing ABI/register-bank policy");
  }
  if (!has_complete_live_preservation(helper.live_preservation_policy) ||
      !has_complete_live_preservation(helper.source_helper->live_preservation_policy)) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary printing requires complete live-preservation facts");
  }
  if (!has_complete_selected_call_ownership(helper.selected_call_ownership) ||
      !has_complete_selected_call_ownership(helper.source_helper->selected_call_ownership)) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary printing requires selected-call ownership facts");
  }
  if (helper.source_helper->callee_name != helper.callee_name ||
      helper.source_helper->helper_family != helper.helper_family ||
      helper.source_helper->helper_kind != helper.helper_kind ||
      helper.source_helper->source_binary_opcode != helper.source_binary_opcode ||
      helper.source_helper->result_ownership != helper.result_ownership) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary source helper facts do not match selected record");
  }

  std::vector<std::string> lines;
  const auto append_move =
      [&](const std::optional<prepare::PreparedI128RuntimeHelper::MarshalingMove>& move,
          prepare::PreparedI128RuntimeHelperMarshalDirection direction,
          prepare::PreparedMovePhase phase) -> std::optional<std::string> {
    return append_i128_helper_move_line(lines, move, direction, phase);
  };

  const auto before_call =
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument;
  const auto after_call =
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane;
  if (const auto error = append_move(helper.source_helper->lhs_low_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->lhs_high_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->rhs_low_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->rhs_high_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  {
    std::ostringstream call;
    call << "bl " << helper.callee_name;
    lines.push_back(call.str());
  }
  if (const auto error = append_move(helper.source_helper->result_low_unmarshal_move,
                                     after_call,
                                     prepare::PreparedMovePhase::AfterCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->result_high_unmarshal_move,
                                     after_call,
                                     prepare::PreparedMovePhase::AfterCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }

  return target_printed(std::move(lines));
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

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaCopy}) {
    if (!call.variadic_va_copy.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "va_copy node is missing structured prepared va_copy provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "va_copy mnemonic is not printable");
    }

    const auto& va_copy = *call.variadic_va_copy;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_copy.destination_va_list)
          << " source=" << prepared_value_home_name(va_copy.source_va_list)
          << " va_list_size=" << va_copy.va_list_size_bytes
          << " va_list_align=" << va_copy.va_list_align_bytes
          << " scratch_registers=" << va_copy.scratch_register_count
          << " scratch_stack=" << va_copy.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_copy.field_copies) {
      std::ostringstream out;
      out << "va.copy.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " source_offset=" << field.source_offset_bytes
          << " destination_offset=" << field.destination_offset_bytes
          << " size=" << field.size_bytes;
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

mir::TargetInstructionPrintResult print_scalar_conversion(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    const ScalarCastRecord& cast) {
  if ((!cast.supported_float_integer_conversion &&
       !cast.supported_float_width_conversion) ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(bad_header(instruction) +
                              "scalar conversion node is outside the printable subset");
  }
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar conversion node requires a structured register source operand");
  }

  std::string_view mnemonic;
  std::optional<std::string> result;
  std::optional<std::string> source;
  switch (cast.operation) {
    case ScalarCastOperationKind::FloatExtend:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::D);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::S);
      break;
    case ScalarCastOperationKind::FloatTruncate:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::S);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::D);
      break;
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat: {
      const auto result_view = floating_register_view(cast.result_type);
      const auto source_bits = integer_type_bit_width(cast.source_type);
      if (!result_view.has_value() || !source_bits.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar int-to-float conversion has unsupported type width");
      }
      const auto source_view = integer_register_view(*source_bits);
      if (!source_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar int-to-float conversion has unsupported integer width");
      }
      mnemonic = cast.operation == ScalarCastOperationKind::SignedIntToFloat ? "scvtf" : "ucvtf";
      result = fp_register_name_with_view(*scalar.result_register, *result_view);
      source = register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt: {
      const auto result_bits = integer_type_bit_width(cast.result_type);
      const auto source_view = floating_register_view(cast.source_type);
      if (!result_bits.has_value() || !source_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar float-to-int conversion has unsupported type width");
      }
      const auto result_view = integer_register_view(*result_bits);
      if (!result_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar float-to-int conversion has unsupported integer width");
      }
      mnemonic = cast.operation == ScalarCastOperationKind::FloatToSignedInt ? "fcvtzs"
                                                                             : "fcvtzu";
      result = register_name_with_view(*scalar.result_register, *result_view);
      source = fp_register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::SignExtend:
    case ScalarCastOperationKind::ZeroExtend:
    case ScalarCastOperationKind::Truncate:
    case ScalarCastOperationKind::Deferred:
      return target_unsupported(bad_header(instruction) +
                                "scalar conversion node is outside the printable subset");
  }
  if (mnemonic.empty() || !result.has_value() || !source.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar conversion node has incomplete printable register facts");
  }
  std::ostringstream out;
  out << mnemonic << " " << *result << ", " << *source;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar(const InstructionRecord& instruction,
                                               const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar node is missing a structured destination register operand");
  }
  if (scalar.scalar_cast.has_value()) {
    const auto& cast = *scalar.scalar_cast;
    if (cast.supported_float_integer_conversion || cast.supported_float_width_conversion) {
      return print_scalar_conversion(instruction, scalar, cast);
    }
    if (!cast.supported_simple_integer_cast ||
        cast.operation == ScalarCastOperationKind::Deferred) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node is outside the printable simple integer subset");
    }
    const auto source_bits = integer_type_bit_width(cast.source_type);
    const auto result_bits = integer_type_bit_width(cast.result_type);
    if (!source_bits.has_value() || !result_bits.has_value() ||
        ((*source_bits >= *result_bits) &&
         cast.operation != ScalarCastOperationKind::Truncate) ||
        ((*source_bits <= *result_bits) &&
         cast.operation == ScalarCastOperationKind::Truncate)) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node requires a supported integer source/result width");
    }
    const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
    if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node requires a structured register source operand");
    }
    const auto result_view = integer_register_view(*result_bits);
    if (!result_view.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node has an unsupported result register width");
    }
    const auto result = register_name_with_view(*scalar.result_register, *result_view);
    if (!result.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node destination is not a printable GPR register");
    }

    std::ostringstream out;
    switch (cast.operation) {
      case ScalarCastOperationKind::SignExtend: {
        if (*source_bits == 1U) {
          const auto source = register_name_with_view(
              *source_register,
              *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
          if (!source.has_value()) {
            return target_unsupported(
                bad_header(instruction) +
                "scalar sign-extend node source is not a printable GPR register");
          }
          out << "sbfx " << *result << ", " << *source << ", #0, #1";
        } else {
          std::string_view mnemonic;
          if (*source_bits == 8U) {
            mnemonic = "sxtb";
          } else if (*source_bits == 16U) {
            mnemonic = "sxth";
          } else if (*source_bits == 32U && *result_bits == 64U) {
            mnemonic = "sxtw";
          } else {
            return target_unsupported(bad_header(instruction) +
                                      "scalar sign-extend node has no printable width form");
          }
          const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
          if (!source.has_value()) {
            return target_unsupported(
                bad_header(instruction) +
                "scalar sign-extend node source is not a printable GPR register");
          }
          out << mnemonic << " " << *result << ", " << *source;
        }
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::ZeroExtend: {
        const auto source = register_name_with_view(
            *source_register,
            *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
        if (!source.has_value()) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar zero-extend node source is not a printable GPR register");
        }
        out << "ubfx " << *result << ", " << *source << ", #0, #" << *source_bits;
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::Truncate: {
        if (*result_bits > 32U) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar truncate node has no printable width form");
        }
        const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
        if (!source.has_value()) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar truncate node source is not a printable GPR register");
        }
        if (*result_bits == 32U) {
          out << "mov " << *result << ", " << *source;
        } else {
          const std::uint64_t mask = (std::uint64_t{1} << *result_bits) - 1U;
          out << "and " << *result << ", " << *source << ", #" << mask;
        }
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::Deferred:
        break;
    }
    return target_unsupported(bad_header(instruction) +
                              "scalar cast node is outside the printable simple integer subset");
  }
  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_floating_operation) {
    const auto& alu = *scalar.scalar_alu;
    if (scalar.inputs.size() != 2) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar FP node requires exactly two structured register operands");
    }
    const auto result_view = floating_register_view(alu.result_type);
    const auto operand_view = floating_register_view(alu.operand_type);
    if (!result_view.has_value() || !operand_view.has_value() ||
        result_view != operand_view) {
      return target_unsupported(bad_header(instruction) +
                                "scalar FP node requires matching F32/F64 operand and result widths");
    }
    const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
    if (scalar.inputs[0].kind != OperandKind::Register ||
        scalar.inputs[1].kind != OperandKind::Register ||
        lhs_register == nullptr || rhs_register == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar FP node requires structured FP/SIMD register operands");
    }
    const auto mnemonic = floating_alu_mnemonic(alu.operation);
    const auto result = fp_register_name_with_view(*scalar.result_register, *result_view);
    const auto lhs = fp_register_name_with_view(*lhs_register, *operand_view);
    const auto rhs = fp_register_name_with_view(*rhs_register, *operand_view);
    if (mnemonic.empty() || !result.has_value() || !lhs.has_value() || !rhs.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar FP node has incomplete printable register facts");
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *lhs << ", " << *rhs;
    return target_printed({out.str()});
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
  if (const auto* address = std::get_if<AddressMaterializationRecord>(&instruction.payload)) {
    return print_address_materialization(instruction, *address);
  }
  if (const auto* transport = std::get_if<I128TransportRecord>(&instruction.payload)) {
    return print_i128_transport(instruction, *transport);
  }
  if (const auto* pair = std::get_if<I128PairOperationRecord>(&instruction.payload)) {
    return print_i128_pair_operation(instruction, *pair);
  }
  if (const auto* shift = std::get_if<I128ShiftRecord>(&instruction.payload)) {
    return print_i128_shift(instruction, *shift);
  }
  if (const auto* compare = std::get_if<I128CompareRecord>(&instruction.payload)) {
    return print_i128_compare(instruction, *compare);
  }
  if (const auto* helper =
          std::get_if<I128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_i128_runtime_helper(instruction, *helper);
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
