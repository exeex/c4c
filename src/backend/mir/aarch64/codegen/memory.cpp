#include "memory.hpp"
#include "alu.hpp"

#include <cstdint>
#include <optional>
#include <sstream>
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

const prepare::PreparedFrameSlot* find_frame_slot_by_slot_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

const prepare::PreparedStackObject* find_stack_object_by_object_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedStackObject* find_stack_object_by_value_name(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedStackLayout& stack_layout,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name) {
  if (function_name == c4c::kInvalidFunctionName || value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto value_text = prepare::prepared_value_name(names, value_name);
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name) {
      continue;
    }
    if (object.value_name == value_name ||
        (!value_text.empty() && prepare::prepared_stack_object_name(names, object) == value_text)) {
      return &object;
    }
  }
  return nullptr;
}

std::optional<FrameSlotOperand> make_frame_slot_operand_from_stack_slot(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedFrameSlot& slot) {
  const auto* object = find_stack_object_by_object_id(stack_layout, slot.object_id);
  if (object == nullptr) {
    return std::nullopt;
  }
  return FrameSlotOperand{
      .slot_id = slot.slot_id,
      .object_id = slot.object_id,
      .function_name = slot.function_name,
      .slot_name = object->slot_name,
      .value_name = object->value_name,
      .type = object->type,
      .offset_bytes = slot.offset_bytes,
      .offset_is_prepared_snapshot = true,
      .size_bytes = slot.size_bytes,
      .align_bytes = slot.align_bytes,
      .fixed_location = slot.fixed_location,
  };
}

bool resolve_frame_slot_memory_offset(const prepare::PreparedStackLayout& stack_layout,
                                      MemoryOperand& address) {
  if (address.base_kind != MemoryBaseKind::FrameSlot) {
    return true;
  }
  if (!address.frame_slot_id.has_value()) {
    return false;
  }
  const auto* slot = find_frame_slot_by_slot_id(stack_layout, *address.frame_slot_id);
  if (slot == nullptr) {
    return false;
  }
  address.byte_offset += static_cast<std::int64_t>(slot->offset_bytes);
  address.byte_offset_is_prepared_snapshot = true;
  return true;
}

bool rewrite_local_address_store_value(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedStackLayout& stack_layout,
    c4c::FunctionNameId function_name,
    const bir::StoreLocalInst& store,
    MemoryInstructionRecord& record) {
  if (record.memory_kind != MemoryInstructionKind::Store ||
      store.value.kind != bir::Value::Kind::Named || store.value.type != bir::TypeKind::Ptr ||
      !record.address.stored_value_name.has_value()) {
    return true;
  }

  const auto* object =
      find_stack_object_by_value_name(
          names, stack_layout, function_name, *record.address.stored_value_name);
  if (object == nullptr) {
    return true;
  }
  const auto* slot = prepare::find_prepared_frame_slot(stack_layout, object->object_id);
  if (slot == nullptr) {
    return false;
  }
  auto operand = make_frame_slot_operand_from_stack_slot(stack_layout, *slot);
  if (!operand.has_value()) {
    return false;
  }
  record.value = make_frame_slot_operand(*operand);
  return true;
}

bool apply_stack_layout_to_memory_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedStackLayout& stack_layout,
    c4c::FunctionNameId function_name,
    const bir::StoreLocalInst* local_store,
    MemoryInstructionRecord& record) {
  if (!resolve_frame_slot_memory_offset(stack_layout, record.address)) {
    return false;
  }
  if (local_store == nullptr) {
    return true;
  }
  return rewrite_local_address_store_value(
      names, stack_layout, function_name, *local_store, record);
}

std::optional<RegisterOperand> find_memory_return_abi_register(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(type);
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BeforeReturn ||
        bundle.block_index != context.block_index) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id != value_id ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }

      abi::PreparedRegisterConversionResult converted;
      if (move.destination_register_placement.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_placement,
                                                   std::nullopt,
                                                   expected_view);
      } else if (move.destination_register_name.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_name,
                                                   std::nullopt,
                                                   std::nullopt,
                                                   expected_view);
      } else {
        continue;
      }
      if (!converted.reg.has_value()) {
        continue;
      }
      return RegisterOperand{
          .reg = *converted.reg,
          .role = RegisterOperandRole::CallAbi,
          .value_id = value_id,
          .value_name = value_name,
          .expected_view = expected_view,
      };
    }
  }
  return std::nullopt;
}

void retarget_load_result_to_return_abi(const module::BlockLoweringContext& context,
                                        MemoryInstructionRecord& record) {
  if (record.memory_kind != MemoryInstructionKind::Load ||
      !record.result_value_id.has_value()) {
    return;
  }
  auto return_register =
      find_memory_return_abi_register(context,
                                      *record.result_value_id,
                                      record.result_value_name,
                                      record.value_type);
  if (!return_register.has_value()) {
    return;
  }
  record.result_register = *return_register;
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
      memory.address_space != bir::AddressSpace::Default ||
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
      if (!memory.symbol_name.has_value() || memory.symbol_label.empty()) {
        return PreparedMemoryOperandRecordError::MissingSymbolName;
      }
      if (address == nullptr) {
        (void)fallback_link_name;
        return PreparedMemoryOperandRecordError::None;
      }
      if (address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol) {
        return PreparedMemoryOperandRecordError::UnsupportedBase;
      }
      if (address->base_link_name_id == c4c::kInvalidLinkName &&
          !address->base_name.empty() &&
          address->base_name != memory.symbol_label) {
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
      memory.symbol_label = prepare::prepared_link_name(names, *access->address.symbol_name);
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

MachineOpcode machine_opcode_from_memory_instruction(const MemoryInstructionRecord& instruction) {
  switch (instruction.memory_kind) {
    case MemoryInstructionKind::Load:
      return MachineOpcode::Load;
    case MemoryInstructionKind::Store:
      return MachineOpcode::Store;
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

bool is_immediate_store_value(const MemoryInstructionRecord& instruction) {
  if (!instruction.value.has_value() ||
      instruction.value->kind != OperandKind::Immediate) {
    return false;
  }
  return std::get_if<ImmediateOperand>(&instruction.value->payload) != nullptr;
}

bool is_supported_immediate_store_width(const MemoryInstructionRecord& instruction) {
  return instruction.address.size_bytes == 4 || instruction.address.size_bytes == 8;
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


bool is_supported_memory_base(MemoryBaseKind base_kind) {
  switch (base_kind) {
    case MemoryBaseKind::FrameSlot:
    case MemoryBaseKind::Symbol:
    case MemoryBaseKind::PointerValue:
      return true;
    case MemoryBaseKind::StringConstant:
    case MemoryBaseKind::None:
    case MemoryBaseKind::Register:
      return false;
  }
  return false;
}

bool symbol_memory_has_identity(const MemoryInstructionRecord& instruction) {
  if (instruction.address.base_kind != MemoryBaseKind::Symbol) {
    return true;
  }
  return instruction.address.symbol_name.has_value() &&
         !instruction.address.symbol_label.empty();
}

MachineNodeStatusRecord memory_selection_status(const MemoryInstructionRecord& instruction) {
  if (instruction.address.support != MemoryOperandSupportKind::Prepared ||
      !is_supported_memory_base(instruction.address.base_kind)) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "memory operand is outside the selected subset"};
  }
  if (!symbol_memory_has_identity(instruction)) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "symbol memory node is missing symbol identity"};
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
  if (!instruction.value.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "store node is missing stored value operand"};
  }
  if (is_immediate_store_value(instruction)) {
    if (!is_supported_immediate_store_width(instruction)) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                     .diagnostic =
                                         "immediate store width is outside the selected subset"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (!instruction.address.stored_value_id.has_value() ||
      !instruction.address.stored_value_name.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "store node is missing prepared stored value identity"};
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


std::string memory_address(const MemoryOperand& address) {
  std::ostringstream out;
  if (address.base_kind == MemoryBaseKind::FrameSlot) {
    out << "[sp";
  } else if (address.base_kind == MemoryBaseKind::PointerValue &&
             address.base_register.has_value()) {
    out << "[" << abi::register_name(address.base_register->reg);
  } else {
    return {};
  }
  if (address.byte_offset != 0) {
    out << ", #" << address.byte_offset;
  }
  out << "]";
  return out.str();
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


[[nodiscard]] std::string f128_transport_error_message(
    PreparedF128TransportRecordError error) {
  std::string message =
      "AArch64 binary128 memory transport lowering requires prepared f128 carrier facts";
  message += "; error=";
  message += prepared_f128_transport_record_error_name(error);
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

PreparedMemoryInstructionRecordResult make_load_memory_instruction_record(
    PreparedMemoryOperandRecordResult operand,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    bir::TypeKind result_type) {
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot &&
      operand.record->base_kind != MemoryBaseKind::Symbol &&
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
    auto base_register = make_prepared_register_operand(
        *pointer_home, *pointer_storage, bir::TypeKind::Ptr, RegisterOperandRole::StoragePlan);
    if (!base_register.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::RegisterConversionFailed);
    }
    operand.record->base_register = *base_register;
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

  auto result_register = make_prepared_register_operand(
      *result_home, *result_storage, result_type, RegisterOperandRole::StoragePlan);
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
              .value_type = result_type,
              .result_register = *result_register,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
}

PreparedMemoryInstructionRecordResult make_prepared_load_memory_instruction_record(
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
  return make_load_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, load),
      value_locations,
      storage_plan,
      load.result.type);
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
  return make_prepared_load_memory_instruction_record(
      names,
      value_locations,
      storage_plan,
      addressing,
      block_label,
      instruction_index,
      load);
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
      operand.record->base_kind != MemoryBaseKind::Symbol &&
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
    if (stored_value.kind == bir::Value::Kind::Immediate) {
      const auto immediate = make_scalar_immediate_operand(stored_value);
      if (!immediate.has_value()) {
        return memory_instruction_record_error(
            PreparedMemoryOperandRecordError::UnsupportedStoredStorage);
      }
      return PreparedMemoryInstructionRecordResult{
          .record =
              MemoryInstructionRecord{
                  .memory_kind = MemoryInstructionKind::Store,
                  .address = *operand.record,
                  .value = make_immediate_operand(*immediate),
                  .value_type = stored_value.type,
              },
          .error = PreparedMemoryOperandRecordError::None,
      };
    }
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

PreparedMemoryInstructionRecordResult make_prepared_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  return make_load_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, load),
      value_locations,
      storage_plan,
      load.result.type);
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
  const auto* global_load = std::get_if<bir::LoadGlobalInst>(&inst);
  const auto* local_store = std::get_if<bir::StoreLocalInst>(&inst);
  const auto* global_store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (load == nullptr && global_load == nullptr &&
      local_store == nullptr && global_store == nullptr) {
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
    prepared = make_prepared_load_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
  } else if (global_load != nullptr) {
    prepared = make_prepared_load_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *global_load);
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
  if (context.function.prepared == nullptr ||
      !apply_stack_layout_to_memory_record(context.function.prepared->names,
                                           context.function.prepared->stack_layout,
                                           context.function.control_flow->function_name,
                                           local_store,
                                           *prepared.record)) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 memory lowering requires prepared frame-slot stack offsets");
    return MemoryInstructionLoweringResult{.handled = true};
  }
  retarget_load_result_to_return_abi(context, *prepared.record);

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

MemoryInstructionLoweringResult lower_i128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if ((load == nullptr || load->result.type != bir::TypeKind::I128) &&
      (store == nullptr || store->value.type != bir::TypeKind::I128)) {
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
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
    return MemoryInstructionLoweringResult{.handled = true};
  }
  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
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
  I128TransportKind transport_kind = I128TransportKind::CarrierSnapshot;
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
    transport_kind = I128TransportKind::LoadFromMemory;
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
    transport_kind = I128TransportKind::StoreToMemory;
  }
  if (!memory.record.has_value()) {
    append_memory_diagnostic(
        diagnostics,
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
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  auto prepared = make_prepared_i128_carrier_transport_record(
      *i128_carriers,
      carrier_value_name,
      transport_kind,
      std::move(memory.record));
  if (!prepared.record.has_value()) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(prepared.error));
    return MemoryInstructionLoweringResult{.handled = true};
  }

  InstructionRecord target = make_i128_transport_instruction(*prepared.record);
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


}  // namespace c4c::backend::aarch64::codegen
