#include "globals.hpp"
#include "alu.hpp"
#include "calls.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

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

std::string_view register_display_name(abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
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
  const auto display_name = register_display_name(reg);
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
  if (context.function.address_materialization_indexes != nullptr) {
    const auto it = context.function.address_materialization_indexes
                        ->materializations_by_block.find(
                            context.control_flow_block->block_label);
    if (it != context.function.address_materialization_indexes
                  ->materializations_by_block.end()) {
      index.materializations = it->second;
    }
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
  index.materializations.reserve(addressing->address_materializations.size());
  for (const auto& materialization : addressing->address_materializations) {
    if (materialization.block_label == context.control_flow_block->block_label) {
      index.materializations.push_back(&materialization);
      index.materializations_by_instruction[materialization.inst_index].push_back(
          &materialization);
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
