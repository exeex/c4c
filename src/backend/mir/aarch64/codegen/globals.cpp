#include "globals.hpp"
#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/calls.hpp"
#include "../../../prealloc/stack_layout/stack_layout.hpp"
#include "alu.hpp"
#include "calls.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_publication.hpp"
#include "frame_slot_address.hpp"
#include "memory.hpp"
#include "select_materialization.hpp"

#include <algorithm>
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

PreparedAddressMaterializationRecordResult address_materialization_record_error(
    PreparedAddressMaterializationRecordError error) {
  return PreparedAddressMaterializationRecordResult{.record = std::nullopt, .error = error};
}

PreparedAddressMaterializationInstructionRecordResult
address_materialization_instruction_record_error(
    PreparedAddressMaterializationRecordError error) {
  return PreparedAddressMaterializationInstructionRecordResult{
      .record = std::nullopt, .error = error};
}

void append_address_materialization_diagnostic(
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

[[nodiscard]] std::string address_materialization_error_message(
    PreparedAddressMaterializationRecordError error) {
  std::string message =
      "AArch64 address materialization lowering requires prepared address facts";
  message += "; error=";
  message += prepared_address_materialization_record_error_name(error);
  return message;
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

std::string bad_address_materialization_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

mir::TargetInstructionPrintResult address_materialization_unsupported(
    std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult address_materialization_printed(
    std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
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

[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_global_memory_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return nullptr;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return addressing != nullptr
             ? prepare::find_prepared_memory_access(
                   *addressing, context.control_flow_block->block_label, instruction_index)
             : nullptr;
}

[[nodiscard]] std::optional<prepare::PreparedSameBlockGlobalLoadAccess>
prepared_current_global_load_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return prepare::find_prepared_global_load_access(
      context.function.prepared->names,
      addressing,
      context.control_flow_block->block_label,
      instruction_index,
      load_global);
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
  if (home.register_name.has_value() && storage.register_name.has_value() &&
      *home.register_name == *storage.register_name) {
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  } else if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    return std::nullopt;
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

std::optional<RegisterOperand> make_call_argument_address_register_operand(
    const prepare::PreparedCallArgumentPlan& argument,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  if (!argument.destination_register_name.has_value() ||
      argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr) {
    return std::nullopt;
  }
  const auto prepared_class =
      register_class_from_bank(*argument.destination_register_bank);
  abi::PreparedRegisterConversionResult converted;
  if (argument.destination_register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *argument.destination_register_placement, prepared_class, abi::RegisterView::X);
  } else {
    converted = abi::convert_prepared_register(
        *argument.destination_register_name,
        argument.destination_register_bank,
        prepared_class,
        abi::RegisterView::X);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepared_class,
      .prepared_bank = *argument.destination_register_bank,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = argument.destination_contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = argument.destination_occupied_register_names.empty()
                                ? occupied_register_views(*converted.reg)
                                : std::vector<std::string_view>(
                                      argument.destination_occupied_register_names.begin(),
                                      argument.destination_occupied_register_names.end()),
  };
}

std::optional<AddressMaterializationKind> selected_address_materialization_kind(
    prepare::PreparedAddressMaterializationKind kind) {
  switch (kind) {
    case prepare::PreparedAddressMaterializationKind::FrameSlot:
      return AddressMaterializationKind::FrameSlot;
    case prepare::PreparedAddressMaterializationKind::DirectGlobal:
      return AddressMaterializationKind::DirectPageLow12;
    case prepare::PreparedAddressMaterializationKind::GotGlobal:
      return AddressMaterializationKind::GotPageLow12;
    case prepare::PreparedAddressMaterializationKind::TlsGlobal:
      return AddressMaterializationKind::TlsRelative;
    case prepare::PreparedAddressMaterializationKind::StringConstant:
      return AddressMaterializationKind::StringConstant;
    case prepare::PreparedAddressMaterializationKind::Label:
      return AddressMaterializationKind::LabelPageLow12;
    case prepare::PreparedAddressMaterializationKind::None:
      return std::nullopt;
  }
  return std::nullopt;
}

PreparedAddressMaterializationRecordError validate_address_materialization_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedAddressMaterialization& materialization,
    AddressMaterializationRecord& record) {
  switch (materialization.kind) {
    case prepare::PreparedAddressMaterializationKind::FrameSlot:
      if (!materialization.frame_slot_id.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingFrameSlotId;
      }
      record.frame_slot_id = materialization.frame_slot_id;
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::DirectGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::TlsGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      if (!materialization.is_thread_local || !materialization.has_tls_address_space ||
          materialization.address_space != bir::AddressSpace::Tls) {
        return PreparedAddressMaterializationRecordError::TlsFactMismatch;
      }
      if (materialization.tls_model !=
              prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
          materialization.tls_thread_pointer_register !=
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
          materialization.tls_high_relocation !=
              prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
          materialization.tls_low_relocation !=
              prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
        return PreparedAddressMaterializationRecordError::MissingTlsMaterializationFacts;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::StringConstant:
      if (!materialization.text_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingStringIdentity;
      }
      record.text_name = materialization.text_name;
      record.text_label = names.texts.lookup(*materialization.text_name);
      if (record.text_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingStringIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::Label:
      if (!materialization.target_label.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingLabelIdentity;
      }
      record.target_label = materialization.target_label;
      record.target_label_name = prepare::prepared_block_label(names, *materialization.target_label);
      if (record.target_label_name.empty()) {
        return PreparedAddressMaterializationRecordError::MissingLabelIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::GotGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      if (materialization.is_thread_local || materialization.has_tls_address_space ||
          materialization.address_space == bir::AddressSpace::Tls) {
        return PreparedAddressMaterializationRecordError::TlsFactMismatch;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::None:
      return PreparedAddressMaterializationRecordError::UnsupportedAddressKind;
  }
  return PreparedAddressMaterializationRecordError::UnsupportedAddressKind;
}

PreparedAddressMaterializationRecordResult make_address_record_from_prepared_materialization(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    const prepare::PreparedAddressMaterialization& materialization) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::InvalidFunction);
  }
  if (materialization.function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::InvalidFunction);
  }

  const auto selected_kind = selected_address_materialization_kind(materialization.kind);
  if (!selected_kind.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedAddressKind);
  }

  AddressMaterializationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind = *selected_kind,
      .prepared_kind = materialization.kind,
      .function_name = materialization.function_name,
      .block_label = materialization.block_label,
      .instruction_index = materialization.inst_index,
      .address_materialization_policy = materialization.address_materialization_policy,
      .byte_offset = materialization.byte_offset,
      .address_space = materialization.address_space,
      .is_thread_local = materialization.is_thread_local,
      .has_tls_address_space = materialization.has_tls_address_space,
      .tls_model = materialization.tls_model,
      .tls_thread_pointer_register = materialization.tls_thread_pointer_register,
      .tls_high_relocation = materialization.tls_high_relocation,
      .tls_low_relocation = materialization.tls_low_relocation,
      .source_materialization = &materialization,
  };

  const auto identity_error =
      validate_address_materialization_identity(names, materialization, record);
  if (identity_error != PreparedAddressMaterializationRecordError::None) {
    return address_materialization_record_error(identity_error);
  }
  if (record.kind == AddressMaterializationKind::DeferredUnsupported) {
    record.result_value_id = materialization.result_value_id;
    record.result_value_name =
        materialization.result_value_name.value_or(c4c::kInvalidValueName);
    record.result_home_kind = materialization.result_home_kind.value_or(
        prepare::PreparedValueHomeKind::None);
    return PreparedAddressMaterializationRecordResult{
        .record = record,
        .error = PreparedAddressMaterializationRecordError::None,
    };
  }
  if (!materialization.result_value_name.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueName);
  }
  const auto result_value_name = *materialization.result_value_name;
  const auto* result_home = prepare::find_prepared_value_home(value_locations, result_value_name);
  if (result_home == nullptr) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedResultStorage);
  }
  auto result_register =
      make_prepared_register_operand(*result_home,
                                     *result_storage,
                                     bir::TypeKind::Ptr,
                                     RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::RegisterConversionFailed);
  }

  record.result_value_id = result_home->value_id;
  record.result_value_name = result_home->value_name;
  record.result_home_kind = result_home->kind;
  record.result_register = *result_register;
  return PreparedAddressMaterializationRecordResult{
      .record = record,
      .error = PreparedAddressMaterializationRecordError::None,
  };
}

PreparedAddressMaterializationRecordResult make_address_record_from_prepared_materialization(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  const auto* materialization =
      prepare::find_prepared_address_materialization(addressing, block_label, instruction_index);
  if (materialization == nullptr) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingPreparedAddressMaterialization);
  }
  return make_address_record_from_prepared_materialization(
      names, value_locations, storage_plan, addressing, *materialization);
}

PreparedAddressMaterializationRecordResult make_call_argument_address_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedAddressingFunction& addressing,
    const prepare::PreparedAddressMaterialization& materialization,
    const prepare::PreparedCallPlan& call_plan) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      materialization.function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::InvalidFunction);
  }
  const auto selected_kind = selected_address_materialization_kind(materialization.kind);
  if (!selected_kind.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedAddressKind);
  }

  AddressMaterializationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind = *selected_kind,
      .prepared_kind = materialization.kind,
      .function_name = materialization.function_name,
      .block_label = materialization.block_label,
      .instruction_index = materialization.inst_index,
      .address_materialization_policy = materialization.address_materialization_policy,
      .byte_offset = materialization.byte_offset,
      .address_space = materialization.address_space,
      .is_thread_local = materialization.is_thread_local,
      .has_tls_address_space = materialization.has_tls_address_space,
      .tls_model = materialization.tls_model,
      .tls_thread_pointer_register = materialization.tls_thread_pointer_register,
      .tls_high_relocation = materialization.tls_high_relocation,
      .tls_low_relocation = materialization.tls_low_relocation,
      .source_materialization = &materialization,
  };
  const auto identity_error =
      validate_address_materialization_identity(names, materialization, record);
  if (identity_error != PreparedAddressMaterializationRecordError::None) {
    return address_materialization_record_error(identity_error);
  }
  if (!materialization.result_value_name.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueName);
  }
  const auto result_value_name = *materialization.result_value_name;
  const std::string_view result_value_text = names.value_names.spelling(result_value_name);
  const auto argument_it =
      std::find_if(call_plan.arguments.begin(),
                   call_plan.arguments.end(),
                   [&](const prepare::PreparedCallArgumentPlan& argument) {
                     const bool same_value_id =
                         materialization.result_value_id.has_value() &&
                         argument.source_value_id == materialization.result_value_id;
                     const bool same_symbol_text =
                         argument.source_symbol_name.has_value() &&
                         *argument.source_symbol_name == result_value_text;
                     return argument.source_encoding ==
                                prepare::PreparedStorageEncodingKind::SymbolAddress &&
                            (same_value_id || same_symbol_text) &&
                            argument.source_value_id.has_value() &&
                            argument.destination_register_bank ==
                                prepare::PreparedRegisterBank::Gpr;
                   });
  if (argument_it == call_plan.arguments.end()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedResultStorage);
  }
  auto result_register =
      make_call_argument_address_register_operand(
          *argument_it, *argument_it->source_value_id, result_value_name);
  if (!result_register.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::RegisterConversionFailed);
  }
  record.result_value_id = argument_it->source_value_id;
  record.result_value_name = result_value_name;
  record.result_home_kind = prepare::PreparedValueHomeKind::Register;
  record.result_register = *result_register;
  return PreparedAddressMaterializationRecordResult{
      .record = record,
      .error = PreparedAddressMaterializationRecordError::None,
  };
}

std::optional<module::MachineInstruction> lower_address_materialization_record(
    const module::BlockLoweringContext& context,
    PreparedAddressMaterializationInstructionRecordResult prepared,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!prepared.record.has_value()) {
    if (prepared.error != PreparedAddressMaterializationRecordError::
                              MissingPreparedAddressMaterialization) {
      append_address_materialization_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          address_materialization_error_message(prepared.error));
    }
    return std::nullopt;
  }

  if (prepared.record->kind == AddressMaterializationKind::FrameSlot &&
      context.function.frame_plan != nullptr &&
      context.function.frame_plan->uses_frame_pointer_for_fixed_slots) {
    prepared.record->uses_frame_pointer_base = true;
  }

  InstructionRecord target =
      make_address_materialization_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_address_materialization_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

}  // namespace

[[nodiscard]] const bir::Global* find_load_global_target(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global) {
  if (context.function.prepared == nullptr) {
    return nullptr;
  }
  const auto& globals = context.function.prepared->module.globals;
  if (load_global.global_name_id != c4c::kInvalidLinkName) {
    const auto it = std::find_if(
        globals.begin(),
        globals.end(),
        [&](const bir::Global& global) {
          return global.link_name_id == load_global.global_name_id;
        });
    if (it != globals.end()) {
      return &*it;
    }
  }
  if (load_global.global_name.empty()) {
    return nullptr;
  }
  const auto it = std::find_if(
      globals.begin(),
      globals.end(),
      [&](const bir::Global& global) {
        return global.name == load_global.global_name;
      });
  return it == globals.end() ? nullptr : &*it;
}

[[nodiscard]] std::string load_global_symbol_label(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const bir::Global* target_global) {
  if (context.function.prepared != nullptr &&
      load_global.global_name_id != c4c::kInvalidLinkName) {
    const std::string_view semantic_name =
        context.function.prepared->module.names.link_names.spelling(
            load_global.global_name_id);
    if (!semantic_name.empty()) {
      return std::string{semantic_name};
    }
  }
  if (target_global != nullptr && !target_global->name.empty()) {
    return target_global->name;
  }
  return load_global.global_name;
}

[[nodiscard]] bool emit_prepared_global_symbol_load_to_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr) {
    return false;
  }
  const auto* access = prepared_global_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto policy = prepare::prepared_global_symbol_address_policy(
      access->address, context.function.target_profile);
  if (!policy.has_value() ||
      *policy != bir::GlobalAddressMaterializationPolicy::Direct) {
    return false;
  }
  const auto symbol_name =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *access->address.symbol_name);
  const auto mnemonic = scalar_load_mnemonic(type);
  const auto target_view = scalar_view_for_type(type);
  const auto target = target_view.has_value()
                          ? gp_register_name(target_index, *target_view)
                          : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (symbol_name.empty() || !mnemonic.has_value() || !target.has_value() ||
      !address.has_value()) {
    return false;
  }
  const auto symbol = relocation_operand(symbol_name, access->address.byte_offset);
  lines.push_back("adrp " + *address + ", " + symbol);
  lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
  lines.push_back(std::string{*mnemonic} + " " + *target + ", [" + *address + "]");
  return true;
}

[[nodiscard]] bool emit_prepared_global_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const prepare::PreparedMemoryAccess& access,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr) {
    return false;
  }
  const auto symbol_label =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *access.address.symbol_name);
  const auto mnemonic = scalar_load_mnemonic(load_global.result.type);
  const auto load_view = scalar_view_for_type(load_global.result.type);
  const auto load_target =
      load_view.has_value() ? gp_register_name(target_index, *load_view) : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (symbol_label.empty() || !mnemonic.has_value() || !load_target.has_value() ||
      !address.has_value()) {
    return false;
  }

  const auto policy =
      prepared_load_global_address_policy(context, load_global, access);
  if (!policy.has_value()) {
    return false;
  }

  switch (*policy) {
    case bir::GlobalAddressMaterializationPolicy::GotRequired:
      lines.push_back("adrp " + *address + ", :got:" + std::string{symbol_label});
      lines.push_back("ldr " + *address + ", [" + *address + ", :got_lo12:" +
                      std::string{symbol_label} + "]");
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                      register_indirect_address(*address, access.address.byte_offset));
      return true;
    case bir::GlobalAddressMaterializationPolicy::Direct: {
      const auto symbol = relocation_operand(symbol_label, access.address.byte_offset);
      lines.push_back("adrp " + *address + ", " + symbol);
      lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", [" + *address +
                      "]");
      return true;
    }
    case bir::GlobalAddressMaterializationPolicy::Unspecified:
      return false;
  }
  return false;
}

std::optional<bir::GlobalAddressMaterializationPolicy>
prepared_load_global_address_policy(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const prepare::PreparedMemoryAccess& access) {
  if (access.address.global_address_materialization_policy !=
      bir::GlobalAddressMaterializationPolicy::Unspecified) {
    return prepare::prepared_global_symbol_address_policy(
        access.address, context.function.target_profile);
  }

  const auto* global = find_load_global_target(context, load_global);
  if (global != nullptr &&
      global->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
    return global->address_materialization_policy;
  }

  return prepare::prepared_global_symbol_address_policy(
      access.address, context.function.target_profile);
}

std::optional<module::MachineInstruction> make_load_global_got_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global,
    BlockScalarLoweringState& scalar_state) {
  const auto prepared_access =
      prepared_current_global_load_access(context, instruction_index, load_global);
  if (!prepared_access.has_value() ||
      prepared_access->access == nullptr ||
      prepared_access->access->address.base_kind !=
          prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !prepared_access->access->address.symbol_name.has_value()) {
    return std::nullopt;
  }
  const auto address_policy =
      prepared_load_global_address_policy(context, load_global, *prepared_access->access);
  if (!address_policy.has_value() ||
      *address_policy != bir::GlobalAddressMaterializationPolicy::GotRequired) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, load_global.result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto result_view = scalar_view_for_type(load_global.result.type);
  if (!result_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_result = make_named_prepared_result_register(context, load_global.result);
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const auto result_register =
      prepared_result.has_value()
          ? std::optional<abi::RegisterReference>{prepared_result->reg}
          : abi::gp_register(scratches.front().index, *result_view);
  if (!result_register.has_value() ||
      result_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto address_register =
      abi::gp_register(result_register->index, abi::RegisterView::X);
  const auto target_register =
      abi::gp_register(result_register->index, *result_view);
  if (!address_register.has_value() || !target_register.has_value()) {
    return std::nullopt;
  }
  const std::string address = abi::register_name(*address_register);
  const std::string target = abi::register_name(*target_register);
  const auto symbol_label =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *prepared_access->access->address.symbol_name);
  if (symbol_label.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  lines.push_back("adrp " + address + ", :got:" + std::string{symbol_label});
  lines.push_back("ldr " + address + ", [" + address + ", :got_lo12:" +
                  std::string{symbol_label} + "]");
  lines.push_back("ldr " + target + ", " +
                  register_indirect_address(address,
                                            prepared_access->access->address.byte_offset));

  if (const auto* home = prepared_value_home_for_value(context, load_global.result);
      home != nullptr &&
      home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    lines.push_back("str " + target + ", " +
                    frame_slot_address(context.function, *home->offset_bytes));
    return make_select_chain_materialization_instruction(
        context, instruction_index, std::move(lines));
  }

  RegisterOperand emitted{
      .reg = *target_register,
      .role = prepared_result.has_value() ? prepared_result->role
                                          : RegisterOperandRole::StoragePlan,
      .value_name = *value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = result_view,
  };
  record_emitted_scalar_register(scalar_state, *value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

PreparedAddressMaterializationRecordResult make_prepared_address_materialization_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  return make_address_record_from_prepared_materialization(
      names, value_locations, storage_plan, addressing, block_label, instruction_index);
}

PreparedAddressMaterializationInstructionRecordResult
make_prepared_address_materialization_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  const auto result = make_prepared_address_materialization_record(
      names, value_locations, storage_plan, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return address_materialization_instruction_record_error(result.error);
  }
  return PreparedAddressMaterializationInstructionRecordResult{
      .record = *result.record,
      .error = PreparedAddressMaterializationRecordError::None,
  };
}

std::optional<module::MachineInstruction> lower_address_materialization(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const auto prepared =
      make_prepared_address_materialization_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          *context.function.storage_plan,
          *addressing,
          context.control_flow_block->block_label,
          instruction_index);
  return lower_address_materialization_record(
      context, prepared, instruction_index, diagnostics);
}

[[nodiscard]] std::optional<std::size_t> prepared_frame_address_offset_for_value(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::optional<std::size_t> before_or_at_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.address_materialization_lookups == nullptr ||
      context.control_flow_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto resolved =
      prepare::find_indexed_prepared_frame_address_offset_for_value(
          context.function.prepared->stack_layout,
          context.function.address_materialization_lookups,
          context.control_flow_block->block_label,
          value_name,
          before_or_at_instruction_index);
  return resolved.has_value()
             ? std::optional<std::size_t>{resolved->stack_offset_bytes}
             : std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> local_aggregate_address_frame_offset(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  return prepared_frame_address_offset_for_value(context, value_name, std::nullopt);
}

[[nodiscard]] bool emit_local_slot_address_publication_to_register_impl(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::optional<std::size_t> instruction_index,
    std::vector<std::string>& lines) {
  if (binary.result.type != bir::TypeKind::Ptr ||
      binary.operand_type != bir::TypeKind::Ptr ||
      (binary.opcode != bir::BinaryOpcode::Add &&
       binary.opcode != bir::BinaryOpcode::Sub) ||
      binary.lhs.kind != bir::Value::Kind::Named ||
      binary.rhs.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  const auto lhs_name = prepared_named_value_id(context, binary.lhs);
  const auto base_offset =
      lhs_name.has_value()
          ? prepared_frame_address_offset_for_value(
                context, *lhs_name, instruction_index)
          : std::nullopt;
  if (!base_offset.has_value()) {
    return false;
  }
  const auto target_register = abi::gp_register(target_index, abi::RegisterView::X);
  const auto target = target_register.has_value()
                          ? std::optional<std::string>{
                                std::string{abi::register_name(*target_register)}}
                          : std::nullopt;
  if (!target.has_value()) {
    return false;
  }
  const std::int64_t signed_base =
      static_cast<std::int64_t>(*base_offset);
  const std::int64_t signed_delta =
      binary.opcode == bir::BinaryOpcode::Add ? binary.rhs.immediate
                                              : -binary.rhs.immediate;
  const std::int64_t adjusted_offset = signed_base + signed_delta;
  if (adjusted_offset < 0) {
    return false;
  }
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  lines.push_back("add " + *target + ", " + std::string{base} + ", #" +
                  std::to_string(adjusted_offset));
  return true;
}

[[nodiscard]] bool emit_local_slot_address_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::optional<std::size_t> before_or_at_instruction_index,
    std::vector<std::string>& lines) {
  return emit_local_slot_address_publication_to_register_impl(
      context, binary, target_index, before_or_at_instruction_index, lines);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_local_slot_address_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr || binary->result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, binary->result);
  std::optional<std::size_t> result_stack_offset_bytes;
  if (!result_register.has_value()) {
    const auto* home = prepared_value_home_for_value(context, binary->result);
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, abi::RegisterView::X);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = RegisterOperand{
        .reg = *scratch,
        .role = RegisterOperandRole::SpillAuthority,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = abi::RegisterView::X,
    };
    result_stack_offset_bytes = *home->offset_bytes;
  }
  if (!abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_local_slot_address_publication_to_register_impl(
          context, *binary, result_register->reg.index, instruction_index, lines)) {
    return std::nullopt;
  }
  if (result_stack_offset_bytes.has_value()) {
    const auto result_name =
        gp_register_name(result_register->reg.index, abi::RegisterView::X);
    if (!result_name.has_value()) {
      return std::nullopt;
    }
    lines.push_back("str " + *result_name + ", " +
                    frame_slot_address(context.function, *result_stack_offset_bytes));
  }
  record_emitted_scalar_register(
      scalar_state, result_register->value_name, *result_register);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

void record_address_materialization_result(
    BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* address_record =
      std::get_if<AddressMaterializationRecord>(&instruction.target.payload);
  if (address_record == nullptr || !address_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 address_record->result_value_name,
                                 *address_record->result_register);
}

BlockAddressMaterializationIndex make_block_address_materialization_index(
    const module::BlockLoweringContext& context) {
  BlockAddressMaterializationIndex index;
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return index;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return index;
  }
  if (const auto* materializations =
          prepare::find_indexed_prepared_address_materializations(
              context.function.address_materialization_lookups,
              context.control_flow_block->block_label)) {
    index.materializations = *materializations;
    for (const auto* materialization : index.materializations) {
      if (materialization != nullptr) {
        index.materializations_by_instruction[materialization->inst_index].push_back(
            materialization);
      }
    }
    std::sort(index.materializations.begin(),
              index.materializations.end(),
              [](const auto* lhs, const auto* rhs) {
                if (lhs == nullptr || rhs == nullptr) {
                  return rhs != nullptr;
                }
                return lhs->inst_index < rhs->inst_index;
              });
    return index;
  }
  index.materializations = prepare::collect_prepared_address_materializations_for_block(
      *addressing, context.control_flow_block->block_label);
  for (const auto* materialization : index.materializations) {
    if (materialization != nullptr) {
      index.materializations_by_instruction[materialization->inst_index].push_back(
          materialization);
    }
  }
  std::sort(index.materializations.begin(),
            index.materializations.end(),
            [](const auto* lhs, const auto* rhs) {
              if (lhs == nullptr || rhs == nullptr) {
                return rhs != nullptr;
              }
              return lhs->inst_index < rhs->inst_index;
            });
  return index;
}

std::vector<module::MachineInstruction> lower_address_materializations(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto index = make_block_address_materialization_index(context);
  return lower_address_materializations(context, index, instruction_index, diagnostics);
}

std::vector<module::MachineInstruction> lower_address_materializations(
    const module::BlockLoweringContext& context,
    const BlockAddressMaterializationIndex& address_materializations,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return lowered;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return lowered;
  }
  const auto* call_plan = find_prepared_call_plan(context, instruction_index);
  auto lower_materialization =
      [&](const prepare::PreparedAddressMaterialization& materialization,
          bool exact_instruction) {
        PreparedAddressMaterializationRecordResult prepared =
            exact_instruction
                ? make_address_record_from_prepared_materialization(
                      context.function.prepared->names,
                      *context.function.value_locations,
                      *context.function.storage_plan,
                      *addressing,
                      materialization)
                : address_materialization_record_error(
                      PreparedAddressMaterializationRecordError::UnsupportedResultStorage);
        if (!prepared.record.has_value() &&
            prepared.error == PreparedAddressMaterializationRecordError::
                                  UnsupportedResultStorage &&
            call_plan != nullptr) {
          prepared = make_call_argument_address_record(
              context.function.prepared->names, *addressing, materialization, *call_plan);
        }
        if (!exact_instruction && !prepared.record.has_value()) {
          return;
        }
        auto instruction = lower_address_materialization_record(
            context,
            PreparedAddressMaterializationInstructionRecordResult{
                .record = std::move(prepared.record),
                .error = prepared.error,
            },
            instruction_index,
            diagnostics);
        if (instruction.has_value()) {
          lowered.push_back(std::move(*instruction));
        }
      };

  if (const auto exact_it =
          address_materializations.materializations_by_instruction.find(instruction_index);
      exact_it != address_materializations.materializations_by_instruction.end()) {
    for (const auto* materialization_ptr : exact_it->second) {
      if (materialization_ptr != nullptr) {
        lower_materialization(*materialization_ptr, true);
      }
    }
  }
  if (call_plan == nullptr) {
    return lowered;
  }
  for (const auto* materialization_ptr : address_materializations.materializations) {
    if (materialization_ptr == nullptr) {
      continue;
    }
    const auto& materialization = *materialization_ptr;
    if (materialization.inst_index > instruction_index) {
      break;
    }
    if (materialization.inst_index == instruction_index) {
      continue;
    }
    lower_materialization(materialization, false);
  }
  return lowered;
}

mir::TargetInstructionPrintResult print_address_materialization_instruction(
    const InstructionRecord& instruction,
    const AddressMaterializationRecord& address) {
  const auto bad_header = bad_address_materialization_header(instruction);
  if (!address.result_register.has_value()) {
    return address_materialization_unsupported(
        bad_header + "address materialization node is missing result register");
  }

  std::string_view label;
  switch (address.kind) {
    case AddressMaterializationKind::FrameSlot:
      break;
    case AddressMaterializationKind::DirectPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return address_materialization_unsupported(
            bad_header + "direct address materialization is missing symbol label");
      }
      break;
    case AddressMaterializationKind::GotPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return address_materialization_unsupported(
            bad_header + "GOT address materialization is missing symbol label");
      }
      if (address.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
        return address_materialization_unsupported(
            bad_header + "GOT address materialization is missing GOT-required policy");
      }
      break;
    case AddressMaterializationKind::StringConstant:
      label = address.text_label;
      if (label.empty()) {
        return address_materialization_unsupported(
            bad_header + "string address materialization is missing text label");
      }
      break;
    case AddressMaterializationKind::LabelPageLow12:
      label = address.target_label_name;
      if (label.empty()) {
        return address_materialization_unsupported(
            bad_header + "label address materialization is missing target label text");
      }
      break;
    case AddressMaterializationKind::TlsRelative:
      label = address.symbol_label;
      if (label.empty()) {
        return address_materialization_unsupported(
            bad_header + "TLS address materialization is missing symbol label");
      }
      if (address.tls_model !=
              prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
          address.tls_thread_pointer_register !=
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0) {
        return address_materialization_unsupported(
            bad_header + "TLS address materialization is missing local-exec TLS facts");
      }
      if (address.tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
          address.tls_low_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
        return address_materialization_unsupported(
            bad_header +
            "TLS address materialization is missing thread-pointer-relative relocations");
      }
      break;
    case AddressMaterializationKind::DeferredUnsupported:
      return address_materialization_unsupported(
          bad_header + "address materialization printer path is deferred for " +
          std::string(prepare::prepared_address_materialization_kind_name(
              address.prepared_kind)));
  }

  const std::string result = abi::register_name(address.result_register->reg);
  if (address.kind == AddressMaterializationKind::FrameSlot) {
    const std::string_view base = address.uses_frame_pointer_base ? "x29" : "sp";
    if (address.byte_offset == 0) {
      return address_materialization_printed({"mov " + result + ", " + std::string{base}});
    }
    return address_materialization_printed({
        "add " + result + ", " + std::string{base} + ", #" +
            std::to_string(address.byte_offset),
    });
  }
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
    return address_materialization_printed(std::move(lines));
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
    return address_materialization_printed(std::move(lines));
  }

  const std::string reloc = relocation_operand(label, address.byte_offset);
  return address_materialization_printed({
      "adrp " + result + ", " + reloc,
      "add " + result + ", " + result + ", :lo12:" + reloc,
  });
}

}  // namespace c4c::backend::aarch64::codegen
