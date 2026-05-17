#include "i128_ops.hpp"

#include "alu.hpp"
#include "calls.hpp"
#include "comparison.hpp"

#include <cstddef>
#include <optional>
#include <string>
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

void append_i128_pair_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    module::ModuleLoweringDiagnosticKind kind,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Scalar,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string i128_pair_error_message(
    PreparedI128PairRecordError error) {
  std::string message =
      "AArch64 i128 pair operation lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_pair_record_error_name(error);
  return message;
}

[[nodiscard]] std::string i128_transport_error_message(
    PreparedI128TransportRecordError error) {
  std::string message =
      "AArch64 i128 transport lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_transport_record_error_name(error);
  return message;
}

[[nodiscard]] std::string i128_runtime_helper_error_message(
    PreparedI128RuntimeHelperRecordError error) {
  std::string message =
      "AArch64 i128 runtime helper-boundary lowering requires prepared i128 helper facts";
  message += "; error=";
  message += prepared_i128_runtime_helper_record_error_name(error);
  return message;
}

[[nodiscard]] const prepare::PreparedI128RuntimeHelper* find_i128_runtime_helper_for_instruction(
    const prepare::PreparedI128RuntimeHelperFunction& helpers,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& helper : helpers.helpers) {
    if (helper.block_index == block_index &&
        helper.instruction_index == instruction_index) {
      return &helper;
    }
  }
  return nullptr;
}

[[nodiscard]] module::MachineInstruction make_i128_bir_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
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

PreparedI128TransportRecordResult i128_transport_record_error(
    PreparedI128TransportRecordError error) {
  return PreparedI128TransportRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

std::optional<RegisterOperand> make_i128_lane_register_operand(
    const prepare::PreparedI128Carrier& carrier,
    const prepare::PreparedI128LaneCarrier& lane) {
  if (!lane.register_name.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *lane.register_name, carrier.register_bank, carrier.register_class, abi::RegisterView::X);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {*lane.register_name},
  };
}

PreparedI128TransportRecordResult make_prepared_i128_carrier_transport_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    c4c::ValueNameId value_name,
    I128TransportKind transport_kind,
    std::optional<MemoryOperand> memory) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName) {
    return i128_transport_record_error(PreparedI128TransportRecordError::InvalidFunction);
  }
  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_name);
  if (carrier == nullptr) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::MissingPreparedI128Carrier);
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }
  if (carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }
  if (transport_kind != I128TransportKind::CarrierSnapshot &&
      transport_kind != I128TransportKind::CopyRegisterPair) {
    if (!memory.has_value()) {
      return i128_transport_record_error(PreparedI128TransportRecordError::MissingMemoryOperand);
    }
    if (memory->size_bytes != carrier->total_size_bytes) {
      return i128_transport_record_error(
          PreparedI128TransportRecordError::MemoryAccessSizeMismatch);
    }
  }

  I128TransportRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .transport_kind = transport_kind,
      .function_name = i128_carriers.function_name,
      .block_label = memory.has_value() ? memory->block_label : c4c::kInvalidBlockLabel,
      .instruction_index = memory.has_value() ? memory->instruction_index : 0,
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .value_type = bir::TypeKind::I128,
      .carrier_kind = carrier->kind,
      .lane_width_bytes = carrier->lane_width_bytes,
      .total_size_bytes = carrier->total_size_bytes,
      .total_align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .contiguous_width = carrier->contiguous_width,
      .occupied_register_names = carrier->occupied_register_names,
      .register_placement = carrier->register_placement,
      .slot_id = carrier->slot_id,
      .stack_offset_bytes = carrier->stack_offset_bytes,
      .low_lane =
          I128LaneTransportRecord{
              .role = carrier->low_lane.role,
              .lane_index = carrier->low_lane.lane_index,
              .width_bytes = carrier->low_lane.width_bytes,
              .slot_id = carrier->low_lane.slot_id,
              .stack_offset_bytes = carrier->low_lane.stack_offset_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = carrier->high_lane.role,
              .lane_index = carrier->high_lane.lane_index,
              .width_bytes = carrier->high_lane.width_bytes,
              .slot_id = carrier->high_lane.slot_id,
              .stack_offset_bytes = carrier->high_lane.stack_offset_bytes,
          },
      .memory = std::move(memory),
      .source_carrier = carrier,
  };

  switch (carrier->kind) {
    case prepare::PreparedI128CarrierKind::RegisterPair: {
      record.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
      record.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
      if (!record.low_lane.reg.has_value() || !record.high_lane.reg.has_value()) {
        return i128_transport_record_error(
            PreparedI128TransportRecordError::RegisterConversionFailed);
      }
      break;
    }
    case prepare::PreparedI128CarrierKind::MemoryBacked:
      if (!record.low_lane.slot_id.has_value() ||
          !record.high_lane.slot_id.has_value() ||
          !record.low_lane.stack_offset_bytes.has_value() ||
          !record.high_lane.stack_offset_bytes.has_value()) {
        return i128_transport_record_error(
            PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
      }
      break;
    case prepare::PreparedI128CarrierKind::Missing:
      return i128_transport_record_error(
          PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }

  return PreparedI128TransportRecordResult{
      .record = std::move(record),
      .error = PreparedI128TransportRecordError::None,
  };
}

PreparedI128TransportRecordResult make_prepared_i128_copy_transport_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::CastInst& cast) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName) {
    return i128_transport_record_error(PreparedI128TransportRecordError::InvalidFunction);
  }
  if (cast.opcode != bir::CastOpcode::Bitcast ||
      cast.result.type != bir::TypeKind::I128 ||
      cast.operand.type != bir::TypeKind::I128 ||
      cast.result.kind != bir::Value::Kind::Named ||
      cast.operand.kind != bir::Value::Kind::Named ||
      cast.result.name.empty() ||
      cast.operand.name.empty()) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::MissingPreparedI128Carrier);
  }

  const auto result_name = names.value_names.find(cast.result.name);
  const auto source_name = names.value_names.find(cast.operand.name);
  if (result_name == c4c::kInvalidValueName ||
      source_name == c4c::kInvalidValueName) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::MissingPreparedI128Carrier);
  }

  auto prepared = make_prepared_i128_carrier_transport_record(
      i128_carriers, result_name, I128TransportKind::CopyRegisterPair, std::nullopt);
  if (!prepared.record.has_value()) {
    return prepared;
  }

  const auto* source = prepare::find_prepared_i128_carrier(i128_carriers, source_name);
  if (source == nullptr) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::MissingPreparedI128Carrier);
  }
  if (!source->missing_required_facts.empty() ||
      source->kind != prepare::PreparedI128CarrierKind::RegisterPair ||
      source->total_size_bytes != 16 ||
      source->lane_width_bytes != 8) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }

  prepared.record->source_value_id = source->value_id;
  prepared.record->source_value_name = source->value_name;
  prepared.record->source_low_lane = I128LaneTransportRecord{
      .role = source->low_lane.role,
      .lane_index = source->low_lane.lane_index,
      .width_bytes = source->low_lane.width_bytes,
  };
  prepared.record->source_high_lane = I128LaneTransportRecord{
      .role = source->high_lane.role,
      .lane_index = source->high_lane.lane_index,
      .width_bytes = source->high_lane.width_bytes,
  };
  prepared.record->source_low_lane.reg =
      make_i128_lane_register_operand(*source, source->low_lane);
  prepared.record->source_high_lane.reg =
      make_i128_lane_register_operand(*source, source->high_lane);
  prepared.record->copy_source_carrier = source;
  if (!prepared.record->source_low_lane.reg.has_value() ||
      !prepared.record->source_high_lane.reg.has_value()) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::RegisterConversionFailed);
  }
  return prepared;
}

PreparedI128PairRecordResult i128_pair_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128PairRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

bool is_supported_i128_pair_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return true;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

I128PairOperationKind i128_pair_operation_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return I128PairOperationKind::Add;
    case bir::BinaryOpcode::Sub:
      return I128PairOperationKind::Sub;
    case bir::BinaryOpcode::And:
      return I128PairOperationKind::And;
    case bir::BinaryOpcode::Or:
      return I128PairOperationKind::Or;
    case bir::BinaryOpcode::Xor:
      return I128PairOperationKind::Xor;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128PairOperationKind::Add;
}

I128PairLaneSemantics i128_pair_lane_semantics_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return I128PairLaneSemantics::CarryPropagating;
    case bir::BinaryOpcode::Sub:
      return I128PairLaneSemantics::BorrowPropagating;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return I128PairLaneSemantics::IndependentBitwise;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128PairLaneSemantics::IndependentBitwise;
}

PreparedI128PairRecordError make_i128_pair_operand_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    c4c::ValueNameId value_name,
    I128PairOperandRecord& operand) {
  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_name);
  if (carrier == nullptr) {
    return PreparedI128PairRecordError::MissingPreparedI128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return PreparedI128PairRecordError::IncompletePreparedI128Carrier;
  }
  if (carrier->kind != prepare::PreparedI128CarrierKind::RegisterPair) {
    return PreparedI128PairRecordError::UnsupportedCarrierKind;
  }

  operand = I128PairOperandRecord{
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .carrier_kind = carrier->kind,
      .low_lane =
          I128LaneTransportRecord{
              .role = carrier->low_lane.role,
              .lane_index = carrier->low_lane.lane_index,
              .width_bytes = carrier->low_lane.width_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = carrier->high_lane.role,
              .lane_index = carrier->high_lane.lane_index,
              .width_bytes = carrier->high_lane.width_bytes,
          },
      .source_carrier = carrier,
  };
  operand.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
  operand.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
  if (!operand.low_lane.reg.has_value() || !operand.high_lane.reg.has_value()) {
    return PreparedI128PairRecordError::RegisterConversionFailed;
  }
  return PreparedI128PairRecordError::None;
}

PreparedI128PairRecordResult make_prepared_i128_pair_operation_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName) {
    return i128_pair_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I128) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_supported_i128_pair_opcode(binary.opcode)) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (binary.lhs.kind != bir::Value::Kind::Named || binary.rhs.kind != bir::Value::Kind::Named ||
      binary.lhs.name.empty() || binary.rhs.name.empty()) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }

  I128PairOperationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .operation = i128_pair_operation_from_binary_opcode(binary.opcode),
      .lane_semantics = i128_pair_lane_semantics_from_binary_opcode(binary.opcode),
      .source_binary_opcode = binary.opcode,
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };

  const auto result_name = names.value_names.find(binary.result.name);
  const auto lhs_name = names.value_names.find(binary.lhs.name);
  const auto rhs_name = names.value_names.find(binary.rhs.name);
  if (result_name == c4c::kInvalidValueName) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (lhs_name == c4c::kInvalidValueName || rhs_name == c4c::kInvalidValueName) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }

  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, result_name, record.result);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, lhs_name, record.lhs);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, rhs_name, record.rhs);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }

  return PreparedI128PairRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

PreparedI128ShiftRecordResult i128_shift_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128ShiftRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedI128CompareRecordResult i128_compare_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128CompareRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedI128RuntimeHelperRecordResult i128_runtime_helper_record_error(
    PreparedI128RuntimeHelperRecordError error) {
  return PreparedI128RuntimeHelperRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

bool is_i128_shift_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::Shl || opcode == bir::BinaryOpcode::LShr ||
         opcode == bir::BinaryOpcode::AShr;
}

I128ShiftKind i128_shift_kind_from_binary_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Shl:
      return I128ShiftKind::Left;
    case bir::BinaryOpcode::LShr:
      return I128ShiftKind::LogicalRight;
    case bir::BinaryOpcode::AShr:
      return I128ShiftKind::ArithmeticRight;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128ShiftKind::Left;
}

I128ShiftLaneSemantics i128_shift_lane_semantics_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Shl:
      return I128ShiftLaneSemantics::CrossLaneLeft;
    case bir::BinaryOpcode::LShr:
      return I128ShiftLaneSemantics::CrossLaneLogicalRight;
    case bir::BinaryOpcode::AShr:
      return I128ShiftLaneSemantics::CrossLaneArithmeticRight;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128ShiftLaneSemantics::CrossLaneLeft;
}

bool is_supported_i128_shift_count(const bir::Value& count) {
  if (count.kind != bir::Value::Kind::Immediate) {
    return scalar_register_view(count.type).has_value();
  }
  return count.immediate >= 0 && count.immediate < 128;
}

PreparedI128ShiftRecordResult make_prepared_i128_shift_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != i128_carriers.function_name ||
      storage_plan.function_name != i128_carriers.function_name) {
    return i128_shift_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I128) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_i128_shift_opcode(binary.opcode)) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.lhs.kind != bir::Value::Kind::Named ||
      binary.result.name.empty() || binary.lhs.name.empty()) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (!is_supported_i128_shift_count(binary.rhs)) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedShiftCount);
  }

  I128ShiftRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .shift_kind = i128_shift_kind_from_binary_opcode(binary.opcode),
      .lane_semantics = i128_shift_lane_semantics_from_binary_opcode(binary.opcode),
      .count_kind = binary.rhs.kind == bir::Value::Kind::Immediate
                        ? I128ShiftCountKind::Immediate
                        : I128ShiftCountKind::Register,
      .source_binary_opcode = binary.opcode,
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };
  const auto result_name = names.value_names.find(binary.result.name);
  const auto source_name = names.value_names.find(binary.lhs.name);
  if (result_name == c4c::kInvalidValueName || source_name == c4c::kInvalidValueName) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, result_name, record.result);
      error != PreparedI128PairRecordError::None) {
    return i128_shift_record_error(error);
  }
  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, source_name, record.source);
      error != PreparedI128PairRecordError::None) {
    return i128_shift_record_error(error);
  }
  OperandRecord count;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.rhs, count);
      error != PreparedScalarAluRecordError::None) {
    return i128_shift_record_error(PreparedI128PairRecordError::MissingShiftCountStorage);
  }
  record.shift_count = count;
  return PreparedI128ShiftRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

I128CompareSignedness i128_compare_signedness_from_predicate(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
      return I128CompareSignedness::Signed;
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return I128CompareSignedness::Unsigned;
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
      return I128CompareSignedness::Equality;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      break;
  }
  return I128CompareSignedness::Equality;
}

I128CompareHighWordSemantics i128_compare_high_word_semantics_from_predicate(
    bir::BinaryOpcode predicate) {
  switch (i128_compare_signedness_from_predicate(predicate)) {
    case I128CompareSignedness::Equality:
      return I128CompareHighWordSemantics::EqualityBothLanes;
    case I128CompareSignedness::Signed:
      return I128CompareHighWordSemantics::SignedHighWordFirst;
    case I128CompareSignedness::Unsigned:
      return I128CompareHighWordSemantics::UnsignedHighWordFirst;
  }
  return I128CompareHighWordSemantics::EqualityBothLanes;
}

PreparedI128CompareRecordResult make_prepared_i128_compare_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != i128_carriers.function_name ||
      storage_plan.function_name != i128_carriers.function_name) {
    return i128_compare_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I1) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_compare_predicate(binary.opcode)) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (binary.lhs.kind != bir::Value::Kind::Named || binary.rhs.kind != bir::Value::Kind::Named ||
      binary.lhs.name.empty() || binary.rhs.name.empty()) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  RegisterOperand result_register;
  switch (make_prepared_scalar_result_register_operand(
      names, value_locations, storage_plan, binary.result, result_register)) {
    case PreparedScalarAluRecordError::None:
      break;
    case PreparedScalarAluRecordError::MissingResultValueHome:
      return i128_compare_record_error(
          PreparedI128PairRecordError::MissingScalarResultValueHome);
    case PreparedScalarAluRecordError::MissingResultStorage:
      return i128_compare_record_error(
          PreparedI128PairRecordError::MissingScalarResultStorage);
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return i128_compare_record_error(
          PreparedI128PairRecordError::UnsupportedScalarResultStorage);
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return i128_compare_record_error(
          PreparedI128PairRecordError::RegisterConversionFailed);
    case PreparedScalarAluRecordError::UnsupportedResultValue:
    case PreparedScalarAluRecordError::UnsupportedOperandType:
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
    case PreparedScalarAluRecordError::MissingOperandValueHome:
    case PreparedScalarAluRecordError::MissingOperandStorage:
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
    case PreparedScalarAluRecordError::InvalidFunction:
    case PreparedScalarAluRecordError::UnsupportedOpcode:
      return i128_compare_record_error(
          PreparedI128PairRecordError::UnsupportedResultValue);
  }

  I128CompareRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .predicate = binary.opcode,
      .signedness = i128_compare_signedness_from_predicate(binary.opcode),
      .high_word_semantics =
          i128_compare_high_word_semantics_from_predicate(binary.opcode),
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .result_value_id = result_register.value_id,
      .result_value_name = result_register.value_name,
      .result_register = result_register,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };
  const auto lhs_name = names.value_names.find(binary.lhs.name);
  const auto rhs_name = names.value_names.find(binary.rhs.name);
  if (lhs_name == c4c::kInvalidValueName || rhs_name == c4c::kInvalidValueName) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, lhs_name, record.lhs);
      error != PreparedI128PairRecordError::None) {
    return i128_compare_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, rhs_name, record.rhs);
      error != PreparedI128PairRecordError::None) {
    return i128_compare_record_error(error);
  }
  return PreparedI128CompareRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

bool is_i128_div_rem_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::SDiv || opcode == bir::BinaryOpcode::UDiv ||
         opcode == bir::BinaryOpcode::SRem || opcode == bir::BinaryOpcode::URem;
}

I128RuntimeHelperBoundaryKind i128_runtime_helper_boundary_kind_from_prepared(
    prepare::PreparedI128RuntimeHelperKind kind) {
  switch (kind) {
    case prepare::PreparedI128RuntimeHelperKind::SignedDiv:
      return I128RuntimeHelperBoundaryKind::SignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedDiv:
      return I128RuntimeHelperBoundaryKind::UnsignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::SignedRem:
      return I128RuntimeHelperBoundaryKind::SignedRem;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedRem:
      return I128RuntimeHelperBoundaryKind::UnsignedRem;
  }
  return I128RuntimeHelperBoundaryKind::SignedDiv;
}

PreparedI128RuntimeHelperRecordError make_i128_helper_operand_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const std::optional<prepare::PreparedI128RuntimeHelper::LaneBinding>& low,
    const std::optional<prepare::PreparedI128RuntimeHelper::LaneBinding>& high,
    I128PairOperandRecord& operand) {
  if (!low.has_value() || !high.has_value()) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }
  if (low->value_id != value_id || high->value_id != value_id ||
      low->value_name != value_name || high->value_name != value_name ||
      low->role != prepare::PreparedI128LaneRole::Low ||
      high->role != prepare::PreparedI128LaneRole::High ||
      low->lane_index != 0 || high->lane_index != 1 ||
      low->width_bytes != 8 || high->width_bytes != 8) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }

  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_id);
  if (carrier == nullptr || carrier->value_name != value_name) {
    return PreparedI128RuntimeHelperRecordError::MissingPreparedI128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128Carrier;
  }
  if (carrier->kind != prepare::PreparedI128CarrierKind::RegisterPair) {
    return PreparedI128RuntimeHelperRecordError::UnsupportedCarrierKind;
  }
  if (carrier->low_lane.register_name != low->register_name ||
      carrier->high_lane.register_name != high->register_name) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }

  operand = I128PairOperandRecord{
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .carrier_kind = carrier->kind,
      .low_lane =
          I128LaneTransportRecord{
              .role = low->role,
              .lane_index = low->lane_index,
              .width_bytes = low->width_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = high->role,
              .lane_index = high->lane_index,
              .width_bytes = high->width_bytes,
          },
      .source_carrier = carrier,
  };
  operand.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
  operand.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
  if (!operand.low_lane.reg.has_value() || !operand.high_lane.reg.has_value()) {
    return PreparedI128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedI128RuntimeHelperRecordError::None;
}

PreparedI128RuntimeHelperRecordResult make_prepared_i128_runtime_helper_boundary_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const prepare::PreparedI128RuntimeHelper& helper) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      helper.function_name != i128_carriers.function_name) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::InvalidFunction);
  }
  if (helper.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedHelperFamily);
  }
  if (!is_i128_div_rem_opcode(helper.source_binary_opcode) ||
      helper.source_type != bir::TypeKind::I128 ||
      helper.result_type != bir::TypeKind::I128 ||
      helper.callee_name.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  if (!helper.missing_required_facts.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper);
  }
  if (helper.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedResultOwnership);
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingBoundaryResourcePolicy);
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
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingBoundaryAbiPolicy);
  }
  if (helper.clobbered_registers.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingClobberPolicy);
  }
  if (!helper.live_preservation_policy.evaluated ||
      !helper.live_preservation_policy.caller_saved_clobbers_modeled ||
      !helper.live_preservation_policy.no_additional_live_preservation_required ||
      !helper.selected_call_ownership.owns_terminal_call ||
      !helper.selected_call_ownership.has_callee_identity ||
      !helper.selected_call_ownership.has_resource_policy ||
      !helper.selected_call_ownership.has_clobber_policy ||
      !helper.selected_call_ownership.has_abi_bindings ||
      !helper.selected_call_ownership.has_marshaling ||
      !helper.selected_call_ownership.has_live_preservation) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper);
  }

  I128RuntimeHelperBoundaryRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .boundary_kind = i128_runtime_helper_boundary_kind_from_prepared(helper.helper_kind),
      .helper_family = helper.helper_family,
      .helper_kind = helper.helper_kind,
      .callee_name = helper.callee_name,
      .source_binary_opcode = helper.source_binary_opcode,
      .function_name = helper.function_name,
      .block_index = helper.block_index,
      .instruction_index = helper.instruction_index,
      .source_type = helper.source_type,
      .result_type = helper.result_type,
      .result_value_id = helper.result_value_id,
      .result_value_name = helper.result_value_name,
      .lhs_value_id = helper.lhs_value_id,
      .lhs_value_name = helper.lhs_value_name,
      .rhs_value_id = helper.rhs_value_id,
      .rhs_value_name = helper.rhs_value_name,
      .result_ownership = helper.result_ownership,
      .lane_width_bytes = helper.abi_policy.lane_width_bytes,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .resource_policy = helper.resource_policy,
      .abi_policy = helper.abi_policy,
      .live_preservation_policy = helper.live_preservation_policy,
      .selected_call_ownership = helper.selected_call_ownership,
      .clobbered_registers = helper.clobbered_registers,
      .source_helper = &helper,
  };
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.result_value_id,
                                          helper.result_value_name,
                                          helper.result_low_lane,
                                          helper.result_high_lane,
                                          record.result);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.lhs_value_id,
                                          helper.lhs_value_name,
                                          helper.lhs_low_lane,
                                          helper.lhs_high_lane,
                                          record.lhs);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.rhs_value_id,
                                          helper.rhs_value_name,
                                          helper.rhs_low_lane,
                                          helper.rhs_high_lane,
                                          record.rhs);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  return PreparedI128RuntimeHelperRecordResult{
      .record = std::move(record),
      .error = PreparedI128RuntimeHelperRecordError::None,
  };
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

I128InstructionLoweringResult lower_i128_pair_operation_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      (binary->operand_type != bir::TypeKind::I128 &&
       binary->result.type != bir::TypeKind::I128)) {
    return I128InstructionLoweringResult{.handled = false};
  }

  if (is_compare_predicate(binary->opcode)) {
    return I128InstructionLoweringResult{
        .handled = true,
        .instruction = lower_prepared_i128_compare_instruction(
            context, *binary, instruction_index, diagnostics),
    };
  }

  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return I128InstructionLoweringResult{.handled = true};
  }
  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return I128InstructionLoweringResult{.handled = true};
  }

  InstructionRecord target;
  if (binary->opcode == bir::BinaryOpcode::Shl ||
      binary->opcode == bir::BinaryOpcode::LShr ||
      binary->opcode == bir::BinaryOpcode::AShr) {
    if (context.function.value_locations == nullptr ||
        context.function.storage_plan == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(PreparedI128PairRecordError::MissingShiftCountStorage));
      return I128InstructionLoweringResult{.handled = true};
    }
    auto prepared = make_prepared_i128_shift_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *i128_carriers,
        *binary);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(prepared.error));
      return I128InstructionLoweringResult{.handled = true};
    }
    target = make_i128_shift_instruction(*prepared.record);
  } else if (is_i128_div_rem_opcode(binary->opcode)) {
    const auto* helper_function =
        prepare::find_prepared_i128_runtime_helpers(*context.function.prepared,
                                                    context.function.control_flow->function_name);
    if (helper_function == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(
              PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper));
      return I128InstructionLoweringResult{.handled = true};
    }
    const auto* helper =
        find_i128_runtime_helper_for_instruction(
            *helper_function, context.block_index, instruction_index);
    if (helper == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(
              PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper));
      return I128InstructionLoweringResult{.handled = true};
    }
    auto prepared =
        make_prepared_i128_runtime_helper_boundary_record(*i128_carriers, *helper);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(prepared.error));
      return I128InstructionLoweringResult{.handled = true};
    }
    target = make_i128_runtime_helper_boundary_instruction(*prepared.record);
  } else {
    auto prepared = make_prepared_i128_pair_operation_record(
        context.function.prepared->names,
        *i128_carriers,
        *binary);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(prepared.error));
      return I128InstructionLoweringResult{.handled = true};
    }
    target = make_i128_pair_operation_instruction(*prepared.record);
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<I128PairOperationRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  } else if (auto* record = std::get_if<I128ShiftRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  } else if (auto* record = std::get_if<I128RuntimeHelperBoundaryRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->block_index = context.block_index;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_pair_diagnostic(diagnostics,
                                module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                context,
                                instruction_index,
                                std::string{target.selection.diagnostic});
    return I128InstructionLoweringResult{.handled = true};
  }

  return I128InstructionLoweringResult{
      .handled = true,
      .instruction =
          make_i128_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

I128InstructionLoweringResult lower_i128_copy_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  if (cast == nullptr ||
      cast->opcode != bir::CastOpcode::Bitcast ||
      cast->result.type != bir::TypeKind::I128 ||
      cast->operand.type != bir::TypeKind::I128) {
    return I128InstructionLoweringResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return I128InstructionLoweringResult{.handled = true};
  }

  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return I128InstructionLoweringResult{.handled = true};
  }

  auto prepared = make_prepared_i128_copy_transport_record(
      context.function.prepared->names, *i128_carriers, *cast);
  if (!prepared.record.has_value()) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(prepared.error));
    return I128InstructionLoweringResult{.handled = true};
  }

  InstructionRecord target = make_i128_transport_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<I128TransportRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_pair_diagnostic(diagnostics,
                                module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                context,
                                instruction_index,
                                std::string{target.selection.diagnostic});
    return I128InstructionLoweringResult{.handled = true};
  }

  return I128InstructionLoweringResult{
      .handled = true,
      .instruction =
          make_i128_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

}  // namespace c4c::backend::aarch64::codegen
