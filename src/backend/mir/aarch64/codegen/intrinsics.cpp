#include "intrinsics.hpp"

#include "alu.hpp"
#include "f128.hpp"
#include "memory.hpp"

#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;

void append_intrinsic_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .message = std::move(message),
  });
}

[[nodiscard]] const prepare::PreparedIntrinsicCarrier* find_prepared_intrinsic_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_intrinsic_carriers(
      *context.function.prepared, context.function.control_flow->function_name);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == context.block_index && carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] std::string intrinsic_error_message(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  std::string message =
      "AArch64 intrinsic lowering requires a complete prepared scalar FP unary carrier";
  message += "; error=";
  message += prepared_scalar_fp_unary_intrinsic_record_error_name(error);
  return message;
}

[[nodiscard]] std::string prepared_intrinsic_carrier_error_message(std::string_view error) {
  std::string message =
      "AArch64 intrinsic lowering requires a complete prepared intrinsic carrier";
  message += "; error=";
  message += error;
  return message;
}

[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
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

[[nodiscard]] std::optional<abi::RegisterView> intrinsic_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::I128:
      return abi::RegisterView::Q;
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage.value_name != home.value_name) {
    return std::nullopt;
  }
  const auto expected_view = intrinsic_register_view(type);
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
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = {*converted.reg},
  };
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::optional<prepare::PreparedValueHome>& home,
    bir::TypeKind type) {
  if (!home.has_value()) {
    return std::nullopt;
  }
  const auto* storage = find_storage_plan_value(storage_plan, home->value_id);
  if (storage == nullptr) {
    return std::nullopt;
  }
  return make_intrinsic_register_operand(*home, *storage, type);
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::vector<std::optional<prepare::PreparedValueHome>>& homes,
    std::size_t index,
    bir::TypeKind type) {
  if (index >= homes.size()) {
    return std::nullopt;
  }
  return make_intrinsic_register_operand(storage_plan, homes[index], type);
}

PreparedScalarFpUnaryIntrinsicInstructionRecordResult
scalar_fp_unary_intrinsic_instruction_record_error(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  return PreparedScalarFpUnaryIntrinsicInstructionRecordResult{.record = std::nullopt,
                                                               .error = error};
}

PreparedScalarFpUnaryIntrinsicRecordError intrinsic_operand_error_from_alu_error(
    PreparedScalarAluRecordError error) {
  switch (error) {
    case PreparedScalarAluRecordError::None:
      return PreparedScalarFpUnaryIntrinsicRecordError::None;
    case PreparedScalarAluRecordError::MissingOperandValueHome:
      return PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandValueHome;
    case PreparedScalarAluRecordError::MissingOperandStorage:
      return PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
      return PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandStorage;
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return PreparedScalarFpUnaryIntrinsicRecordError::RegisterConversionFailed;
    case PreparedScalarAluRecordError::UnsupportedOperandType:
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
    case PreparedScalarAluRecordError::InvalidFunction:
    case PreparedScalarAluRecordError::UnsupportedOpcode:
    case PreparedScalarAluRecordError::UnsupportedResultValue:
    case PreparedScalarAluRecordError::MissingResultValueHome:
    case PreparedScalarAluRecordError::MissingResultStorage:
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandType;
  }
  return PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandType;
}

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
        resource.value_id =
            memory->result_value_id.has_value() ? memory->result_value_id : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name =
            memory->symbol_name.has_value() ? memory->symbol_name : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

MachineEffectResource prepared_value_def(std::optional<prepare::PreparedValueId> value_id,
                                         c4c::ValueNameId value_name) {
  return MachineEffectResource{.kind = MachineEffectResourceKind::PreparedValue,
                               .value_id = value_id,
                               .value_name = value_name};
}

std::vector<MachineEffectResource> effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(effect_from_operand(operand));
  }
  return effects;
}

bool has_exact_roles(const std::vector<bir::IntrinsicOperandRole>& roles,
                     std::initializer_list<bir::IntrinsicOperandRole> expected) {
  return roles.size() == expected.size() &&
         std::equal(roles.begin(), roles.end(), expected.begin());
}

bool is_complete_intrinsic_source(const prepare::PreparedIntrinsicCarrier* source) {
  return source != nullptr &&
         source->carrier_kind == prepare::PreparedIntrinsicCarrierKind::Complete;
}

MachineNodeStatusRecord scalar_fp_unary_intrinsic_selection_status(
    const ScalarFpUnaryIntrinsicRecord& instruction) {
  if (!is_complete_intrinsic_source(instruction.source_carrier)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "scalar FP unary intrinsic node requires a complete prepared carrier"};
  }
  if (instruction.family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
      instruction.operation != bir::IntrinsicOperationKind::FAbs) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "intrinsic operation is outside the selected scalar FP unary subset"};
  }
  if ((instruction.operand_type != bir::TypeKind::F32 &&
       instruction.operand_type != bir::TypeKind::F64) ||
      instruction.operand_type != instruction.result_type) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "scalar FP unary intrinsic type is outside the selected subset"};
  }
  if (!instruction.has_prepared_call_plan) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "scalar FP unary intrinsic node requires prepared call-plan authority"};
  }
  if (!instruction.operand_value_id.has_value() ||
      instruction.operand_value_name == c4c::kInvalidValueName ||
      !instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      !instruction.result_register.has_value() ||
      instruction.operand.kind != OperandKind::Register) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "scalar FP unary intrinsic node is missing operand or result register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord crc32w_intrinsic_selection_status(
    const Crc32WIntrinsicRecord& instruction) {
  if (!is_complete_intrinsic_source(instruction.source_carrier)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "CRC32W intrinsic node requires a complete prepared carrier"};
  }
  if (instruction.family != bir::IntrinsicFamilyKind::Crc ||
      instruction.operation != bir::IntrinsicOperationKind::Crc32W ||
      instruction.required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
      !instruction.requires_feature) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "intrinsic operation is outside the selected CRC32W subset"};
  }
  if (instruction.operand_type != bir::TypeKind::I32 ||
      instruction.result_type != bir::TypeKind::I32 ||
      instruction.signedness != bir::IntrinsicSignedness::Unsigned ||
      !has_exact_roles(instruction.operand_roles,
                       {bir::IntrinsicOperandRole::Accumulator,
                        bir::IntrinsicOperandRole::Data})) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "CRC32W intrinsic node is missing typed operand-role facts"};
  }
  if (!instruction.has_prepared_call_plan ||
      !instruction.accumulator_value_id.has_value() ||
      instruction.accumulator_value_name == c4c::kInvalidValueName ||
      !instruction.data_value_id.has_value() ||
      instruction.data_value_name == c4c::kInvalidValueName ||
      !instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      instruction.accumulator.kind != OperandKind::Register ||
      instruction.data.kind != OperandKind::Register ||
      !instruction.result_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "CRC32W intrinsic node is missing operand or result register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord vector_shape_status(std::size_t element_width_bytes,
                                            std::size_t lane_count,
                                            std::size_t total_width_bytes,
                                            bir::TypeKind element_type,
                                            bir::IntrinsicSignedness signedness) {
  if (element_type != bir::TypeKind::I8 || element_width_bytes != 1 ||
      lane_count != 16 || total_width_bytes != 16 ||
      signedness != bir::IntrinsicSignedness::Unsigned) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "vector intrinsic node is missing v16i8 shape authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord vector_load_intrinsic_selection_status(
    const VectorLoadIntrinsicRecord& instruction) {
  if (!is_complete_intrinsic_source(instruction.source_carrier)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "vector-load intrinsic node requires a complete prepared carrier"};
  }
  if (instruction.family != bir::IntrinsicFamilyKind::VectorMemory ||
      instruction.operation != bir::IntrinsicOperationKind::VectorLoad ||
      instruction.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !instruction.requires_feature) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "intrinsic operation is outside the selected vector-load subset"};
  }
  const auto shape = vector_shape_status(instruction.vector_element_width_bytes,
                                         instruction.vector_lane_count,
                                         instruction.vector_total_width_bytes,
                                         instruction.vector_element_type,
                                         instruction.signedness);
  if (shape.status != MachineNodeSelectionStatus::Selected) {
    return shape;
  }
  if (instruction.operand_type != bir::TypeKind::Ptr ||
      instruction.result_type != bir::TypeKind::I128 ||
      instruction.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      !has_exact_roles(instruction.operand_roles, {bir::IntrinsicOperandRole::Pointer})) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "vector-load intrinsic node is missing memory access facts"};
  }
  if (!instruction.has_prepared_call_plan ||
      !instruction.pointer_value_id.has_value() ||
      instruction.pointer_value_name == c4c::kInvalidValueName ||
      !instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      instruction.pointer.kind != OperandKind::Register ||
      instruction.memory.base_kind != MemoryBaseKind::PointerValue ||
      instruction.memory.size_bytes != instruction.vector_total_width_bytes ||
      instruction.memory.address_space != bir::AddressSpace::Default ||
      !instruction.result_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "vector-load intrinsic node is missing pointer, memory, or result register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord vector_add_intrinsic_selection_status(
    const VectorAddIntrinsicRecord& instruction) {
  if (!is_complete_intrinsic_source(instruction.source_carrier)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "vector-add intrinsic node requires a complete prepared carrier"};
  }
  if (instruction.family != bir::IntrinsicFamilyKind::VectorOperation ||
      instruction.operation != bir::IntrinsicOperationKind::VectorAdd ||
      instruction.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !instruction.requires_feature) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "intrinsic operation is outside the selected vector-add subset"};
  }
  const auto shape = vector_shape_status(instruction.vector_element_width_bytes,
                                         instruction.vector_lane_count,
                                         instruction.vector_total_width_bytes,
                                         instruction.vector_element_type,
                                         instruction.signedness);
  if (shape.status != MachineNodeSelectionStatus::Selected) {
    return shape;
  }
  if (instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128 ||
      instruction.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !has_exact_roles(instruction.operand_roles,
                       {bir::IntrinsicOperandRole::VectorLhs,
                        bir::IntrinsicOperandRole::VectorRhs})) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "vector-add intrinsic node is missing typed operand-role facts"};
  }
  if (!instruction.has_prepared_call_plan ||
      !instruction.lhs_value_id.has_value() ||
      instruction.lhs_value_name == c4c::kInvalidValueName ||
      !instruction.rhs_value_id.has_value() ||
      instruction.rhs_value_name == c4c::kInvalidValueName ||
      !instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      instruction.lhs.kind != OperandKind::Register ||
      instruction.rhs.kind != OperandKind::Register ||
      !instruction.result_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "vector-add intrinsic node is missing operand or result register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

[[nodiscard]] InstructionRecord make_crc32w_intrinsic_record_from_carrier(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto accumulator =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::I32);
  const auto data =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 1, bir::TypeKind::I32);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I32);

  return make_crc32w_intrinsic_instruction(Crc32WIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .signedness = carrier.signedness,
      .accumulator_value_id = accumulator ? accumulator->value_id : std::nullopt,
      .accumulator_value_name = accumulator ? accumulator->value_name : c4c::kInvalidValueName,
      .data_value_id = data ? data->value_id : std::nullopt,
      .data_value_name = data ? data->value_name : c4c::kInvalidValueName,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .accumulator = accumulator ? make_register_operand(*accumulator) : OperandRecord{},
      .data = data ? make_register_operand(*data) : OperandRecord{},
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
  });
}

[[nodiscard]] InstructionRecord make_vector_load_intrinsic_record_from_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto pointer =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::Ptr);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I128);
  const auto pointer_value_name = pointer ? pointer->value_name : c4c::kInvalidValueName;
  const auto pointer_value_id = pointer ? pointer->value_id : std::nullopt;
  MemoryOperand memory{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name =
          result ? std::optional<c4c::ValueNameId>{result->value_name} : std::nullopt,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = pointer,
      .pointer_value_name = pointer_value_name,
      .pointer_value_id = pointer_value_id,
      .byte_offset = carrier.memory_operand ? carrier.memory_operand->byte_offset : 0,
      .size_bytes = carrier.vector_total_width_bytes,
      .align_bytes =
          carrier.memory_operand ? carrier.memory_operand->align_bytes
                                 : carrier.vector_total_width_bytes,
      .address_space = carrier.memory_operand ? carrier.memory_operand->address_space
                                              : bir::AddressSpace::Default,
  };

  return make_vector_load_intrinsic_instruction(VectorLoadIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .vector_element_type = carrier.vector_element_type,
      .vector_element_width_bytes = carrier.vector_element_width_bytes,
      .vector_lane_count = carrier.vector_lane_count,
      .vector_total_width_bytes = carrier.vector_total_width_bytes,
      .signedness = carrier.signedness,
      .memory_access = carrier.memory_access,
      .pointer_value_id = pointer_value_id,
      .pointer_value_name = pointer_value_name,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .pointer = pointer ? make_register_operand(*pointer) : OperandRecord{},
      .memory = memory,
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
  });
}

[[nodiscard]] InstructionRecord make_vector_add_intrinsic_record_from_carrier(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto lhs =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::I128);
  const auto rhs =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 1, bir::TypeKind::I128);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I128);

  return make_vector_add_intrinsic_instruction(VectorAddIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .vector_element_type = carrier.vector_element_type,
      .vector_element_width_bytes = carrier.vector_element_width_bytes,
      .vector_lane_count = carrier.vector_lane_count,
      .vector_total_width_bytes = carrier.vector_total_width_bytes,
      .signedness = carrier.signedness,
      .memory_access = carrier.memory_access,
      .lhs_value_id = lhs ? lhs->value_id : std::nullopt,
      .lhs_value_name = lhs ? lhs->value_name : c4c::kInvalidValueName,
      .rhs_value_id = rhs ? rhs->value_id : std::nullopt,
      .rhs_value_name = rhs ? rhs->value_name : c4c::kInvalidValueName,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .lhs = lhs ? make_register_operand(*lhs) : OperandRecord{},
      .rhs = rhs ? make_register_operand(*rhs) : OperandRecord{},
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
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
      .origin = c4c::backend::mir::MachineOrigin{
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

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
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

bool has_exact_intrinsic_roles(const std::vector<bir::IntrinsicOperandRole>& roles,
                               std::initializer_list<bir::IntrinsicOperandRole> expected) {
  return roles.size() == expected.size() &&
         std::equal(roles.begin(), roles.end(), expected.begin(), expected.end());
}

bool has_v16i8_unsigned_shape(bir::TypeKind element_type,
                              std::size_t element_width_bytes,
                              std::size_t lane_count,
                              std::size_t total_width_bytes,
                              bir::IntrinsicSignedness signedness) {
  return element_type == bir::TypeKind::I8 && element_width_bytes == 1 &&
         lane_count == 16 && total_width_bytes == 16 &&
         signedness == bir::IntrinsicSignedness::Unsigned;
}

mir::TargetInstructionPrintResult print_scalar_fp_unary_intrinsic(
    const InstructionRecord& instruction,
    const ScalarFpUnaryIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::ScalarFpUnaryIntrinsic) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires an intrinsic machine opcode");
  }
  if (!is_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
      intrinsic.operation != bir::IntrinsicOperationKind::FAbs) {
    return target_unsupported(
        bad_header(instruction) +
        "intrinsic operation is outside the printable scalar FP unary subset");
  }
  if ((intrinsic.operand_type != bir::TypeKind::F32 &&
       intrinsic.operand_type != bir::TypeKind::F64) ||
      intrinsic.operand_type != intrinsic.result_type) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires matching F32/F64 operand and result types");
  }
  if (!intrinsic.has_prepared_call_plan) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires prepared call-plan authority");
  }
  if (intrinsic.has_side_effects || intrinsic.requires_feature) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer only supports side-effect-free feature-free fabs");
  }
  const auto* operand_register = std::get_if<RegisterOperand>(&intrinsic.operand.payload);
  if (!intrinsic.result_register.has_value() ||
      intrinsic.operand.kind != OperandKind::Register ||
      operand_register == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires explicit operand and result registers");
  }
  const auto view = intrinsic_register_view(intrinsic.operand_type);
  if (!view.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has no register view for operand type");
  }
  const auto result = fp_register_name_with_view(*intrinsic.result_register, *view);
  const auto operand = fp_register_name_with_view(*operand_register, *view);
  if (!result.has_value() || !operand.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has incomplete printable FPR register facts");
  }
  std::ostringstream out;
  out << "fabs " << *result << ", " << *operand;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_crc32w_intrinsic(
    const InstructionRecord& instruction,
    const Crc32WIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::Crc32WIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires a CRC32W machine opcode");
  }
  if (!is_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::Crc ||
      intrinsic.operation != bir::IntrinsicOperationKind::Crc32W ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I32 ||
      intrinsic.result_type != bir::TypeKind::I32 ||
      intrinsic.signedness != bir::IntrinsicSignedness::Unsigned ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Accumulator,
                                  bir::IntrinsicOperandRole::Data})) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires selected CRC32W carrier facts");
  }
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires prepared register authority");
  }
  const auto* accumulator = std::get_if<RegisterOperand>(&intrinsic.accumulator.payload);
  const auto* data = std::get_if<RegisterOperand>(&intrinsic.data.payload);
  if (intrinsic.accumulator.kind != OperandKind::Register ||
      intrinsic.data.kind != OperandKind::Register ||
      accumulator == nullptr || data == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires explicit operand registers");
  }
  const auto result_name =
      register_name_with_view(*intrinsic.result_register, abi::RegisterView::W);
  const auto accumulator_name = register_name_with_view(*accumulator, abi::RegisterView::W);
  const auto data_name = register_name_with_view(*data, abi::RegisterView::W);
  if (!result_name.has_value() || !accumulator_name.has_value() || !data_name.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer has incomplete printable W-register facts");
  }
  std::ostringstream out;
  out << "crc32w " << *result_name << ", " << *accumulator_name << ", " << *data_name;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_load_intrinsic(
    const InstructionRecord& instruction,
    const VectorLoadIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorLoadIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-load intrinsic printer requires a vector-load machine opcode");
  }
  if (!is_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorMemory ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorLoad ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::Ptr ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Pointer}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires selected v16i8 load carrier facts");
  }
  const auto* pointer = std::get_if<RegisterOperand>(&intrinsic.pointer.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.pointer.kind != OperandKind::Register || pointer == nullptr ||
      intrinsic.memory.base_kind != MemoryBaseKind::PointerValue ||
      !intrinsic.memory.base_register.has_value() ||
      intrinsic.memory.byte_offset != 0 ||
      intrinsic.memory.size_bytes != intrinsic.vector_total_width_bytes ||
      intrinsic.memory.address_space != bir::AddressSpace::Default) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires pointer memory and result register authority");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto pointer_name = register_name_with_view(*pointer, abi::RegisterView::X);
  const auto memory_base_name =
      register_name_with_view(*intrinsic.memory.base_register, abi::RegisterView::X);
  if (!result_name.has_value() || !pointer_name.has_value() ||
      !memory_base_name.has_value() || *pointer_name != *memory_base_name) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer has incomplete printable vector or pointer register facts");
  }
  std::ostringstream out;
  out << "ld1 {" << *result_name << "}, [" << *memory_base_name << "]";
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_add_intrinsic(
    const InstructionRecord& instruction,
    const VectorAddIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorAddIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-add intrinsic printer requires a vector-add machine opcode");
  }
  if (!is_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorOperation ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorAdd ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I128 ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::VectorLhs,
                                  bir::IntrinsicOperandRole::VectorRhs}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires selected v16i8 add carrier facts");
  }
  const auto* lhs = std::get_if<RegisterOperand>(&intrinsic.lhs.payload);
  const auto* rhs = std::get_if<RegisterOperand>(&intrinsic.rhs.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.lhs.kind != OperandKind::Register ||
      intrinsic.rhs.kind != OperandKind::Register ||
      lhs == nullptr || rhs == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires explicit operand and result registers");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto lhs_name = f128_vector_register_name(*lhs);
  const auto rhs_name = f128_vector_register_name(*rhs);
  if (!result_name.has_value() || !lhs_name.has_value() || !rhs_name.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer has incomplete printable vector register facts");
  }
  std::ostringstream out;
  out << "add " << *result_name << ", " << *lhs_name << ", " << *rhs_name;
  return target_printed({out.str()});
}

}  // namespace

InstructionRecord make_scalar_fp_unary_intrinsic_instruction(
    ScalarFpUnaryIntrinsicRecord instruction) {
  std::vector<OperandRecord> operands = {instruction.operand};
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }
  std::vector<MachineSideEffectKind> side_effects;
  if (instruction.has_side_effects) {
    side_effects.push_back(MachineSideEffectKind::Call);
  }

  return InstructionRecord{
      .family = InstructionFamily::Intrinsic,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::ScalarFpUnaryIntrinsic,
      .selection = scalar_fp_unary_intrinsic_selection_status(instruction),
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

InstructionRecord make_crc32w_intrinsic_instruction(Crc32WIntrinsicRecord instruction) {
  std::vector<OperandRecord> operands = {instruction.accumulator, instruction.data};
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }

  return InstructionRecord{
      .family = InstructionFamily::Intrinsic,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Crc32WIntrinsic,
      .selection = crc32w_intrinsic_selection_status(instruction),
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .payload = instruction,
  };
}

InstructionRecord make_vector_load_intrinsic_instruction(VectorLoadIntrinsicRecord instruction) {
  std::vector<OperandRecord> operands = {instruction.pointer,
                                         make_memory_operand(instruction.memory)};
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }

  return InstructionRecord{
      .family = InstructionFamily::Intrinsic,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::VectorLoadIntrinsic,
      .selection = vector_load_intrinsic_selection_status(instruction),
      .function_name = instruction.memory.function_name,
      .block_label = instruction.memory.block_label,
      .instruction_index = instruction.memory.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .side_effects = {MachineSideEffectKind::MemoryRead},
      .payload = instruction,
  };
}

InstructionRecord make_vector_add_intrinsic_instruction(VectorAddIntrinsicRecord instruction) {
  std::vector<OperandRecord> operands = {instruction.lhs, instruction.rhs};
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }

  return InstructionRecord{
      .family = InstructionFamily::Intrinsic,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::VectorAddIntrinsic,
      .selection = vector_add_intrinsic_selection_status(instruction),
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .payload = instruction,
  };
}

PreparedScalarFpUnaryIntrinsicInstructionRecordResult
make_prepared_scalar_fp_unary_intrinsic_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name ||
      carrier.function_name != value_locations.function_name) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::InvalidFunction);
  }
  if (carrier.carrier_kind == prepare::PreparedIntrinsicCarrierKind::Missing) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        carrier.missing_required_facts.empty()
            ? PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedIntrinsicCarrier
            : PreparedScalarFpUnaryIntrinsicRecordError::IncompletePreparedIntrinsicCarrier);
  }
  if (carrier.family != bir::IntrinsicFamilyKind::ScalarFpUnary) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicFamily);
  }
  if (carrier.operation != bir::IntrinsicOperationKind::FAbs) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicOperation);
  }
  if ((carrier.operand_type != bir::TypeKind::F32 && carrier.operand_type != bir::TypeKind::F64) ||
      carrier.operand_type != carrier.result_type) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandType);
  }
  if (!carrier.has_prepared_call_plan) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedCallPlan);
  }
  if (!carrier.operand.has_value() || !carrier.operand_value_name.has_value()) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandValueHome);
  }
  if (!carrier.result.has_value() || !carrier.result_value_name.has_value()) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::MissingResultValueHome);
  }

  OperandRecord operand;
  if (const auto error = make_prepared_scalar_operand(
          names, value_locations, storage_plan, *carrier.operand, operand);
      error != PreparedScalarAluRecordError::None) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        intrinsic_operand_error_from_alu_error(error));
  }
  const auto* operand_register = std::get_if<RegisterOperand>(&operand.payload);
  if (operand.kind != OperandKind::Register || operand_register == nullptr ||
      operand_register->prepared_bank != prepare::PreparedRegisterBank::Fpr) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandStorage);
  }

  const auto* result_home =
      prepare::find_prepared_value_home(value_locations, *carrier.result_value_name);
  if (result_home == nullptr) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      result_storage->bank != prepare::PreparedRegisterBank::Fpr) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedResultStorage);
  }
  const auto result_register =
      make_intrinsic_register_operand(*result_home, *result_storage, carrier.result_type);
  if (!result_register.has_value()) {
    return scalar_fp_unary_intrinsic_instruction_record_error(
        PreparedScalarFpUnaryIntrinsicRecordError::RegisterConversionFailed);
  }

  return PreparedScalarFpUnaryIntrinsicInstructionRecordResult{
      .record = ScalarFpUnaryIntrinsicRecord{
          .surface = RecordSurfaceKind::RecordOnly,
          .source_carrier = &carrier,
          .family = carrier.family,
          .operation = carrier.operation,
          .operand_type = carrier.operand_type,
          .result_type = carrier.result_type,
          .operand_value_id = operand_register->value_id,
          .operand_value_name = *carrier.operand_value_name,
          .result_value_id = result_home->value_id,
          .result_value_name = result_home->value_name,
          .operand = operand,
          .result_register = result_register,
          .has_side_effects = carrier.has_side_effects,
          .requires_feature = carrier.requires_feature,
          .source_callee_name = carrier.source_callee_name,
          .has_prepared_call_plan = carrier.has_prepared_call_plan,
      },
      .error = PreparedScalarFpUnaryIntrinsicRecordError::None,
  };
}

std::optional<module::MachineInstruction> lower_intrinsic_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* carrier = find_prepared_intrinsic_carrier(context, instruction_index);
  if (carrier == nullptr) {
    return std::nullopt;
  }
  if (context.function.prepared == nullptr || context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr || context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_intrinsic_diagnostic(
        diagnostics,
        context,
        instruction_index,
        intrinsic_error_message(
            PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedIntrinsicCarrier));
    return std::nullopt;
  }

  InstructionRecord target;
  if (carrier->family == bir::IntrinsicFamilyKind::ScalarFpUnary) {
    const auto prepared = make_prepared_scalar_fp_unary_intrinsic_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *carrier);
    if (!prepared.record.has_value()) {
      append_intrinsic_diagnostic(diagnostics,
                                  context,
                                  instruction_index,
                                  intrinsic_error_message(prepared.error));
      return std::nullopt;
    }

    target = make_scalar_fp_unary_intrinsic_instruction(*prepared.record);
  } else if (carrier->carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete) {
    append_intrinsic_diagnostic(
        diagnostics,
        context,
        instruction_index,
        prepared_intrinsic_carrier_error_message("incomplete_prepared_intrinsic_carrier"));
    return std::nullopt;
  } else if (carrier->family == bir::IntrinsicFamilyKind::Crc &&
             carrier->operation == bir::IntrinsicOperationKind::Crc32W) {
    target = make_crc32w_intrinsic_record_from_carrier(*context.function.storage_plan, *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorMemory &&
             carrier->operation == bir::IntrinsicOperationKind::VectorLoad) {
    target = make_vector_load_intrinsic_record_from_carrier(
        context, instruction_index, *context.function.storage_plan, *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorOperation &&
             carrier->operation == bir::IntrinsicOperationKind::VectorAdd) {
    target = make_vector_add_intrinsic_record_from_carrier(*context.function.storage_plan,
                                                           *carrier);
  } else {
    append_intrinsic_diagnostic(
        diagnostics,
        context,
        instruction_index,
        prepared_intrinsic_carrier_error_message("unsupported_intrinsic_family"));
    return std::nullopt;
  }

  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_intrinsic_diagnostic(diagnostics,
                                context,
                                instruction_index,
                                std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

bool has_prepared_intrinsic_carrier(const module::BlockLoweringContext& context,
                                    std::size_t instruction_index) {
  return find_prepared_intrinsic_carrier(context, instruction_index) != nullptr;
}

mir::TargetInstructionPrintResult print_intrinsic_instruction(
    const InstructionRecord& instruction) {
  if (const auto* intrinsic = std::get_if<ScalarFpUnaryIntrinsicRecord>(&instruction.payload)) {
    return print_scalar_fp_unary_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<Crc32WIntrinsicRecord>(&instruction.payload)) {
    return print_crc32w_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorLoadIntrinsicRecord>(&instruction.payload)) {
    return print_vector_load_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorAddIntrinsicRecord>(&instruction.payload)) {
    return print_vector_add_intrinsic(instruction, *intrinsic);
  }
  return target_unsupported(bad_header(instruction) +
                            "intrinsic instruction payload is not in the printable subset");
}

}  // namespace c4c::backend::aarch64::codegen
