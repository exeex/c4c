#include "i128_ops.hpp"

#include "calls.hpp"

#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

MachineEffectResource effect_from_operand(const OperandRecord& operand) {
  MachineEffectResource resource;
  resource.operand = operand;
  switch (operand.kind) {
    case OperandKind::Register: {
      const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Register;
      if (reg != nullptr) {
        resource.value_id = reg->value_id;
        resource.value_name = reg->value_name;
        resource.reg = reg->reg;
      }
      break;
    }
    case OperandKind::Immediate: {
      const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (immediate != nullptr) {
        resource.value_id = immediate->source_value_id;
        resource.value_name = immediate->source_value_name;
      }
      break;
    }
    case OperandKind::PreparedValue: {
      const auto* value = std::get_if<PreparedValueOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (value != nullptr) {
        resource.value_id = value->value_id;
        resource.value_name = value->value_name;
      }
      break;
    }
    case OperandKind::FrameSlot: {
      const auto* slot = std::get_if<FrameSlotOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::FrameSlot;
      if (slot != nullptr) {
        resource.frame_slot_id = slot->slot_id;
        if (slot->value_name.has_value()) {
          resource.value_name = *slot->value_name;
        }
      }
      break;
    }
    case OperandKind::Symbol: {
      const auto* symbol = std::get_if<SymbolOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Symbol;
      if (symbol != nullptr) {
        resource.symbol_name = symbol->link_name;
      }
      break;
    }
    case OperandKind::BranchTarget: {
      const auto* target = std::get_if<BranchTargetOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::BranchTarget;
      if (target != nullptr) {
        resource.value_id = target->condition_value_id;
        resource.block_label = target->block_label;
      }
      break;
    }
    case OperandKind::Memory: {
      const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Memory;
      if (memory != nullptr) {
        resource.value_id = memory->result_value_id.has_value() ? memory->result_value_id
                                                                : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name = memory->symbol_name.has_value() ? memory->symbol_name
                                                               : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

MachineEffectResource prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

}  // namespace

std::string_view i128_transport_kind_name(I128TransportKind kind) {
  switch (kind) {
    case I128TransportKind::CarrierSnapshot:
      return "carrier_snapshot";
    case I128TransportKind::LoadFromMemory:
      return "load_from_memory";
    case I128TransportKind::StoreToMemory:
      return "store_to_memory";
    case I128TransportKind::CopyRegisterPair:
      return "copy_register_pair";
  }
  return "unknown";
}

std::string_view prepared_i128_transport_record_error_name(
    PreparedI128TransportRecordError error) {
  switch (error) {
    case PreparedI128TransportRecordError::None:
      return "none";
    case PreparedI128TransportRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128TransportRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128TransportRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128TransportRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128TransportRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128TransportRecordError::MissingMemoryOperand:
      return "missing_memory_operand";
    case PreparedI128TransportRecordError::MemoryAccessSizeMismatch:
      return "memory_access_size_mismatch";
  }
  return "unknown";
}

std::string_view i128_pair_operation_kind_name(I128PairOperationKind kind) {
  switch (kind) {
    case I128PairOperationKind::Add:
      return "add";
    case I128PairOperationKind::Sub:
      return "sub";
    case I128PairOperationKind::And:
      return "and";
    case I128PairOperationKind::Or:
      return "or";
    case I128PairOperationKind::Xor:
      return "xor";
  }
  return "unknown";
}

std::string_view i128_pair_lane_semantics_name(
    I128PairLaneSemantics semantics) {
  switch (semantics) {
    case I128PairLaneSemantics::CarryPropagating:
      return "carry_propagating";
    case I128PairLaneSemantics::BorrowPropagating:
      return "borrow_propagating";
    case I128PairLaneSemantics::IndependentBitwise:
      return "independent_bitwise";
  }
  return "unknown";
}

std::string_view i128_shift_kind_name(I128ShiftKind kind) {
  switch (kind) {
    case I128ShiftKind::Left:
      return "left";
    case I128ShiftKind::LogicalRight:
      return "logical_right";
    case I128ShiftKind::ArithmeticRight:
      return "arithmetic_right";
  }
  return "unknown";
}

std::string_view i128_shift_lane_semantics_name(
    I128ShiftLaneSemantics semantics) {
  switch (semantics) {
    case I128ShiftLaneSemantics::CrossLaneLeft:
      return "cross_lane_left";
    case I128ShiftLaneSemantics::CrossLaneLogicalRight:
      return "cross_lane_logical_right";
    case I128ShiftLaneSemantics::CrossLaneArithmeticRight:
      return "cross_lane_arithmetic_right";
  }
  return "unknown";
}

std::string_view i128_shift_count_kind_name(I128ShiftCountKind kind) {
  switch (kind) {
    case I128ShiftCountKind::Immediate:
      return "immediate";
    case I128ShiftCountKind::Register:
      return "register";
  }
  return "unknown";
}

std::string_view i128_compare_signedness_name(
    I128CompareSignedness signedness) {
  switch (signedness) {
    case I128CompareSignedness::Equality:
      return "equality";
    case I128CompareSignedness::Signed:
      return "signed";
    case I128CompareSignedness::Unsigned:
      return "unsigned";
  }
  return "unknown";
}

std::string_view i128_compare_high_word_semantics_name(
    I128CompareHighWordSemantics semantics) {
  switch (semantics) {
    case I128CompareHighWordSemantics::EqualityBothLanes:
      return "equality_both_lanes";
    case I128CompareHighWordSemantics::SignedHighWordFirst:
      return "signed_high_word_first";
    case I128CompareHighWordSemantics::UnsignedHighWordFirst:
      return "unsigned_high_word_first";
  }
  return "unknown";
}

std::string_view i128_runtime_helper_boundary_kind_name(
    I128RuntimeHelperBoundaryKind kind) {
  switch (kind) {
    case I128RuntimeHelperBoundaryKind::SignedDiv:
      return "signed_div";
    case I128RuntimeHelperBoundaryKind::UnsignedDiv:
      return "unsigned_div";
    case I128RuntimeHelperBoundaryKind::SignedRem:
      return "signed_rem";
    case I128RuntimeHelperBoundaryKind::UnsignedRem:
      return "unsigned_rem";
  }
  return "unknown";
}

std::string_view prepared_i128_pair_record_error_name(
    PreparedI128PairRecordError error) {
  switch (error) {
    case PreparedI128PairRecordError::None:
      return "none";
    case PreparedI128PairRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128PairRecordError::UnsupportedOpcode:
      return "unsupported_opcode";
    case PreparedI128PairRecordError::UnsupportedOperandType:
      return "unsupported_operand_type";
    case PreparedI128PairRecordError::UnsupportedResultValue:
      return "unsupported_result_value";
    case PreparedI128PairRecordError::UnsupportedOperandValue:
      return "unsupported_operand_value";
    case PreparedI128PairRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128PairRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128PairRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128PairRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128PairRecordError::MissingScalarResultValueHome:
      return "missing_scalar_result_value_home";
    case PreparedI128PairRecordError::MissingScalarResultStorage:
      return "missing_scalar_result_storage";
    case PreparedI128PairRecordError::UnsupportedScalarResultStorage:
      return "unsupported_scalar_result_storage";
    case PreparedI128PairRecordError::UnsupportedShiftCount:
      return "unsupported_shift_count";
    case PreparedI128PairRecordError::MissingShiftCountStorage:
      return "missing_shift_count_storage";
  }
  return "unknown";
}

std::string_view prepared_i128_runtime_helper_record_error_name(
    PreparedI128RuntimeHelperRecordError error) {
  switch (error) {
    case PreparedI128RuntimeHelperRecordError::None:
      return "none";
    case PreparedI128RuntimeHelperRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper:
      return "missing_prepared_i128_runtime_helper";
    case PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper:
      return "incomplete_prepared_i128_runtime_helper";
    case PreparedI128RuntimeHelperRecordError::UnsupportedHelperFamily:
      return "unsupported_helper_family";
    case PreparedI128RuntimeHelperRecordError::UnsupportedSourceOperation:
      return "unsupported_source_operation";
    case PreparedI128RuntimeHelperRecordError::UnsupportedResultOwnership:
      return "unsupported_result_ownership";
    case PreparedI128RuntimeHelperRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128RuntimeHelperRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128RuntimeHelperRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128RuntimeHelperRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128RuntimeHelperRecordError::MissingBoundaryResourcePolicy:
      return "missing_boundary_resource_policy";
    case PreparedI128RuntimeHelperRecordError::MissingBoundaryAbiPolicy:
      return "missing_boundary_abi_policy";
    case PreparedI128RuntimeHelperRecordError::MissingClobberPolicy:
      return "missing_clobber_policy";
  }
  return "unknown";
}

MachineNodeStatusRecord i128_transport_selection_status(
    const I128TransportRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport is missing prepared i128 carrier provenance"};
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::Missing) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport carrier is missing complete low/high authority"};
  }
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport carrier has invalid size, lane width, or alignment"};
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
      (!instruction.low_lane.reg.has_value() || !instruction.high_lane.reg.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 register-pair transport is missing low/high registers"};
  }
  if (instruction.transport_kind == I128TransportKind::CopyRegisterPair) {
    if (instruction.carrier_kind != prepare::PreparedI128CarrierKind::RegisterPair ||
        instruction.copy_source_carrier == nullptr ||
        !instruction.source_value_id.has_value() ||
        instruction.source_value_name == c4c::kInvalidValueName ||
        !instruction.source_low_lane.reg.has_value() ||
        !instruction.source_high_lane.reg.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "i128 copy transport is missing structured source low/high registers"};
    }
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::MemoryBacked &&
      (!instruction.low_lane.slot_id.has_value() ||
       !instruction.low_lane.stack_offset_bytes.has_value() ||
       !instruction.high_lane.slot_id.has_value() ||
       !instruction.high_lane.stack_offset_bytes.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 memory-backed transport is missing low/high memory offsets"};
  }
  if ((instruction.transport_kind == I128TransportKind::LoadFromMemory ||
       instruction.transport_kind == I128TransportKind::StoreToMemory) &&
      !instruction.memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 memory transport is missing structured memory operand"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_transport_instruction(I128TransportRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.memory.has_value()) {
    operands.push_back(make_memory_operand(*instruction.memory));
  }
  if (instruction.low_lane.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.low_lane.reg));
  }
  if (instruction.high_lane.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.high_lane.reg));
  }
  if (instruction.transport_kind == I128TransportKind::CopyRegisterPair) {
    if (instruction.source_low_lane.reg.has_value()) {
      operands.push_back(make_register_operand(*instruction.source_low_lane.reg));
    }
    if (instruction.source_high_lane.reg.has_value()) {
      operands.push_back(make_register_operand(*instruction.source_high_lane.reg));
    }
  }

  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.transport_kind == I128TransportKind::LoadFromMemory ||
      instruction.transport_kind == I128TransportKind::CarrierSnapshot ||
      instruction.transport_kind == I128TransportKind::CopyRegisterPair) {
    if (instruction.low_lane.reg.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.low_lane.reg)));
    }
    if (instruction.high_lane.reg.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.high_lane.reg)));
    }
    if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::MemoryBacked) {
      defs.push_back(prepared_value_def(instruction.value_id, instruction.value_name));
    }
  }
  if (instruction.transport_kind == I128TransportKind::StoreToMemory ||
      instruction.transport_kind == I128TransportKind::CarrierSnapshot) {
    if (instruction.low_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.low_lane.reg)));
    }
    if (instruction.high_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.high_lane.reg)));
    }
  }
  if (instruction.transport_kind == I128TransportKind::CopyRegisterPair) {
    if (instruction.source_low_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.source_low_lane.reg)));
    }
    if (instruction.source_high_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.source_high_lane.reg)));
    }
  }
  if (instruction.memory.has_value()) {
    auto memory_effect = effect_from_operand(make_memory_operand(*instruction.memory));
    if (instruction.transport_kind == I128TransportKind::LoadFromMemory) {
      uses.push_back(std::move(memory_effect));
    } else if (instruction.transport_kind == I128TransportKind::StoreToMemory) {
      defs.push_back(std::move(memory_effect));
    }
  }

  std::vector<MachineSideEffectKind> side_effects;
  if (instruction.transport_kind == I128TransportKind::LoadFromMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
  } else if (instruction.transport_kind == I128TransportKind::StoreToMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }

  return InstructionRecord{
      .family = InstructionFamily::I128Transport,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Transport,
      .selection = i128_transport_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_pair_selection_status(
    const I128PairOperationRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation is missing result register-pair carrier"};
  }
  if (!has_pair_registers(instruction.lhs) || !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation is missing source register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_pair_operation_instruction(
    I128PairOperationRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Pair,
      .selection = i128_pair_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_shift_selection_status(
    const I128ShiftRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 shift has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result) ||
      !has_pair_registers(instruction.source)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 shift is missing source/result register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_shift_instruction(I128ShiftRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.source.low_lane.reg);
  add_use(instruction.source.high_lane.reg);
  operands.push_back(instruction.shift_count);
  uses.push_back(effect_from_operand(instruction.shift_count));

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Shift,
      .selection = i128_shift_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_compare_selection_status(
    const I128CompareRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I1) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare has invalid type, size, or alignment facts"};
  }
  if (!instruction.result_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare is missing scalar result register"};
  }
  if (!has_pair_registers(instruction.lhs) || !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare is missing source register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_compare_instruction(I128CompareRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.result_register.has_value()) {
    const auto result = make_register_operand(*instruction.result_register);
    operands.push_back(result);
    defs.push_back(effect_from_operand(result));
  }
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Compare,
      .selection = i128_compare_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_runtime_helper_boundary_selection_status(
    const I128RuntimeHelperBoundaryRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.source_helper == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing prepared helper provenance"};
  }
  if (instruction.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem ||
      instruction.callee_name.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing helper family or callee identity"};
  }
  if (instruction.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "i128 helper boundary requires direct low/high result ownership"};
  }
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.source_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result) ||
      !has_pair_registers(instruction.lhs) ||
      !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing low/high register-pair lanes"};
  }
  if (!instruction.resource_policy.call_boundary ||
      !instruction.resource_policy.runtime_helper_callee ||
      !instruction.resource_policy.caller_saved_clobbers ||
      !instruction.resource_policy.preserves_source_operation_identity) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing resource policy facts"};
  }
  if (instruction.abi_policy.transition !=
          prepare::PreparedI128RuntimeHelperAbiTransition::
              DirectRegisterPairArgumentsAndResult ||
      instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      instruction.abi_policy.argument_count != 2 ||
      instruction.abi_policy.lanes_per_argument != 2 ||
      instruction.abi_policy.result_lane_count != 2 ||
      instruction.abi_policy.lane_width_bytes != 8) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing ABI/register-bank policy facts"};
  }
  if (instruction.clobbered_registers.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing caller-saved clobber policy"};
  }
  if (!instruction.live_preservation_policy.evaluated ||
      !instruction.live_preservation_policy.caller_saved_clobbers_modeled ||
      !instruction.live_preservation_policy.no_additional_live_preservation_required) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing live-preservation policy"};
  }
  if (!instruction.selected_call_ownership.owns_terminal_call ||
      !instruction.selected_call_ownership.has_callee_identity ||
      !instruction.selected_call_ownership.has_resource_policy ||
      !instruction.selected_call_ownership.has_clobber_policy ||
      !instruction.selected_call_ownership.has_abi_bindings ||
      !instruction.selected_call_ownership.has_marshaling ||
      !instruction.selected_call_ownership.has_live_preservation) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing selected-call ownership policy"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_runtime_helper_boundary_instruction(
    I128RuntimeHelperBoundaryRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128RuntimeHelper,
      .selection = i128_runtime_helper_boundary_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .side_effects = {MachineSideEffectKind::Call},
      .payload = instruction,
  };
}

}  // namespace c4c::backend::aarch64::codegen
