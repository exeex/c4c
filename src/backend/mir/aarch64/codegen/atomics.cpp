#include "atomics.hpp"
#include "alu.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

void append_memory_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Memory,
      .message = std::move(message),
  });
}

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
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

const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

std::optional<abi::RegisterView> scalar_fp_register_view(bir::TypeKind type) {
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

std::optional<abi::RegisterView> scalar_storage_register_view(bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
}

prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

std::vector<std::string_view> occupied_register_views(abi::RegisterReference reg) {
  if (reg.index >= 32U) {
    return {};
  }

  static const auto names = [] {
    std::array<std::array<std::string, 32>, 7> result{};
    for (std::uint8_t index = 0; index < 32U; ++index) {
      result[0][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::GeneralPurpose,
                                                    abi::RegisterView::X,
                                                    index});
      result[1][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::GeneralPurpose,
                                                    abi::RegisterView::W,
                                                    index});
      result[2][index] = abi::register_name(abi::stack_pointer_register());
      result[3][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::S,
                                                    index});
      result[4][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::D,
                                                    index});
      result[5][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::Q,
                                                    index});
      result[6][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::V,
                                                    index});
    }
    return result;
  }();

  std::string_view display_name;
  switch (reg.view) {
    case abi::RegisterView::X:
      display_name = names[0][reg.index];
      break;
    case abi::RegisterView::W:
      display_name = names[1][reg.index];
      break;
    case abi::RegisterView::Sp:
      display_name = names[2][reg.index];
      break;
    case abi::RegisterView::S:
      display_name = names[3][reg.index];
      break;
    case abi::RegisterView::D:
      display_name = names[4][reg.index];
      break;
    case abi::RegisterView::Q:
      display_name = names[5][reg.index];
      break;
    case abi::RegisterView::V:
      display_name = names[6][reg.index];
      break;
  }
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

std::vector<abi::RegisterReference> occupied_register_references(
    abi::RegisterReference reg) {
  return {reg};
}

bool same_gp_allocation_register(abi::RegisterReference lhs,
                                 abi::RegisterReference rhs) {
  return lhs.bank == abi::RegisterBank::GeneralPurpose &&
         rhs.bank == abi::RegisterBank::GeneralPurpose &&
         lhs.index == rhs.index;
}

bool record_uses_gp_register(const AtomicMemoryInstructionRecord& record,
                             abi::RegisterReference candidate) {
  const auto used_by = [&](const std::optional<RegisterOperand>& operand) {
    return operand.has_value() && same_gp_allocation_register(operand->reg, candidate);
  };
  return used_by(record.pointer_register) || used_by(record.result_register) ||
         used_by(record.stored_register) || used_by(record.expected_register) ||
         used_by(record.desired_register) ||
         used_by(record.exclusive_status_register) ||
         used_by(record.rmw_new_value_register) ||
         used_by(record.compare_loaded_register);
}

std::optional<RegisterOperand> next_reserved_gp_scratch_operand(
    const AtomicMemoryInstructionRecord& record,
    abi::RegisterView expected_view) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (record_uses_gp_register(record, scratch)) {
      continue;
    }
    const auto viewed = abi::gp_register(scratch.index, expected_view);
    if (!viewed.has_value()) {
      continue;
    }
    return RegisterOperand{
        .reg = *viewed,
        .role = RegisterOperandRole::ReservedMirScratch,
        .prepared_class = prepare::PreparedRegisterClass::General,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = expected_view,
        .contiguous_width = 1,
        .occupied_register_references = occupied_register_references(*viewed),
        .occupied_registers = occupied_register_views(*viewed),
    };
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> atomic_value_register_view(std::size_t width_bytes) {
  if (width_bytes == 0 || width_bytes > 8) {
    return std::nullopt;
  }
  return width_bytes == 8 ? abi::RegisterView::X : abi::RegisterView::W;
}

std::optional<RegisterOperand> make_prepared_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type,
    RegisterOperandRole role) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register) {
    return std::nullopt;
  }

  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_class = register_class_from_bank(storage.bank);
  abi::PreparedRegisterConversionResult converted;
  if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    if (!home.register_name.has_value() || !storage.register_name.has_value() ||
        *home.register_name != *storage.register_name) {
      return std::nullopt;
    }
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

PreparedAtomicOperationInstructionRecordResult atomic_instruction_record_error(
    PreparedAtomicOperationRecordError error) {
  return PreparedAtomicOperationInstructionRecordResult{.record = std::nullopt,
                                                        .error = error};
}

bool supported_atomic_width(std::size_t width_bytes) {
  return width_bytes == 1 || width_bytes == 2 || width_bytes == 4 || width_bytes == 8;
}

bool atomic_ordering_has_acquire(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool atomic_ordering_has_release(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_load_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_store_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_fence_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_rmw_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_compare_exchange_success_ordering(bir::AtomicOrdering ordering) {
  return supported_atomic_rmw_ordering(ordering);
}

bool supported_atomic_compare_exchange_failure_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_rmw_opcode(bir::AtomicRmwOpcode opcode) {
  return opcode == bir::AtomicRmwOpcode::Exchange ||
         opcode == bir::AtomicRmwOpcode::Add ||
         opcode == bir::AtomicRmwOpcode::Sub ||
         opcode == bir::AtomicRmwOpcode::And ||
         opcode == bir::AtomicRmwOpcode::Or ||
         opcode == bir::AtomicRmwOpcode::Xor;
}

PreparedAtomicOperationRecordError register_for_named_atomic_value(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    std::optional<c4c::ValueNameId> value_name,
    bir::TypeKind type,
    PreparedAtomicOperationRecordError missing_name_error,
    PreparedAtomicOperationRecordError missing_home_error,
    PreparedAtomicOperationRecordError missing_storage_error,
    std::optional<prepare::PreparedValueId>& value_id,
    std::optional<RegisterOperand>& reg) {
  if (!value_name.has_value() || *value_name == c4c::kInvalidValueName) {
    return missing_name_error;
  }
  const auto* home = prepare::find_prepared_value_home(value_locations, *value_name);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register) {
    return missing_home_error;
  }
  const auto* storage = find_storage_plan_value(storage_plan, home->value_id);
  if (storage == nullptr || storage->value_name != *value_name) {
    return missing_storage_error;
  }
  if (storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return missing_storage_error;
  }
  auto register_operand =
      make_prepared_register_operand(*home, *storage, type, RegisterOperandRole::StoragePlan);
  if (!register_operand.has_value()) {
    return PreparedAtomicOperationRecordError::RegisterConversionFailed;
  }
  value_id = home->value_id;
  reg = *register_operand;
  return PreparedAtomicOperationRecordError::None;
}

MachineOpcode machine_opcode_from_atomic_memory_instruction(
    const AtomicMemoryInstructionRecord& instruction) {
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      return MachineOpcode::AtomicLoad;
    case AtomicMemoryInstructionKind::Store:
      return MachineOpcode::AtomicStore;
    case AtomicMemoryInstructionKind::Fence:
      return MachineOpcode::AtomicFence;
    case AtomicMemoryInstructionKind::RmwLoop:
      return MachineOpcode::AtomicRmw;
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return MachineOpcode::AtomicCompareExchange;
  }
  return MachineOpcode::Unspecified;
}

MachineEffectResource effect_from_memory_operand(const OperandRecord& operand) {
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
    case OperandKind::Immediate:
    case OperandKind::PreparedValue:
    case OperandKind::FrameSlot:
    case OperandKind::Symbol:
    case OperandKind::BranchTarget:
      break;
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

std::vector<MachineEffectResource> memory_effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(effect_from_memory_operand(operand));
  }
  return effects;
}

std::vector<MachineSideEffectKind> atomic_memory_side_effects(
    const AtomicMemoryInstructionRecord& instruction) {
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::Store:
      return {MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::Fence:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::RmwLoop:
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
  }
  return {};
}

MachineNodeStatusRecord atomic_memory_selection_status(
    const AtomicMemoryInstructionRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "atomic memory node is missing prepared atomic carrier provenance"};
  }
  if (instruction.source_carrier->carrier_kind !=
      prepare::PreparedAtomicOperationCarrierKind::Complete) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "atomic memory node requires a complete prepared carrier"};
  }
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic load is missing pointer or result register authority"};
      }
      break;
    case AtomicMemoryInstructionKind::Store:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.stored_value_id.has_value() ||
          !instruction.stored_value_name.has_value() ||
          !instruction.stored_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic store is missing pointer or stored register authority"};
      }
      break;
    case AtomicMemoryInstructionKind::Fence:
      if (!instruction.memory_barrier_required) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::DeferredUnsupported,
            .diagnostic = "relaxed atomic fence is outside the selected subset"};
      }
      break;
    case AtomicMemoryInstructionKind::RmwLoop:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.stored_value_id.has_value() ||
          !instruction.stored_value_name.has_value() ||
          !instruction.stored_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value() ||
          !instruction.rmw_new_value_register.has_value() ||
          !instruction.exclusive_status_register.has_value() ||
          instruction.rmw_opcode == bir::AtomicRmwOpcode::None ||
          instruction.result_mode != bir::AtomicResultMode::OldValue ||
          !instruction.exclusive_retry_loop) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic rmw loop is missing operand, result, scratch, status, opcode, or retry-loop authority"};
      }
      break;
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.expected_value_id.has_value() ||
          !instruction.expected_value_name.has_value() ||
          !instruction.expected_register.has_value() ||
          !instruction.desired_value_id.has_value() ||
          !instruction.desired_value_name.has_value() ||
          !instruction.desired_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value() ||
          !instruction.exclusive_status_register.has_value() ||
          !instruction.exclusive_retry_loop ||
          !instruction.compare_exchange_failure_clears_monitor ||
          instruction.failure_ordering == bir::AtomicOrdering::None) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing operand, result, status, ordering, or monitor-clear authority"};
      }
      if (!instruction.compare_exchange_result_is_boolean &&
          !instruction.compare_exchange_result_is_old_value) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing result-mode authority"};
      }
      if (instruction.compare_exchange_result_is_boolean &&
          !instruction.compare_loaded_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing loaded-value register authority"};
      }
      break;
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

}  // namespace

std::string_view prepared_atomic_operation_record_error_name(
    PreparedAtomicOperationRecordError error) {
  switch (error) {
    case PreparedAtomicOperationRecordError::None:
      return "none";
    case PreparedAtomicOperationRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedAtomicOperationRecordError::MissingPreparedAtomicOperation:
      return "missing_prepared_atomic_operation";
    case PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation:
      return "incomplete_prepared_atomic_operation";
    case PreparedAtomicOperationRecordError::UnsupportedOperationKind:
      return "unsupported_operation_kind";
    case PreparedAtomicOperationRecordError::UnsupportedOrdering:
      return "unsupported_ordering";
    case PreparedAtomicOperationRecordError::UnsupportedFailureOrdering:
      return "unsupported_failure_ordering";
    case PreparedAtomicOperationRecordError::UnsupportedWidth:
      return "unsupported_width";
    case PreparedAtomicOperationRecordError::UnsupportedRmwOpcode:
      return "unsupported_rmw_opcode";
    case PreparedAtomicOperationRecordError::UnsupportedResultMode:
      return "unsupported_result_mode";
    case PreparedAtomicOperationRecordError::MissingPointerValueName:
      return "missing_pointer_value_name";
    case PreparedAtomicOperationRecordError::MissingPointerValueHome:
      return "missing_pointer_value_home";
    case PreparedAtomicOperationRecordError::MissingPointerValueStorage:
      return "missing_pointer_value_storage";
    case PreparedAtomicOperationRecordError::MissingResultValueName:
      return "missing_result_value_name";
    case PreparedAtomicOperationRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedAtomicOperationRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedAtomicOperationRecordError::MissingStoredValueName:
      return "missing_stored_value_name";
    case PreparedAtomicOperationRecordError::MissingStoredValueHome:
      return "missing_stored_value_home";
    case PreparedAtomicOperationRecordError::MissingStoredStorage:
      return "missing_stored_storage";
    case PreparedAtomicOperationRecordError::MissingExpectedValueName:
      return "missing_expected_value_name";
    case PreparedAtomicOperationRecordError::MissingExpectedValueHome:
      return "missing_expected_value_home";
    case PreparedAtomicOperationRecordError::MissingExpectedStorage:
      return "missing_expected_storage";
    case PreparedAtomicOperationRecordError::MissingDesiredValueName:
      return "missing_desired_value_name";
    case PreparedAtomicOperationRecordError::MissingDesiredValueHome:
      return "missing_desired_value_home";
    case PreparedAtomicOperationRecordError::MissingDesiredStorage:
      return "missing_desired_storage";
    case PreparedAtomicOperationRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
  }
  return "unknown";
}

std::string_view atomic_memory_instruction_kind_name(
    AtomicMemoryInstructionKind kind) {
  switch (kind) {
    case AtomicMemoryInstructionKind::Load:
      return "load";
    case AtomicMemoryInstructionKind::Store:
      return "store";
    case AtomicMemoryInstructionKind::Fence:
      return "fence";
    case AtomicMemoryInstructionKind::RmwLoop:
      return "rmw_loop";
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return "compare_exchange_loop";
  }
  return "unknown";
}

[[nodiscard]] std::string atomic_operation_error_message(
    PreparedAtomicOperationRecordError error) {
  std::string message =
      "AArch64 atomic lowering requires complete prepared atomic operation facts";
  message += "; error=";
  message += prepared_atomic_operation_record_error_name(error);
  return message;
}

InstructionRecord make_atomic_memory_instruction(
    AtomicMemoryInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.pointer_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.pointer_register));
  }
  if (instruction.stored_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.stored_register));
  }
  if (instruction.expected_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.expected_register));
  }
  if (instruction.desired_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.desired_register));
  }

  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_memory_operand(
        make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value() &&
             instruction.result_value_name.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id,
                                      *instruction.result_value_name));
  }
  if (instruction.rmw_new_value_register.has_value()) {
    defs.push_back(effect_from_memory_operand(
        make_register_operand(*instruction.rmw_new_value_register)));
  }
  if (instruction.compare_loaded_register.has_value()) {
    defs.push_back(effect_from_memory_operand(
        make_register_operand(*instruction.compare_loaded_register)));
  }
  if (instruction.exclusive_status_register.has_value()) {
    defs.push_back(effect_from_memory_operand(
        make_register_operand(*instruction.exclusive_status_register)));
  }

  const auto selection = atomic_memory_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_atomic_memory_instruction(instruction),
      .selection = selection,
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = memory_effects_from_operands(operands),
      .side_effects = atomic_memory_side_effects(instruction),
      .payload = instruction,
  };
}

PreparedAtomicOperationInstructionRecordResult
make_prepared_atomic_operation_instruction_record(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAtomicOperationCarrier& operation) {
  if (value_locations.function_name != storage_plan.function_name ||
      operation.function_name != value_locations.function_name) {
    return atomic_instruction_record_error(
        PreparedAtomicOperationRecordError::InvalidFunction);
  }
  if (operation.carrier_kind !=
      prepare::PreparedAtomicOperationCarrierKind::Complete) {
    return atomic_instruction_record_error(
        PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation);
  }

  AtomicMemoryInstructionRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .function_name = operation.function_name,
      .block_label = operation.block_label,
      .instruction_index = operation.inst_index,
      .value_type = operation.value_type,
      .width_bytes = operation.width_bytes,
      .ordering = operation.ordering,
      .result_mode = operation.result_mode,
      .address_space = operation.address_space,
      .acquire_semantics = atomic_ordering_has_acquire(operation.ordering),
      .release_semantics = atomic_ordering_has_release(operation.ordering),
      .sequentially_consistent = operation.ordering == bir::AtomicOrdering::SeqCst,
      .memory_barrier_required = operation.ordering != bir::AtomicOrdering::Relaxed,
      .source_carrier = &operation,
  };

  switch (operation.operation_kind) {
    case bir::AtomicOperationKind::Load: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_load_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Load;
      record.result_value_name = operation.result_value_name;
      record.pointer_value_name = operation.pointer_value_name;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (operation.result_mode != bir::AtomicResultMode::LoadedValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      break;
    }
    case bir::AtomicOperationKind::Store: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_store_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Store;
      record.pointer_value_name = operation.pointer_value_name;
      record.stored_value_name = operation.value_name;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingStoredValueName,
              PreparedAtomicOperationRecordError::MissingStoredValueHome,
              PreparedAtomicOperationRecordError::MissingStoredStorage,
              record.stored_value_id,
              record.stored_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      break;
    }
    case bir::AtomicOperationKind::Fence:
      if (!supported_atomic_fence_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Fence;
      record.value_type = bir::TypeKind::Void;
      record.width_bytes = 0;
      break;
    case bir::AtomicOperationKind::Rmw: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_rmw_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      if (!supported_atomic_rmw_opcode(operation.rmw_opcode)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedRmwOpcode);
      }
      if (operation.result_mode != bir::AtomicResultMode::OldValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedResultMode);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::RmwLoop;
      record.pointer_value_name = operation.pointer_value_name;
      record.stored_value_name = operation.value_name;
      record.result_value_name = operation.result_value_name;
      record.rmw_opcode = operation.rmw_opcode;
      record.exclusive_retry_loop = true;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingStoredValueName,
              PreparedAtomicOperationRecordError::MissingStoredValueHome,
              PreparedAtomicOperationRecordError::MissingStoredStorage,
              record.stored_value_id,
              record.stored_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      const auto value_view = atomic_value_register_view(operation.width_bytes);
      if (!value_view.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      record.rmw_new_value_register =
          next_reserved_gp_scratch_operand(record, *value_view);
      record.exclusive_status_register =
          next_reserved_gp_scratch_operand(record, abi::RegisterView::W);
      if (!record.rmw_new_value_register.has_value() ||
          !record.exclusive_status_register.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::RegisterConversionFailed);
      }
      break;
    }
    case bir::AtomicOperationKind::CompareExchange: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_compare_exchange_success_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      if (!supported_atomic_compare_exchange_failure_ordering(
              operation.failure_ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedFailureOrdering);
      }
      if (operation.result_mode != bir::AtomicResultMode::BooleanSuccess &&
          operation.result_mode != bir::AtomicResultMode::OldValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedResultMode);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::CompareExchangeLoop;
      record.pointer_value_name = operation.pointer_value_name;
      record.expected_value_name = operation.expected_value_name;
      record.desired_value_name = operation.desired_value_name;
      record.result_value_name = operation.result_value_name;
      record.failure_ordering = operation.failure_ordering;
      record.exclusive_retry_loop = true;
      record.compare_exchange_failure_clears_monitor = true;
      record.compare_exchange_result_is_boolean =
          operation.result_mode == bir::AtomicResultMode::BooleanSuccess;
      record.compare_exchange_result_is_old_value =
          operation.result_mode == bir::AtomicResultMode::OldValue;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.expected_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingExpectedValueName,
              PreparedAtomicOperationRecordError::MissingExpectedValueHome,
              PreparedAtomicOperationRecordError::MissingExpectedStorage,
              record.expected_value_id,
              record.expected_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.desired_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingDesiredValueName,
              PreparedAtomicOperationRecordError::MissingDesiredValueHome,
              PreparedAtomicOperationRecordError::MissingDesiredStorage,
              record.desired_value_id,
              record.desired_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.result_mode == bir::AtomicResultMode::BooleanSuccess
                  ? bir::TypeKind::I1
                  : operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      const auto value_view = atomic_value_register_view(operation.width_bytes);
      if (!value_view.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (record.compare_exchange_result_is_boolean) {
        record.compare_loaded_register =
            next_reserved_gp_scratch_operand(record, *value_view);
      }
      record.exclusive_status_register =
          next_reserved_gp_scratch_operand(record, abi::RegisterView::W);
      if ((record.compare_exchange_result_is_boolean &&
           !record.compare_loaded_register.has_value()) ||
          !record.exclusive_status_register.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::RegisterConversionFailed);
      }
      break;
    }
    case bir::AtomicOperationKind::None:
      return atomic_instruction_record_error(
          PreparedAtomicOperationRecordError::UnsupportedOperationKind);
  }

  return PreparedAtomicOperationInstructionRecordResult{
      .record = std::move(record),
      .error = PreparedAtomicOperationRecordError::None,
  };
}


std::vector<module::MachineInstruction> lower_atomic_memory_operations_for_block(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> instructions;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return instructions;
  }

  const auto* atomic_operations =
      prepare::find_prepared_atomic_operations(
          *context.function.prepared, context.function.control_flow->function_name);
  if (atomic_operations == nullptr) {
    return instructions;
  }

  for (const auto& operation : atomic_operations->operations) {
    if (operation.block_label != context.control_flow_block->block_label) {
      continue;
    }

    auto prepared = make_prepared_atomic_operation_instruction_record(
        *context.function.value_locations,
        *context.function.storage_plan,
        operation);
    if (!prepared.record.has_value()) {
      append_memory_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          operation.inst_index,
          atomic_operation_error_message(prepared.error));
      continue;
    }

    InstructionRecord target = make_atomic_memory_instruction(*prepared.record);
    target.function_name = context.function.control_flow->function_name;
    target.block_label = context.control_flow_block->block_label;
    target.block_index = context.block_index;
    target.instruction_index = operation.inst_index;
    if (auto* record = std::get_if<AtomicMemoryInstructionRecord>(&target.payload)) {
      record->block_index = context.block_index;
    }
    if (target.selection.status != MachineNodeSelectionStatus::Selected) {
      append_memory_diagnostic(diagnostics,
                               module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                               context,
                               operation.inst_index,
                               std::string{target.selection.diagnostic});
      continue;
    }

    instructions.push_back(make_bir_machine_instruction(
        context, operation.inst_index, std::move(target)));
  }
  return instructions;
}

}  // namespace c4c::backend::aarch64::codegen
