#include "globals.hpp"
#include "alu.hpp"

#include <optional>
#include <string_view>
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

std::optional<AddressMaterializationKind> selected_address_materialization_kind(
    prepare::PreparedAddressMaterializationKind kind) {
  switch (kind) {
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
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::InvalidFunction);
  }
  const auto* materialization =
      prepare::find_prepared_address_materialization(addressing, block_label, instruction_index);
  if (materialization == nullptr || materialization->function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingPreparedAddressMaterialization);
  }

  const auto selected_kind = selected_address_materialization_kind(materialization->kind);
  if (!selected_kind.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedAddressKind);
  }

  AddressMaterializationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind = *selected_kind,
      .prepared_kind = materialization->kind,
      .function_name = materialization->function_name,
      .block_label = materialization->block_label,
      .instruction_index = materialization->inst_index,
      .address_materialization_policy = materialization->address_materialization_policy,
      .byte_offset = materialization->byte_offset,
      .address_space = materialization->address_space,
      .is_thread_local = materialization->is_thread_local,
      .has_tls_address_space = materialization->has_tls_address_space,
      .tls_model = materialization->tls_model,
      .tls_thread_pointer_register = materialization->tls_thread_pointer_register,
      .tls_high_relocation = materialization->tls_high_relocation,
      .tls_low_relocation = materialization->tls_low_relocation,
      .source_materialization = materialization,
  };

  const auto identity_error =
      validate_address_materialization_identity(names, *materialization, record);
  if (identity_error != PreparedAddressMaterializationRecordError::None) {
    return address_materialization_record_error(identity_error);
  }
  if (record.kind == AddressMaterializationKind::DeferredUnsupported) {
    record.result_value_id = materialization->result_value_id;
    record.result_value_name =
        materialization->result_value_name.value_or(c4c::kInvalidValueName);
    record.result_home_kind = materialization->result_home_kind.value_or(
        prepare::PreparedValueHomeKind::None);
    return PreparedAddressMaterializationRecordResult{
        .record = record,
        .error = PreparedAddressMaterializationRecordError::None,
    };
  }
  if (!materialization->result_value_name.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueName);
  }
  const auto result_value_name = *materialization->result_value_name;
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

}  // namespace c4c::backend::aarch64::codegen
