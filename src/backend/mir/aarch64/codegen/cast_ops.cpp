#include "cast_ops.hpp"

#include "comparison.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "memory.hpp"
#include "select_materialization.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace mir = c4c::backend::mir;
namespace abi = c4c::backend::aarch64::abi;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> std::optional<bir::Value> {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result;
        }
        return std::nullopt;
      },
      inst);
}

[[nodiscard]] mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

[[nodiscard]] mir::TargetInstructionPrintResult target_printed(
    std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

[[nodiscard]] std::string diagnostic(std::string_view prefix, std::string_view message) {
  std::string out{prefix};
  out += message;
  return out;
}

[[nodiscard]] std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
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

[[nodiscard]] std::optional<std::string> register_name_with_view(
    const RegisterOperand& operand,
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

[[nodiscard]] std::optional<std::string> fp_register_name_with_view(
    const RegisterOperand& operand,
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

[[nodiscard]] std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

[[nodiscard]] std::string scalar_cast_immediate_name(
    const ImmediateOperand& operand) {
  return "#" + std::to_string(operand.signed_value);
}

[[nodiscard]] std::optional<abi::RegisterView> floating_register_view(
    bir::TypeKind type) {
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

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(names, value_locations, value.name);
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

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_register_view(
    bir::TypeKind type) {
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

[[nodiscard]] std::optional<abi::RegisterView> scalar_storage_register_view(
    bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::int64_t> prepared_frame_load_offset(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedMemoryAccess* source_access = nullptr;
  for (const auto& access : addressing->accesses) {
    if (access.result_value_name != std::optional<c4c::ValueNameId>{home.value_name}) {
      continue;
    }
    if (source_access != nullptr) {
      return std::nullopt;
    }
    source_access = &access;
  }
  if (source_access == nullptr ||
      source_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !source_access->address.frame_slot_id.has_value()) {
    return std::nullopt;
  }

  const auto* slot = find_frame_slot_by_id(
      context.function.prepared->stack_layout, *source_access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return static_cast<std::int64_t>(slot->offset_bytes) +
         source_access->address.byte_offset;
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

[[nodiscard]] std::string_view register_display_name(abi::RegisterReference reg) {
  if (reg.index >= 32U) {
    return {};
  }

  static const auto names = [] {
    std::array<std::array<std::string, 32>, 4> result{};
    for (std::uint8_t index = 0; index < 32U; ++index) {
      result[0][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::GeneralPurpose,
                                                    abi::RegisterView::X,
                                                    index});
      result[1][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::GeneralPurpose,
                                                    abi::RegisterView::W,
                                                    index});
      result[2][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::S,
                                                    index});
      result[3][index] =
          abi::register_name(abi::RegisterReference{abi::RegisterBank::FpSimd,
                                                    abi::RegisterView::D,
                                                    index});
    }
    return result;
  }();

  switch (reg.view) {
    case abi::RegisterView::X:
      return names[0][reg.index];
    case abi::RegisterView::W:
      return names[1][reg.index];
    case abi::RegisterView::S:
      return names[2][reg.index];
    case abi::RegisterView::D:
      return names[3][reg.index];
    case abi::RegisterView::V:
    case abi::RegisterView::Q:
    case abi::RegisterView::Sp:
      return {};
  }
  return {};
}

[[nodiscard]] std::optional<RegisterOperand> make_prepared_register_operand(
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

  const auto display_name = register_display_name(*converted.reg);
  if (display_name.empty()) {
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
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {display_name},
  };
}

[[nodiscard]] bool register_spelling_matches_storage_view(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register) {
    return false;
  }
  if (storage.register_placement.has_value()) {
    return true;
  }
  if (!home.register_name.has_value() || !storage.register_name.has_value() ||
      *home.register_name != *storage.register_name) {
    return false;
  }
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return false;
  }
  const auto prepared_class = register_class_from_bank(storage.bank);
  const auto converted = abi::convert_prepared_register(
      *storage.register_name, storage.bank, prepared_class, expected_view);
  return converted.has_value();
}

[[nodiscard]] PreparedScalarCastRecordResult scalar_cast_record_error(
    PreparedScalarCastRecordError error) {
  return PreparedScalarCastRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarCastInstructionRecordResult
scalar_cast_instruction_record_error(PreparedScalarCastRecordError error) {
  return PreparedScalarCastInstructionRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarCastRecordError scalar_cast_operand_error_from_alu_error(
    PreparedScalarAluRecordError error) {
  switch (error) {
    case PreparedScalarAluRecordError::None:
      return PreparedScalarCastRecordError::None;
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
      return PreparedScalarCastRecordError::UnsupportedOperandValue;
    case PreparedScalarAluRecordError::MissingOperandValueHome:
      return PreparedScalarCastRecordError::MissingOperandValueHome;
    case PreparedScalarAluRecordError::MissingOperandStorage:
      return PreparedScalarCastRecordError::MissingOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
      return PreparedScalarCastRecordError::UnsupportedOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandType:
      return PreparedScalarCastRecordError::UnsupportedOperandType;
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return PreparedScalarCastRecordError::RegisterConversionFailed;
    case PreparedScalarAluRecordError::InvalidFunction:
      return PreparedScalarCastRecordError::InvalidFunction;
    case PreparedScalarAluRecordError::UnsupportedOpcode:
      return PreparedScalarCastRecordError::UnsupportedOpcode;
    case PreparedScalarAluRecordError::UnsupportedResultValue:
      return PreparedScalarCastRecordError::UnsupportedResultValue;
    case PreparedScalarAluRecordError::MissingResultValueHome:
      return PreparedScalarCastRecordError::MissingResultValueHome;
    case PreparedScalarAluRecordError::MissingResultStorage:
      return PreparedScalarCastRecordError::MissingResultStorage;
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return PreparedScalarCastRecordError::UnsupportedResultStorage;
  }
  return PreparedScalarCastRecordError::UnsupportedOperandType;
}

[[nodiscard]] bool same_named_value(const bir::Value& lhs, const bir::Value& rhs) {
  return lhs.kind == bir::Value::Kind::Named &&
         rhs.kind == bir::Value::Kind::Named &&
         !lhs.name.empty() &&
         lhs.name == rhs.name;
}

[[nodiscard]] bool instruction_select_defines_value(const bir::Inst& inst,
                                                    const bir::Value& value) {
  if (const auto* select = std::get_if<bir::SelectInst>(&inst);
      select != nullptr && same_named_value(select->result, value)) {
    return true;
  }
  return false;
}

[[nodiscard]] bool has_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    if (instruction_select_defines_value(context.bir_block->insts[index - 1], value)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<RegisterOperand> make_prepared_consumer_register_source(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t instruction_index,
    const prepare::PreparedValueHome& source_home,
    const ScalarInstructionRecord& scalar) {
  if (context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      scalar.scalar_cast == std::nullopt ||
      !scalar.result_value_id.has_value() ||
      !scalar.result_register.has_value()) {
    return std::nullopt;
  }
  if (scalar.scalar_cast->operation != ScalarCastOperationKind::SignExtend &&
      scalar.scalar_cast->operation != ScalarCastOperationKind::ZeroExtend) {
    return std::nullopt;
  }
  if (source_home.kind != prepare::PreparedValueHomeKind::Register &&
      source_home.kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  const bool stack_source_has_same_block_select_producer =
      source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      has_same_block_select_producer(context, cast.operand, instruction_index);
  if (scalar.scalar_cast->source.kind == OperandKind::Register) {
    return std::nullopt;
  }

  const auto source_view = scalar_register_view(cast.operand.type);
  if (!source_view.has_value()) {
    return std::nullopt;
  }
  const auto* result_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        scalar.result_value_name);
  if (result_home == nullptr || result_home->value_id != *scalar.result_value_id ||
      result_home->kind != prepare::PreparedValueHomeKind::Register) {
    return std::nullopt;
  }
  const auto* result_storage =
      find_storage_plan_value(*context.function.storage_plan, result_home->value_id);
  if (result_storage == nullptr ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return std::nullopt;
  }

  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BeforeInstruction,
      context.block_index,
      instruction_index);
  if (bundle == nullptr) {
    return std::nullopt;
  }

  for (const auto& move : bundle->moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.source_immediate_i32.has_value() ||
        move.from_value_id != source_home.value_id ||
        move.to_value_id != result_home->value_id) {
      continue;
    }
    if (source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
        (!stack_source_has_same_block_select_producer ||
         move.reason != "consumer_stack_to_register")) {
      continue;
    }

    const auto prepared_class = register_class_from_bank(result_storage->bank);
    abi::PreparedRegisterConversionResult converted;
    if (move.destination_register_placement.has_value()) {
      converted = abi::convert_prepared_register(
          *move.destination_register_placement, prepared_class, source_view);
    } else if (move.destination_register_name.has_value()) {
      converted = abi::convert_prepared_register(
          *move.destination_register_name,
          result_storage->bank,
          prepared_class,
          std::nullopt);
    } else if (result_storage->register_placement.has_value()) {
      converted = abi::convert_prepared_register(
          *result_storage->register_placement, prepared_class, source_view);
    } else if (result_storage->register_name.has_value()) {
      converted = abi::convert_prepared_register(
          *result_storage->register_name,
          result_storage->bank,
          prepared_class,
          std::nullopt);
    } else {
      return std::nullopt;
    }
    if (converted.has_value()) {
      converted.reg->view = *source_view;
    }
    if (!converted.has_value() || !abi::is_gp_register(*converted.reg)) {
      return std::nullopt;
    }
    if (converted.reg->bank != scalar.result_register->reg.bank ||
        converted.reg->index != scalar.result_register->reg.index) {
      return std::nullopt;
    }
    const auto display_name = register_display_name(*converted.reg);
    if (display_name.empty()) {
      return std::nullopt;
    }
    return RegisterOperand{
        .reg = *converted.reg,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = source_home.value_id,
        .value_name = source_home.value_name,
        .prepared_class = prepared_class,
        .prepared_bank = result_storage->bank,
        .expected_view = source_view,
        .contiguous_width = move.destination_contiguous_width,
        .occupied_register_references = {*converted.reg},
        .occupied_registers = {display_name},
    };
  }
  return std::nullopt;
}

}  // namespace

namespace {

struct ScalarCastOpcodeSemanticFacts {
  bool simple_integer_cast = false;
  bool scalar_conversion_cast = false;
  ScalarCastOperationKind operation = ScalarCastOperationKind::Deferred;
};

[[nodiscard]] ScalarCastOpcodeSemanticFacts scalar_cast_opcode_semantic_facts(
    bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
      return ScalarCastOpcodeSemanticFacts{
          .simple_integer_cast = true,
          .scalar_conversion_cast = false,
          .operation = scalar_cast_operation_from_cast_opcode(opcode),
      };
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
      return ScalarCastOpcodeSemanticFacts{
          .simple_integer_cast = false,
          .scalar_conversion_cast = true,
          .operation = scalar_cast_operation_from_cast_opcode(opcode),
      };
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return ScalarCastOpcodeSemanticFacts{
          .simple_integer_cast = false,
          .scalar_conversion_cast = false,
          .operation = ScalarCastOperationKind::Deferred,
      };
  }
  return ScalarCastOpcodeSemanticFacts{};
}

}  // namespace

bool is_simple_integer_cast_opcode(bir::CastOpcode opcode) {
  const auto facts = scalar_cast_opcode_semantic_facts(opcode);
  return facts.simple_integer_cast;
}

bool is_supported_scalar_conversion_cast_opcode(bir::CastOpcode opcode) {
  const auto facts = scalar_cast_opcode_semantic_facts(opcode);
  return facts.scalar_conversion_cast;
}

ScalarCastOperationKind scalar_cast_operation_from_cast_opcode(
    bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
      return ScalarCastOperationKind::SignExtend;
    case bir::CastOpcode::ZExt:
      return ScalarCastOperationKind::ZeroExtend;
    case bir::CastOpcode::Trunc:
      return ScalarCastOperationKind::Truncate;
    case bir::CastOpcode::FPTrunc:
      return ScalarCastOperationKind::FloatTruncate;
    case bir::CastOpcode::FPExt:
      return ScalarCastOperationKind::FloatExtend;
    case bir::CastOpcode::FPToSI:
      return ScalarCastOperationKind::FloatToSignedInt;
    case bir::CastOpcode::FPToUI:
      return ScalarCastOperationKind::FloatToUnsignedInt;
    case bir::CastOpcode::SIToFP:
      return ScalarCastOperationKind::SignedIntToFloat;
    case bir::CastOpcode::UIToFP:
      return ScalarCastOperationKind::UnsignedIntToFloat;
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return ScalarCastOperationKind::Deferred;
  }
  return ScalarCastOperationKind::Deferred;
}

namespace {

struct ScalarCastSemanticFacts {
  ScalarCastOpcodeSemanticFacts opcode;
  bool source_is_integer = false;
  bool result_is_integer = false;
  bool source_is_float = false;
  bool result_is_float = false;
  bool supported_float_width_conversion = false;
  bool supported_float_integer_conversion = false;
  bool supported = false;
};

[[nodiscard]] ScalarCastSemanticFacts scalar_cast_semantic_facts(
    const bir::CastInst& cast) {
  const auto opcode = scalar_cast_opcode_semantic_facts(cast.opcode);
  const bool source_is_integer = scalar_register_view(cast.operand.type).has_value();
  const bool result_is_integer = scalar_register_view(cast.result.type).has_value();
  const bool source_is_float = is_scalar_alu_floating_type(cast.operand.type);
  const bool result_is_float = is_scalar_alu_floating_type(cast.result.type);
  const bool supported_float_width_conversion =
      (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F32 &&
       cast.result.type == bir::TypeKind::F64) ||
      (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F64 &&
       cast.result.type == bir::TypeKind::F32);
  const bool supported_float_integer_conversion =
      ((cast.opcode == bir::CastOpcode::SIToFP || cast.opcode == bir::CastOpcode::UIToFP) &&
       source_is_integer && result_is_float) ||
      ((cast.opcode == bir::CastOpcode::FPToSI || cast.opcode == bir::CastOpcode::FPToUI) &&
       source_is_float && result_is_integer);
  const bool supported =
      (opcode.simple_integer_cast && source_is_integer && result_is_integer) ||
      supported_float_width_conversion ||
      supported_float_integer_conversion;
  return ScalarCastSemanticFacts{
      .opcode = opcode,
      .source_is_integer = source_is_integer,
      .result_is_integer = result_is_integer,
      .source_is_float = source_is_float,
      .result_is_float = result_is_float,
      .supported_float_width_conversion = supported_float_width_conversion,
      .supported_float_integer_conversion = supported_float_integer_conversion,
      .supported = supported,
  };
}

}  // namespace

MachineOpcode machine_opcode_from_scalar_cast(ScalarCastOperationKind operation) {
  switch (operation) {
    case ScalarCastOperationKind::SignExtend:
      return MachineOpcode::SignExtend;
    case ScalarCastOperationKind::ZeroExtend:
      return MachineOpcode::ZeroExtend;
    case ScalarCastOperationKind::Truncate:
      return MachineOpcode::Truncate;
    case ScalarCastOperationKind::FloatExtend:
    case ScalarCastOperationKind::FloatTruncate:
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat:
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt:
    case ScalarCastOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

ScalarInstructionRecord make_scalar_cast_instruction_record(ScalarCastRecord cast) {
  return ScalarInstructionRecord{
      .result_value_id = cast.result_value_id,
      .result_value_name = cast.result_value_name,
      .result_type = cast.result_type,
      .result_register = cast.result_register,
      .inputs = {cast.source},
      .source_cast_opcode = cast.source_cast_opcode,
      .scalar_cast = cast,
  };
}

PreparedScalarCastRecordResult make_prepared_scalar_cast_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::InvalidFunction);
  }
  const auto semantic = scalar_cast_semantic_facts(cast);
  if (!semantic.opcode.simple_integer_cast && !semantic.opcode.scalar_conversion_cast) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOpcode);
  }
  if (cast.result.kind != bir::Value::Kind::Named || cast.result.name.empty()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultValue);
  }
  if (!semantic.supported) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandType);
  }

  const auto* result_home = find_named_value_home(names, value_locations, cast.result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() || !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultStorage);
  }
  const auto result_register = make_prepared_register_operand(
      *result_home, *result_storage, cast.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::RegisterConversionFailed);
  }

  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, cast.operand, source);
      error != PreparedScalarAluRecordError::None) {
    return scalar_cast_record_error(scalar_cast_operand_error_from_alu_error(error));
  }
  if (cast.operand.kind == bir::Value::Kind::Named) {
    const auto* source_home = find_named_value_home(names, value_locations, cast.operand);
    const auto* source_storage =
        source_home != nullptr ? find_storage_plan_value(storage_plan, source_home->value_id)
                               : nullptr;
    if (source_home != nullptr && source_storage != nullptr &&
        source_storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
        !register_spelling_matches_storage_view(*source_home, *source_storage,
                                                cast.operand.type)) {
      return scalar_cast_record_error(PreparedScalarCastRecordError::RegisterConversionFailed);
    }
  }
  const auto* source_register = std::get_if<RegisterOperand>(&source.payload);
  if ((semantic.opcode.scalar_conversion_cast ||
       semantic.supported_float_width_conversion) &&
      (source.kind != OperandKind::Register || source_register == nullptr)) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandStorage);
  }
  const auto source_bank =
      source_register != nullptr ? source_register->prepared_bank
                                 : prepare::PreparedRegisterBank::None;
  const auto result_bank = result_register->prepared_bank;

  return PreparedScalarCastRecordResult{
      .record =
          ScalarCastRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = semantic.opcode.operation,
              .source_cast_opcode = cast.opcode,
              .source_type = cast.operand.type,
              .result_value_id = result_home->value_id,
              .result_value_name = result_home->value_name,
              .result_type = cast.result.type,
              .result_register = result_register,
              .source = source,
              .source_register_bank = source_bank,
              .result_register_bank = result_bank,
              .crosses_register_bank = source_bank != prepare::PreparedRegisterBank::None &&
                                       result_bank != prepare::PreparedRegisterBank::None &&
                                       source_bank != result_bank,
              .supported_simple_integer_cast = semantic.opcode.simple_integer_cast,
              .supported_float_integer_conversion =
                  semantic.supported_float_integer_conversion,
              .supported_float_width_conversion =
                  semantic.supported_float_width_conversion,
          },
      .error = PreparedScalarCastRecordError::None,
  };
}

PreparedScalarCastInstructionRecordResult make_prepared_scalar_cast_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  const auto result = make_prepared_scalar_cast_record(names, value_locations, storage_plan, cast);
  if (!result.record.has_value()) {
    return scalar_cast_instruction_record_error(result.error);
  }
  return PreparedScalarCastInstructionRecordResult{
      .record = make_scalar_cast_instruction_record(*result.record),
      .error = PreparedScalarCastRecordError::None,
  };
}

mir::TargetInstructionPrintResult print_scalar_conversion_instruction(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    const ScalarCastRecord& cast,
    std::string_view diagnostic_prefix) {
  (void)instruction;
  if ((!cast.supported_float_integer_conversion &&
       !cast.supported_float_width_conversion) ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node is outside the printable subset"));
  }
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node requires a structured register source operand"));
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
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar int-to-float conversion has unsupported type width"));
      }
      const auto source_view = integer_register_view(*source_bits);
      if (!source_view.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar int-to-float conversion has unsupported integer width"));
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
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar float-to-int conversion has unsupported type width"));
      }
      const auto result_view = integer_register_view(*result_bits);
      if (!result_view.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar float-to-int conversion has unsupported integer width"));
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
      return target_unsupported(diagnostic(
          diagnostic_prefix,
          "scalar conversion node is outside the printable subset"));
  }
  if (mnemonic.empty() || !result.has_value() || !source.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node has incomplete printable register facts"));
  }
  std::ostringstream out;
  out << mnemonic << " " << *result << ", " << *source;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar_cast_instruction(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    std::string_view diagnostic_prefix) {
  (void)instruction;
  if (!scalar.result_register.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar node is missing a structured destination register operand"));
  }
  if (!scalar.scalar_cast.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar node is missing scalar cast metadata"));
  }

  const auto& cast = *scalar.scalar_cast;
  if (cast.supported_float_integer_conversion || cast.supported_float_width_conversion) {
    return print_scalar_conversion_instruction(
        instruction, scalar, cast, diagnostic_prefix);
  }
  if (!cast.supported_simple_integer_cast ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node is outside the printable simple integer subset"));
  }
  const auto source_bits = integer_type_bit_width(cast.source_type);
  const auto result_bits = integer_type_bit_width(cast.result_type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      ((*source_bits >= *result_bits) &&
       cast.operation != ScalarCastOperationKind::Truncate &&
       !(cast.operation == ScalarCastOperationKind::ZeroExtend &&
         *source_bits == *result_bits)) ||
      ((*source_bits <= *result_bits) &&
       cast.operation == ScalarCastOperationKind::Truncate)) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node requires a supported integer source/result width"));
  }
  const auto result_view = integer_register_view(*result_bits);
  if (!result_view.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node has an unsupported result register width"));
  }
  const auto result = register_name_with_view(*scalar.result_register, *result_view);
  if (!result.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node destination is not a printable GPR register"));
  }

  std::vector<std::string> lines;
  std::optional<RegisterOperand> materialized_source_register;
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    const auto* immediate = std::get_if<ImmediateOperand>(&cast.source.payload);
    if (cast.source.kind == OperandKind::Immediate && immediate != nullptr) {
      abi::RegisterView materialized_view = abi::RegisterView::W;
      if (cast.operation == ScalarCastOperationKind::SignExtend && *source_bits == 1U) {
        materialized_view = *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X;
      } else if (cast.operation == ScalarCastOperationKind::ZeroExtend) {
        materialized_view = *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X;
      }
      const auto materialized_name =
          register_name_with_view(*scalar.result_register, materialized_view);
      if (!materialized_name.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar cast node cannot materialize immediate source into a GPR register"));
      }
      std::ostringstream materialize;
      materialize << "mov " << *materialized_name << ", "
                  << scalar_cast_immediate_name(*immediate);
      lines.push_back(materialize.str());
      materialized_source_register = *scalar.result_register;
      source_register = &*materialized_source_register;
    } else {
      return target_unsupported(diagnostic(
          diagnostic_prefix,
          "scalar cast node requires a structured register source operand"));
    }
  }

  std::ostringstream out;
  switch (cast.operation) {
    case ScalarCastOperationKind::SignExtend: {
      if (*source_bits == 1U) {
        const auto source = register_name_with_view(
            *source_register,
            *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
        if (!source.has_value()) {
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node source is not a printable GPR register"));
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
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node has no printable width form"));
        }
        const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
        if (!source.has_value()) {
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node source is not a printable GPR register"));
        }
        out << mnemonic << " " << *result << ", " << *source;
      }
      lines.push_back(out.str());
      return target_printed(std::move(lines));
    }
    case ScalarCastOperationKind::ZeroExtend: {
      const auto source = register_name_with_view(
          *source_register,
          *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
      if (!source.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar zero-extend node source is not a printable GPR register"));
      }
      if (*source_bits == *result_bits) {
        out << "mov " << *result << ", " << *source;
      } else {
        out << "ubfx " << *result << ", " << *source << ", #0, #" << *source_bits;
      }
      lines.push_back(out.str());
      return target_printed(std::move(lines));
    }
    case ScalarCastOperationKind::Truncate: {
      if (*result_bits > 32U) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar truncate node has no printable width form"));
      }
      const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
      if (!source.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar truncate node source is not a printable GPR register"));
      }
      if (*result_bits == 32U) {
        out << "mov " << *result << ", " << *source;
      } else {
        const std::uint64_t mask = (std::uint64_t{1} << *result_bits) - 1U;
        out << "and " << *result << ", " << *source << ", #" << mask;
      }
      lines.push_back(out.str());
      return target_printed(std::move(lines));
    }
    case ScalarCastOperationKind::FloatExtend:
    case ScalarCastOperationKind::FloatTruncate:
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat:
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt:
    case ScalarCastOperationKind::Deferred:
      break;
  }
  return target_unsupported(diagnostic(
      diagnostic_prefix,
      "scalar cast node is outside the printable simple integer subset"));
}

std::optional<module::MachineInstruction> lower_stack_scalar_float_width_cast(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      cast.result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const bool is_float_extend =
      cast.opcode == bir::CastOpcode::FPExt &&
      cast.operand.type == bir::TypeKind::F32 &&
      cast.result.type == bir::TypeKind::F64;
  const bool is_float_truncate =
      cast.opcode == bir::CastOpcode::FPTrunc &&
      cast.operand.type == bir::TypeKind::F64 &&
      cast.result.type == bir::TypeKind::F32;
  if (!is_float_extend && !is_float_truncate) {
    return std::nullopt;
  }

  const auto* result_home =
      find_named_value_home(context.function.prepared->names,
                            *context.function.value_locations,
                            cast.result);
  const auto* result_storage =
      result_home != nullptr
          ? find_storage_plan_value(*context.function.storage_plan,
                                    result_home->value_id)
          : nullptr;
  if (result_home == nullptr || result_storage == nullptr) {
    return std::nullopt;
  }

  const auto scratch = abi::reserved_mir_scratch_fp_simd_registers().front();
  const auto result_scratch_view =
      abi::fp_simd_register(scratch.index,
                            is_float_extend ? abi::RegisterView::D
                                            : abi::RegisterView::S);
  const auto source_scratch_view =
      abi::fp_simd_register(scratch.index,
                            is_float_extend ? abi::RegisterView::S
                                            : abi::RegisterView::D);
  if (!result_scratch_view.has_value() || !source_scratch_view.has_value()) {
    return std::nullopt;
  }

  const auto* source_home =
      find_named_value_home(context.function.prepared->names,
                            *context.function.value_locations,
                            cast.operand);
  std::vector<std::string> lines;
  std::optional<std::string> source_name;
  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(context.function.prepared->names,
                                       *context.function.value_locations,
                                       *context.function.storage_plan,
                                       cast.operand,
                                       source);
      error == PreparedScalarAluRecordError::None) {
    const auto* source_register = std::get_if<RegisterOperand>(&source.payload);
    if (source.kind == OperandKind::Register && source_register != nullptr &&
        abi::is_fp_simd_register(source_register->reg)) {
      source_name =
          fp_register_name_with_view(*source_register,
                                     is_float_extend ? abi::RegisterView::S
                                                     : abi::RegisterView::D);
    }
  }
  if (!source_name.has_value() &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->offset_bytes.has_value()) {
    const auto source_offset =
        prepared_frame_load_offset(context, *source_home)
            .value_or(static_cast<std::int64_t>(*source_home->offset_bytes));
    std::ostringstream load;
    load << "ldr " << abi::register_name(*source_scratch_view) << ", [sp";
    if (source_offset != 0) {
      load << ", #" << source_offset;
    }
    load << "]";
    lines.push_back(load.str());
    source_name = std::string{abi::register_name(*source_scratch_view)};
  }
  if (!source_name.has_value()) {
    return std::nullopt;
  }

  std::optional<std::string> result_name;
  const bool result_is_stack =
      result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      result_home->offset_bytes.has_value();
  const bool result_is_register =
      result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
      result_home->register_name.has_value();
  if (result_is_stack) {
    result_name = std::string{abi::register_name(*result_scratch_view)};
  } else if (result_is_register) {
    if (const auto result_register =
            make_prepared_register_operand(*result_home,
                                           *result_storage,
                                           cast.result.type,
                                           RegisterOperandRole::StoragePlan)) {
      result_name =
          fp_register_name_with_view(*result_register,
                                     is_float_extend ? abi::RegisterView::D
                                                     : abi::RegisterView::S);
    }
  }
  if (!result_name.has_value()) {
    return std::nullopt;
  }

  std::ostringstream asm_text;
  asm_text << "fcvt " << *result_name << ", " << *source_name;
  lines.push_back(asm_text.str());
  if (result_is_stack) {
    std::ostringstream store;
    store << "str " << *result_name << ", [sp";
    if (*result_home->offset_bytes != 0) {
      store << ", #" << *result_home->offset_bytes;
    }
    store << "]";
    lines.push_back(store.str());
  }

  std::string asm_payload;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_payload += '\n';
    }
    asm_payload += lines[index];
  }
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = result_home->value_id,
          .value_name = result_home->value_name,
      }},
      .uses = source_home != nullptr
                  ? std::vector<MachineEffectResource>{MachineEffectResource{
                        .kind = MachineEffectResourceKind::PreparedValue,
                        .value_id = source_home->value_id,
                        .value_name = source_home->value_name,
                    }}
                  : std::vector<MachineEffectResource>{},
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_payload),
          },
  };
  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  const auto result = instruction_result_value(inst);
  const bool supported_binary =
      binary != nullptr &&
      branch_condition_suffix(binary->opcode).has_value() &&
      (binary->operand_type == bir::TypeKind::F32 ||
       binary->operand_type == bir::TypeKind::F64);
  if ((cast == nullptr && !supported_binary) || !result.has_value()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, *result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto* storage = [&]() -> const prepare::PreparedStoragePlanValue* {
    if (context.function.storage_plan == nullptr) {
      return nullptr;
    }
    for (const auto& value : context.function.storage_plan->values) {
      if (value.value_id == home->value_id) {
        return &value;
      }
    }
    return nullptr;
  }();
  if (storage == nullptr ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage->register_name != home->register_name) {
    return std::nullopt;
  }
  const auto expected_view = scalar_view_for_type(result->type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto target_register = abi::gp_register(parsed->index, *expected_view);
  if (!target_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != parsed->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          parsed->index,
                                          *scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *target_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] bool has_block_entry_stack_edge_consumer_preservation(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    const prepare::PreparedValueHome& destination_home) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr ||
      cast.operand.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto source_name = prepared_named_value_id(context, cast.operand);
  if (!source_name.has_value()) {
    return false;
  }
  const auto* source_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *source_name);
  if (source_home == nullptr) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
          move.authority_kind == prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
          move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
          move.from_value_id == source_home->value_id &&
          move.to_value_id == destination_home.value_id) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool append_simple_integer_cast_from_preserved_stack_source(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    const prepare::PreparedValueHome& destination_home,
    std::vector<std::string>& lines) {
  if (!has_block_entry_stack_edge_consumer_preservation(
          context, cast, destination_home)) {
    return false;
  }
  const auto source_bits = integer_bit_width(cast.operand.type);
  const auto result_bits = integer_bit_width(cast.result.type);
  const auto source_view = scalar_view_for_type(cast.operand.type);
  const auto result_view = scalar_view_for_type(cast.result.type);
  const auto load_mnemonic = scalar_load_mnemonic(cast.operand.type);
  const auto store_mnemonic = scalar_store_mnemonic(cast.result.type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      !source_view.has_value() || !result_view.has_value() ||
      !load_mnemonic.has_value() || !store_mnemonic.has_value() ||
      !destination_home.offset_bytes.has_value()) {
    return false;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return false;
  }
  const auto source_register =
      gp_register_name(scratches.front().index, *source_view);
  const auto result_register =
      gp_register_name(scratches.front().index, *result_view);
  if (!source_register.has_value() || !result_register.has_value()) {
    return false;
  }

  lines.push_back(std::string{*load_mnemonic} + " " + *source_register + ", " +
                  frame_slot_address(context.function,
                                     *destination_home.offset_bytes));

  std::ostringstream cast_line;
  switch (cast.opcode) {
    case bir::CastOpcode::SExt:
      if (*source_bits == 1U) {
        cast_line << "sbfx " << *result_register << ", " << *source_register
                  << ", #0, #1";
      } else if (*source_bits == 8U) {
        cast_line << "sxtb " << *result_register << ", " << *source_register;
      } else if (*source_bits == 16U) {
        cast_line << "sxth " << *result_register << ", " << *source_register;
      } else if (*source_bits == 32U && *result_bits == 64U) {
        cast_line << "sxtw " << *result_register << ", " << *source_register;
      } else {
        return false;
      }
      break;
    case bir::CastOpcode::ZExt:
      if (*source_bits == *result_bits) {
        cast_line << "mov " << *result_register << ", " << *source_register;
      } else {
        cast_line << "ubfx " << *result_register << ", " << *source_register
                  << ", #0, #" << *source_bits;
      }
      break;
    case bir::CastOpcode::Trunc:
      if (*result_bits > 32U) {
        return false;
      }
      if (*result_bits == 32U) {
        cast_line << "mov " << *result_register << ", " << *source_register;
      } else {
        const std::uint64_t mask = (std::uint64_t{1} << *result_bits) - 1U;
        cast_line << "and " << *result_register << ", " << *source_register
                  << ", #" << mask;
      }
      break;
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  lines.push_back(cast_line.str());
  lines.push_back(std::string{*store_mnemonic} + " " + *result_register + ", " +
                  frame_slot_address(context.function,
                                     *destination_home.offset_bytes));
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_stack(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto result = instruction_result_value(inst);
  if (cast == nullptr || !result.has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto mnemonic = scalar_store_mnemonic(result->type);
  const auto value_view = scalar_view_for_type(result->type);
  if (!mnemonic.has_value() || !value_view.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  const auto target = gp_register_name(scratches.front().index, *value_view);
  if (!target.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (append_simple_integer_cast_from_preserved_stack_source(
          context, *cast, *home, lines)) {
    return make_select_chain_materialization_instruction(
        context, instruction_index, std::move(lines));
  }
  const auto source_bits = integer_bit_width(cast->operand.type);
  const auto result_bits = integer_bit_width(cast->result.type);
  if (cast->opcode == bir::CastOpcode::ZExt && source_bits.has_value() &&
      result_bits.has_value() && *source_bits == *result_bits &&
      cast->operand.kind == bir::Value::Kind::Named) {
    const auto source_name = prepared_named_value_id(context, cast->operand);
    const auto source_view = scalar_view_for_type(cast->operand.type);
    const auto emitted_source =
        source_name.has_value()
            ? find_emitted_scalar_register(scalar_state, *source_name)
            : std::nullopt;
    if (source_view.has_value() && emitted_source.has_value() &&
        emitted_source->reg.bank == abi::RegisterBank::GeneralPurpose) {
      const auto source_register =
          abi::gp_register(emitted_source->reg.index, *source_view);
      if (source_register.has_value()) {
        lines.push_back(std::string{*mnemonic} + " " +
                        std::string{abi::register_name(*source_register)} + ", " +
                        frame_slot_address(context.function, *home->offset_bytes));
        return make_select_chain_materialization_instruction(
            context, instruction_index, std::move(lines));
      }
    }
  }
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          scratches.front().index,
                                          scratches[1].index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  frame_slot_address(context.function, *home->offset_bytes));
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

std::optional<module::MachineInstruction> lower_scalar_cast_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }

  const auto* cast = std::get_if<bir::CastInst>(&inst);
  if (cast == nullptr) {
    return std::nullopt;
  }

  auto prepared = make_prepared_scalar_cast_instruction_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.function.storage_plan,
      *cast);
  if (!prepared.record.has_value()) {
    if (auto stack_cast =
            lower_stack_scalar_float_width_cast(context, *cast, instruction_index)) {
      const auto* result_home =
          find_named_value_home(context.function.prepared->names,
                                *context.function.value_locations,
                                cast->result);
      const auto* result_storage =
          result_home != nullptr
              ? find_storage_plan_value(*context.function.storage_plan,
                                        result_home->value_id)
              : nullptr;
      if (result_home != nullptr && result_storage != nullptr) {
        if (const auto result_register =
                make_prepared_register_operand(*result_home,
                                               *result_storage,
                                               cast->result.type,
                                               RegisterOperandRole::StoragePlan)) {
          record_emitted_scalar_register(scalar_state,
                                         result_home->value_name,
                                         *result_register);
        }
      }
      return stack_cast;
    }
    return std::nullopt;
  }

  if (prepared.record->scalar_cast.has_value() && prepared.record->inputs.size() == 1) {
    if (const auto* source_home =
            find_named_value_home(context.function.prepared->names,
                                  *context.function.value_locations,
                                  cast->operand);
        source_home != nullptr) {
      const auto emitted =
          find_emitted_scalar_register(scalar_state, source_home->value_name);
      if (emitted.has_value()) {
        prepared.record->scalar_cast->source = make_register_operand(*emitted);
        prepared.record->inputs[0] = prepared.record->scalar_cast->source;
      } else if (const auto consumer_source =
                     make_prepared_consumer_register_source(
                         context, *cast, instruction_index, *source_home, *prepared.record)) {
        prepared.record->scalar_cast->source = make_register_operand(*consumer_source);
        prepared.record->inputs[0] = prepared.record->scalar_cast->source;
      }
    }
  }
  if (prepared.record->scalar_cast.has_value()) {
    const auto source_is_register =
        prepared.record->scalar_cast->source.kind == OperandKind::Register &&
        std::get_if<RegisterOperand>(&prepared.record->scalar_cast->source.payload) != nullptr;
    const auto source_is_immediate =
        prepared.record->scalar_cast->source.kind == OperandKind::Immediate &&
        std::get_if<ImmediateOperand>(&prepared.record->scalar_cast->source.payload) != nullptr;
    if (!source_is_register && !source_is_immediate &&
        prepared.record->scalar_cast->supported_simple_integer_cast) {
      return std::nullopt;
    }
  }

  InstructionRecord target = make_scalar_instruction(*prepared.record);
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    return std::nullopt;
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (prepared.record->result_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   prepared.record->result_value_name,
                                   *prepared.record->result_register);
  }

  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

}  // namespace c4c::backend::aarch64::codegen
