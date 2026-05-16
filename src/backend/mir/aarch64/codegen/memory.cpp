#include "memory.hpp"
#include "alu.hpp"

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

PreparedMemoryOperandRecordResult memory_operand_record_error(
    PreparedMemoryOperandRecordError error) {
  return PreparedMemoryOperandRecordResult{.record = std::nullopt, .error = error};
}

PreparedMemoryInstructionRecordResult memory_instruction_record_error(
    PreparedMemoryOperandRecordError error) {
  return PreparedMemoryInstructionRecordResult{.record = std::nullopt, .error = error};
}

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

std::optional<prepare::PreparedValueId> find_value_home_id(
    const prepare::PreparedValueLocationFunction& value_locations,
    c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  for (const auto& home : value_locations.value_homes) {
    if (home.value_name == value_name) {
      return home.value_id;
    }
  }
  return std::nullopt;
}

PreparedMemoryOperandRecordError find_unique_value_home_id(
    const prepare::PreparedValueLocationFunction& value_locations,
    c4c::ValueNameId value_name,
    prepare::PreparedValueId& out) {
  if (value_name == c4c::kInvalidValueName) {
    return PreparedMemoryOperandRecordError::MissingPointerValueName;
  }

  std::optional<prepare::PreparedValueId> found;
  for (const auto& home : value_locations.value_homes) {
    if (home.value_name != value_name) {
      continue;
    }
    if (found.has_value()) {
      return PreparedMemoryOperandRecordError::AmbiguousPointerValueHome;
    }
    found = home.value_id;
  }
  if (!found.has_value()) {
    return PreparedMemoryOperandRecordError::MissingPointerValueHome;
  }

  out = *found;
  return PreparedMemoryOperandRecordError::None;
}

std::optional<c4c::ValueNameId> named_value_id(
    const prepare::PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

PreparedMemoryOperandRecordError validate_structured_memory_address_facts(
    const bir::MemoryAddress& address,
    const MemoryOperand& memory) {
  if (address.byte_offset != memory.byte_offset ||
      (address.size_bytes != 0 && address.size_bytes != memory.size_bytes) ||
      (address.align_bytes != 0 && address.align_bytes != memory.align_bytes) ||
      address.address_space != memory.address_space ||
      address.is_volatile != memory.is_volatile) {
    return PreparedMemoryOperandRecordError::AddressFactMismatch;
  }
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError validate_unstructured_memory_instruction_facts(
    std::size_t instruction_byte_offset,
    std::size_t instruction_align_bytes,
    const MemoryOperand& memory) {
  if (static_cast<std::int64_t>(instruction_byte_offset) != memory.byte_offset ||
      (instruction_align_bytes != 0 && instruction_align_bytes != memory.align_bytes) ||
      memory.size_bytes != 0 || memory.address_space != bir::AddressSpace::Default ||
      memory.is_volatile) {
    return PreparedMemoryOperandRecordError::AddressFactMismatch;
  }
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError validate_memory_instruction_facts(
    const bir::MemoryAddress* address,
    std::size_t instruction_byte_offset,
    std::size_t instruction_align_bytes,
    const MemoryOperand& memory) {
  if (address != nullptr) {
    return validate_structured_memory_address_facts(*address, memory);
  }
  return validate_unstructured_memory_instruction_facts(
      instruction_byte_offset, instruction_align_bytes, memory);
}

std::optional<c4c::LinkNameId> global_symbol_id_from_address_or_inst(
    const bir::MemoryAddress* address,
    c4c::LinkNameId fallback_link_name) {
  if (address != nullptr) {
    if (address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol) {
      return std::nullopt;
    }
    if (address->base_link_name_id != c4c::kInvalidLinkName) {
      return address->base_link_name_id;
    }
  }
  if (fallback_link_name != c4c::kInvalidLinkName) {
    return fallback_link_name;
  }
  return std::nullopt;
}

PreparedMemoryOperandRecordError validate_memory_base_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::MemoryAddress* address,
    c4c::LinkNameId fallback_link_name,
    MemoryOperand& memory) {
  switch (memory.base_kind) {
    case MemoryBaseKind::FrameSlot:
      if (address != nullptr &&
          address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
        return PreparedMemoryOperandRecordError::UnsupportedBase;
      }
      return PreparedMemoryOperandRecordError::None;
    case MemoryBaseKind::Symbol: {
      const auto symbol = global_symbol_id_from_address_or_inst(address, fallback_link_name);
      if (!symbol.has_value() || memory.symbol_name != *symbol) {
        return PreparedMemoryOperandRecordError::SymbolMismatch;
      }
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::PointerValue: {
      if (!memory.pointer_value_name.has_value()) {
        return PreparedMemoryOperandRecordError::MissingPointerValueName;
      }
      if (address == nullptr ||
          address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
        return PreparedMemoryOperandRecordError::PointerValueMismatch;
      }
      const auto pointer_value_name = named_value_id(names, address->base_value);
      if (!pointer_value_name.has_value() || *pointer_value_name != *memory.pointer_value_name) {
        return PreparedMemoryOperandRecordError::PointerValueMismatch;
      }

      prepare::PreparedValueId pointer_value_id = 0;
      const auto error =
          find_unique_value_home_id(value_locations, *memory.pointer_value_name, pointer_value_id);
      if (error != PreparedMemoryOperandRecordError::None) {
        return error;
      }
      memory.pointer_value_id = pointer_value_id;
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::StringConstant: {
      if (!memory.string_symbol_name.has_value()) {
        return PreparedMemoryOperandRecordError::MissingSymbolName;
      }
      if (address == nullptr ||
          address->base_kind != bir::MemoryAddress::BaseKind::StringConstant) {
        return PreparedMemoryOperandRecordError::StringIdentityMismatch;
      }

      const std::string_view prepared_symbol =
          prepare::prepared_link_name(names, *memory.string_symbol_name);
      if (prepared_symbol.empty()) {
        return PreparedMemoryOperandRecordError::MissingSymbolName;
      }
      if (address->base_link_name_id != c4c::kInvalidLinkName) {
        if (address->base_link_name_id != *memory.string_symbol_name) {
          return PreparedMemoryOperandRecordError::StringIdentityMismatch;
        }
      } else if (address->base_name != prepared_symbol) {
        return PreparedMemoryOperandRecordError::StringIdentityMismatch;
      }

      const auto text_id = names.texts.find(prepared_symbol);
      if (text_id != c4c::kInvalidText) {
        memory.string_name = text_id;
      }
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::None:
    case MemoryBaseKind::Register:
      return PreparedMemoryOperandRecordError::UnsupportedBase;
  }
  return PreparedMemoryOperandRecordError::UnsupportedBase;
}

PreparedMemoryOperandRecordError apply_load_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedMemoryAccess& access,
    const bir::Value& result,
    MemoryOperand& memory) {
  if (!access.result_value_name.has_value() || access.stored_value_name.has_value()) {
    return PreparedMemoryOperandRecordError::ResultValueMismatch;
  }
  const auto result_name = named_value_id(names, result);
  if (!result_name.has_value() || *result_name != *access.result_value_name) {
    return PreparedMemoryOperandRecordError::ResultValueMismatch;
  }
  memory.result_value_name = *result_name;
  memory.result_value_id = find_value_home_id(value_locations, *result_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError apply_store_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedMemoryAccess& access,
    const bir::Value& stored,
    MemoryOperand& memory) {
  if (access.result_value_name.has_value()) {
    return PreparedMemoryOperandRecordError::StoredValueMismatch;
  }
  const auto stored_name = named_value_id(names, stored);
  if (!stored_name.has_value()) {
    if (access.stored_value_name.has_value()) {
      return PreparedMemoryOperandRecordError::StoredValueMismatch;
    }
    return PreparedMemoryOperandRecordError::None;
  }
  if (!access.stored_value_name.has_value() || *stored_name != *access.stored_value_name) {
    return PreparedMemoryOperandRecordError::StoredValueMismatch;
  }
  memory.stored_value_name = *stored_name;
  memory.stored_value_id = find_value_home_id(value_locations, *stored_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordResult make_memory_record_from_prepared_access(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name) {
    return memory_operand_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  const auto* access =
      prepare::find_prepared_memory_access(addressing, block_label, instruction_index);
  if (access == nullptr || access->function_name != addressing.function_name) {
    return memory_operand_record_error(
        PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess);
  }

  MemoryOperand memory{
      .surface = RecordSurfaceKind::RecordOnly,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = access->function_name,
      .block_label = access->block_label,
      .instruction_index = access->inst_index,
      .byte_offset = access->address.byte_offset,
      .size_bytes = access->address.size_bytes,
      .align_bytes = access->address.align_bytes,
      .address_space = access->address_space,
      .is_volatile = access->is_volatile,
      .can_use_base_plus_offset = access->address.can_use_base_plus_offset,
  };

  switch (access->address.base_kind) {
    case prepare::PreparedAddressBaseKind::FrameSlot:
      if (!access->address.frame_slot_id.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingFrameSlotId);
      }
      memory.base_kind = MemoryBaseKind::FrameSlot;
      memory.frame_slot_id = access->address.frame_slot_id;
      break;
    case prepare::PreparedAddressBaseKind::GlobalSymbol:
      if (!access->address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::Symbol;
      memory.symbol_name = access->address.symbol_name;
      break;
    case prepare::PreparedAddressBaseKind::PointerValue:
      if (!access->address.pointer_value_name.has_value()) {
        return memory_operand_record_error(
            PreparedMemoryOperandRecordError::MissingPointerValueName);
      }
      memory.base_kind = MemoryBaseKind::PointerValue;
      memory.pointer_value_name = access->address.pointer_value_name;
      break;
    case prepare::PreparedAddressBaseKind::StringConstant:
      if (!access->address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::StringConstant;
      memory.string_symbol_name = access->address.symbol_name;
      if (const auto text_id =
              names.texts.find(prepare::prepared_link_name(
                  names, *access->address.symbol_name));
          text_id != c4c::kInvalidText) {
        memory.string_name = text_id;
      }
      break;
    case prepare::PreparedAddressBaseKind::None:
      return memory_operand_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }

  return PreparedMemoryOperandRecordResult{
      .record = memory,
      .error = PreparedMemoryOperandRecordError::None,
  };
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

MachineOpcode machine_opcode_from_memory_instruction(const MemoryInstructionRecord& instruction) {
  switch (instruction.memory_kind) {
    case MemoryInstructionKind::Load:
      return MachineOpcode::Load;
    case MemoryInstructionKind::Store:
      return MachineOpcode::Store;
  }
  return MachineOpcode::Unspecified;
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

std::vector<MachineSideEffectKind> memory_side_effects(
    const MemoryInstructionRecord& instruction) {
  std::vector<MachineSideEffectKind> side_effects;
  side_effects.push_back(instruction.memory_kind == MemoryInstructionKind::Load
                             ? MachineSideEffectKind::MemoryRead
                             : MachineSideEffectKind::MemoryWrite);
  if (instruction.address.is_volatile) {
    side_effects.push_back(MachineSideEffectKind::VolatileMemoryAccess);
  }
  return side_effects;
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

bool is_supported_memory_base(MemoryBaseKind base_kind) {
  switch (base_kind) {
    case MemoryBaseKind::FrameSlot:
    case MemoryBaseKind::PointerValue:
      return true;
    case MemoryBaseKind::Symbol:
    case MemoryBaseKind::StringConstant:
    case MemoryBaseKind::None:
    case MemoryBaseKind::Register:
      return false;
  }
  return false;
}

MachineNodeStatusRecord memory_selection_status(const MemoryInstructionRecord& instruction) {
  if (instruction.address.support != MemoryOperandSupportKind::Prepared ||
      !is_supported_memory_base(instruction.address.base_kind)) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "memory operand is outside the selected subset"};
  }
  if (instruction.memory_kind == MemoryInstructionKind::Load) {
    if (!instruction.result_value_id.has_value() ||
        instruction.result_value_name == c4c::kInvalidValueName ||
        !instruction.address.result_value_id.has_value() ||
        !instruction.address.result_value_name.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                     .diagnostic =
                                         "load node is missing prepared result identity"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (!instruction.value.has_value() || !instruction.address.stored_value_id.has_value() ||
      !instruction.address.stored_value_name.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "store node is missing prepared stored value identity"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
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

std::string_view memory_base_kind_name(MemoryBaseKind kind) {
  switch (kind) {
    case MemoryBaseKind::None:
      return "none";
    case MemoryBaseKind::FrameSlot:
      return "frame_slot";
    case MemoryBaseKind::Symbol:
      return "symbol";
    case MemoryBaseKind::PointerValue:
      return "pointer_value";
    case MemoryBaseKind::StringConstant:
      return "string_constant";
    case MemoryBaseKind::Register:
      return "register";
  }
  return "unknown";
}

std::string_view memory_operand_support_kind_name(MemoryOperandSupportKind kind) {
  switch (kind) {
    case MemoryOperandSupportKind::Prepared:
      return "prepared";
    case MemoryOperandSupportKind::DeferredUnsupported:
      return "deferred_unsupported";
  }
  return "unknown";
}

std::string_view memory_instruction_kind_name(MemoryInstructionKind kind) {
  switch (kind) {
    case MemoryInstructionKind::Load:
      return "load";
    case MemoryInstructionKind::Store:
      return "store";
  }
  return "unknown";
}

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

std::string_view prepared_memory_operand_record_error_name(
    PreparedMemoryOperandRecordError error) {
  switch (error) {
    case PreparedMemoryOperandRecordError::None:
      return "none";
    case PreparedMemoryOperandRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess:
      return "missing_prepared_memory_access";
    case PreparedMemoryOperandRecordError::UnsupportedBase:
      return "unsupported_base";
    case PreparedMemoryOperandRecordError::MissingFrameSlotId:
      return "missing_frame_slot_id";
    case PreparedMemoryOperandRecordError::MissingSymbolName:
      return "missing_symbol_name";
    case PreparedMemoryOperandRecordError::SymbolMismatch:
      return "symbol_mismatch";
    case PreparedMemoryOperandRecordError::AddressFactMismatch:
      return "address_fact_mismatch";
    case PreparedMemoryOperandRecordError::MissingPointerValueName:
      return "missing_pointer_value_name";
    case PreparedMemoryOperandRecordError::MissingPointerValueHome:
      return "missing_pointer_value_home";
    case PreparedMemoryOperandRecordError::MissingPointerValueStorage:
      return "missing_pointer_value_storage";
    case PreparedMemoryOperandRecordError::UnsupportedPointerValueStorage:
      return "unsupported_pointer_value_storage";
    case PreparedMemoryOperandRecordError::AmbiguousPointerValueHome:
      return "ambiguous_pointer_value_home";
    case PreparedMemoryOperandRecordError::PointerValueMismatch:
      return "pointer_value_mismatch";
    case PreparedMemoryOperandRecordError::StringIdentityMismatch:
      return "string_identity_mismatch";
    case PreparedMemoryOperandRecordError::ResultValueMismatch:
      return "result_value_mismatch";
    case PreparedMemoryOperandRecordError::StoredValueMismatch:
      return "stored_value_mismatch";
    case PreparedMemoryOperandRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedMemoryOperandRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedMemoryOperandRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedMemoryOperandRecordError::MissingStoredValueHome:
      return "missing_stored_value_home";
    case PreparedMemoryOperandRecordError::MissingStoredStorage:
      return "missing_stored_storage";
    case PreparedMemoryOperandRecordError::UnsupportedStoredStorage:
      return "unsupported_stored_storage";
    case PreparedMemoryOperandRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
  }
  return "unknown";
}

std::string memory_error_message(PreparedMemoryOperandRecordError error) {
  std::string message =
      "AArch64 memory lowering requires prepared memory and destination facts";
  message += "; error=";
  message += prepared_memory_operand_record_error_name(error);
  return message;
}

[[nodiscard]] std::string atomic_operation_error_message(
    PreparedAtomicOperationRecordError error) {
  std::string message =
      "AArch64 atomic lowering requires complete prepared atomic operation facts";
  message += "; error=";
  message += prepared_atomic_operation_record_error_name(error);
  return message;
}

[[nodiscard]] std::string f128_transport_error_message(
    PreparedF128TransportRecordError error) {
  std::string message =
      "AArch64 binary128 memory transport lowering requires prepared f128 carrier facts";
  message += "; error=";
  message += prepared_f128_transport_record_error_name(error);
  return message;
}

OperandRecord make_memory_operand(MemoryOperand operand) {
  return OperandRecord{.kind = OperandKind::Memory, .payload = operand};
}

InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction) {
  std::vector<OperandRecord> operands = {make_memory_operand(instruction.address)};
  if (instruction.value.has_value()) {
    operands.push_back(*instruction.value);
  }
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses = memory_effects_from_operands(operands);
  if (instruction.memory_kind == MemoryInstructionKind::Load) {
    if (instruction.result_register.has_value()) {
      defs.push_back(
          effect_from_memory_operand(make_register_operand(*instruction.result_register)));
    } else {
      defs.push_back(
          prepared_value_def(instruction.result_value_id, instruction.result_value_name));
    }
  }
  const auto selection = memory_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_memory_instruction(instruction),
      .selection = selection,
      .function_name = instruction.address.function_name,
      .block_label = instruction.address.block_label,
      .instruction_index = instruction.address.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .side_effects = memory_side_effects(instruction),
      .payload = instruction,
  };
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

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          load.address ? &*load.address : nullptr,
          c4c::kInvalidLinkName,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          load.address ? &*load.address : nullptr,
          load.byte_offset,
          load.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_load_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          load.result,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryInstructionRecordResult
make_prepared_frame_slot_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }

  const auto operand = make_prepared_memory_operand_record(
      names, value_locations, addressing, block_label, instruction_index, load);
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (!operand.record->result_value_id.has_value() ||
      !operand.record->result_value_name.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingResultValueHome);
  }

  const auto* result_home =
      prepare::find_prepared_value_home(value_locations, *operand.record->result_value_id);
  if (result_home == nullptr ||
      result_home->value_name != *operand.record->result_value_name ||
      result_home->kind != prepare::PreparedValueHomeKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingResultValueHome);
  }

  const auto* result_storage =
      find_storage_plan_value(storage_plan, *operand.record->result_value_id);
  if (result_storage == nullptr ||
      result_storage->value_name != *operand.record->result_value_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::MissingResultStorage);
  }
  if (result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedResultStorage);
  }

  auto result_register =
      make_prepared_register_operand(
          *result_home, *result_storage, load.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::RegisterConversionFailed);
  }

  return PreparedMemoryInstructionRecordResult{
      .record =
          MemoryInstructionRecord{
              .memory_kind = MemoryInstructionKind::Load,
              .address = *operand.record,
              .result_value_id = operand.record->result_value_id,
              .result_value_name = *operand.record->result_value_name,
              .value_type = load.result.type,
              .result_register = *result_register,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
}

PreparedMemoryInstructionRecordResult make_store_memory_instruction_record(
    PreparedMemoryOperandRecordResult operand,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& stored_value) {
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot &&
      operand.record->base_kind != MemoryBaseKind::PointerValue) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (operand.record->base_kind == MemoryBaseKind::PointerValue) {
    if (!operand.record->pointer_value_id.has_value() ||
        !operand.record->pointer_value_name.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueHome);
    }
    const auto* pointer_home =
        prepare::find_prepared_value_home(value_locations, *operand.record->pointer_value_id);
    if (pointer_home == nullptr ||
        pointer_home->value_name != *operand.record->pointer_value_name ||
        pointer_home->kind != prepare::PreparedValueHomeKind::Register) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueHome);
    }
    const auto* pointer_storage =
        find_storage_plan_value(storage_plan, *operand.record->pointer_value_id);
    if (pointer_storage == nullptr ||
        pointer_storage->value_name != *operand.record->pointer_value_name) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueStorage);
    }
    if (pointer_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::UnsupportedPointerValueStorage);
    }
    auto base_register =
        make_prepared_register_operand(
            *pointer_home, *pointer_storage, bir::TypeKind::Ptr, RegisterOperandRole::StoragePlan);
    if (!base_register.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::RegisterConversionFailed);
    }
    operand.record->base_register = *base_register;
  }
  if (!operand.record->stored_value_id.has_value() ||
      !operand.record->stored_value_name.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingStoredValueHome);
  }

  const auto* stored_home =
      prepare::find_prepared_value_home(value_locations, *operand.record->stored_value_id);
  if (stored_home == nullptr ||
      stored_home->value_name != *operand.record->stored_value_name ||
      stored_home->kind != prepare::PreparedValueHomeKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingStoredValueHome);
  }

  const auto* stored_storage =
      find_storage_plan_value(storage_plan, *operand.record->stored_value_id);
  if (stored_storage == nullptr ||
      stored_storage->value_name != *operand.record->stored_value_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::MissingStoredStorage);
  }
  if (stored_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedStoredStorage);
  }

  auto stored_register =
      make_prepared_register_operand(
          *stored_home, *stored_storage, stored_value.type, RegisterOperandRole::StoragePlan);
  if (!stored_register.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::RegisterConversionFailed);
  }

  return PreparedMemoryInstructionRecordResult{
      .record =
          MemoryInstructionRecord{
              .memory_kind = MemoryInstructionKind::Store,
              .address = *operand.record,
              .value = make_register_operand(*stored_register),
              .value_type = stored_value.type,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          store.address ? &*store.address : nullptr,
          c4c::kInvalidLinkName,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          store.address ? &*store.address : nullptr,
          store.byte_offset,
          store.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_store_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          store.value,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryInstructionRecordResult make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  return make_store_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, store),
      value_locations,
      storage_plan,
      store.value);
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          load.address ? &*load.address : nullptr,
          load.global_name_id,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          load.address ? &*load.address : nullptr,
          load.byte_offset,
          load.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_load_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          load.result,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          store.address ? &*store.address : nullptr,
          store.global_name_id,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          store.address ? &*store.address : nullptr,
          store.byte_offset,
          store.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_store_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          store.value,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryInstructionRecordResult make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  return make_store_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, store),
      value_locations,
      storage_plan,
      store.value);
}

MemoryInstructionLoweringResult lower_memory_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* local_store = std::get_if<bir::StoreLocalInst>(&inst);
  const auto* global_store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (load == nullptr && local_store == nullptr && global_store == nullptr) {
    return MemoryInstructionLoweringResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  PreparedMemoryInstructionRecordResult prepared;
  if (load != nullptr) {
    prepared = make_prepared_frame_slot_load_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
  } else if (local_store != nullptr) {
    prepared = make_prepared_store_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *local_store);
  } else {
    prepared = make_prepared_store_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *global_store);
  }
  if (!prepared.record.has_value()) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(prepared.error));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  InstructionRecord target = make_memory_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        std::string{target.selection.diagnostic});
    return MemoryInstructionLoweringResult{.handled = true};
  }

  return MemoryInstructionLoweringResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

MemoryInstructionLoweringResult lower_f128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if ((load == nullptr || load->result.type != bir::TypeKind::F128) &&
      (store == nullptr || store->value.type != bir::TypeKind::F128)) {
    return MemoryInstructionLoweringResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return MemoryInstructionLoweringResult{.handled = true};
  }
  const auto* f128_carriers =
      prepare::find_prepared_f128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (f128_carriers == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return MemoryInstructionLoweringResult{.handled = true};
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  PreparedMemoryOperandRecordResult memory;
  c4c::ValueNameId carrier_value_name = c4c::kInvalidValueName;
  F128TransportKind transport_kind = F128TransportKind::CarrierSnapshot;
  if (load != nullptr) {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
    carrier_value_name =
        context.function.prepared->names.value_names.find(load->result.name);
    transport_kind = F128TransportKind::LoadFromMemory;
  } else {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *store);
    carrier_value_name =
        store->value.kind == bir::Value::Kind::Named
            ? context.function.prepared->names.value_names.find(store->value.name)
            : c4c::kInvalidValueName;
    transport_kind = F128TransportKind::StoreToMemory;
  }
  if (!memory.record.has_value()) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                             context,
                             instruction_index,
                             memory_error_message(memory.error));
    return MemoryInstructionLoweringResult{.handled = true};
  }
  if (carrier_value_name == c4c::kInvalidValueName) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  auto prepared = make_prepared_f128_carrier_transport_record(
      *f128_carriers,
      carrier_value_name,
      transport_kind,
      std::move(memory.record));
  if (!prepared.record.has_value()) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                             context,
                             instruction_index,
                             f128_transport_error_message(prepared.error));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  InstructionRecord target = make_f128_transport_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                             context,
                             instruction_index,
                             std::string{target.selection.diagnostic});
    return MemoryInstructionLoweringResult{.handled = true};
  }

  return MemoryInstructionLoweringResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
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
