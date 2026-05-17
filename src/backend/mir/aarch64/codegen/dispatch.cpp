#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "memory.hpp"
#include "returns.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(
    c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] std::optional<prepare::PreparedVariadicEntryHelperKind>
variadic_entry_helper_kind(std::string_view callee) {
  if (callee == "llvm.va_start.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaStart;
  }
  if (callee == "llvm.va_copy.p0.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaCopy;
  }
  constexpr std::string_view va_arg_prefix = "llvm.va_arg.";
  if (callee.substr(0, va_arg_prefix.size()) == va_arg_prefix) {
    if (callee == "llvm.va_arg.aggregate") {
      return prepare::PreparedVariadicEntryHelperKind::VaArgAggregate;
    }
    return prepare::PreparedVariadicEntryHelperKind::VaArg;
  }
  return std::nullopt;
}

void append_missing_variadic_entry_fact(std::vector<std::string>& missing,
                                        std::string_view fact) {
  missing.push_back(std::string{fact});
}

[[nodiscard]] std::vector<std::string> missing_variadic_entry_facts(
    const prepare::PreparedVariadicEntryPlanFunction& plan) {
  std::vector<std::string> missing = plan.missing_required_facts;
  if (!plan.named_register_counts.gp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.gp");
  }
  if (!plan.named_register_counts.fp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.fp");
  }
  if (plan.register_save_area.required) {
    if (!plan.register_save_area.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.size_bytes");
    }
    if (!plan.register_save_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.align_bytes");
    }
    if (!plan.register_save_area.slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.slot_id");
    }
    if (!plan.register_save_area.stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.stack_offset_bytes");
    }
    if (!plan.register_save_area.gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_offset_bytes");
    }
    if (!plan.register_save_area.fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_offset_bytes");
    }
    if (!plan.register_save_area.gp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_slot_size_bytes");
    }
    if (!plan.register_save_area.fp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_slot_size_bytes");
    }
    if (!plan.register_save_area.saved_gp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_gp_register_count");
    }
    if (!plan.register_save_area.saved_fp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_fp_register_count");
    }
    if (!plan.register_save_area.initial_gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_gp_offset_bytes");
    }
    if (!plan.register_save_area.initial_fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_fp_offset_bytes");
    }
  }
  if (plan.overflow_area.required) {
    if (!plan.overflow_area.base_slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_slot_id");
    }
    if (!plan.overflow_area.base_stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_stack_offset_bytes");
    }
    if (!plan.overflow_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.align_bytes");
    }
  }
  if (plan.va_list_layout.required) {
    if (!plan.va_list_layout.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.size_bytes");
    }
    if (!plan.va_list_layout.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.align_bytes");
    }
    if (plan.va_list_layout.fields.empty()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.fields");
    }
  }
  if (!plan.helper_resources.required_helpers.empty()) {
    if (!plan.helper_resources.scratch_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "helper_resources.scratch_register_count");
    }
    if (!plan.helper_resources.scratch_stack_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "helper_resources.scratch_stack_bytes");
    }
    if (plan.helper_operand_homes.empty()) {
      append_missing_variadic_entry_fact(missing, "helper_operand_homes");
    }
  }
  return missing;
}

[[nodiscard]] bool variadic_helper_operand_homes_complete(
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  const auto scalar_access_plan_complete = [&homes]() {
    if (!homes.scalar_access_plan.has_value()) {
      return false;
    }
    const auto& plan = *homes.scalar_access_plan;
    return plan.source_class !=
               prepare::PreparedVariadicScalarVaArgSourceClass::Unknown &&
           plan.value_type != bir::TypeKind::Void &&
           plan.value_size_bytes != 0 &&
           plan.value_align_bytes != 0 &&
           plan.result_home.has_value() &&
           plan.source_field.has_value() &&
           plan.source_field_offset_bytes.has_value() &&
           plan.source_slot_size_bytes.has_value() &&
           plan.progression_field.has_value() &&
           plan.progression_field_offset_bytes.has_value() &&
           plan.progression_stride_bytes.has_value() &&
           plan.overflow_source_field.has_value() &&
           plan.overflow_source_field_offset_bytes.has_value() &&
           plan.overflow_stride_bytes.has_value();
  };
  switch (homes.helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return homes.destination_va_list.has_value();
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return homes.scalar_result.has_value() && homes.source_va_list.has_value() &&
             scalar_access_plan_complete();
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(
          homes);
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return homes.destination_va_list.has_value() && homes.source_va_list.has_value();
  }
  return false;
}

[[nodiscard]] std::string variadic_helper_missing_consumption_fact_message(
    prepare::PreparedVariadicEntryHelperKind helper) {
  switch (helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return {};
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return "AArch64 scalar va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg.scalar_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return "AArch64 aggregate va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg_aggregate.aggregate_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return "AArch64 va_copy lowering requires prepared source and destination "
             "va_list homes";
  }
  return {};
}

[[nodiscard]] std::string variadic_entry_missing_fact_message(
    const std::vector<std::string>& missing) {
  std::string message =
      "AArch64 variadic entry helper lowering requires complete prepared variadic entry facts";
  if (!missing.empty()) {
    message += "; missing fact=";
    message += missing.front();
  }
  return message;
}

[[nodiscard]] const prepare::PreparedVariadicEntryPlanFunction*
require_prepared_variadic_entry_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto function_name = context.function.control_flow != nullptr
                                 ? context.function.control_flow->function_name
                                 : c4c::kInvalidFunctionName;
  const auto* entry_plan =
      context.function.prepared != nullptr
          ? prepare::find_prepared_variadic_entry_plan(*context.function.prepared,
                                                       function_name)
          : nullptr;
  if (entry_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 variadic entry helper lowering requires a PreparedVariadicEntryPlanFunction");
    return nullptr;
  }
  const auto missing = missing_variadic_entry_facts(*entry_plan);
  if (!missing.empty()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        variadic_entry_missing_fact_message(missing));
    return nullptr;
  }
  return entry_plan;
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
    if (carrier.block_index == context.block_index &&
        carrier.inst_index == instruction_index) {
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

[[nodiscard]] std::string prepared_intrinsic_carrier_error_message(
    std::string_view error) {
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

[[nodiscard]] std::optional<abi::RegisterView> intrinsic_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::I128:
      return abi::RegisterView::Q;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I64:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
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
      .accumulator_value_name =
          accumulator ? accumulator->value_name : c4c::kInvalidValueName,
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
  const auto pointer_value_name =
      pointer ? pointer->value_name : c4c::kInvalidValueName;
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

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_intrinsic_instruction(
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
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
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
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          intrinsic_error_message(prepared.error));
      return std::nullopt;
    }

    target = make_scalar_fp_unary_intrinsic_instruction(*prepared.record);
  } else if (carrier->carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        prepared_intrinsic_carrier_error_message("incomplete_prepared_intrinsic_carrier"));
    return std::nullopt;
  } else if (carrier->family == bir::IntrinsicFamilyKind::Crc &&
             carrier->operation == bir::IntrinsicOperationKind::Crc32W) {
    target = make_crc32w_intrinsic_record_from_carrier(*context.function.storage_plan,
                                                       *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorMemory &&
             carrier->operation == bir::IntrinsicOperationKind::VectorLoad) {
    target = make_vector_load_intrinsic_record_from_carrier(
        context, instruction_index, *context.function.storage_plan, *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorOperation &&
             carrier->operation == bir::IntrinsicOperationKind::VectorAdd) {
    target = make_vector_add_intrinsic_record_from_carrier(*context.function.storage_plan,
                                                           *carrier);
  } else {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
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
    append_call_diagnostic(diagnostics,
                           module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                           context,
                           instruction_index,
                           std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (call->inline_asm.has_value() ||
            has_prepared_inline_asm_carrier(context, instruction_index)) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (find_prepared_intrinsic_carrier(context, instruction_index) != nullptr) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          auto before_call_moves =
              lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            block.instructions.push_back(std::move(before_call_move));
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          if (call_plan != nullptr) {
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        if (const auto* address_record =
                std::get_if<AddressMaterializationRecord>(&lowered->target.payload);
            address_record != nullptr && address_record->result_register.has_value()) {
          record_emitted_scalar_register(scalar_state,
                                         address_record->result_value_name,
                                         *address_record->result_register);
        }
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_copy =
                     lower_i128_copy_instruction(context, inst, instruction_index, diagnostics);
                 lowered_i128_copy.handled) {
        if (lowered_i128_copy.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_copy.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_prepared_scalar_float_alu_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, instruction_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (lowered_ordinary_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_ordinary_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (auto lowered =
            lower_prepared_conditional_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen
