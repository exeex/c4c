#include "memory.hpp"

#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "constant_materialization.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "fp_value_materialization.hpp"
#include "float_ops.hpp"
#include "frame_slot_address.hpp"
#include "globals.hpp"
#include "memory_store_retargeting.hpp"
#include "operands.hpp"
#include "select_materialization.hpp"
#include "variadic.hpp"
#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/select_chain_lookups.hpp"
#include "../../../prealloc/stack_layout/stack_layout.hpp"
#include "../../../prealloc/value_locations.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t byte_offset) {
  std::string address{"["};
  address += base;
  if (byte_offset != 0) {
    address += ", #";
    address += std::to_string(byte_offset);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"ldrb"};
    case bir::TypeKind::I16:
      return std::string_view{"ldrh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"ldr"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> dispatch_publication_scalar_type_size_bytes(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic_for_width(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedTypedStackSourcePublicationEmission>
emit_same_width_i32_stack_source_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedTypedStackSourcePublication& typed_stack_source) {
  if (typed_stack_source.status !=
          prepare::PreparedTypedStackSourcePublicationStatus::Available ||
      typed_stack_source.source_type != bir::TypeKind::I32 ||
      typed_stack_source.destination_type != bir::TypeKind::I32 ||
      typed_stack_source.extension_policy !=
          prepare::PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension ||
      !typed_stack_source.source_stack_offset_bytes.has_value() ||
      !typed_stack_source.source_stack_size_bytes.has_value() ||
      *typed_stack_source.source_stack_size_bytes != 4 ||
      !typed_stack_source.destination_register_placement.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(typed_stack_source.destination_type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *typed_stack_source.destination_register_placement,
      prepare::PreparedRegisterClass::General,
      *expected_view);
  if (!converted.reg.has_value() ||
      converted.reg->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto mnemonic =
      scalar_load_mnemonic_for_width(*typed_stack_source.source_stack_size_bytes);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }
  return PreparedTypedStackSourcePublicationEmission{
      .destination_register = *converted.reg,
      .lines = {std::string{*mnemonic} + " " +
                abi::register_name(*converted.reg) + ", " +
                frame_slot_address(context.function,
                                   *typed_stack_source.source_stack_offset_bytes)},
  };
}

[[nodiscard]] std::optional<std::string_view> scalar_store_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> fixed_formal_scalar_store_mnemonic(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

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

[[nodiscard]] std::optional<std::size_t> scalar_store_width_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return std::size_t{8};
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return std::size_t{16};
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool scalar_frame_slot_store_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t width_bytes) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      width_bytes == 0) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % width_bytes == 0 && offset / width_bytes <= 4095U;
}

[[nodiscard]] bool append_scalar_store_to_memory(
    std::string_view mnemonic,
    std::string_view value_register,
    const MemoryOperand& memory,
    std::optional<abi::RegisterReference> address_scratch,
    std::vector<std::string>& lines) {
  if (memory.base_kind == MemoryBaseKind::FrameSlot &&
      !scalar_frame_slot_store_offset_is_encodable(memory, memory.size_bytes)) {
    if (!address_scratch.has_value()) {
      return false;
    }
    auto address_lines = materialize_frame_slot_memory_address_lines(*address_scratch, memory);
    if (address_lines.empty()) {
      return false;
    }
    lines.insert(lines.end(), address_lines.begin(), address_lines.end());
    lines.push_back(std::string{mnemonic} + " " + std::string{value_register} +
                    ", [" + std::string{abi::register_name(*address_scratch)} + "]");
    return true;
  }

  const auto address = memory_address(memory);
  if (address.empty()) {
    return false;
  }
  lines.push_back(std::string{mnemonic} + " " + std::string{value_register} +
                  ", " + address);
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_byte_immediate_store_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryInstructionRecord& memory) {
  if (memory.memory_kind != MemoryInstructionKind::Store ||
      memory.address.size_bytes != 1 || !memory.value.has_value() ||
      memory.value->kind != OperandKind::Immediate) {
    return std::nullopt;
  }
  const auto* immediate = std::get_if<ImmediateOperand>(&memory.value->payload);
  if (immediate == nullptr) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const auto scratch = abi::w_register(scratches.front().index);
  const auto address_scratch =
      scratches.size() > 1U ? std::optional<abi::RegisterReference>{abi::x_register(scratches[1].index)}
                            : std::nullopt;
  const auto byte_value = static_cast<unsigned>(immediate->unsigned_value & 0xffU);

  std::vector<std::string> lines{
      "movz " + std::string{abi::register_name(scratch)} + ", #" +
      std::to_string(byte_value)};
  if (!append_scalar_store_to_memory("strb",
                                     abi::register_name(scratch),
                                     memory.address,
                                     address_scratch,
                                     lines)) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {make_memory_operand(memory.address)},
      .side_effects = {MachineSideEffectKind::MemoryWrite},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = [&] {
                std::string text;
                for (std::size_t index = 0; index < lines.size(); ++index) {
                  if (index != 0) {
                    text += '\n';
                  }
                  text += lines[index];
                }
                return text;
              }(),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
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

bool memory_load_result_feeds_before_return_fpr_abi(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id) {
  return prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
             context.function.move_bundle_lookups,
             context.function.value_locations,
             context.block_index,
             value_id,
             prepare::PreparedRegisterBank::Fpr) != nullptr;
}

[[nodiscard]] bool symbol_fp_load_has_explicit_storage_placement(
    const module::BlockLoweringContext& context,
    const MemoryInstructionRecord& memory_record) {
  if (memory_record.address.base_kind != MemoryBaseKind::Symbol ||
      (memory_record.value_type != bir::TypeKind::F32 &&
       memory_record.value_type != bir::TypeKind::F64) ||
      !memory_record.result_value_id.has_value() ||
      !memory_record.result_register.has_value() ||
      !abi::is_fp_simd_register(memory_record.result_register->reg) ||
      context.function.storage_plan == nullptr) {
    return false;
  }
  const auto* storage =
      find_storage_plan_value(*context.function.storage_plan,
                              *memory_record.result_value_id);
  return storage != nullptr &&
         storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
         storage->bank == prepare::PreparedRegisterBank::Fpr &&
         storage->register_placement.has_value();
}

void apply_frame_pointer_base_policy(const module::FunctionLoweringContext& context,
                                     MemoryInstructionRecord& record) {
  const bool use_frame_pointer =
      context.frame_plan != nullptr &&
      context.frame_plan->uses_frame_pointer_for_fixed_slots;
  if (!use_frame_pointer) {
    return;
  }
  if (record.address.base_kind == MemoryBaseKind::FrameSlot) {
    record.address.uses_frame_pointer_base = true;
  }
  if (!record.value.has_value()) {
    return;
  }
  if (auto* source_memory = std::get_if<MemoryOperand>(&record.value->payload);
      record.value->kind == OperandKind::Memory && source_memory != nullptr &&
      source_memory->base_kind == MemoryBaseKind::FrameSlot) {
    source_memory->uses_frame_pointer_base = true;
  } else if (auto* source_slot = std::get_if<FrameSlotOperand>(&record.value->payload);
             record.value->kind == OperandKind::FrameSlot && source_slot != nullptr) {
    source_slot->uses_frame_pointer_base = true;
  }
}

std::optional<RegisterOperand> find_memory_return_abi_register(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto destination_bank = scalar_fp_register_view(type).has_value()
                                    ? prepare::PreparedRegisterBank::Fpr
                                    : prepare::PreparedRegisterBank::Gpr;
  const auto* move =
      prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          context.function.move_bundle_lookups,
          context.function.value_locations,
          context.block_index,
          value_id,
          destination_bank);
  if (move == nullptr) {
    return std::nullopt;
  }

  abi::PreparedRegisterConversionResult converted;
  if (move->destination_register_placement.has_value()) {
    converted = abi::convert_prepared_register(*move->destination_register_placement,
                                               std::nullopt,
                                               expected_view);
  } else if (move->destination_register_name.has_value()) {
    converted = abi::convert_prepared_register(*move->destination_register_name,
                                               std::nullopt,
                                               std::nullopt,
                                               expected_view);
  } else {
    return std::nullopt;
  }
  if (!converted.reg.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = value_id,
      .value_name = value_name,
      .expected_view = expected_view,
  };
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

std::optional<prepare::PreparedValueId> indexed_value_home_id(
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const prepare::PreparedRegallocFunction* regalloc,
    const prepare::PreparedValueLocationFunction* value_locations,
    c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  return prepare::find_indexed_prepared_value_id(
      value_home_lookups, regalloc, value_locations, value_name);
}

bool indexed_value_home_id_is_ambiguous(
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    c4c::ValueNameId value_name,
    prepare::PreparedValueId selected_value_id) {
  if (value_home_lookups == nullptr || value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& [value_id, home] : value_home_lookups->homes_by_id) {
    if (home != nullptr &&
        home->value_name == value_name &&
        value_id != selected_value_id) {
      return true;
    }
  }
  return false;
}

PreparedMemoryOperandRecordError require_indexed_value_home_id(
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const prepare::PreparedRegallocFunction* regalloc,
    const prepare::PreparedValueLocationFunction* value_locations,
    c4c::ValueNameId value_name,
    prepare::PreparedValueId& out) {
  if (value_name == c4c::kInvalidValueName) {
    return PreparedMemoryOperandRecordError::MissingPointerValueName;
  }

  const auto found =
      indexed_value_home_id(value_home_lookups, regalloc, value_locations, value_name);
  if (!found.has_value()) {
    return PreparedMemoryOperandRecordError::MissingPointerValueHome;
  }
  if (indexed_value_home_id_is_ambiguous(value_home_lookups, value_name, *found)) {
    return PreparedMemoryOperandRecordError::AmbiguousPointerValueHome;
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
    std::size_t instruction_byte_offset,
    const MemoryOperand& memory) {
  const auto selected_byte_offset =
      address.byte_offset + static_cast<std::int64_t>(instruction_byte_offset);
  const bool prepared_address_selected_offset =
      address.byte_offset == memory.byte_offset;
  const bool prepared_frame_slot_local_address =
      memory.base_kind == MemoryBaseKind::FrameSlot &&
      memory.byte_offset == 0 &&
      (address.base_kind == bir::MemoryAddress::BaseKind::LocalSlot ||
       address.base_kind == bir::MemoryAddress::BaseKind::PointerValue);
  if ((!prepared_frame_slot_local_address &&
       !prepared_address_selected_offset &&
       selected_byte_offset != memory.byte_offset) ||
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
    return validate_structured_memory_address_facts(
        *address, instruction_byte_offset, memory);
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
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const prepare::PreparedRegallocFunction* regalloc,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::MemoryAddress* address,
    c4c::LinkNameId fallback_link_name,
    MemoryOperand& memory) {
  switch (memory.base_kind) {
    case MemoryBaseKind::FrameSlot:
      if (address != nullptr &&
          address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot &&
          address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
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
          require_indexed_value_home_id(value_home_lookups,
                                        regalloc,
                                        &value_locations,
                                        *memory.pointer_value_name,
                                        pointer_value_id);
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
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const prepare::PreparedRegallocFunction* regalloc,
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
  memory.result_value_id =
      indexed_value_home_id(value_home_lookups, regalloc, &value_locations, *result_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError apply_store_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const prepare::PreparedRegallocFunction* regalloc,
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
  memory.stored_value_id =
      indexed_value_home_id(value_home_lookups, regalloc, &value_locations, *stored_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordResult make_memory_record_from_prepared_access(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    const prepare::PreparedMemoryAccess& access) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name ||
      access.function_name != addressing.function_name) {
    return memory_operand_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }

  MemoryOperand memory{
      .surface = RecordSurfaceKind::RecordOnly,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = access.function_name,
      .block_label = access.block_label,
      .instruction_index = access.inst_index,
      .byte_offset = access.address.byte_offset,
      .size_bytes = access.address.size_bytes,
      .align_bytes = access.address.align_bytes,
      .address_space = access.address_space,
      .is_volatile = access.is_volatile,
      .can_use_base_plus_offset = access.address.can_use_base_plus_offset,
  };

  switch (access.address.base_kind) {
    case prepare::PreparedAddressBaseKind::FrameSlot:
      if (!access.address.frame_slot_id.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingFrameSlotId);
      }
      memory.base_kind = MemoryBaseKind::FrameSlot;
      memory.frame_slot_id = access.address.frame_slot_id;
      break;
    case prepare::PreparedAddressBaseKind::GlobalSymbol:
      if (!access.address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::Symbol;
      memory.symbol_name = access.address.symbol_name;
      memory.symbol_label = prepare::prepared_link_name(names, *access.address.symbol_name);
      break;
    case prepare::PreparedAddressBaseKind::PointerValue:
      if (!access.address.pointer_value_name.has_value()) {
        return memory_operand_record_error(
            PreparedMemoryOperandRecordError::MissingPointerValueName);
      }
      memory.base_kind = MemoryBaseKind::PointerValue;
      memory.pointer_value_name = access.address.pointer_value_name;
      break;
    case prepare::PreparedAddressBaseKind::StringConstant:
      if (!access.address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::StringConstant;
      memory.string_symbol_name = access.address.symbol_name;
      if (const auto text_id =
              names.texts.find(prepare::prepared_link_name(
                  names, *access.address.symbol_name));
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

PreparedMemoryOperandRecordResult make_memory_record_from_prepared_access(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  const auto* access =
      prepare::find_prepared_memory_access(addressing, block_label, instruction_index);
  if (access == nullptr) {
    return memory_operand_record_error(
        PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess);
  }
  return make_memory_record_from_prepared_access(names, value_locations, addressing, *access);
}

std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  const auto view = c4c::backend::aarch64::codegen::scalar_fp_register_view(type);
  return view.has_value() ? abi::fp_simd_register(reg.index, *view) : std::nullopt;
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

std::optional<RegisterOperand> make_value_home_register_operand(
    const prepare::PreparedValueHome& home,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  std::optional<abi::RegisterReference> reg;
  prepare::PreparedRegisterClass prepared_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank prepared_bank = prepare::PreparedRegisterBank::None;
  if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
    reg = abi::gp_register(parsed->index, *expected_view);
    prepared_class = prepare::PreparedRegisterClass::General;
    prepared_bank = prepare::PreparedRegisterBank::Gpr;
  } else if (parsed->bank == abi::RegisterBank::FpSimd) {
    reg = abi::fp_simd_register(parsed->index, *expected_view);
    prepared_class = (*expected_view == abi::RegisterView::Q)
                         ? prepare::PreparedRegisterClass::Vector
                         : prepare::PreparedRegisterClass::Float;
    prepared_bank = (*expected_view == abi::RegisterView::Q)
                        ? prepare::PreparedRegisterBank::Vreg
                        : prepare::PreparedRegisterBank::Fpr;
  }
  if (!reg.has_value()) {
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *reg,
      .role = RegisterOperandRole::ValueHome,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = prepared_bank,
      .expected_view = expected_view,
      .occupied_register_references = occupied_register_references(*reg),
      .occupied_registers = occupied_register_views(*reg),
  };
}

struct PreparedPointerValueBaseRegisterResult {
  std::optional<RegisterOperand> reg;
  PreparedMemoryOperandRecordError error = PreparedMemoryOperandRecordError::None;
};

PreparedPointerValueBaseRegisterResult make_prepared_pointer_value_base_register(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::optional<prepare::PreparedValueId>& pointer_value_id,
    const std::optional<c4c::ValueNameId>& pointer_value_name) {
  if (!pointer_value_id.has_value() || !pointer_value_name.has_value()) {
    return {.error = PreparedMemoryOperandRecordError::MissingPointerValueHome};
  }
  const auto* pointer_home =
      prepare::find_prepared_value_home(value_locations, *pointer_value_id);
  if (pointer_home == nullptr ||
      pointer_home->value_name != *pointer_value_name ||
      pointer_home->kind != prepare::PreparedValueHomeKind::Register) {
    return {.error = PreparedMemoryOperandRecordError::MissingPointerValueHome};
  }
  const auto* pointer_storage = find_storage_plan_value(storage_plan, *pointer_value_id);
  if (pointer_storage == nullptr ||
      pointer_storage->value_name != *pointer_value_name) {
    return {.error = PreparedMemoryOperandRecordError::MissingPointerValueStorage};
  }
  if (pointer_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return {.error = PreparedMemoryOperandRecordError::UnsupportedPointerValueStorage};
  }
  auto base_register = make_prepared_register_operand(
      *pointer_home, *pointer_storage, bir::TypeKind::Ptr, RegisterOperandRole::StoragePlan);
  if (!base_register.has_value()) {
    return {.error = PreparedMemoryOperandRecordError::RegisterConversionFailed};
  }
  return {.reg = std::move(base_register), .error = PreparedMemoryOperandRecordError::None};
}

PreparedMemoryOperandRecordError resolve_pointer_value_base_register(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    MemoryOperand& operand) {
  if (operand.base_kind != MemoryBaseKind::PointerValue) {
    return PreparedMemoryOperandRecordError::None;
  }
  auto base_register = make_prepared_pointer_value_base_register(
      value_locations,
      storage_plan,
      operand.pointer_value_id,
      operand.pointer_value_name);
  if (base_register.error != PreparedMemoryOperandRecordError::None) {
    return base_register.error;
  }
  operand.base_register = std::move(*base_register.reg);
  return PreparedMemoryOperandRecordError::None;
}

struct PreparedLoadResultValueStorage {
  const prepare::PreparedValueHome* home = nullptr;
  const prepare::PreparedStoragePlanValue* storage = nullptr;
  std::optional<std::int64_t> stack_offset_bytes;
  PreparedMemoryOperandRecordError error = PreparedMemoryOperandRecordError::None;
};

PreparedLoadResultValueStorage decode_prepared_load_result_value_storage(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::optional<prepare::PreparedValueId>& result_value_id,
    const std::optional<c4c::ValueNameId>& result_value_name) {
  if (!result_value_id.has_value() || !result_value_name.has_value()) {
    return {.error = PreparedMemoryOperandRecordError::MissingResultValueHome};
  }

  const auto* result_home =
      prepare::find_prepared_value_home(value_locations, *result_value_id);
  if (result_home == nullptr || result_home->value_name != *result_value_name ||
      (result_home->kind != prepare::PreparedValueHomeKind::Register &&
       result_home->kind != prepare::PreparedValueHomeKind::StackSlot)) {
    return {.error = PreparedMemoryOperandRecordError::MissingResultValueHome};
  }

  const auto* result_storage = find_storage_plan_value(storage_plan, *result_value_id);
  if (result_storage == nullptr || result_storage->value_name != *result_value_name) {
    return {.error = PreparedMemoryOperandRecordError::MissingResultStorage};
  }

  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::Register) {
    return {.home = result_home, .storage = result_storage};
  }

  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      result_storage->stack_offset_bytes.has_value()) {
    return {
        .home = result_home,
        .storage = result_storage,
        .stack_offset_bytes =
            static_cast<std::int64_t>(*result_storage->stack_offset_bytes),
    };
  }

  return {.error = PreparedMemoryOperandRecordError::UnsupportedResultStorage};
}

enum class PreparedStoredValueStorageKind {
  Register,
  RematerializableImmediate,
  FrameSlot,
  RegisterHomeFrameSlotStorage,
};

struct PreparedStoredValueStorage {
  const prepare::PreparedValueHome* home = nullptr;
  const prepare::PreparedStoragePlanValue* storage = nullptr;
  PreparedStoredValueStorageKind kind = PreparedStoredValueStorageKind::Register;
  std::optional<std::int32_t> immediate_i32;
  std::optional<std::int64_t> stack_offset_bytes;
  PreparedMemoryOperandRecordError error = PreparedMemoryOperandRecordError::None;
};

PreparedStoredValueStorage decode_prepared_stored_value_storage(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::optional<prepare::PreparedValueId>& stored_value_id,
    const std::optional<c4c::ValueNameId>& stored_value_name) {
  if (!stored_value_id.has_value() || !stored_value_name.has_value()) {
    return {.error = PreparedMemoryOperandRecordError::MissingStoredValueHome};
  }

  const auto* stored_home =
      prepare::find_prepared_value_home(value_locations, *stored_value_id);
  if (stored_home == nullptr ||
      stored_home->value_name != *stored_value_name ||
      (stored_home->kind != prepare::PreparedValueHomeKind::Register &&
       stored_home->kind != prepare::PreparedValueHomeKind::StackSlot &&
       stored_home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate)) {
    return {.error = PreparedMemoryOperandRecordError::MissingStoredValueHome};
  }

  const auto* stored_storage = find_storage_plan_value(storage_plan, *stored_value_id);
  if (stored_storage == nullptr ||
      stored_storage->value_name != *stored_value_name) {
    return {.error = PreparedMemoryOperandRecordError::MissingStoredStorage};
  }

  if (stored_home->kind == prepare::PreparedValueHomeKind::RematerializableImmediate &&
      stored_storage->encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      stored_home->immediate_i32.has_value() &&
      stored_storage->immediate_i32.has_value() &&
      *stored_home->immediate_i32 == *stored_storage->immediate_i32) {
    return {
        .home = stored_home,
        .storage = stored_storage,
        .kind = PreparedStoredValueStorageKind::RematerializableImmediate,
        .immediate_i32 = static_cast<std::int32_t>(*stored_storage->immediate_i32),
    };
  }

  if (stored_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      stored_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      stored_storage->stack_offset_bytes.has_value()) {
    return {
        .home = stored_home,
        .storage = stored_storage,
        .kind = PreparedStoredValueStorageKind::FrameSlot,
        .stack_offset_bytes =
            static_cast<std::int64_t>(*stored_storage->stack_offset_bytes),
    };
  }

  if (stored_home->kind == prepare::PreparedValueHomeKind::Register &&
      stored_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot) {
    return {
        .home = stored_home,
        .storage = stored_storage,
        .kind = PreparedStoredValueStorageKind::RegisterHomeFrameSlotStorage,
    };
  }

  if (stored_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return {.error = PreparedMemoryOperandRecordError::UnsupportedStoredStorage};
  }

  return {
      .home = stored_home,
      .storage = stored_storage,
      .kind = PreparedStoredValueStorageKind::Register,
  };
}

std::optional<RegisterOperand> make_load_result_stack_publication_scratch(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type,
    const prepare::PreparedTypedStackSourcePublication* typed_stack_source) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      storage.encoding != prepare::PreparedStorageEncodingKind::FrameSlot) {
    return std::nullopt;
  }
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  std::optional<abi::RegisterReference> scratch;
  if (typed_stack_source != nullptr &&
      typed_stack_source->status ==
          prepare::PreparedTypedStackSourcePublicationStatus::Available &&
      typed_stack_source->destination_register_placement.has_value()) {
    const auto converted = abi::convert_prepared_register(
        *typed_stack_source->destination_register_placement,
        std::nullopt,
        expected_view);
    if (!converted.reg.has_value()) {
      return std::nullopt;
    }
    scratch = converted.reg;
  } else if (type == bir::TypeKind::F32 || type == bir::TypeKind::F64) {
    const auto scratches = abi::reserved_mir_scratch_fp_simd_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    scratch = abi::fp_simd_register(scratches.front().index, *expected_view);
  } else {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    scratch = abi::gp_register(scratches.front().index, *expected_view);
  }
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *scratch,
      .role = RegisterOperandRole::SpillAuthority,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = register_class_from_bank(storage.bank),
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = occupied_register_references(*scratch),
      .occupied_registers = occupied_register_views(*scratch),
  };
}

const prepare::PreparedEdgePublication* find_unique_load_result_stack_source_publication(
    const prepare::PreparedEdgePublicationLookups* edge_publications,
    c4c::BlockLabelId predecessor_label,
    const bir::LoadLocalInst* load_local,
    const bir::LoadGlobalInst* load_global) {
  if (edge_publications == nullptr ||
      predecessor_label == c4c::kInvalidBlockLabel ||
      (load_local == nullptr && load_global == nullptr)) {
    return nullptr;
  }
  const prepare::PreparedEdgePublication* matched = nullptr;
  for (const auto& publication : edge_publications->publications) {
    const bool matches_load =
        (load_local != nullptr && publication.source_load_local == load_local) ||
        (load_global != nullptr && publication.source_load_global == load_global);
    if (publication.predecessor_label != predecessor_label || !matches_load) {
      continue;
    }
    if (matched != nullptr) {
      return nullptr;
    }
    matched = &publication;
  }
  return matched;
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
  return instruction.address.size_bytes == 2 || instruction.address.size_bytes == 4 ||
         instruction.address.size_bytes == 8;
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
    case MemoryBaseKind::Register:
      return true;
    case MemoryBaseKind::StringConstant:
    case MemoryBaseKind::None:
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

[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_memory_access(
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

[[nodiscard]] bool prepared_memory_access_matches_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryAccess* access,
    const bir::Inst& inst) {
  if (access == nullptr) {
    return false;
  }
  return std::visit(
      [&](const auto& op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          const auto result_name = prepared_named_value_id(context, op.result);
          return result_name.has_value() &&
                 access->result_value_name.has_value() &&
                 *access->result_value_name == *result_name;
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          if (op.value.kind != bir::Value::Kind::Named) {
            return !access->stored_value_name.has_value();
          }
          const auto stored_name = prepared_named_value_id(context, op.value);
          return stored_name.has_value() &&
                 access->stored_value_name.has_value() &&
                 *access->stored_value_name == *stored_name;
        }
        return false;
      },
      inst);
}

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
    out << (address.uses_frame_pointer_base ? "[x29" : "[sp");
  } else if (address.base_kind == MemoryBaseKind::PointerValue &&
             address.base_register.has_value()) {
    out << "[" << abi::register_name(address.base_register->reg);
  } else if (address.base_kind == MemoryBaseKind::Register &&
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

void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction) {
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->result_stack_offset_bytes.has_value() ||
      !memory_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 memory_record->result_value_name,
                                 *memory_record->result_register);
}

void retarget_memory_result_to_prepared_home(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  const bool frame_slot_return_publication =
      memory_record != nullptr &&
      memory_record->address.base_kind == MemoryBaseKind::FrameSlot &&
      memory_record->result_value_id.has_value() &&
      memory_load_result_feeds_before_return_fpr_abi(
          context, *memory_record->result_value_id);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Load ||
      (memory_record->address.base_kind != MemoryBaseKind::Symbol &&
       !frame_slot_return_publication) ||
      !memory_record->result_value_id.has_value() ||
      !memory_record->result_register.has_value() ||
      context.function.value_locations == nullptr) {
    return;
  }
  if (symbol_fp_load_has_explicit_storage_placement(context, *memory_record)) {
    return;
  }

  const auto* home =
      prepare::find_indexed_prepared_value_home(
          context.function.value_home_lookups,
          context.function.value_locations,
          *memory_record->result_value_id);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return;
  }

  const auto expected_view = memory_record->result_register->expected_view;
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != memory_record->result_register->reg.bank) {
    return;
  }
  auto viewed = *parsed;
  if (expected_view.has_value()) {
    viewed.view = *expected_view;
  }
  memory_record->result_register->reg = viewed;
  memory_record->result_register->value_id = home->value_id;
  memory_record->result_register->value_name = home->value_name;
  memory_record->result_register->occupied_register_references = {viewed};
  memory_record->result_register->occupied_registers = {abi::register_name(viewed)};
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
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&value_locations);
  if (const auto error = validate_memory_base_identity(
          names,
          &value_home_lookups,
          nullptr,
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
          &value_home_lookups,
          nullptr,
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
    bir::TypeKind result_type,
    const prepare::PreparedTypedStackSourcePublication* typed_stack_source = nullptr) {
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot &&
      operand.record->base_kind != MemoryBaseKind::Symbol &&
      operand.record->base_kind != MemoryBaseKind::PointerValue &&
      operand.record->base_kind != MemoryBaseKind::Register) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (operand.record->base_kind == MemoryBaseKind::PointerValue) {
    if (const auto error =
            resolve_pointer_value_base_register(value_locations, storage_plan, *operand.record);
        error != PreparedMemoryOperandRecordError::None) {
      return memory_instruction_record_error(error);
    }
  }
  const auto result_storage = decode_prepared_load_result_value_storage(
      value_locations,
      storage_plan,
      operand.record->result_value_id,
      operand.record->result_value_name);
  if (result_storage.error != PreparedMemoryOperandRecordError::None) {
    return memory_instruction_record_error(result_storage.error);
  }

  std::optional<RegisterOperand> result_register;
  std::optional<std::int64_t> result_stack_offset_bytes;
  if (result_storage.home->kind == prepare::PreparedValueHomeKind::Register) {
    result_register = make_prepared_register_operand(
        *result_storage.home,
        *result_storage.storage,
        result_type,
        RegisterOperandRole::StoragePlan);
    if (!result_register.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::RegisterConversionFailed);
    }
  } else if (result_storage.home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    result_register = make_load_result_stack_publication_scratch(
        *result_storage.home,
        *result_storage.storage,
        result_type,
        typed_stack_source);
    if (!result_register.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::RegisterConversionFailed);
    }
    result_stack_offset_bytes = result_storage.stack_offset_bytes;
  } else {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedResultStorage);
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
              .result_stack_offset_bytes = result_stack_offset_bytes,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
}

PreparedMemoryInstructionRecordResult make_prepared_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    const prepare::PreparedEdgePublicationLookups* edge_publications,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  const auto typed_stack_source =
      prepare::prepare_same_width_i32_stack_source_publication(
          find_unique_load_result_stack_source_publication(
              edge_publications, block_label, &load, nullptr));
  return make_load_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, load),
      value_locations,
      storage_plan,
      load.result.type,
      &typed_stack_source);
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
      operand.record->base_kind != MemoryBaseKind::PointerValue &&
      operand.record->base_kind != MemoryBaseKind::Register) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (operand.record->base_kind == MemoryBaseKind::PointerValue) {
    if (const auto error =
            resolve_pointer_value_base_register(value_locations, storage_plan, *operand.record);
        error != PreparedMemoryOperandRecordError::None) {
      return memory_instruction_record_error(error);
    }
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

  const auto stored_storage = decode_prepared_stored_value_storage(
      value_locations,
      storage_plan,
      operand.record->stored_value_id,
      operand.record->stored_value_name);
  if (stored_storage.error != PreparedMemoryOperandRecordError::None) {
    return memory_instruction_record_error(stored_storage.error);
  }

  if (stored_storage.kind == PreparedStoredValueStorageKind::RematerializableImmediate) {
    const auto immediate = make_scalar_immediate_operand(
        bir::Value::immediate_i32(*stored_storage.immediate_i32),
        stored_storage.home->value_id,
        stored_storage.home->value_name);
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
  if (stored_storage.kind == PreparedStoredValueStorageKind::FrameSlot) {
    return PreparedMemoryInstructionRecordResult{
        .record =
            MemoryInstructionRecord{
                .memory_kind = MemoryInstructionKind::Store,
                .address = *operand.record,
                .value =
                    make_memory_operand(MemoryOperand{
                        .support = MemoryOperandSupportKind::Prepared,
                        .function_name = operand.record->function_name,
                        .block_label = operand.record->block_label,
                        .instruction_index = operand.record->instruction_index,
                        .result_value_id = stored_storage.home->value_id,
                        .result_value_name = stored_storage.home->value_name,
                        .base_kind = MemoryBaseKind::FrameSlot,
                        .frame_slot_id = stored_storage.storage->slot_id,
                        .byte_offset = *stored_storage.stack_offset_bytes,
                        .byte_offset_is_prepared_snapshot = true,
                        .size_bytes = operand.record->size_bytes,
                        .align_bytes = operand.record->align_bytes,
                        .can_use_base_plus_offset = true,
                    }),
                .value_type = stored_value.type,
            },
        .error = PreparedMemoryOperandRecordError::None,
    };
  }
  if (operand.record->base_kind == MemoryBaseKind::FrameSlot &&
      stored_storage.kind == PreparedStoredValueStorageKind::RegisterHomeFrameSlotStorage) {
    auto stored_register =
        make_value_home_register_operand(*stored_storage.home, stored_value.type);
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
  if (stored_storage.kind != PreparedStoredValueStorageKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedStoredStorage);
  }

  auto stored_register =
      make_prepared_register_operand(
          *stored_storage.home,
          *stored_storage.storage,
          stored_value.type,
          RegisterOperandRole::StoragePlan);
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
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&value_locations);
  if (const auto error = validate_memory_base_identity(
          names,
          &value_home_lookups,
          nullptr,
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
          &value_home_lookups,
          nullptr,
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

class VaListFieldMemoryOwner final {
 public:
  [[nodiscard]] PreparedMemoryInstructionRecordResult load_record(
      const module::BlockLoweringContext& context,
      std::size_t instruction_index,
      const bir::LoadLocalInst& load) const;

  [[nodiscard]] PreparedMemoryInstructionRecordResult store_record(
      const module::BlockLoweringContext& context,
      std::size_t instruction_index,
      const bir::StoreLocalInst& store) const;

  [[nodiscard]] std::optional<module::MachineInstruction> cursor_update_instruction(
      const module::BlockLoweringContext& context,
      std::size_t instruction_index,
      const bir::StoreLocalInst& store) const;

 private:
  struct FieldAddress {
    RegisterOperand base_register;
    std::size_t field_offset_bytes = 0;
    std::size_t field_size_bytes = 0;
    std::size_t field_align_bytes = 0;
  };

  [[nodiscard]] std::optional<std::size_t> parse_field_suffix(
      std::string_view base,
      std::string_view slot_name) const;

  [[nodiscard]] std::size_t scalar_type_size_bytes(bir::TypeKind type) const;

  [[nodiscard]] std::optional<FieldAddress> find_field_address(
      const module::BlockLoweringContext& context,
      std::string_view slot_name) const;

  [[nodiscard]] MemoryOperand memory_operand(
      const module::BlockLoweringContext& context,
      std::size_t instruction_index,
      const FieldAddress& field,
      std::size_t size_bytes,
      std::size_t align_bytes) const;

  [[nodiscard]] const bir::BinaryInst* cursor_update_producer(
      const module::BlockLoweringContext& context,
      std::size_t instruction_index,
      const bir::StoreLocalInst& store) const;
};

std::optional<std::size_t> VaListFieldMemoryOwner::parse_field_suffix(
    std::string_view base,
    std::string_view slot_name) const {
  if (slot_name.size() <= base.size() + 1 ||
      slot_name.substr(0, base.size()) != base ||
      slot_name[base.size()] != '.') {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = base.size() + 1; index < slot_name.size(); ++index) {
    const char ch = slot_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

std::size_t VaListFieldMemoryOwner::scalar_type_size_bytes(bir::TypeKind type) const {
  switch (type) {
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
      return 0;
  }
  return 0;
}

std::optional<VaListFieldMemoryOwner::FieldAddress> VaListFieldMemoryOwner::find_field_address(
    const module::BlockLoweringContext& context,
    std::string_view slot_name) const {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.function.storage_plan == nullptr) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(
          *context.function.prepared,
          context.function.control_flow->function_name);
  if (entry_plan == nullptr) {
    return std::nullopt;
  }
  for (const auto& homes : entry_plan->helper_operand_homes) {
    if (homes.helper != prepare::PreparedVariadicEntryHelperKind::VaStart ||
        !homes.destination_va_list.has_value()) {
      continue;
    }
    const auto& va_list_home = *homes.destination_va_list;
    const auto base_name =
        prepare::prepared_value_name(context.function.prepared->names,
                                     va_list_home.value_name);
    const auto field_offset =
        parse_field_suffix(base_name, slot_name);
    if (!field_offset.has_value()) {
      continue;
    }
    const auto* field = [&]() -> const prepare::PreparedVariadicVaListField* {
      for (const auto& candidate : entry_plan->va_list_layout.fields) {
        if (candidate.offset_bytes == *field_offset) {
          return &candidate;
        }
      }
      return nullptr;
    }();
    if (field == nullptr) {
      continue;
    }
    const auto* storage =
        find_storage_plan_value(*context.function.storage_plan,
                                va_list_home.value_id);
    if (storage == nullptr || storage->value_name != va_list_home.value_name) {
      continue;
    }
    if (va_list_home.kind != prepare::PreparedValueHomeKind::Register ||
        !va_list_home.register_name.has_value()) {
      continue;
    }
    const auto converted = abi::convert_prepared_register(
        *va_list_home.register_name,
        storage->bank,
        register_class_from_bank(storage->bank),
        abi::RegisterView::X);
    if (!converted.has_value()) {
      continue;
    }
    return FieldAddress{
        .base_register =
            RegisterOperand{
                .reg = *converted.reg,
                .role = RegisterOperandRole::StoragePlan,
                .value_id = va_list_home.value_id,
                .value_name = va_list_home.value_name,
                .prepared_class = register_class_from_bank(storage->bank),
                .prepared_bank = storage->bank,
                .expected_view = abi::RegisterView::X,
                .contiguous_width = storage->contiguous_width,
                .occupied_register_references =
                    occupied_register_references(*converted.reg),
                .occupied_registers = occupied_register_views(*converted.reg),
            },
        .field_offset_bytes = field->offset_bytes,
        .field_size_bytes = field->size_bytes,
        .field_align_bytes = field->size_bytes,
    };
  }
  return std::nullopt;
}

MemoryOperand VaListFieldMemoryOwner::memory_operand(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const FieldAddress& field,
    std::size_t size_bytes,
    std::size_t align_bytes) const {
  return MemoryOperand{
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::Register,
      .base_register = field.base_register,
      .byte_offset = static_cast<std::int64_t>(field.field_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .can_use_base_plus_offset = true,
  };
}

PreparedMemoryInstructionRecordResult VaListFieldMemoryOwner::load_record(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) const {
  const auto field = find_field_address(context, load.slot_name);
  if (!field.has_value() ||
      load.byte_offset != 0 ||
      field->field_size_bytes != scalar_type_size_bytes(load.result.type)) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess);
  }
  auto address = memory_operand(
      context, instruction_index, *field, field->field_size_bytes, field->field_align_bytes);
  const auto result_name = named_value_id(context.function.prepared->names, load.result);
  if (!result_name.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingResultValueHome);
  }
  address.result_value_name = *result_name;
  address.result_value_id =
      indexed_value_home_id(context.function.value_home_lookups,
                            context.function.regalloc,
                            context.function.value_locations,
                            *result_name);
  return make_load_memory_instruction_record(
      PreparedMemoryOperandRecordResult{
          .record = address,
          .error = PreparedMemoryOperandRecordError::None,
      },
      *context.function.value_locations,
      *context.function.storage_plan,
      load.result.type);
}

PreparedMemoryInstructionRecordResult VaListFieldMemoryOwner::store_record(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) const {
  const auto field = find_field_address(context, store.slot_name);
  if (!field.has_value() ||
      store.byte_offset != 0 ||
      field->field_size_bytes != scalar_type_size_bytes(store.value.type)) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess);
  }
  auto address = memory_operand(
      context, instruction_index, *field, field->field_size_bytes, field->field_align_bytes);
  const auto stored_name = named_value_id(context.function.prepared->names, store.value);
  if (stored_name.has_value()) {
    address.stored_value_name = *stored_name;
    address.stored_value_id =
        indexed_value_home_id(context.function.value_home_lookups,
                              context.function.regalloc,
                              context.function.value_locations,
                              *stored_name);
  }
  return make_store_memory_instruction_record(
      PreparedMemoryOperandRecordResult{
          .record = address,
          .error = PreparedMemoryOperandRecordError::None,
      },
      *context.function.value_locations,
      *context.function.storage_plan,
      store.value);
}

const bir::BinaryInst* VaListFieldMemoryOwner::cursor_update_producer(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) const {
  if (context.bir_block == nullptr ||
      instruction_index == 0 ||
      store.value.kind != bir::Value::Kind::Named ||
      store.value.type != bir::TypeKind::Ptr) {
    return nullptr;
  }
  const auto* binary =
      std::get_if<bir::BinaryInst>(&context.bir_block->insts[instruction_index - 1U]);
  if (binary == nullptr ||
      binary->opcode != bir::BinaryOpcode::Add ||
      binary->result != store.value ||
      binary->lhs.kind != bir::Value::Kind::Named ||
      binary->lhs.type != bir::TypeKind::Ptr ||
      binary->rhs.kind != bir::Value::Kind::Immediate) {
    return nullptr;
  }
  const auto* load =
      instruction_index >= 2U
          ? std::get_if<bir::LoadLocalInst>(&context.bir_block->insts[instruction_index - 2U])
          : nullptr;
  if (load == nullptr ||
      load->result != binary->lhs ||
      load->slot_name != store.slot_name ||
      load->byte_offset != 0 ||
      load->result.type != bir::TypeKind::Ptr) {
    return nullptr;
  }
  return binary;
}

std::optional<module::MachineInstruction> VaListFieldMemoryOwner::cursor_update_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) const {
  const auto field = find_field_address(context, store.slot_name);
  const auto* producer =
      cursor_update_producer(context, instruction_index, store);
  if (!field.has_value() ||
      producer == nullptr ||
      producer->rhs.immediate < 0 ||
      store.byte_offset != 0 ||
      field->field_size_bytes != scalar_type_size_bytes(store.value.type)) {
    return std::nullopt;
  }

  const auto cursor = make_named_prepared_result_register(context, producer->lhs);
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<abi::RegisterReference> scratch;
  for (const auto candidate : scratches) {
    if (cursor.has_value() &&
        abi::is_gp_register(cursor->reg) &&
        cursor->reg.index == candidate.index) {
      continue;
    }
    if (field->base_register.reg.index == candidate.index) {
      continue;
    }
    scratch = abi::gp_register(candidate.index, abi::RegisterView::X);
    if (scratch.has_value()) {
      break;
    }
  }
  if (!scratch.has_value()) {
    return std::nullopt;
  }

  const auto scratch_name = gp_register_name(scratch->index, abi::RegisterView::X);
  const auto store_mnemonic = scalar_store_mnemonic(store.value.type);
  if (!scratch_name.has_value() || !store_mnemonic.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  const auto stride = static_cast<std::uint64_t>(producer->rhs.immediate);
  if (cursor.has_value()) {
    const std::string cursor_name{abi::register_name(cursor->reg)};
    if (stride == 0) {
      lines.push_back("mov " + *scratch_name + ", " + cursor_name);
    } else {
      lines.push_back("add " + *scratch_name + ", " + cursor_name + ", #" +
                      std::to_string(stride));
    }
  } else {
    const std::string base_name{abi::register_name(field->base_register.reg)};
    lines.push_back("ldr " + *scratch_name + ", " +
                    register_indirect_address(base_name, field->field_offset_bytes));
    if (stride != 0) {
      lines.push_back("add " + *scratch_name + ", " + *scratch_name + ", #" +
                      std::to_string(stride));
    }
  }

  const std::string base_name{abi::register_name(field->base_register.reg)};
  lines.push_back(std::string{*store_mnemonic} + " " + *scratch_name + ", " +
                  register_indirect_address(base_name, field->field_offset_bytes));

  auto address = memory_operand(
      context, instruction_index, *field, field->field_size_bytes, field->field_align_bytes);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {make_memory_operand(std::move(address))},
      .side_effects = {MachineSideEffectKind::MemoryWrite},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = [&] {
                std::string text;
                for (std::size_t index = 0; index < lines.size(); ++index) {
                  if (index != 0) {
                    text += '\n';
                  }
                  text += lines[index];
                }
                return text;
              }(),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
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
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&value_locations);
  if (const auto error = validate_memory_base_identity(
          names,
          &value_home_lookups,
          nullptr,
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
          &value_home_lookups,
          nullptr,
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
    const prepare::PreparedEdgePublicationLookups* edge_publications,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  const auto typed_stack_source =
      prepare::prepare_same_width_i32_stack_source_publication(
          find_unique_load_result_stack_source_publication(
              edge_publications, block_label, nullptr, &load));
  return make_load_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, load),
      value_locations,
      storage_plan,
      load.result.type,
      &typed_stack_source);
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
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&value_locations);
  if (const auto error = validate_memory_base_identity(
          names,
          &value_home_lookups,
          nullptr,
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
          &value_home_lookups,
          nullptr,
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
  const prepare::PreparedStoragePlanFunction empty_storage_plan{
      .function_name = context.function.control_flow->function_name,
      .values = {},
  };
  const auto& storage_plan =
      context.function.storage_plan != nullptr ? *context.function.storage_plan
                                               : empty_storage_plan;
  const auto* edge_publications =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publications
          : nullptr;
  const VaListFieldMemoryOwner va_list_fields;

  PreparedMemoryInstructionRecordResult prepared;
  if (load != nullptr) {
    prepared = va_list_fields.load_record(context, instruction_index, *load);
    if (!prepared.record.has_value()) {
      prepared = make_prepared_load_memory_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          storage_plan,
          *addressing,
          edge_publications,
          context.control_flow_block->block_label,
          instruction_index,
          *load);
    }
  } else if (global_load != nullptr) {
    prepared = make_prepared_load_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        storage_plan,
        *addressing,
        edge_publications,
        context.control_flow_block->block_label,
        instruction_index,
        *global_load);
  } else if (local_store != nullptr) {
    if (auto va_list_cursor_update =
            va_list_fields.cursor_update_instruction(context, instruction_index, *local_store)) {
      return MemoryInstructionLoweringResult{.handled = true,
                                             .instruction = std::move(va_list_cursor_update)};
    }
    prepared = va_list_fields.store_record(context, instruction_index, *local_store);
    if (!prepared.record.has_value()) {
      prepared = make_prepared_store_memory_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          storage_plan,
          *addressing,
          context.control_flow_block->block_label,
          instruction_index,
          *local_store);
    }
  } else {
    prepared = make_prepared_store_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        storage_plan,
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
  const auto local_value_home_lookups =
      context.function.value_home_lookups == nullptr
          ? prepare::make_prepared_value_home_lookups(context.function.value_locations)
          : prepare::PreparedValueHomeLookups{};
  const auto* value_home_lookups =
      context.function.value_home_lookups == nullptr ? &local_value_home_lookups
                                                     : context.function.value_home_lookups;
  if (context.function.prepared == nullptr ||
      !apply_stack_layout_to_memory_record(context.function.prepared->stack_layout,
                                           context.function.address_materialization_lookups,
                                           value_home_lookups,
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
  apply_frame_pointer_base_policy(context.function, *prepared.record);
  if (auto byte_immediate_store =
          make_byte_immediate_store_machine_instruction(context, instruction_index, *prepared.record);
      byte_immediate_store.has_value()) {
    return MemoryInstructionLoweringResult{.handled = true,
                                           .instruction = std::move(byte_immediate_store)};
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
  const auto* global_load = std::get_if<bir::LoadGlobalInst>(&inst);
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if ((load == nullptr || load->result.type != bir::TypeKind::F128) &&
      (global_load == nullptr || global_load->result.type != bir::TypeKind::F128) &&
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
  } else if (global_load != nullptr) {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *global_load);
    carrier_value_name =
        context.function.prepared->names.value_names.find(global_load->result.name);
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
  if (!context.function.prepared->stack_layout.frame_slots.empty() &&
      !resolve_frame_slot_memory_offset(context.function.prepared->stack_layout,
                                        *memory.record)) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 f128 transport requires prepared frame-slot stack offsets");
    return MemoryInstructionLoweringResult{.handled = true};
  }
  if (memory.record->base_kind == MemoryBaseKind::PointerValue) {
    if (context.function.storage_plan == nullptr) {
      append_memory_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          memory_error_message(PreparedMemoryOperandRecordError::MissingPointerValueStorage));
      return MemoryInstructionLoweringResult{.handled = true};
    }
    if (const auto error = resolve_pointer_value_base_register(
            *context.function.value_locations, *context.function.storage_plan, *memory.record);
        error != PreparedMemoryOperandRecordError::None) {
      append_memory_diagnostic(diagnostics,
                               module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                               context,
                               instruction_index,
                               memory_error_message(error));
      return MemoryInstructionLoweringResult{.handled = true};
    }
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


[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_store_local_access(
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
                   *addressing,
                   context.control_flow_block->block_label,
                   instruction_index)
             : nullptr;
}
[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!mnemonic.has_value() || !target.has_value() || !address.has_value()) {
    return false;
  }
  const auto* pointer_home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *access->address.pointer_value_name);
  if (pointer_home == nullptr) {
    return false;
  }
  if (pointer_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      pointer_home->offset_bytes.has_value()) {
    if (is_byval_formal_value_name(context, *access->address.pointer_value_name)) {
      const auto offset =
          static_cast<std::int64_t>(*pointer_home->offset_bytes) +
          access->address.byte_offset;
      if (offset < 0) {
        return false;
      }
      lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                      frame_slot_address(context.function,
                                         static_cast<std::size_t>(offset)));
      return true;
    }
    lines.push_back("ldr " + *address + ", " +
                    frame_slot_address(context.function, *pointer_home->offset_bytes));
  } else if (pointer_home->kind == prepare::PreparedValueHomeKind::Register &&
             pointer_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*pointer_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto viewed = abi::gp_register(parsed->index, abi::RegisterView::X);
    if (!viewed.has_value()) {
      return false;
    }
    const auto source = abi::register_name(*viewed);
    if (source != *address) {
      lines.push_back("mov " + *address + ", " + std::string{source});
    }
  } else {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  register_indirect_address(*address,
                                            static_cast<std::size_t>(
                                                access->address.byte_offset)));
  return true;
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  if (load == nullptr || context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr) {
    return std::nullopt;
  }
  if (access->address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
      load->result.type == bir::TypeKind::Ptr) {
    if (context.function.prepared == nullptr ||
        !access->address.frame_slot_id.has_value()) {
      return std::nullopt;
    }
    const auto* source_slot =
        find_frame_slot(context.function.prepared->stack_layout,
                        *access->address.frame_slot_id);
    const auto* source_object =
        source_slot != nullptr
            ? find_stack_object(context.function.prepared->stack_layout,
                                source_slot->object_id)
            : nullptr;
    if (source_object == nullptr ||
        source_object->source_kind != std::string_view{"local_slot"} ||
        source_object->type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto source_name =
        prepare::prepared_stack_object_name(context.function.prepared->names,
                                            *source_object);
    constexpr std::string_view variadic_local_prefix{"%lv.ap."};
    if (source_name.size() >= variadic_local_prefix.size() &&
        source_name.substr(0, variadic_local_prefix.size()) ==
            variadic_local_prefix) {
      return std::nullopt;
    }
    const auto result_name = prepared_named_value_id(context, load->result);
    if (!result_name.has_value()) {
      return std::nullopt;
    }
    const auto* result_home =
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.regalloc,
                                                  context.function.value_locations,
                                                  *result_name);
    if (result_home == nullptr) {
      return std::nullopt;
    }
    if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      return std::nullopt;
    }
    const auto* fallback_storage_plan =
        context.function.prepared != nullptr &&
                context.function.control_flow != nullptr
            ? prepare::find_prepared_storage_plan(
                  *context.function.prepared,
                  context.function.control_flow->function_name)
            : nullptr;
    const auto* selected_storage_plan =
        context.function.storage_plan != nullptr ? context.function.storage_plan
                                                 : fallback_storage_plan;
    const auto* result_storage =
        selected_storage_plan != nullptr
            ? find_storage_plan_value(*selected_storage_plan,
                                      result_home->value_id)
            : nullptr;
    const auto target_view = scalar_view_for_type(load->result.type);
    if (!target_view.has_value()) {
      return std::nullopt;
    }

    std::optional<std::uint8_t> target_index;
    if (result_storage != nullptr &&
        result_storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
        result_storage->register_name.has_value()) {
      const auto parsed =
          abi::parse_aarch64_register_name(*result_storage->register_name);
      if (parsed.has_value() &&
          parsed->bank == abi::RegisterBank::GeneralPurpose) {
        target_index = parsed->index;
      }
    }
    if (!target_index.has_value() &&
        result_storage != nullptr &&
        result_storage->register_placement.has_value()) {
      const auto converted =
          abi::convert_prepared_register(*result_storage->register_placement,
                                         prepare::PreparedRegisterClass::General,
                                         *target_view);
      if (converted.reg.has_value() &&
          converted.reg->bank == abi::RegisterBank::GeneralPurpose) {
        target_index = converted.reg->index;
      }
    }
    if (!target_index.has_value() &&
        result_home->kind == prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto parsed = abi::parse_aarch64_register_name(*result_home->register_name);
      if (parsed.has_value() &&
          parsed->bank == abi::RegisterBank::GeneralPurpose) {
        target_index = parsed->index;
      }
    }
    if (!target_index.has_value() &&
        result_home->target_register_identity.has_value() &&
        result_home->target_register_identity->bank ==
            prepare::PreparedRegisterBank::Gpr) {
      target_index =
          static_cast<std::uint8_t>(
              result_home->target_register_identity->physical_index);
    }
    if (!target_index.has_value()) {
      const auto scratches = abi::reserved_mir_scratch_gp_registers();
      if (scratches.empty()) {
        return std::nullopt;
      }
      target_index = scratches.front().index;
    }
    const auto target = gp_register_name(*target_index, *target_view);
    const auto offset = prepared_local_load_offset(context, instruction_index);
    if (!target.has_value() || !offset.has_value()) {
      return std::nullopt;
    }

    std::vector<std::string> lines;
    lines.push_back("ldr " + *target + ", " +
                    frame_slot_address(context.function, *offset));
    RegisterOperand emitted{
        .reg = *abi::gp_register(*target_index, *target_view),
        .role = RegisterOperandRole::StoragePlan,
        .value_id = result_home->value_id,
        .value_name = result_home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = target_view,
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
    return make_select_chain_materialization_instruction(
        context, instruction_index, std::move(lines));
  }
  if (access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value()) {
    return std::nullopt;
  }
  const auto* pointer_home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *access->address.pointer_value_name);
  if (pointer_home == nullptr ||
      pointer_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !pointer_home->offset_bytes.has_value()) {
    return std::nullopt;
  }

  const auto result_name = prepared_named_value_id(context, load->result);
  if (!result_name.has_value()) {
    return std::nullopt;
  }
  const auto* result_home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_name);
  if (result_home == nullptr) {
    return std::nullopt;
  }
  const auto target_view = scalar_view_for_type(load->result.type);
  if (!target_view.has_value()) {
    return std::nullopt;
  }

  std::optional<std::uint8_t> target_index;
  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*result_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return std::nullopt;
    }
    target_index = parsed->index;
  } else if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             result_home->offset_bytes.has_value()) {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    target_index = scratches.front().index;
  } else {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != *target_index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_prepared_pointer_value_load_to_register(
          context, *load, instruction_index, *target_index, *scratch_index, lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    const auto mnemonic = scalar_store_mnemonic(load->result.type);
    const auto target = gp_register_name(*target_index, *target_view);
    if (!mnemonic.has_value() || !target.has_value() ||
        !result_home->offset_bytes.has_value()) {
      return std::nullopt;
    }
    lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                    frame_slot_address(context.function, *result_home->offset_bytes));
  } else if (result_home->kind == prepare::PreparedValueHomeKind::Register) {
    const auto reg = abi::gp_register(*target_index, *target_view);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    RegisterOperand emitted{
        .reg = *reg,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = result_home->value_id,
        .value_name = result_home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = target_view,
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  }

  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

namespace {

[[nodiscard]] std::optional<prepare::PreparedRecoveredStoreSourcePublication>
prepared_recovered_narrow_store_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer* source_producer) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source_producer == nullptr ||
      source_producer->kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      source_producer->load_local == nullptr ||
      source_producer->block_label != context.control_flow_block->block_label) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(
      context.function.prepared->names,
      context.function.prepared->module.names,
      context.function.prepared->stack_layout,
      addressing,
      context.control_flow_block->block_label,
      context.bir_block,
      *source_producer->load_local,
      source_producer->instruction_index);
}
[[nodiscard]] const prepare::PreparedEdgePublicationSourceProducer*
prepared_store_source_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value() || context.function.prepared_lookups == nullptr) {
    return nullptr;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
      &context.function.prepared_lookups->edge_publication_source_producers,
      *value_name);
  return producer != nullptr &&
                 producer->kind !=
                     prepare::PreparedEdgePublicationSourceProducerKind::Unknown
             ? producer
             : nullptr;
}
[[nodiscard]] bool prepared_store_source_producer_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan,
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  return plan.source_producer_kind ==
             kind &&
         plan.source_producer_instruction_index.has_value() &&
         plan.source_producer_block_label.has_value() &&
         context.control_flow_block != nullptr &&
         *plan.source_producer_block_label == context.control_flow_block->block_label;
}
[[nodiscard]] bool prepared_store_source_cast_producer_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  return prepared_store_source_producer_is_complete(
             context,
             plan,
             prepare::PreparedEdgePublicationSourceProducerKind::Cast) &&
         plan.source_cast != nullptr;
}
[[nodiscard]] bool prepared_store_source_select_producer_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  return prepared_store_source_producer_is_complete(
             context,
             plan,
             prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization) &&
         plan.source_select != nullptr;
}
[[nodiscard]] bool prepared_store_source_scalar_fp_binary_producer_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  return prepared_store_source_producer_is_complete(
             context,
             plan,
             prepare::PreparedEdgePublicationSourceProducerKind::Binary) &&
         plan.source_binary != nullptr &&
         is_prepared_scalar_float_alu_operation(*plan.source_binary);
}
[[nodiscard]] std::string load_local_global_symbol_name(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load) {
  if (!load.address.has_value() ||
      load.address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol) {
    return {};
  }
  if (context.function.prepared != nullptr &&
      load.address->base_link_name_id != c4c::kInvalidLinkName) {
    const std::string_view symbol =
        prepare::prepared_link_name(context.function.prepared->names,
                                    load.address->base_link_name_id);
    if (!symbol.empty()) {
      return std::string{symbol};
    }
  }
  return load.address->base_name;
}
[[nodiscard]] bool emit_load_local_global_symbol_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  const auto symbol_name = load_local_global_symbol_name(context, load);
  if (!load.address.has_value() || symbol_name.empty() ||
      load.address->byte_offset < 0) {
    return false;
  }
  std::optional<std::string_view> mnemonic = scalar_load_mnemonic(load.result.type);
  std::optional<std::string> target;
  if (const auto target_view = scalar_view_for_type(load.result.type);
      target_view.has_value()) {
    target = gp_register_name(target_index, *target_view);
  } else if (load.result.type == bir::TypeKind::F32 ||
             load.result.type == bir::TypeKind::F64) {
    const auto base = abi::fp_simd_register(target_index, abi::RegisterView::D);
    const auto viewed =
        base.has_value() ? scalar_fp_register_view(*base, load.result.type)
                         : std::nullopt;
    if (viewed.has_value()) {
      mnemonic = std::string_view{"ldr"};
      target = std::string{abi::register_name(*viewed)};
    }
  }
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!mnemonic.has_value() || !target.has_value() || !address.has_value()) {
    return false;
  }
  const auto symbol =
      relocation_operand(symbol_name,
                         static_cast<std::size_t>(load.address->byte_offset));
  lines.push_back("adrp " + *address + ", " + symbol);
  lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
  lines.push_back(std::string{*mnemonic} + " " + *target + ", [" + *address + "]");
  return true;
}
[[nodiscard]] bool prepared_store_source_global_symbol_load_local_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  if (!prepared_store_source_producer_is_complete(
          context,
          plan,
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) ||
      plan.source_load_local == nullptr ||
      !plan.source_producer_instruction_index.has_value()) {
    return false;
  }
  return !load_local_global_symbol_name(context, *plan.source_load_local).empty();
}
[[nodiscard]] bool emit_scalar_conversion_cast_to_register(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t cast_instruction_index,
    const RegisterOperand& target_register,
    std::vector<std::string>& lines) {
  const auto gp_scratches = abi::reserved_mir_scratch_gp_registers();
  const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
  if (gp_scratches.empty() || fp_scratches.empty()) {
    return false;
  }
  const std::uint8_t gp_scratch_index =
      abi::is_gp_register(target_register.reg) &&
              target_register.reg.index == gp_scratches.front().index
          ? gp_scratches.back().index
          : gp_scratches.front().index;
  const std::uint8_t fp_scratch_index =
      abi::is_fp_simd_register(target_register.reg) &&
              target_register.reg.index == fp_scratches.front().index
          ? fp_scratches.back().index
          : fp_scratches.front().index;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_gp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto source_width = scalar_integer_width_bits(cast.operand.type);
      const auto gp_source = source_width.has_value()
                                 ? abi::gp_register(gp_scratch_index,
                                                    *source_width == 64U
                                                        ? abi::RegisterView::X
                                                        : abi::RegisterView::W)
                                 : std::nullopt;
      if (!destination.has_value() || !source_width.has_value() || !gp_source.has_value()) {
        return false;
      }
      if (cast.operand.kind == bir::Value::Kind::Immediate) {
        auto materialize = materialize_integer_constant_lines(
            *gp_source, immediate_integer_bits(cast.operand, *source_width), *source_width);
        if (materialize.empty()) {
          return false;
        }
        lines.insert(lines.end(), materialize.begin(), materialize.end());
      } else if (!emit_value_publication_to_register(context,
                                                     cast.operand,
                                                     cast_instruction_index,
                                                     gp_scratch_index,
                                                     target_register.reg.index,
                                                     lines)) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*gp_source)});
      return true;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}
[[nodiscard]] const prepare::PreparedFrameSlot* store_local_destination_frame_slot(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryAccess* access) {
  if (context.function.prepared == nullptr || access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return nullptr;
  }
  return find_frame_slot(context.function.prepared->stack_layout,
                         *access->address.frame_slot_id);
}
[[nodiscard]] const prepare::PreparedStackObject* store_local_destination_stack_object(
    const module::BlockLoweringContext& context,
    const prepare::PreparedFrameSlot* slot) {
  if (context.function.prepared == nullptr || slot == nullptr) {
    return nullptr;
  }
  return find_stack_object(context.function.prepared->stack_layout, slot->object_id);
}
[[nodiscard]] prepare::PreparedStoreSourcePublicationPlan
plan_store_local_source_publication(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  const auto* access = prepared_memory_access(context, instruction_index);
  const auto* destination_slot =
      store_local_destination_frame_slot(context, access);
  const auto* destination_object =
      store_local_destination_stack_object(context, destination_slot);
  const auto* source_home = prepared_value_home_for_value(context, store.value);
  const auto* source_producer =
      prepared_store_source_producer(context, store.value);
  const auto recovered_source =
      prepared_recovered_narrow_store_source(context, source_producer);
  const auto* addressing =
      context.function.prepared != nullptr && context.function.control_flow != nullptr
          ? prepare::find_prepared_addressing(*context.function.prepared,
                                              context.function.control_flow->function_name)
          : nullptr;
  const bool byval_load_local_source =
      context.function.prepared != nullptr
          ? prepare::prepared_store_source_load_local_is_byval_formal_pointer_source(
                context.function.prepared->names,
                context.function.bir_function,
                addressing,
                source_producer)
          : false;
  std::optional<prepare::PreparedEdgePublicationSourceProducerLookups>
      fallback_source_producers;
  const auto* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : nullptr;
  if (source_producers == nullptr &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr) {
    fallback_source_producers =
        prepare::make_prepared_edge_publication_source_producer_lookups(
            *context.function.prepared,
            *context.function.control_flow);
    source_producers = &*fallback_source_producers;
  }
  prepare::PreparedStoreSourceDirectGlobalSelectChainDependency
      direct_global_select_chain;
  if (context.function.prepared != nullptr &&
      context.control_flow_block != nullptr) {
    direct_global_select_chain =
        prepare::find_prepared_store_source_direct_global_select_chain_dependency(
            context.function.prepared->names,
            source_producers,
            context.control_flow_block->block_label,
            context.bir_block,
            store.value,
            instruction_index);
  }
  const bir::Value* recovered_value =
      recovered_source.has_value() ? &recovered_source->stored_value : nullptr;
  return prepare::plan_prepared_store_source_publication({
      .source_value = &store.value,
      .destination_access = access,
      .source_home = source_home,
      .destination_frame_slot = destination_slot,
      .destination_stack_object = destination_object,
      .recovered_source_value = recovered_value,
      .recovered_source_instruction_index =
          recovered_source.has_value()
              ? std::optional<std::size_t>{recovered_source->instruction_index}
              : std::nullopt,
      .byval_load_local_source = byval_load_local_source,
      .direct_global_select_chain_source =
          direct_global_select_chain.contains_direct_global_load,
      .direct_global_select_chain_root_is_select =
          direct_global_select_chain.root_is_select,
      .direct_global_select_chain_root_instruction_index =
          direct_global_select_chain.root_instruction_index,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      .source_producer = source_producer,
  });
}
[[nodiscard]] prepare::PreparedStoreSourcePublicationPlan
plan_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    const prepare::PreparedMemoryAccess* access,
    const prepare::PreparedValueHome* pointer_home) {
  return prepare::plan_prepared_store_source_publication({
      .source_value = &store.value,
      .destination_access = access,
      .source_home = prepared_value_home_for_value(context, store.value),
      .pointer_base_home = pointer_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback,
      .pointer_store_writeback = true,
  });
}
[[nodiscard]] prepare::PreparedStoreSourcePublicationPlan
plan_pointer_base_plus_offset_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index,
    const std::optional<c4c::ValueNameId>& value_name) {
  const auto* access = prepared_memory_access(context, instruction_index);
  const auto* destination_slot =
      store_local_destination_frame_slot(context, access);
  const auto* destination_object =
      store_local_destination_stack_object(context, destination_slot);
  const auto* source_home =
      value_name.has_value()
          ? prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                      context.function.regalloc,
                                                      context.function.value_locations,
                                                      *value_name)
          : nullptr;
  return prepare::plan_prepared_store_source_publication({
      .source_value = &store.value,
      .destination_access = access,
      .source_home = source_home,
      .destination_frame_slot = destination_slot,
      .destination_stack_object = destination_object,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      .source_producer = prepared_store_source_producer(context, store.value),
  });
}

}  // namespace

[[nodiscard]] std::optional<prepare::PreparedFixedFormalStoreSourcePublication>
plan_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.bir_function == nullptr ||
      context.function.value_locations == nullptr ||
      store.value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* access = prepared_store_local_access(context, instruction_index);
  const auto* source_home = prepared_value_home_for_value(context, store.value);
  const auto* destination_slot =
      access != nullptr &&
              access->address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
              access->address.frame_slot_id.has_value()
          ? find_frame_slot(context.function.prepared->stack_layout,
                            *access->address.frame_slot_id)
          : nullptr;
  const auto* destination_object =
      destination_slot != nullptr
          ? find_stack_object(context.function.prepared->stack_layout,
                              destination_slot->object_id)
          : nullptr;
  auto planned = prepare::plan_prepared_fixed_formal_store_source_publication(
      prepare::PreparedFormalPublicationInputs{
          .names = &context.function.prepared->names,
          .function = context.function.bir_function,
          .value_locations = context.function.value_locations,
          .value_home_lookups = context.function.value_home_lookups,
      },
      prepare::PreparedStoreSourcePublicationInputs{
          .source_value = &store.value,
          .destination_access = access,
          .source_home = source_home,
          .destination_frame_slot = destination_slot,
          .destination_stack_object = destination_object,
          .intent =
              prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      });
  if (!planned.fixed_formal_source ||
      !prepare::prepared_store_source_publication_available(planned.store_source)) {
    return std::nullopt;
  }
  return planned;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto planned =
      plan_fixed_formal_store_local_publication(context, *store, instruction_index);
  if (!planned.has_value() ||
      planned->store_source.destination_base_kind !=
          prepare::PreparedAddressBaseKind::FrameSlot ||
      !planned->store_source.destination_can_use_base_plus_offset ||
      !planned->store_source.destination_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto emitted =
      find_emitted_scalar_register(scalar_state,
                                   planned->store_source.source_value_name);
  const auto mnemonic = fixed_formal_scalar_store_mnemonic(store->value.type);
  if (!emitted.has_value() || !mnemonic.has_value() ||
      !abi::is_gp_register(emitted->reg)) {
    return std::nullopt;
  }
  const std::int64_t signed_offset =
      static_cast<std::int64_t>(
          *planned->store_source.destination_stack_offset_bytes) +
      planned->store_source.destination_byte_offset;
  if (signed_offset < 0) {
    return std::nullopt;
  }
  const auto address =
      frame_slot_address(context.function, static_cast<std::size_t>(signed_offset));
  auto store_reg = emitted->reg;
  if (const auto expected_view = scalar_register_view(store->value.type);
      expected_view.has_value()) {
    const auto resized = abi::gp_register(emitted->reg.index, *expected_view);
    if (!resized.has_value()) {
      return std::nullopt;
    }
    store_reg = *resized;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::string{*mnemonic} + " " +
                                 std::string{abi::register_name(store_reg)} +
                                 ", " + address,
      },
  };
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

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_local_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const BlockScalarLoweringState& scalar_state,
    const module::MachineBlock& block) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr) {
    return std::nullopt;
  }
  const auto store_source_plan =
      plan_store_local_source_publication(context, *store, instruction_index);
  const bool store_source_plan_available =
      prepare::prepared_store_source_publication_available(store_source_plan);
  const bool has_prepared_cast_producer =
      prepared_store_source_cast_producer_is_complete(context, store_source_plan);
  const bool has_prepared_select_producer =
      prepared_store_source_select_producer_is_complete(context, store_source_plan);
  const bool has_prepared_scalar_fp_binary_producer =
      prepared_store_source_scalar_fp_binary_producer_is_complete(
          context, store_source_plan);
  const bool has_prepared_global_symbol_load_local =
      prepared_store_source_global_symbol_load_local_is_complete(
          context, store_source_plan);
  const bool has_direct_global_select_chain =
      store_source_plan.direct_global_select_chain_source;
  if ((!store_source_plan.byval_load_local_source &&
       !store_source_plan.recovered_source_value.has_value() &&
       !has_prepared_select_producer &&
       !has_prepared_scalar_fp_binary_producer &&
       !has_prepared_cast_producer &&
       !has_prepared_global_symbol_load_local &&
       !has_direct_global_select_chain)) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store_source_plan.source_value;
  if (has_prepared_select_producer &&
      !block.instructions.empty() &&
      block.instructions.back().origin.has_value() &&
      block.instructions.back().origin->instruction_index ==
          *store_source_plan.source_producer_instruction_index) {
    return std::nullopt;
  }
  if (!has_prepared_select_producer && has_direct_global_select_chain) {
    if (store_source_plan.direct_global_select_chain_root_is_select &&
        store_source_plan.direct_global_select_chain_root_instruction_index.has_value() &&
        !block.instructions.empty() &&
        block.instructions.back().origin.has_value() &&
        block.instructions.back().origin->instruction_index ==
            *store_source_plan.direct_global_select_chain_root_instruction_index) {
      return std::nullopt;
    }
  }
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (const auto value_name = prepared_named_value_id(context, value);
        value_name.has_value() &&
        has_prepared_scalar_fp_binary_producer) {
      const auto emitted_value =
          find_emitted_scalar_register(scalar_state, *value_name);
      if (emitted_value.has_value() &&
          register_operands_share_physical_register(
              *emitted_value, *target_register)) {
        return std::nullopt;
      }
    }
    if (store_local_uses_pointer_value_address(*store)) {
      const auto value_register =
          prepared_or_emitted_store_value_register(context, value, scalar_state);
      if (value_register.has_value() &&
          register_operands_share_physical_register(*value_register, *target_register)) {
        return std::nullopt;
      }
    }
    if (abi::is_reserved_mir_scratch(target_register->reg)) {
      const auto* value_home = store_source_plan.source_home != nullptr
                                   ? store_source_plan.source_home
                                   : prepared_value_home_for_value(context, value);
      const auto store_mnemonic = scalar_store_mnemonic(value.type);
      const auto store_view = scalar_view_for_type(value.type);
      const auto store_register =
          store_view.has_value()
              ? gp_register_name(target_register->reg.index, *store_view)
              : std::nullopt;
      std::optional<std::uint8_t> alternate_scratch;
      for (const auto& scratch : scratches) {
        if (scratch.index != target_register->reg.index) {
          alternate_scratch = scratch.index;
          break;
        }
      }
      if (!has_prepared_select_producer &&
          !has_prepared_global_symbol_load_local) {
        return std::nullopt;
      }
      if (has_prepared_select_producer &&
          (value_home == nullptr ||
           value_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
           !value_home->offset_bytes.has_value())) {
        return std::nullopt;
      }
      if (!store_mnemonic.has_value() ||
          !store_register.has_value() ||
          !alternate_scratch.has_value()) {
        return std::nullopt;
      }
      if (has_prepared_global_symbol_load_local &&
          store_source_plan.source_load_local != nullptr) {
        emitted = emit_load_local_global_symbol_to_register(
            context,
            *store_source_plan.source_load_local,
            target_register->reg.index,
            *alternate_scratch,
            lines);
      } else {
        emitted = emit_value_publication_to_register(context,
                                                     value,
                                                     instruction_index,
                                                     target_register->reg.index,
                                                     *alternate_scratch,
                                                     lines,
                                                     true);
      }
      if (emitted) {
        if (has_prepared_select_producer) {
          MemoryOperand stack_home{
              .base_kind = MemoryBaseKind::FrameSlot,
              .byte_offset = static_cast<std::int64_t>(*value_home->offset_bytes),
              .size_bytes = scalar_store_width_bytes(value.type).value_or(1),
              .align_bytes = scalar_store_width_bytes(value.type).value_or(1),
              .can_use_base_plus_offset = true,
              .uses_frame_pointer_base = fixed_slots_use_frame_pointer(context.function),
          };
          if (!append_scalar_store_to_memory(*store_mnemonic,
                                             *store_register,
                                             stack_home,
                                             abi::x_register(*alternate_scratch),
                                             lines)) {
            return std::nullopt;
          }
          published_stack_home = true;
        }
      }
    } else if (store_source_plan.source_producer_kind ==
               prepare::PreparedEdgePublicationSourceProducerKind::Cast) {
      if (!has_prepared_cast_producer) {
        return std::nullopt;
      }
      emitted = emit_scalar_conversion_cast_to_register(
          context,
          *store_source_plan.source_cast,
          *store_source_plan.source_producer_instruction_index,
          *target_register,
          lines);
      if (!emitted) {
        return std::nullopt;
      }
    }
    if (!emitted && abi::is_fp_simd_register(target_register->reg) &&
        has_prepared_global_symbol_load_local &&
        store_source_plan.source_load_local != nullptr &&
        !scratches.empty()) {
      lines.clear();
      emitted = emit_load_local_global_symbol_to_register(
          context,
          *store_source_plan.source_load_local,
          target_register->reg.index,
          scratches.front().index,
          lines);
    }
    if (!emitted && abi::is_fp_simd_register(target_register->reg) &&
        has_prepared_scalar_fp_binary_producer &&
        !scratches.empty()) {
      lines.clear();
      emitted = emit_fp_value_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg,
                                          scratches.front().index,
                                          lines);
    }
    if (!emitted && abi::is_gp_register(target_register->reg) && !scratches.empty()) {
      lines.clear();
      if (has_prepared_global_symbol_load_local &&
          store_source_plan.source_load_local != nullptr) {
        emitted = emit_load_local_global_symbol_to_register(
            context,
            *store_source_plan.source_load_local,
            target_register->reg.index,
            scratches.front().index,
            lines);
      } else {
        emitted = emit_value_publication_to_register(context,
                                                     value,
                                                     instruction_index,
                                                     target_register->reg.index,
                                                     scratches.front().index,
                                                     lines);
      }
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr &&
             (has_prepared_select_producer ||
              has_prepared_cast_producer ||
              has_prepared_global_symbol_load_local ||
              has_direct_global_select_chain)) {
    if (store_source_plan_available &&
        (store_source_plan.destination_base_kind !=
             prepare::PreparedAddressBaseKind::FrameSlot ||
         !store_source_plan.destination_can_use_base_plus_offset)) {
      return std::nullopt;
    }
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U ||
        !store_mnemonic.has_value() ||
        !store_register.has_value()) {
      return std::nullopt;
    }
    if (has_prepared_global_symbol_load_local &&
        store_source_plan.source_load_local != nullptr) {
      emitted = emit_load_local_global_symbol_to_register(
          context,
          *store_source_plan.source_load_local,
          scratches.front().index,
          scratches[1].index,
          lines);
    } else if (has_prepared_cast_producer &&
        store_source_plan.source_cast != nullptr &&
        store_source_plan.source_producer_instruction_index.has_value() &&
        (store_source_plan.source_cast->opcode == bir::CastOpcode::IntToPtr ||
         store_source_plan.source_cast->opcode == bir::CastOpcode::PtrToInt)) {
      emitted = emit_value_publication_to_register(
          context,
          store_source_plan.source_cast->operand,
          *store_source_plan.source_producer_instruction_index,
          scratches.front().index,
          scratches[1].index,
          lines);
    } else {
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   scratches.front().index,
                                                   scratches[1].index,
                                                   lines);
    }
    if (emitted) {
      auto store_memory = *target_memory;
      store_memory.size_bytes = scalar_store_width_bytes(value.type).value_or(
          target_memory->size_bytes);
      if (!append_scalar_store_to_memory(*store_mnemonic,
                                         *store_register,
                                         store_memory,
                                         abi::x_register(scratches[1].index),
                                         lines)) {
        return std::nullopt;
      }
      published_stack_home = true;
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
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
[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store->address.has_value() ||
      store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset < 0) {
    return std::nullopt;
  }
  const auto* pointer_home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *access->address.pointer_value_name);
  if (pointer_home == nullptr ||
      pointer_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !pointer_home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto store_source_plan =
      plan_stack_homed_pointer_store_writeback(context, *store, access, pointer_home);
  if (!prepare::prepared_store_source_publication_available(store_source_plan) ||
      store_source_plan.intent !=
          prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback ||
      !store_source_plan.pointer_store_writeback ||
      store_source_plan.destination_base_kind !=
          prepare::PreparedAddressBaseKind::PointerValue ||
      !store_source_plan.destination_pointer_value_name.has_value() ||
      !store_source_plan.destination_can_use_base_plus_offset ||
      store_source_plan.destination_byte_offset < 0 ||
      store_source_plan.pointer_base_home_kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      !store_source_plan.pointer_base_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto pointer_base_stack_offset =
      *store_source_plan.pointer_base_stack_offset_bytes;
  const auto destination_byte_offset =
      static_cast<std::size_t>(store_source_plan.destination_byte_offset);

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto& value = store_source_plan.source_value;
  const auto value_view = scalar_view_for_type(value.type);
  const auto store_mnemonic = scalar_store_mnemonic(value.type);
  if (scratches.size() < 2U || !value_view.has_value() ||
      !store_mnemonic.has_value()) {
    return std::nullopt;
  }
  const auto value_register = gp_register_name(scratches[0].index, *value_view);
  const auto address_register_ref =
      abi::gp_register(scratches[1].index, abi::RegisterView::X);
  const auto address_register = gp_register_name(scratches[1].index, abi::RegisterView::X);
  if (!value_register.has_value() || !address_register_ref.has_value() ||
      !address_register.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string stored_register = *value_register;
  if (auto prepared_store_value =
          make_named_prepared_result_register(context, value);
      prepared_store_value.has_value() &&
      !register_operands_share_physical_register(
          *prepared_store_value,
          RegisterOperand{.reg = *address_register_ref})) {
    const auto source_register = std::string{abi::register_name(prepared_store_value->reg)};
    if (source_register != stored_register) {
      lines.push_back("mov " + stored_register + ", " + source_register);
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            value,
                                            instruction_index,
                                            scratches[0].index,
                                            scratches[1].index,
                                            lines,
                                            true)) {
      return std::nullopt;
    }
  }
  lines.push_back("ldr " + *address_register + ", " +
                  frame_slot_address(context.function, pointer_base_stack_offset));
  lines.push_back(std::string{*store_mnemonic} + " " + stored_register +
                  ", " + register_indirect_address(
                              *address_register, destination_byte_offset));

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects =
          {MachineSideEffectKind::MemoryRead, MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
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

namespace {

[[nodiscard]] std::optional<std::string> prepared_global_symbol_from_link_name(
    const module::BlockLoweringContext& context,
    std::optional<c4c::LinkNameId> symbol_name) {
  if (context.function.prepared == nullptr || !symbol_name.has_value()) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(context.function.prepared->names, *symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }
  return std::string{symbol};
}
[[nodiscard]] bool emit_global_symbol_address_to_register(
    std::string_view symbol,
    std::int64_t delta,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto target = gp_register_name(target_index, abi::RegisterView::X);
  if (!target.has_value() || symbol.empty()) {
    return false;
  }
  if (delta >= 0) {
    const auto reloc = relocation_operand(std::string{symbol}, static_cast<std::size_t>(delta));
    lines.push_back("adrp " + *target + ", " + reloc);
    lines.push_back("add " + *target + ", " + *target + ", :lo12:" + reloc);
    return true;
  }
  lines.push_back("adrp " + *target + ", " + std::string{symbol});
  lines.push_back("add " + *target + ", " + *target + ", :lo12:" + std::string{symbol});
  lines.push_back("sub " + *target + ", " + *target + ", #" +
                  std::to_string(-delta));
  return true;
}
struct PreparedPointerBasePlusOffsetMaterialization {
  const prepare::PreparedValueHome* value_home = nullptr;
  std::optional<std::string> base_symbol{};
  const prepare::PreparedValueHome* base_home = nullptr;
};
[[nodiscard]] std::optional<PreparedPointerBasePlusOffsetMaterialization>
find_prepared_pointer_base_plus_offset_materialization(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome* value_home) {
  if (value_home == nullptr ||
      value_home->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !value_home->pointer_base_value_name.has_value()) {
    return std::nullopt;
  }
  PreparedPointerBasePlusOffsetMaterialization materialization{
      .value_home = value_home,
  };
  materialization.base_symbol =
      prepared_global_symbol_from_link_name(context,
                                            value_home->pointer_base_symbol_name);
  if (materialization.base_symbol.has_value()) {
    return materialization;
  }
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  materialization.base_home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *value_home->pointer_base_value_name);
  if (materialization.base_home == nullptr) {
    return std::nullopt;
  }
  return materialization;
}
[[nodiscard]] bool emit_pointer_base_plus_offset_to_register(
    const module::BlockLoweringContext& context,
    const PreparedPointerBasePlusOffsetMaterialization& materialization,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto* value_home = materialization.value_home;
  if (value_home == nullptr ||
      value_home->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !value_home->pointer_base_value_name.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, abi::RegisterView::X);
  if (!target.has_value()) {
    return false;
  }

  const auto delta = value_home->pointer_byte_delta.value_or(0);
  if (materialization.base_symbol.has_value()) {
    return emit_global_symbol_address_to_register(
        *materialization.base_symbol, delta, target_index, lines);
  }

  const auto* base_home = materialization.base_home;
  if (base_home == nullptr) {
    return false;
  }
  if (base_home->value_name != kInvalidValueName &&
      context.function.prepared != nullptr) {
    const auto block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel;
    const auto frame_address =
        prepare::find_indexed_prepared_frame_address_offset_for_value(
            context.function.prepared->stack_layout,
            context.function.address_materialization_lookups,
            block_label,
            base_home->value_name,
            instruction_index);
    if (frame_address.has_value()) {
      const auto address_delta =
          frame_address->materialization_byte_offset + delta;
      if (address_delta >= 0) {
        lines.push_back("add " + *target + ", sp, #" +
                        std::to_string(address_delta));
      } else {
        lines.push_back("sub " + *target + ", sp, #" +
                        std::to_string(-address_delta));
      }
      return true;
    }
  }
  if (base_home->kind == prepare::PreparedValueHomeKind::Register &&
      base_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*base_home->register_name);
    if (!parsed.has_value() || !abi::is_gp_register(*parsed)) {
      return false;
    }
    const auto source = gp_register_name(parsed->index, abi::RegisterView::X);
    if (!source.has_value()) {
      return false;
    }
    if (delta == 0) {
      if (*source != *target) {
        lines.push_back("mov " + *target + ", " + *source);
      }
    } else if (delta > 0) {
      lines.push_back("add " + *target + ", " + *source + ", #" +
                      std::to_string(delta));
    } else {
      lines.push_back("sub " + *target + ", " + *source + ", #" +
                      std::to_string(-delta));
    }
    return true;
  }
  if (base_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      base_home->offset_bytes.has_value()) {
    lines.push_back("ldr " + *target + ", " + frame_slot_address(context.function, *base_home->offset_bytes));
    if (delta > 0) {
      lines.push_back("add " + *target + ", " + *target + ", #" +
                      std::to_string(delta));
    } else if (delta < 0) {
      lines.push_back("sub " + *target + ", " + *target + ", #" +
                      std::to_string(-delta));
    }
    return true;
  }
  return false;
}

}  // namespace

[[nodiscard]] std::optional<module::MachineInstruction>
lower_pointer_base_plus_offset_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto store_source_plan =
      plan_pointer_base_plus_offset_store_local_publication(
          context, *store, instruction_index, value_name);
  if (!prepare::prepared_store_source_publication_available(store_source_plan) ||
      store_source_plan.destination_base_kind !=
          prepare::PreparedAddressBaseKind::FrameSlot ||
      !store_source_plan.destination_can_use_base_plus_offset ||
      store_source_plan.destination_byte_offset < 0 ||
      !store_source_plan.destination_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto destination_offset =
      *store_source_plan.destination_stack_offset_bytes +
      static_cast<std::size_t>(store_source_plan.destination_byte_offset);
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  const auto address_register = gp_register_name(scratches.front().index, abi::RegisterView::X);
  if (!address_register.has_value()) {
    return std::nullopt;
  }

  bool emitted = false;
  if (auto symbol = prepared_global_symbol_from_link_name(
          context, store_source_plan.source_pointer_base_symbol_name)) {
    emitted = emit_global_symbol_address_to_register(
        *symbol,
        store_source_plan.source_pointer_byte_delta.value_or(0),
        scratches.front().index,
        lines);
  } else {
    const auto* value_home = store_source_plan.source_home;
    const auto materialization =
        find_prepared_pointer_base_plus_offset_materialization(context, value_home);
    emitted = value_home != nullptr &&
              store_source_plan.source_home_kind ==
                  prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
              materialization.has_value() &&
              emit_pointer_base_plus_offset_to_register(
                  context,
                  *materialization,
                  instruction_index,
                  scratches.front().index,
                  lines);
  }
  if (!emitted) {
    return std::nullopt;
  }
  lines.push_back("str " + *address_register + ", " +
                  frame_slot_address(context.function, destination_offset));

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
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

namespace {

[[nodiscard]] const prepare::PreparedAddressingFunction*
prepared_store_global_addressing(const module::BlockLoweringContext& context) {
  return context.function.prepared != nullptr &&
                 context.function.control_flow != nullptr
             ? prepare::find_prepared_addressing(*context.function.prepared,
                                                 context.function.control_flow->function_name)
             : nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome*
find_prepared_store_global_stack_publication_home(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId result_name) {
  const auto* home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                result_name);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return nullptr;
  }
  return home;
}

[[nodiscard]] bool prepared_pending_store_global_source_producer_is_complete(
    const module::BlockLoweringContext& context,
    const prepare::PreparedStoreSourcePublicationPlan& plan,
    std::size_t store_instruction_index) {
  return context.control_flow_block != nullptr &&
         context.bir_block != nullptr &&
         plan.source_producer_kind !=
             prepare::PreparedEdgePublicationSourceProducerKind::Unknown &&
         plan.source_producer_kind !=
             prepare::PreparedEdgePublicationSourceProducerKind::Immediate &&
         plan.source_producer_block_label.has_value() &&
         *plan.source_producer_block_label ==
             context.control_flow_block->block_label &&
         plan.source_producer_instruction_index.has_value() &&
         *plan.source_producer_instruction_index < store_instruction_index &&
         *plan.source_producer_instruction_index < context.bir_block->insts.size();
}

}  // namespace

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication_from_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const prepare::PreparedStoreSourcePublicationPlan& plan);

void lower_pending_store_global_stack_value_publications(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::unordered_set<c4c::ValueNameId>& published_stack_values,
    module::MachineBlock& block) {
  const auto* addressing = prepared_store_global_addressing(context);
  const auto* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : nullptr;
  const auto pending_publications =
      prepare::plan_pending_prepared_store_global_publications(
          context.function.value_locations,
          addressing,
          context.control_flow_block != nullptr
              ? context.control_flow_block->block_label
              : c4c::kInvalidBlockLabel,
          context.bir_block,
          instruction_index,
          source_producers);
  if (pending_publications.empty()) {
    return;
  }
  for (const auto& pending : pending_publications) {
    const auto& plan = pending.store_source;
    if (!prepare::prepared_store_source_publication_available(plan) ||
        plan.intent !=
            prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
        !plan.pending_publication ||
        !plan.stack_homes_only ||
        plan.duplicate_publication ||
        plan.source_value_name == c4c::kInvalidValueName) {
      continue;
    }
    if (!prepared_pending_store_global_source_producer_is_complete(
            context, plan, pending.instruction_index)) {
      continue;
    }
    std::unordered_set<c4c::ValueNameId> pending_published_stack_values{
        plan.source_value_name};
    if (auto publication =
            lower_published_store_global_stack_value_publication(
                context,
                context.bir_block->insts[*plan.source_producer_instruction_index],
                *plan.source_producer_instruction_index,
                pending_published_stack_values)) {
      published_stack_values.insert(plan.source_value_name);
      block.instructions.push_back(std::move(*publication));
    }
  }
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_published_store_global_stack_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const std::unordered_set<c4c::ValueNameId>& published_stack_values) {
  const auto result = instruction_result_value(inst);
  if (!result.has_value() ||
      result->kind != bir::Value::Kind::Named ||
      result->name.empty()) {
    return std::nullopt;
  }
  const auto result_name = prepared_named_value_id(context, *result);
  if (!result_name.has_value() ||
      published_stack_values.find(*result_name) == published_stack_values.end()) {
    return std::nullopt;
  }
  const auto* home =
      find_prepared_store_global_stack_publication_home(context, *result_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto store_mnemonic = scalar_store_mnemonic(result->type);
  const auto store_view = scalar_view_for_type(result->type);
  const auto store_register =
      store_view.has_value() && !scratches.empty()
          ? gp_register_name(scratches.front().index, *store_view)
          : std::nullopt;
  if (scratches.size() < 2U || !store_mnemonic.has_value() ||
      !store_register.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1U,
                                          scratches.front().index,
                                          scratches[1].index,
                                          lines,
                                          true)) {
    return std::nullopt;
  }
  lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                  ", " + frame_slot_address(context.function, *home->offset_bytes));

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
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

[[nodiscard]] bool future_store_local_stack_value_publication_covers_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr) {
    return false;
  }
  const auto result = instruction_result_value(inst);
  if (!result.has_value() ||
      result->kind != bir::Value::Kind::Named ||
      result->name.empty()) {
    return false;
  }
  const auto result_name = prepared_named_value_id(context, *result);
  if (!result_name.has_value()) {
    return false;
  }
  const auto* home =
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_name);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return false;
  }

  for (std::size_t index = instruction_index + 1U;
       index < context.bir_block->insts.size();
       ++index) {
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[index]);
    if (store == nullptr) {
      continue;
    }
    if (store->value.kind != bir::Value::Kind::Named ||
        store->value.name != result->name ||
        store->value.type != result->type) {
      continue;
    }
    const auto plan = plan_store_local_source_publication(context, *store, index);
    if (!prepare::prepared_store_source_publication_available(plan) ||
        plan.source_home_kind != prepare::PreparedValueHomeKind::StackSlot) {
      return false;
    }
    if (plan.source_producer_instruction_index == instruction_index) {
      return true;
    }
    return plan.direct_global_select_chain_root_instruction_index == instruction_index;
  }
  return false;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication_from_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  if (!prepare::prepared_store_source_publication_available(plan) ||
      plan.intent !=
          prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
      plan.duplicate_publication ||
      plan.source_value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value() ||
      (memory_record->value->kind != OperandKind::Register &&
       memory_record->value->kind != OperandKind::Memory)) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = plan.source_value;
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (plan.stack_homes_only) {
      return std::nullopt;
    }
    if (abi::is_reserved_mir_scratch(target_register->reg) || scratches.empty()) {
      return std::nullopt;
    }
    if (abi::is_gp_register(target_register->reg)) {
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   scratches.front().index,
                                                   lines);
    } else if (abi::is_fp_simd_register(target_register->reg)) {
      emitted = emit_fp_value_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg,
                                          scratches.front().index,
                                          lines);
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr) {
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    const auto stack_home = memory_address(*target_memory);
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U || !store_mnemonic.has_value() ||
        !store_register.has_value() || stack_home.empty()) {
      return std::nullopt;
    }
    emitted = emit_value_publication_to_register(context,
                                                 value,
                                                 instruction_index,
                                                 scratches.front().index,
                                                 scratches[1].index,
                                                 lines,
                                                 plan.stack_homes_only);
    if (emitted) {
      lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                      ", " + stack_home);
      published_stack_home = true;
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
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

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    std::unordered_set<c4c::ValueNameId>*,
    bool stack_homes_only) {
  const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (store == nullptr) {
    return std::nullopt;
  }
  const auto plan = prepare::plan_prepared_store_global_publication(
      context.function.value_locations,
      prepared_store_global_addressing(context),
      context.control_flow_block != nullptr
          ? context.control_flow_block->block_label
          : c4c::kInvalidBlockLabel,
      *store,
      instruction_index,
      false,
      stack_homes_only);
  return lower_store_global_value_publication_from_plan(context,
                                                        instruction_index,
                                                        lowered_memory,
                                                        plan);
}

}  // namespace c4c::backend::aarch64::codegen
