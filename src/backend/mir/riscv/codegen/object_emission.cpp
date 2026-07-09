#include "object_emission.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/formal_publications.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/publication_plans.hpp"
#include "../../../prealloc/target_register_profile.hpp"
#include "emit.hpp"
#include "prepared_call_emit.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_function_emit.hpp"
#include "prepared_global_memory_emit.hpp"
#include "prepared_local_memory_emit.hpp"
#include "prepared_module_emit.hpp"
#include "prepared_edge_publication_emit.hpp"
#include "prepared_scalar_emit.hpp"
#include "rv64_line_assembler.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
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

namespace c4c::backend::riscv::codegen {
namespace {

namespace object = c4c::backend::mir::object;
namespace prepare = c4c::backend::prepare;

constexpr std::uint32_t kRv64VaStartOverflowAreaScratch = 6;  // t1
constexpr std::uint32_t kRv64VaStartDestinationScratch = 5;    // t0
namespace bir = c4c::backend::bir;

constexpr std::uint16_t kElfMachineRiscv = 243;
constexpr std::uint32_t kRiscvElfFlagsRv64DoubleFloatAbi = 0x5;
constexpr std::uint32_t kRiscvReloc64 = 2;
constexpr std::uint32_t kRiscvRelocCallPlt = 19;
constexpr std::uint32_t kRiscvRelocPcrelHi20 = 23;
constexpr std::uint32_t kRiscvRelocPcrelLo12I = 24;
constexpr std::uint32_t kRiscvRelocBranch = 16;
constexpr std::uint32_t kRiscvRelocJal = 17;
constexpr std::uint32_t kRv64StackFrameScratchRegister = 5;  // t0

std::uint32_t encode_u_type(std::uint32_t opcode, std::uint32_t rd,
                            std::uint32_t imm20) {
  return rv64_encode_u_type(opcode, rd, imm20);
}

std::uint32_t encode_i_type(std::uint32_t opcode, std::uint32_t rd,
                            std::uint32_t funct3, std::uint32_t rs1,
                            std::int32_t imm12) {
  return rv64_encode_i_type(opcode, rd, funct3, rs1, imm12);
}

std::uint32_t encode_s_type(std::uint32_t opcode, std::uint32_t funct3,
                            std::uint32_t rs1, std::uint32_t rs2,
                            std::int32_t imm12) {
  return rv64_encode_s_type(opcode, funct3, rs1, rs2, imm12);
}

std::uint32_t encode_r_type(std::uint32_t opcode, std::uint32_t rd,
                            std::uint32_t funct3, std::uint32_t rs1,
                            std::uint32_t rs2, std::uint32_t funct7) {
  return rv64_encode_r_type(opcode, rd, funct3, rs1, rs2, funct7);
}

std::uint32_t encode_b_type(std::uint32_t opcode, std::uint32_t funct3,
                            std::uint32_t rs1, std::uint32_t rs2,
                            std::int32_t imm13) {
  return rv64_encode_b_type(opcode, funct3, rs1, rs2, imm13);
}

std::uint32_t encode_j_type(std::uint32_t opcode, std::uint32_t rd,
                            std::int32_t imm21) {
  return rv64_encode_j_type(opcode, rd, imm21);
}

void append_fragment(RiscvEncodedFragment& destination,
                     RiscvEncodedFragment source) {
  const auto base_offset = destination.bytes.size();
  destination.bytes.insert(destination.bytes.end(),
                           source.bytes.begin(),
                           source.bytes.end());
  for (auto& label : source.labels) {
    label.offset_bytes += base_offset;
    destination.labels.push_back(std::move(label));
  }
  for (auto& fixup : source.fixups) {
    fixup.offset_bytes += base_offset;
    destination.fixups.push_back(std::move(fixup));
  }
}

void append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  rv64_append_le32(bytes, word);
}

void append_le64(std::vector<std::uint8_t>& bytes, std::uint64_t word) {
  rv64_append_le64(bytes, word);
}

void append_rv64_fragment(RiscvEncodedFragment& destination,
                          RiscvEncodedFragment source) {
  const auto base_offset = destination.bytes.size();
  destination.bytes.insert(destination.bytes.end(),
                           source.bytes.begin(),
                           source.bytes.end());
  for (auto& label : source.labels) {
    label.offset_bytes += base_offset;
    destination.labels.push_back(std::move(label));
  }
  for (auto& fixup : source.fixups) {
    fixup.offset_bytes += base_offset;
    destination.fixups.push_back(std::move(fixup));
  }
}

constexpr bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

object::SymbolBinding binding_for_function(const RiscvObjectFunction& function) {
  return function.global ? object::SymbolBinding::Global
                         : object::SymbolBinding::Local;
}

object::SymbolKind symbol_kind_for_fixup_target(
    RiscvObjectFixupTargetKind kind) {
  switch (kind) {
    case RiscvObjectFixupTargetKind::Function:
      return object::SymbolKind::Function;
    case RiscvObjectFixupTargetKind::Object:
      return object::SymbolKind::Object;
    case RiscvObjectFixupTargetKind::NoType:
      return object::SymbolKind::NoType;
  }
  return object::SymbolKind::NoType;
}

struct PreparedObjectCompare {
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

std::string rv64_normalize_prepared_object_symbol(std::string symbol_name) {
  if (!symbol_name.empty() && symbol_name.front() == '@') {
    symbol_name.erase(symbol_name.begin());
  }
  return symbol_name;
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_call_argument_object_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
  std::string symbol_name;
  if (argument.source_symbol_name_id.has_value()) {
    symbol_name =
        std::string{prepare::prepared_link_name(prepared.names,
                                                *argument.source_symbol_name_id)};
  } else if (argument.source_symbol_name.has_value()) {
    symbol_name = *argument.source_symbol_name;
  }
  if (symbol_name.empty()) {
    return std::nullopt;
  }

  const std::string normalized = rv64_normalize_prepared_object_symbol(symbol_name);
  for (const auto& constant : prepared.module.string_constants) {
    std::string label = constant.name;
    if (constant.name_id != c4c::kInvalidText) {
      const std::string_view spelling =
          prepared.module.names.texts.lookup(constant.name_id);
      if (!spelling.empty()) {
        label = std::string{spelling};
      }
    }
    if (symbol_name == constant.name || symbol_name == label || normalized == label) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          label, RiscvObjectFixupTargetKind::Object};
    }
  }
  for (const auto& global : prepared.module.globals) {
    std::string label = global.name;
    if (global.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view spelling =
          prepared.module.names.link_names.spelling(global.link_name_id);
      if (!spelling.empty()) {
        label = std::string{spelling};
      }
    }
    if (symbol_name == global.name || symbol_name == label || normalized == label) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          label, RiscvObjectFixupTargetKind::Object};
    }
  }

  return std::pair<std::string, RiscvObjectFixupTargetKind>{
      normalized.empty() ? std::move(symbol_name) : normalized,
      RiscvObjectFixupTargetKind::Object};
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_call_relationship_object_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::CallArgumentSourceRelationship& relationship) {
  if (relationship.source_encoding !=
          c4c::backend::bir::CallArgumentSourceEncodingKind::ComputedAddress ||
      !relationship.source_base_value_name.has_value() ||
      relationship.source_base_value_name->empty() ||
      relationship.source_base_value_name->front() != '@' ||
      !relationship.source_pointer_byte_delta.has_value()) {
    return std::nullopt;
  }

  c4c::backend::prepare::PreparedCallArgumentPlan argument;
  argument.source_symbol_name = relationship.source_base_value_name;
  return prepared_call_argument_object_symbol(prepared, argument);
}

struct RiscvPreparedObjectFunctionResult {
  std::optional<RiscvObjectFunction> function;
  std::optional<prepare::PreparedObjectConsumerDiagnosticCategory>
      prepared_consumer_category;
  std::string diagnostic;
};

RiscvPreparedObjectFunctionResult make_rv64_prepared_function_rejection(
    std::string diagnostic) {
  return RiscvPreparedObjectFunctionResult{
      .diagnostic = std::move(diagnostic),
  };
}

std::string_view rv64_prepared_move_destination_kind_name(
    prepare::PreparedMoveDestinationKind kind) {
  switch (kind) {
    case prepare::PreparedMoveDestinationKind::Value:
      return "value";
    case prepare::PreparedMoveDestinationKind::CallArgumentAbi:
      return "call_argument_abi";
    case prepare::PreparedMoveDestinationKind::CallResultAbi:
      return "call_result_abi";
    case prepare::PreparedMoveDestinationKind::FunctionReturnAbi:
      return "function_return_abi";
  }
  return "unknown";
}

std::string_view rv64_prepared_move_storage_kind_name(
    prepare::PreparedMoveStorageKind kind) {
  switch (kind) {
    case prepare::PreparedMoveStorageKind::None:
      return "none";
    case prepare::PreparedMoveStorageKind::Register:
      return "register";
    case prepare::PreparedMoveStorageKind::StackSlot:
      return "stack_slot";
  }
  return "unknown";
}

std::string_view rv64_prepared_move_op_kind_name(
    prepare::PreparedMoveResolutionOpKind kind) {
  switch (kind) {
    case prepare::PreparedMoveResolutionOpKind::Move:
      return "move";
    case prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp:
      return "save_destination_to_temp";
  }
  return "unknown";
}

std::string_view rv64_prepared_function_name(
    const prepare::PreparedNameTables& names,
    c4c::FunctionNameId id) {
  if (id == c4c::kInvalidFunctionName) {
    return "<none>";
  }
  return prepare::prepared_function_name(names, id);
}

std::string_view rv64_prepared_block_label(
    const prepare::PreparedNameTables& names,
    c4c::BlockLabelId id) {
  if (id == c4c::kInvalidBlockLabel) {
    return "<none>";
  }
  return prepare::prepared_block_label(names, id);
}

std::optional<std::uint32_t> gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home);

std::optional<std::uint32_t> rv64_register_number(std::string_view name);

std::optional<std::size_t> rv64_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout);

std::optional<std::string> rv64_variadic_incoming_gpr_publications_diagnostic(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan,
    std::size_t stack_frame_bytes);

std::optional<std::string> rv64_variadic_function_admission_diagnostic(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  std::string diagnostic =
      "unsupported_function_admission: variadic functions are not supported by the RV64 object route";
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr) {
    diagnostic += "; missing variadic entry contract facts were not prepared";
    return diagnostic;
  }
  if (!entry_plan->missing_required_facts.empty()) {
    diagnostic += "; missing_required_facts=[";
    for (std::size_t index = 0; index < entry_plan->missing_required_facts.size();
         ++index) {
      if (index != 0) {
        diagnostic += ", ";
      }
      diagnostic += entry_plan->missing_required_facts[index];
    }
    diagnostic += "]";
    return diagnostic;
  }
  if (entry_plan->helper_resources.required_helpers.empty()) {
    if (rv64_variadic_helper_free_entry_contract_is_complete(*entry_plan)) {
      return std::nullopt;
    }
    return std::string{
        "unsupported_function_admission: RV64 helper-free variadic entry requires a complete one-field overflow-area va_list contract"};
  }
  const auto stack_frame_bytes =
      rv64_object_stack_frame_size(
          prepare::find_prepared_addressing(prepared, function_name),
          prepare::find_prepared_frame_plan(prepared, function_name),
          prepared.stack_layout);
  if (!stack_frame_bytes.has_value()) {
    return std::nullopt;
  }
  if (auto diagnostic = rv64_variadic_incoming_gpr_publications_diagnostic(
          prepared.stack_layout,
          *entry_plan,
          *stack_frame_bytes)) {
    return diagnostic;
  }
  return std::nullopt;
}

std::string rv64_variadic_helper_unsupported_diagnostic(
    prepare::PreparedVariadicEntryHelperKind helper) {
  std::string diagnostic =
      "unsupported_variadic_helper_lowering: RV64 object route does not yet lower ";
  diagnostic += prepare::prepared_variadic_entry_helper_kind_name(helper);
  diagnostic += " helper";
  return diagnostic;
}

std::optional<std::string> rv64_variadic_va_start_runtime_state_diagnostic(
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  if (!entry_plan.overflow_area.base_slot_id.has_value() ||
      !entry_plan.overflow_area.base_stack_offset_bytes.has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state"};
  }
  return std::nullopt;
}

std::optional<std::int32_t> prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes);

std::optional<std::string> rv64_variadic_incoming_gpr_publications_diagnostic(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan,
    std::size_t stack_frame_bytes) {
  constexpr std::size_t kRv64ArgumentGprCount = 8;
  constexpr std::size_t kRv64ArgumentGprBytes = 8;

  if (!entry_plan.named_register_counts.gp.has_value()) {
    return std::string{
        "unsupported_function_admission: RV64 variadic entry requires prepared named GPR count before incoming GPR publication"};
  }
  if (!entry_plan.overflow_area.base_slot_id.has_value() ||
      !entry_plan.overflow_area.base_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }

  const std::size_t named_gp_count =
      std::min(*entry_plan.named_register_counts.gp, kRv64ArgumentGprCount);
  const std::size_t expected_count = kRv64ArgumentGprCount - named_gp_count;
  if (entry_plan.rv64_incoming_variadic_gpr_publications.size() !=
      expected_count) {
    return std::string{
        "unsupported_function_admission: RV64 variadic entry requires one prepared incoming GPR publication for each post-named argument register"};
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *entry_plan.overflow_area.base_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      slot_it->offset_bytes !=
          *entry_plan.overflow_area.base_stack_offset_bytes ||
      slot_it->size_bytes < expected_count * kRv64ArgumentGprBytes ||
      slot_it->align_bytes > kRv64ArgumentGprBytes ||
      slot_it->offset_bytes > stack_frame_bytes ||
      stack_frame_bytes - slot_it->offset_bytes < slot_it->size_bytes) {
    return std::string{
        "unsupported_function_admission: RV64 variadic entry incoming GPR publications require a supported overflow-area backing slot"};
  }

  std::unordered_set<std::size_t> seen_abi_gprs;
  std::unordered_set<std::size_t> seen_destination_offsets;
  for (const auto& publication :
       entry_plan.rv64_incoming_variadic_gpr_publications) {
    if (!seen_abi_gprs.insert(publication.abi_gpr_index).second ||
        !seen_destination_offsets.insert(
             publication.destination_stack_offset_bytes)
             .second) {
      return std::string{
          "unsupported_function_admission: RV64 variadic entry incoming GPR publications must not contain duplicate sources or destinations"};
    }
    if (publication.abi_gpr_index < named_gp_count ||
        publication.abi_gpr_index >= kRv64ArgumentGprCount ||
        publication.variadic_argument_index !=
            publication.abi_gpr_index - named_gp_count) {
      return std::string{
          "unsupported_function_admission: RV64 variadic entry incoming GPR publication does not match the prepared post-named GPR range"};
    }
    std::string expected_source = "a";
    expected_source += std::to_string(publication.abi_gpr_index);
    if (publication.source_register_name != expected_source ||
        rv64_register_number(publication.source_register_name) !=
            std::optional<std::uint32_t>{
                static_cast<std::uint32_t>(10 + publication.abi_gpr_index)}) {
      return std::string{
          "unsupported_function_admission: RV64 variadic entry incoming GPR publication source register is malformed"};
    }
    const std::size_t expected_destination_offset =
        publication.variadic_argument_index * kRv64ArgumentGprBytes;
    const std::size_t expected_stack_offset =
        *entry_plan.overflow_area.base_stack_offset_bytes +
        expected_destination_offset;
    if (publication.destination_slot_id !=
            *entry_plan.overflow_area.base_slot_id ||
        publication.destination_offset_bytes != expected_destination_offset ||
        publication.destination_stack_offset_bytes != expected_stack_offset ||
        publication.size_bytes != kRv64ArgumentGprBytes ||
        publication.align_bytes != kRv64ArgumentGprBytes) {
      return std::string{
          "unsupported_function_admission: RV64 variadic entry incoming GPR publication destination shape is malformed"};
    }
    if (publication.destination_stack_offset_bytes < slot_it->offset_bytes ||
        publication.destination_stack_offset_bytes + publication.size_bytes >
            slot_it->offset_bytes + slot_it->size_bytes ||
        publication.destination_stack_offset_bytes > stack_frame_bytes ||
        stack_frame_bytes - publication.destination_stack_offset_bytes <
            publication.size_bytes ||
        !fits_signed_12_bit_immediate(static_cast<std::int64_t>(
            publication.destination_stack_offset_bytes))) {
      return std::string{
          "unsupported_function_admission: RV64 variadic entry incoming GPR publication destination is outside the prepared overflow-area backing slot"};
    }
  }

  return std::nullopt;
}

std::optional<std::string> rv64_variadic_va_start_materialization_diagnostic(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes,
    std::size_t stack_frame_bytes) {
  const auto* payload = prepare::find_prepared_variadic_va_start_operand_homes(homes);
  if (payload == nullptr) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared va_start helper operand homes"};
  }
  if (auto diagnostic = rv64_variadic_va_start_runtime_state_diagnostic(entry_plan)) {
    return diagnostic;
  }
  const auto* overflow_field =
      rv64_variadic_va_list_overflow_arg_area_field(entry_plan);
  if (overflow_field == nullptr || overflow_field->size_bytes != 8 ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(overflow_field->offset_bytes))) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires a supported overflow-arg-area va_list field"};
  }
  const auto destination_address =
      gpr_register_number_for_home(payload->destination_va_list_address);
  const bool destination_address_is_stack_slot =
      payload->destination_va_list_address.kind ==
          prepare::PreparedValueHomeKind::StackSlot &&
      prepared_stack_slot_home_offset(stack_layout,
                                      payload->destination_va_list_address,
                                      stack_frame_bytes,
                                      8)
          .has_value();
  if ((payload->destination_va_list_address.kind !=
           prepare::PreparedValueHomeKind::Register ||
       !destination_address.has_value()) &&
      !destination_address_is_stack_slot) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR or stack-slot home"};
  }
  if (destination_address.has_value() &&
      (*destination_address == kRv64VaStartOverflowAreaScratch ||
       *destination_address == kRv64VaStartDestinationScratch)) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper destination va_list address aliases a helper scratch register"};
  }
  if (!entry_plan.va_list_layout.size_bytes.has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list in a supported prepared stack-slot home"};
  }
  if (!prepared_stack_slot_home_offset(stack_layout,
                                       payload->destination_va_list,
                                       stack_frame_bytes,
                                       *entry_plan.va_list_layout.size_bytes)
           .has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list in a supported prepared stack-slot home"};
  }
  if (destination_address_is_stack_slot &&
      payload->destination_va_list_address.slot_id ==
          payload->destination_va_list.slot_id) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper destination va_list address aliases the destination va_list stack slot"};
  }
  if (!fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(*entry_plan.overflow_area.base_stack_offset_bytes))) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper overflow-area initial base offset exceeds RV64 immediate range"};
  }
  return std::nullopt;
}

std::optional<std::string>
rv64_variadic_va_arg_aggregate_materialization_diagnostic(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes,
    std::size_t stack_frame_bytes) {
  constexpr std::size_t kMaxSupportedCopySize = 128;
  const auto* payload =
      prepare::find_prepared_variadic_aggregate_va_arg_operand_homes(homes);
  if (payload == nullptr) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared va_arg_aggregate helper operand homes"};
  }

  const auto* overflow_field =
      rv64_variadic_va_list_overflow_arg_area_field(entry_plan);
  if (overflow_field == nullptr || overflow_field->size_bytes != 8 ||
      overflow_field->offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(overflow_field->offset_bytes))) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires a supported overflow-arg-area va_list field"};
  }
  if (payload->source_va_list.kind != prepare::PreparedValueHomeKind::Register ||
      !gpr_register_number_for_home(payload->source_va_list).has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires source va_list in a prepared GPR home"};
  }
  if (entry_plan.helper_resources.scratch_register_count < 2) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires prepared scratch registers"};
  }
  const auto& plan = payload->aggregate_access_plan;
  if (!plan.payload_write_address.has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared va_arg_aggregate helper operand homes"};
  }

  if (plan.source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      plan.source_field != prepare::PreparedVariadicVaListFieldKind::OverflowArgArea ||
      plan.progression_field !=
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea ||
      plan.source_field_offset_bytes != overflow_field->offset_bytes ||
      plan.progression_field_offset_bytes != overflow_field->offset_bytes ||
      !plan.source_payload_offset_bytes.has_value() ||
      !plan.source_slot_size_bytes.has_value() ||
      !plan.copy_size_bytes.has_value() ||
      !plan.copy_align_bytes.has_value() ||
      !plan.progression_stride_bytes.has_value() ||
      !plan.overflow_stride_bytes.has_value() ||
      plan.overflow_source_field_offset_bytes != overflow_field->offset_bytes ||
      *plan.progression_stride_bytes != *plan.overflow_stride_bytes ||
      *plan.source_slot_size_bytes != *plan.progression_stride_bytes ||
      *plan.copy_size_bytes == 0 ||
      *plan.copy_size_bytes > plan.payload_size_bytes ||
      *plan.copy_size_bytes > *plan.source_slot_size_bytes ||
      *plan.source_payload_offset_bytes >= *plan.source_slot_size_bytes ||
      *plan.source_payload_offset_bytes >
          *plan.source_slot_size_bytes - *plan.copy_size_bytes ||
      *plan.copy_size_bytes > kMaxSupportedCopySize ||
      *plan.copy_align_bytes == 0 ||
      *plan.copy_align_bytes > 8 ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(*plan.progression_stride_bytes))) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires a supported overflow-area aggregate access plan"};
  }

  const auto& write_address = *plan.payload_write_address;
  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == write_address.frame_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      write_address.stack_offset_bytes < slot_it->offset_bytes ||
      write_address.stack_offset_bytes - slot_it->offset_bytes >
          slot_it->size_bytes ||
      slot_it->size_bytes -
              (write_address.stack_offset_bytes - slot_it->offset_bytes) <
          *plan.copy_size_bytes ||
      write_address.stack_offset_bytes > stack_frame_bytes ||
      stack_frame_bytes - write_address.stack_offset_bytes <
          *plan.copy_size_bytes ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(write_address.stack_offset_bytes))) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires payload_write_address in a supported frame slot"};
  }

  return std::nullopt;
}

std::optional<std::string> diagnose_unsupported_prepared_variadic_helper_fragment(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::Inst& inst,
    std::size_t stack_frame_bytes) {
  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr) {
    return std::nullopt;
  }
  const auto helper = prepare::prepared_variadic_entry_helper_kind_for_call(*call);
  if (!helper.has_value()) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr) {
    return std::string{
        "unsupported_variadic_helper_lowering: missing variadic entry contract facts were not prepared"};
  }
  const auto* homes = prepare::find_prepared_variadic_entry_helper_operand_homes(
      *entry_plan,
      block_index,
      instruction_index);
  if (homes == nullptr || homes->helper != *helper) {
    std::string diagnostic =
        "unsupported_variadic_helper_lowering: RV64 object route requires prepared ";
    diagnostic += prepare::prepared_variadic_entry_helper_kind_name(*helper);
    diagnostic += " helper operand homes";
    return diagnostic;
  }
  bool has_typed_payload = false;
  switch (*helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      has_typed_payload =
          prepare::find_prepared_variadic_va_start_operand_homes(*homes) !=
          nullptr;
      break;
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      has_typed_payload =
          prepare::find_prepared_variadic_scalar_va_arg_operand_homes(*homes) !=
          nullptr;
      break;
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      has_typed_payload =
          prepare::find_prepared_variadic_aggregate_va_arg_operand_homes(
              *homes) != nullptr;
      break;
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      has_typed_payload =
          prepare::find_prepared_variadic_va_copy_operand_homes(*homes) !=
          nullptr;
      break;
  }
  if (!has_typed_payload) {
    std::string diagnostic =
        "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared ";
    diagnostic += prepare::prepared_variadic_entry_helper_kind_name(*helper);
    diagnostic += " helper operand homes";
    return diagnostic;
  }
  const auto* aggregate_payload =
      *helper == prepare::PreparedVariadicEntryHelperKind::VaArgAggregate
          ? prepare::find_prepared_variadic_aggregate_va_arg_operand_homes(*homes)
          : nullptr;
  if (aggregate_payload != nullptr &&
      !aggregate_payload->aggregate_access_plan.payload_write_address.has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared va_arg_aggregate helper operand homes"};
  }
  if (*helper == prepare::PreparedVariadicEntryHelperKind::VaStart) {
    if (auto diagnostic =
            rv64_variadic_va_start_materialization_diagnostic(
                prepared.stack_layout,
                *entry_plan,
                *homes,
                stack_frame_bytes)) {
      return diagnostic;
    }
    return std::nullopt;
  }
  if (*helper == prepare::PreparedVariadicEntryHelperKind::VaArgAggregate) {
    if (auto diagnostic =
            rv64_variadic_va_arg_aggregate_materialization_diagnostic(
                prepared.stack_layout,
                *entry_plan,
                *homes,
                stack_frame_bytes)) {
      return diagnostic;
    }
    return std::nullopt;
  }
  return rv64_variadic_helper_unsupported_diagnostic(*helper);
}

std::optional<std::string> diagnose_first_unsupported_prepared_variadic_helper(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::Function& function,
    std::size_t stack_frame_bytes) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      if (auto diagnostic =
              diagnose_unsupported_prepared_variadic_helper_fragment(
                  prepared,
                  function_name,
                  block_index,
                  instruction_index,
                  block.insts[instruction_index],
                  stack_frame_bytes)) {
        return diagnostic;
      }
    }
  }
  return std::nullopt;
}

struct RiscvLaidOutFragment {
  const RiscvEncodedFragment* fragment = nullptr;
  std::uint64_t section_offset = 0;
};

std::optional<std::uint32_t> rv64_register_number(std::string_view name) {
  return rv64_prepared_register_number(name);
}

std::optional<std::uint32_t> gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  return rv64_prepared_gpr_register_number_for_home(home);
}

std::optional<std::uint32_t> rv64_fpr_register_number(std::string_view name) {
  if (name == "f0" || name == "ft0") return 0;
  if (name == "f1" || name == "ft1") return 1;
  if (name == "f2" || name == "ft2") return 2;
  if (name == "f3" || name == "ft3") return 3;
  if (name == "f4" || name == "ft4") return 4;
  if (name == "f5" || name == "ft5") return 5;
  if (name == "f6" || name == "ft6") return 6;
  if (name == "f7" || name == "ft7") return 7;
  if (name == "f8" || name == "fs0") return 8;
  if (name == "f9" || name == "fs1") return 9;
  if (name == "f10" || name == "fa0") return 10;
  if (name == "f11" || name == "fa1") return 11;
  if (name == "f12" || name == "fa2") return 12;
  if (name == "f13" || name == "fa3") return 13;
  if (name == "f14" || name == "fa4") return 14;
  if (name == "f15" || name == "fa5") return 15;
  if (name == "f16" || name == "fa6") return 16;
  if (name == "f17" || name == "fa7") return 17;
  if (name == "f18" || name == "fs2") return 18;
  if (name == "f19" || name == "fs3") return 19;
  if (name == "f20" || name == "fs4") return 20;
  if (name == "f21" || name == "fs5") return 21;
  if (name == "f22" || name == "fs6") return 22;
  if (name == "f23" || name == "fs7") return 23;
  if (name == "f24" || name == "fs8") return 24;
  if (name == "f25" || name == "fs9") return 25;
  if (name == "f26" || name == "fs10") return 26;
  if (name == "f27" || name == "fs11") return 27;
  if (name == "f28" || name == "ft8") return 28;
  if (name == "f29" || name == "ft9") return 29;
  if (name == "f30" || name == "ft10") return 30;
  if (name == "f31" || name == "ft11") return 31;
  return std::nullopt;
}

void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source);

bool append_rv64_store_register_to_stack(RiscvEncodedFragment& fragment,
                                         std::uint32_t source_register,
                                         std::int32_t offset,
                                         std::size_t size_bytes = 4);

bool append_rv64_load_stack_to_register(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes = 4);

std::optional<std::uint32_t> gpr_register_number_for_target_identity(
    const c4c::backend::prepare::PreparedTargetRegisterIdentity& identity) {
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != c4c::backend::prepare::PreparedRegisterBank::Gpr ||
      identity.register_class !=
          c4c::backend::prepare::PreparedRegisterClass::General ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}

bool append_rv64_callee_saved_gpr_preservation_effect(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    return false;
  }

  const auto& source = effect.source;
  const auto& destination = effect.destination;
  if (source.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      destination.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      source.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      destination.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !source.register_name.has_value() ||
      !destination.register_name.has_value() ||
      source.contiguous_width != 1 ||
      destination.contiguous_width != 1 ||
      source.occupied_register_names.size() > 1 ||
      destination.occupied_register_names.size() > 1) {
    return false;
  }

  const auto source_register = rv64_register_number(*source.register_name);
  const auto destination_register = rv64_register_number(*destination.register_name);
  if (!source_register.has_value() || !destination_register.has_value()) {
    return false;
  }

  append_rv64_move(fragment, *destination_register, *source_register);
  return true;
}

std::optional<std::uint32_t> fpr_register_number_for_target_identity(
    const c4c::backend::prepare::PreparedTargetRegisterIdentity& identity);

bool append_rv64_fpr_move(RiscvEncodedFragment& fragment,
                          std::uint32_t destination,
                          std::uint32_t source,
                          c4c::backend::bir::TypeKind type);

void append_rv64_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate);

bool rv64_fpr_endpoint_names_one_register(
    const c4c::backend::prepare::PreparedCallBoundaryEffectEndpoint& endpoint) {
  return endpoint.register_name.has_value() &&
         !endpoint.register_name->empty() &&
         endpoint.contiguous_width == 1 &&
         endpoint.occupied_register_names.size() == 1 &&
         endpoint.occupied_register_names.front() == *endpoint.register_name;
}

std::optional<std::uint32_t> fpr_register_number_for_boundary_endpoint(
    const c4c::backend::prepare::PreparedCallBoundaryEffectEndpoint& endpoint) {
  namespace prepare = c4c::backend::prepare;

  if (endpoint.encoding != prepare::PreparedStorageEncodingKind::Register ||
      endpoint.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      endpoint.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Fpr} ||
      !rv64_fpr_endpoint_names_one_register(endpoint)) {
    return std::nullopt;
  }
  const auto by_name = rv64_fpr_register_number(*endpoint.register_name);
  if (!by_name.has_value()) {
    return std::nullopt;
  }
  if (!endpoint.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto by_identity =
      fpr_register_number_for_target_identity(*endpoint.target_register_identity);
  if (!by_identity.has_value() || *by_identity != *by_name) {
    return std::nullopt;
  }
  if (endpoint.register_placement.has_value() &&
      (endpoint.register_placement->bank != prepare::PreparedRegisterBank::Fpr ||
       endpoint.register_placement->contiguous_width != 1)) {
    return std::nullopt;
  }
  return by_name;
}

bool rv64_is_callee_saved_fpr_endpoint(
    const c4c::backend::prepare::PreparedCallBoundaryEffectEndpoint& endpoint) {
  namespace prepare = c4c::backend::prepare;

  return endpoint.callee_saved_save_index.has_value() &&
         endpoint.register_placement.has_value() &&
         endpoint.register_placement->bank == prepare::PreparedRegisterBank::Fpr &&
         endpoint.register_placement->pool ==
             prepare::PreparedRegisterSlotPool::CalleeSaved &&
         endpoint.register_placement->contiguous_width == 1;
}

bool append_rv64_callee_saved_fpr_preservation_effect(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    return false;
  }

  const auto source =
      fpr_register_number_for_boundary_endpoint(effect.source);
  const auto destination =
      fpr_register_number_for_boundary_endpoint(effect.destination);
  if (!source.has_value() || !destination.has_value()) {
    return false;
  }
  const bool source_is_callee_saved =
      rv64_is_callee_saved_fpr_endpoint(effect.source);
  const bool destination_is_callee_saved =
      rv64_is_callee_saved_fpr_endpoint(effect.destination);
  if (effect_kind ==
          prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation) {
    if (source_is_callee_saved || !destination_is_callee_saved) {
      return false;
    }
  } else if (effect_kind ==
             prepare::PreparedCallBoundaryEffectKind::PreservationRepublication) {
    if (!source_is_callee_saved || destination_is_callee_saved) {
      return false;
    }
  } else {
    return false;
  }

  return append_rv64_fpr_move(
      fragment, *destination, *source, c4c::backend::bir::TypeKind::F64);
}

bool append_rv64_register_source_stack_slot_preservation_effect(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route != prepare::PreparedCallPreservationRoute::StackSlot) {
    return false;
  }

  const auto& source = effect.source;
  const auto& destination = effect.destination;
  if (source.encoding != prepare::PreparedStorageEncodingKind::Register ||
      source.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      source.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !source.register_name.has_value() ||
      source.register_name->empty() ||
      source.contiguous_width != 1 ||
      source.occupied_register_names.empty() ||
      !source.target_register_identity.has_value() ||
      destination.encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      destination.storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      !destination.slot_id.has_value() ||
      !destination.stack_offset_bytes.has_value() ||
      !destination.stack_size_bytes.has_value() ||
      !destination.stack_align_bytes.has_value() ||
      *destination.stack_size_bytes == 0 ||
      *destination.stack_align_bytes > *destination.stack_size_bytes ||
      source.value_id != destination.value_id ||
      source.value_name != destination.value_name) {
    return false;
  }

  const auto source_by_name = rv64_register_number(*source.register_name);
  const auto source_by_identity =
      gpr_register_number_for_target_identity(*source.target_register_identity);
  if (!source_by_name.has_value() || !source_by_identity.has_value() ||
      *source_by_name != *source_by_identity ||
      *destination.stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !append_rv64_store_register_to_stack(
          fragment,
          *source_by_identity,
          static_cast<std::int32_t>(*destination.stack_offset_bytes),
          *destination.stack_size_bytes)) {
    return false;
  }
  return true;
}

bool append_rv64_stack_slot_source_register_preservation_effect(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route != prepare::PreparedCallPreservationRoute::StackSlot) {
    return false;
  }

  const auto& source = effect.source;
  const auto& destination = effect.destination;
  if (source.encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      source.storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      !source.slot_id.has_value() ||
      !source.stack_offset_bytes.has_value() ||
      !source.stack_size_bytes.has_value() ||
      !source.stack_align_bytes.has_value() ||
      *source.stack_size_bytes == 0 ||
      *source.stack_align_bytes > *source.stack_size_bytes ||
      destination.encoding != prepare::PreparedStorageEncodingKind::Register ||
      destination.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      destination.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !destination.register_name.has_value() ||
      destination.register_name->empty() ||
      destination.contiguous_width != 1 ||
      destination.occupied_register_names.empty() ||
      !destination.target_register_identity.has_value() ||
      source.value_id != destination.value_id ||
      source.value_name != destination.value_name) {
    return false;
  }

  const auto destination_by_name = rv64_register_number(*destination.register_name);
  const auto destination_by_identity =
      gpr_register_number_for_target_identity(*destination.target_register_identity);
  if (!destination_by_name.has_value() || !destination_by_identity.has_value() ||
      *destination_by_name != *destination_by_identity ||
      *source.stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !append_rv64_load_stack_to_register(
          fragment,
          *destination_by_identity,
          static_cast<std::int32_t>(*source.stack_offset_bytes),
          *source.stack_size_bytes)) {
    return false;
  }
  return true;
}

bool rv64_noop_stack_slot_preservation_effect(
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route != prepare::PreparedCallPreservationRoute::StackSlot) {
    return false;
  }
  const auto& source = effect.source;
  const auto& destination = effect.destination;
  return source.storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
         destination.storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
         source.encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
         destination.encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
         source.value_id == destination.value_id &&
         source.slot_id.has_value() &&
         source.slot_id == destination.slot_id &&
         source.stack_offset_bytes.has_value() &&
         source.stack_offset_bytes == destination.stack_offset_bytes &&
         source.stack_size_bytes.has_value() &&
         source.stack_size_bytes == destination.stack_size_bytes &&
         source.stack_align_bytes.has_value() &&
         source.stack_align_bytes == destination.stack_align_bytes;
}

std::optional<std::uint32_t> fpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != c4c::backend::prepare::PreparedRegisterBank::Fpr ||
      identity.register_class !=
          c4c::backend::prepare::PreparedRegisterClass::Float ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}

std::optional<std::uint32_t> fpr_register_number_for_target_identity(
    const c4c::backend::prepare::PreparedTargetRegisterIdentity& identity) {
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != c4c::backend::prepare::PreparedRegisterBank::Fpr ||
      identity.register_class !=
          c4c::backend::prepare::PreparedRegisterClass::Float ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}

std::optional<std::uint32_t> fpr_register_number_for_abi_placement(
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedRegisterPlacement& placement) {
  const auto identity =
      c4c::backend::prepare::target_register_identity_for_abi_register_placement(
          target_profile, placement);
  return identity.has_value() ? fpr_register_number_for_target_identity(*identity)
                              : std::nullopt;
}

std::optional<std::uint32_t> rv64_fpr_move_funct7(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x10;  // fmv.s is fsgnj.s fd, fs, fs
    case c4c::backend::bir::TypeKind::F64:
      return 0x11;  // fmv.d is fsgnj.d fd, fs, fs
    default:
      return std::nullopt;
  }
}

bool append_rv64_fpr_move(RiscvEncodedFragment& fragment,
                          std::uint32_t destination,
                          std::uint32_t source,
                          c4c::backend::bir::TypeKind type) {
  const auto funct7 = rv64_fpr_move_funct7(type);
  if (!funct7.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53, destination, 0, source, source, *funct7));
  return true;
}

std::optional<std::uint32_t> rv64_gpr_to_fpr_move_funct7(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x78;  // fmv.w.x
    case c4c::backend::bir::TypeKind::F64:
      return 0x79;  // fmv.d.x
    default:
      return std::nullopt;
  }
}

bool append_rv64_gpr_to_fpr_move(RiscvEncodedFragment& fragment,
                                 std::uint32_t destination,
                                 std::uint32_t source,
                                 c4c::backend::bir::TypeKind type) {
  const auto funct7 = rv64_gpr_to_fpr_move_funct7(type);
  if (!funct7.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53, destination, 0, source, 0, *funct7));
  return true;
}

constexpr std::uint32_t kRv64FprImmediateCallArgumentScratchGpr = 5;  // t0

bool append_rv64_fpr_immediate_call_argument(
    RiscvEncodedFragment& fragment,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind arg_type) {
  namespace prepare = c4c::backend::prepare;

  if (argument.value_bank != prepare::PreparedRegisterBank::Fpr ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !argument.source_literal.has_value() ||
      argument.source_literal->kind != c4c::backend::bir::Value::Kind::Immediate ||
      argument.source_literal->type != arg_type ||
      (arg_type != c4c::backend::bir::TypeKind::F32 &&
       arg_type != c4c::backend::bir::TypeKind::F64) ||
      (argument.source_register_bank.has_value() &&
       argument.source_register_bank != prepare::PreparedRegisterBank::None) ||
      argument.destination_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Fpr} ||
      !argument.destination_register_name.has_value() ||
      argument.destination_contiguous_width != 1 ||
      argument.destination_occupied_register_names.size() != 1 ||
      argument.destination_occupied_register_names.front() !=
          *argument.destination_register_name ||
      !argument.destination_register_placement.has_value() ||
      argument.destination_register_placement->bank !=
          prepare::PreparedRegisterBank::Fpr ||
      argument.destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument ||
      argument.destination_register_placement->contiguous_width != 1 ||
      !argument.destination_target_register_identity.has_value()) {
    return false;
  }

  const auto destination_by_name =
      rv64_fpr_register_number(*argument.destination_register_name);
  const auto destination_by_placement = fpr_register_number_for_abi_placement(
      target_profile, *argument.destination_register_placement);
  const auto destination_by_identity = fpr_register_number_for_target_identity(
      *argument.destination_target_register_identity);
  if (!destination_by_name.has_value() || !destination_by_placement.has_value() ||
      !destination_by_identity.has_value() ||
      *destination_by_name != *destination_by_placement ||
      *destination_by_name != *destination_by_identity) {
    return false;
  }

  append_rv64_load_immediate(
      fragment,
      kRv64FprImmediateCallArgumentScratchGpr,
      static_cast<std::int64_t>(argument.source_literal->immediate_bits));
  return append_rv64_gpr_to_fpr_move(
      fragment,
      *destination_by_placement,
      kRv64FprImmediateCallArgumentScratchGpr,
      arg_type);
}

const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (lookups == nullptr || value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return nullptr;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  return home_it == lookups->value_homes.homes_by_id.end() ? nullptr
                                                           : home_it->second;
}

std::optional<std::int64_t> integer_immediate_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
      break;
    default:
      return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(home);
  if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  const auto fact = prepare::as_rematerializable_integer_immediate_fact(*home);
  return fact.has_value() ? std::optional<std::int64_t>{fact->signed_value}
                          : std::nullopt;
}

bool is_rv64_null_pointer_value(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         value.type == c4c::backend::bir::TypeKind::Ptr &&
         value.immediate == 0 && value.immediate_bits == 0;
}

std::optional<std::uint32_t> gpr_register_number_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt : gpr_register_number_for_home(*home);
}

std::optional<std::uint32_t> fpr_register_number_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt : fpr_register_number_for_home(*home);
}

bool prepared_global_byte_storage_access_has_authority(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::size_t expected_size_bytes,
    std::size_t expected_align_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if ((expected_size_bytes != 4 && expected_size_bytes != 16) ||
      expected_align_bytes != expected_size_bytes) {
    return false;
  }
  if (access == nullptr ||
      access->result_value_name != result_value_name ||
      access->stored_value_name != stored_value_name ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      access->address.size_bytes != expected_size_bytes ||
      access->address.align_bytes != expected_align_bytes ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset) ||
      access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::ByteStorageAggregate) {
    return false;
  }
  if (expected_size_bytes == 16 &&
      !fits_signed_12_bit_immediate(access->address.byte_offset + 8)) {
    return false;
  }
  const auto& extent = access->address.provenance.object_extent;
  const auto& range = access->address.provenance.requested_range;
  if (!extent.size_known ||
      extent.completeness != bir::MemoryObjectExtentCompleteness::Complete ||
      extent.size_bytes == 0 ||
      !range.available ||
      range.overflowed ||
      !range.end_available ||
      range.begin != access->address.byte_offset ||
      range.size_bytes != expected_size_bytes ||
      access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      range.begin < 0 ||
      range.end < range.begin ||
      static_cast<std::size_t>(range.end - range.begin) !=
          expected_size_bytes ||
      static_cast<std::size_t>(range.end) > extent.size_bytes) {
    return false;
  }
  return access->address.byte_offset >= 0;
}

bool prepared_global_byte_storage_access_has_fact_authority(
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::size_t expected_size_bytes,
    std::size_t expected_align_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;
  if ((expected_size_bytes != 4 && expected_size_bytes != 16) ||
      expected_align_bytes != expected_size_bytes) {
    return false;
  }
  return access != nullptr &&
         access->result_value_name == result_value_name &&
         access->stored_value_name == stored_value_name &&
         access->address_space == bir::AddressSpace::Default &&
         !access->is_volatile &&
         access->address.base_kind == prepare::PreparedAddressBaseKind::GlobalSymbol &&
         access->address.symbol_name.has_value() &&
         access->address.size_bytes == expected_size_bytes &&
         access->address.align_bytes == expected_align_bytes &&
         access->address.can_use_base_plus_offset &&
         access->address.provenance.layout_authority ==
             bir::MemoryLayoutAuthorityKind::ByteStorageAggregate &&
         access->address.provenance.object_extent.size_known &&
         access->address.provenance.object_extent.completeness ==
             bir::MemoryObjectExtentCompleteness::Complete &&
         access->address.provenance.object_extent.size_bytes != 0 &&
         access->address.provenance.requested_range.available &&
         !access->address.provenance.requested_range.overflowed &&
         access->address.provenance.requested_range.end_available &&
         access->address.provenance.requested_range.begin ==
             access->address.byte_offset &&
         access->address.provenance.requested_range.size_bytes ==
             expected_size_bytes &&
         access->address.provenance.range_verdict ==
             bir::MemoryRangeVerdict::ProvenInBounds &&
         access->address.provenance.requested_range.begin >= 0 &&
         access->address.provenance.requested_range.end >=
             access->address.provenance.requested_range.begin &&
         static_cast<std::size_t>(
             access->address.provenance.requested_range.end -
             access->address.provenance.requested_range.begin) ==
             expected_size_bytes &&
         static_cast<std::size_t>(
             access->address.provenance.requested_range.end) <=
             access->address.provenance.object_extent.size_bytes &&
         fits_signed_12_bit_immediate(access->address.byte_offset) &&
         (expected_size_bytes != 16 ||
          fits_signed_12_bit_immediate(access->address.byte_offset + 8)) &&
         access->address.byte_offset >= 0;
}

bool prepared_global_16_byte_byte_storage_access_has_authority(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name) {
  return prepared_global_byte_storage_access_has_authority(
      prepared, access, result_value_name, stored_value_name, 16, 16);
}

bool prepared_global_16_byte_byte_storage_access_has_fact_authority(
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name) {
  return prepared_global_byte_storage_access_has_fact_authority(
      access, result_value_name, stored_value_name, 16, 16);
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_f32_byte_storage_global_load(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadGlobalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  if (load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::F32) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName ||
      !prepared_global_byte_storage_access_has_authority(
          prepared, access, result_value_name, std::nullopt, 4, 4)) {
    return std::nullopt;
  }
  const std::string_view symbol =
      c4c::backend::prepare::prepared_link_name(prepared.names,
                                                *access->address.symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for(names, lookups, load.result);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto destination = fpr_register_number_for_home(*home);
  const auto destination_offset =
      rv64_prepared_stack_slot_home_offset(stack_layout,
                                           *home,
                                           stack_frame_bytes,
                                           4);
  if (!destination.has_value() && !destination_offset.has_value()) {
    return std::nullopt;
  }

  const std::uint32_t destination_register = destination.value_or(0);
  constexpr std::uint32_t address_register = 6;  // t1
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      address_register,
      std::string{symbol},
      ".Lpcrel_hi_global_load_f32_byte_storage_" +
          std::to_string(access->function_name) + "_" +
          std::to_string(access->block_label) + "_" +
          std::to_string(access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  const auto global_offset =
      static_cast<std::int32_t>(access->address.byte_offset);
  if (!fits_signed_12_bit_immediate(global_offset)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x07,
                            destination_register,
                            2,
                            address_register,
                            global_offset));
  if (destination_offset.has_value()) {
    if (!fits_signed_12_bit_immediate(*destination_offset)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_s_type(0x27,
                              2,
                              2,
                              destination_register,
                              *destination_offset));
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_16_byte_byte_storage_global_load(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadGlobalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  if (load.result.kind != bir::Value::Kind::Named ||
      (load.result.type != bir::TypeKind::I128 &&
       load.result.type != bir::TypeKind::F128)) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName ||
      !prepared_global_16_byte_byte_storage_access_has_authority(
          prepared, access, result_value_name, std::nullopt)) {
    return std::nullopt;
  }
  if (load.result.type == bir::TypeKind::F128) {
    const auto has_store_local_publication =
        std::any_of(prepared.store_source_publications.records.begin(),
                    prepared.store_source_publications.records.end(),
                    [&](const auto& record) {
                      return c4c::backend::prepare::
                                 prepared_store_source_publication_available(
                                     record.plan) &&
                             record.plan.intent ==
                                 c4c::backend::prepare::
                                     PreparedStoreSourcePublicationIntent::
                                         StoreLocalPublication &&
                             record.plan.source_value_name ==
                                 result_value_name &&
                             record.plan.source_load_global != nullptr &&
                             record.plan.source_producer_block_label ==
                                 access->block_label &&
                             record.plan.source_producer_instruction_index ==
                                 access->inst_index;
                    });
    if (!has_store_local_publication) {
      return std::nullopt;
    }
    return RiscvEncodedFragment{};
  }
  const auto* home = prepared_value_home_for(names, lookups, load.result);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto destination_offset =
      rv64_prepared_stack_slot_home_absolute_offset(stack_layout,
                                                    *home,
                                                    stack_frame_bytes,
                                                    16);
  if (!destination_offset.has_value() ||
      *destination_offset > std::numeric_limits<std::int32_t>::max() ||
      *destination_offset + 8 > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }
  const std::string_view symbol =
      c4c::backend::prepare::prepared_link_name(prepared.names,
                                                *access->address.symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }

  constexpr std::uint32_t address_register = 6;  // t1
  constexpr std::uint32_t scratch_register = 5;  // t0
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      address_register,
      std::string{symbol},
      ".Lpcrel_hi_global_load16_" + std::to_string(access->function_name) +
          "_" + std::to_string(access->block_label) + "_" +
          std::to_string(access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  const auto global_offset =
      static_cast<std::int32_t>(access->address.byte_offset);
  const auto stack_offset = static_cast<std::int32_t>(*destination_offset);
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            scratch_register,
                            3,
                            address_register,
                            global_offset));
  if (!append_rv64_prepared_store_register_to_stack(fragment,
                                                   scratch_register,
                                                   stack_offset,
                                                   8)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            scratch_register,
                            3,
                            address_register,
                            global_offset + 8));
  if (!append_rv64_prepared_store_register_to_stack(fragment,
                                                   scratch_register,
                                                   stack_offset + 8,
                                                   8)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_16_byte_byte_storage_global_store(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::StoreGlobalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  if (store.value.kind != bir::Value::Kind::Named ||
      store.value.type != bir::TypeKind::I128) {
    return std::nullopt;
  }
  const auto stored_value_name = names.value_names.find(store.value.name);
  if (stored_value_name == c4c::kInvalidValueName ||
      !prepared_global_16_byte_byte_storage_access_has_authority(
          prepared, access, std::nullopt, stored_value_name)) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for(names, lookups, store.value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto source_offset =
      rv64_prepared_stack_slot_home_absolute_offset(stack_layout,
                                                    *home,
                                                    stack_frame_bytes,
                                                    16);
  if (!source_offset.has_value() ||
      *source_offset > std::numeric_limits<std::int32_t>::max() ||
      *source_offset + 8 > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }
  const std::string_view symbol =
      c4c::backend::prepare::prepared_link_name(prepared.names,
                                                *access->address.symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }

  constexpr std::uint32_t address_register = 5;  // t0
  constexpr std::uint32_t scratch_register = 6;  // t1
  RiscvEncodedFragment fragment;
  append_fragment(
      fragment,
      make_rv64_pcrel_address_fragment(
          address_register,
          std::string{symbol},
          ".Lpcrel_hi_global_store16_" + std::to_string(access->function_name) +
              "_" + std::to_string(access->block_label) + "_" +
              std::to_string(access->inst_index),
          RiscvObjectFixupTargetKind::Object,
          0));
  const auto global_offset =
      static_cast<std::int32_t>(access->address.byte_offset);
  const auto stack_offset = static_cast<std::int32_t>(*source_offset);
  if (!append_rv64_prepared_load_stack_to_register(fragment,
                                                  scratch_register,
                                                  stack_offset,
                                                  8)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            3,
                            address_register,
                            scratch_register,
                            global_offset));
  if (!append_rv64_prepared_load_stack_to_register(fragment,
                                                  scratch_register,
                                                  stack_offset + 8,
                                                  8)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            3,
                            address_register,
                            scratch_register,
                            global_offset + 8));
  return fragment;
}

const c4c::backend::prepare::PreparedStoreSourcePublicationRecord*
prepared_store_source_publication_for_instruction(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  const c4c::backend::prepare::PreparedStoreSourcePublicationRecord* selected =
      nullptr;
  for (const auto& record : prepared.store_source_publications.records) {
    if (record.function_name != function_name ||
        record.block_label != block_label ||
        record.instruction_index != instruction_index ||
        !c4c::backend::prepare::prepared_store_source_publication_available(
            record.plan) ||
        record.plan.intent !=
            c4c::backend::prepare::PreparedStoreSourcePublicationIntent::
                StoreLocalPublication) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &record;
  }
  return selected;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_16_byte_byte_storage_global_store_local_publication(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::StoreLocalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* destination_access,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (store.value.kind != bir::Value::Kind::Named ||
      (store.value.type != bir::TypeKind::F128 &&
       store.value.type != bir::TypeKind::I128)) {
    return std::nullopt;
  }
  const auto source_value_name =
      prepared.names.value_names.find(store.value.name);
  if (source_value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* publication =
      prepared_store_source_publication_for_instruction(prepared,
                                                        function_name,
                                                        block_label,
                                                        instruction_index);
  if (publication == nullptr ||
      publication->plan.source_value_name != source_value_name ||
      publication->plan.source_load_global == nullptr ||
      publication->plan.destination_access != destination_access) {
    return std::nullopt;
  }
  if (lookups == nullptr ||
      !publication->plan.source_producer_block_label.has_value() ||
      !publication->plan.source_producer_instruction_index.has_value()) {
    return std::nullopt;
  }
  const auto* source_access = prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      *publication->plan.source_producer_block_label,
      *publication->plan.source_producer_instruction_index);
  if (!prepared_global_16_byte_byte_storage_access_has_authority(
          prepared,
          source_access,
          source_value_name,
          std::nullopt)) {
    return std::nullopt;
  }
  if (destination_access == nullptr ||
      destination_access->address_space != bir::AddressSpace::Default ||
      destination_access->is_volatile ||
      destination_access->address.base_kind !=
          prepare::PreparedAddressBaseKind::FrameSlot ||
      destination_access->address.size_bytes != 16 ||
      destination_access->address.align_bytes != 16 ||
      !destination_access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto destination_offset =
      prepared_frame_slot_absolute_byte_offset(stack_layout,
                                               destination_access,
                                               stack_frame_bytes,
                                               16);
  if (!destination_offset.has_value() ||
      *destination_offset > std::numeric_limits<std::int32_t>::max() ||
      *destination_offset + 8 > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names,
                                  *source_access->address.symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }

  constexpr std::uint32_t address_register = 6;  // t1
  constexpr std::uint32_t scratch_register = 5;  // t0
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      address_register,
      std::string{symbol},
      ".Lpcrel_hi_global_load16_store_local_" +
          std::to_string(source_access->function_name) + "_" +
          std::to_string(source_access->block_label) + "_" +
          std::to_string(source_access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  const auto global_offset =
      static_cast<std::int32_t>(source_access->address.byte_offset);
  const auto stack_offset = static_cast<std::int32_t>(*destination_offset);
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            scratch_register,
                            3,
                            address_register,
                            global_offset));
  if (!append_rv64_prepared_store_register_to_stack(fragment,
                                                   scratch_register,
                                                   stack_offset,
                                                   8)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            scratch_register,
                            3,
                            address_register,
                            global_offset + 8));
  if (!append_rv64_prepared_store_register_to_stack(fragment,
                                                   scratch_register,
                                                   stack_offset + 8,
                                                   8)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<std::uint32_t> gpr_register_number_for_value_name(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::ValueNameId value_name) {
  if (lookups == nullptr || value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  return gpr_register_number_for_home(*home_it->second);
}

std::uint32_t rv64_temporary_gpr_avoiding(std::uint32_t reserved_register) {
  constexpr std::uint32_t t1 = 6;
  constexpr std::uint32_t t2 = 7;
  return reserved_register == t1 ? t2 : t1;
}

std::optional<std::uint32_t> rv64_unoccupied_temporary_gpr(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if (lookups == nullptr) {
    return std::nullopt;
  }
  constexpr std::array<std::uint32_t, 3> candidates = {6, 7, 28};  // t1, t2, t3
  for (const auto candidate : candidates) {
    bool occupied = false;
    for (const auto& home_entry : lookups->value_homes.homes_by_id) {
      const auto* home = home_entry.second;
      if (home == nullptr) {
        continue;
      }
      const auto home_register = gpr_register_number_for_home(*home);
      if (home_register.has_value() && *home_register == candidate) {
        occupied = true;
        break;
      }
    }
    if (!occupied) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<std::uint32_t> rv64_unoccupied_temporary_gpr_avoiding(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::uint32_t reserved_register) {
  const auto candidate = rv64_unoccupied_temporary_gpr(lookups);
  if (candidate.has_value() && *candidate != reserved_register) {
    return candidate;
  }
  constexpr std::array<std::uint32_t, 3> candidates = {6, 7, 28};  // t1, t2, t3
  for (const auto fallback : candidates) {
    if (fallback == reserved_register) {
      continue;
    }
    bool occupied = false;
    if (lookups != nullptr) {
      for (const auto& home_entry : lookups->value_homes.homes_by_id) {
        const auto* home = home_entry.second;
        if (home == nullptr) {
          continue;
        }
        const auto home_register = gpr_register_number_for_home(*home);
        if (home_register.has_value() && *home_register == fallback) {
          occupied = true;
          break;
        }
      }
    }
    if (!occupied) {
      return fallback;
    }
  }
  return std::nullopt;
}

std::optional<std::uint32_t> rv64_unoccupied_temporary_gpr_avoiding(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::initializer_list<std::uint32_t> reserved_registers) {
  if (lookups == nullptr) {
    return std::nullopt;
  }
  constexpr std::array<std::uint32_t, 7> candidates = {
      5, 6, 7, 28, 29, 30, 31};  // t0-t2, t3-t6
  for (const auto candidate : candidates) {
    if (std::find(reserved_registers.begin(),
                  reserved_registers.end(),
                  candidate) != reserved_registers.end()) {
      continue;
    }
    bool occupied = false;
    for (const auto& home_entry : lookups->value_homes.homes_by_id) {
      const auto* home = home_entry.second;
      if (home == nullptr) {
        continue;
      }
      const auto home_register = gpr_register_number_for_home(*home);
      if (home_register.has_value() && *home_register == candidate) {
        occupied = true;
        break;
      }
    }
    if (!occupied) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> rv64_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type);

bool rv64_fixed_integer_type(c4c::backend::bir::TypeKind type);

bool rv64_floating_type(c4c::backend::bir::TypeKind type);

std::optional<std::size_t> rv64_formal_entry_home_store_size(
    const c4c::backend::bir::Param& param) {
  if (param.is_sret && param.type == c4c::backend::bir::TypeKind::Ptr) {
    return std::size_t{8};
  }
  return rv64_scalar_memory_size_for_type(param.type);
}

std::optional<std::uint32_t> rv64_gpr_formal_argument_register_number(
    const c4c::backend::bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  std::size_t gpr_index = 0;
  for (std::size_t index = 0; index <= param_index; ++index) {
    const auto& param = function.params[index];
    if (!param.abi.has_value() || !param.abi->passed_in_register ||
        param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer) {
      continue;
    }
    if (index == param_index) {
      break;
    }
    ++gpr_index;
  }
  if (gpr_index >= 8) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(10 + gpr_index);
}

std::optional<std::size_t> prepared_stack_slot_home_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  return rv64_prepared_stack_slot_home_absolute_offset(
      stack_layout, home, stack_frame_bytes, size_bytes);
}

std::optional<std::int32_t> prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  return rv64_prepared_stack_slot_home_offset(
      stack_layout, home, stack_frame_bytes, size_bytes);
}

std::optional<std::uint32_t> rv64_load_store_funct3_for_size(std::size_t size_bytes) {
  return rv64_prepared_load_store_funct3_for_size(size_bytes);
}

bool append_rv64_store_register_to_stack(RiscvEncodedFragment& fragment,
                                         std::uint32_t source_register,
                                         std::int32_t offset,
                                         std::size_t size_bytes) {
  return append_rv64_prepared_store_register_to_stack(
      fragment, source_register, offset, size_bytes);
}

bool append_rv64_store_register_to_base(RiscvEncodedFragment& fragment,
                                        std::uint32_t source_register,
                                        std::uint32_t base_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            *funct3,
                            base_register,
                            source_register,
                            offset));
  return true;
}

bool append_rv64_load_stack_to_register(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes) {
  return append_rv64_prepared_load_stack_to_register(
      fragment, destination_register, offset, size_bytes);
}

bool append_rv64_load_base_to_register(RiscvEncodedFragment& fragment,
                                       std::uint32_t destination_register,
                                       std::uint32_t base_register,
                                       std::int32_t offset,
                                       std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            destination_register,
                            *funct3,
                            base_register,
                            offset));
  return true;
}

bool append_rv64_store_fpr_to_stack(RiscvEncodedFragment& fragment,
                                    std::uint32_t source_register,
                                    std::int32_t offset,
                                    c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  append_le32(fragment.bytes,
              encode_s_type(0x27, funct3, 2, source_register, offset));
  return true;
}

bool append_rv64_store_fpr_to_base(RiscvEncodedFragment& fragment,
                                   std::uint32_t source_register,
                                   std::uint32_t base_register,
                                   std::int32_t offset,
                                   c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  append_le32(fragment.bytes,
              encode_s_type(0x27,
                            funct3,
                            base_register,
                            source_register,
                            offset));
  return true;
}

bool append_rv64_load_stack_to_fpr(RiscvEncodedFragment& fragment,
                                   std::uint32_t destination_register,
                                   std::int32_t offset,
                                   c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  append_le32(fragment.bytes,
              encode_i_type(0x07, destination_register, funct3, 2, offset));
  return true;
}

bool append_rv64_load_base_to_fpr(RiscvEncodedFragment& fragment,
                                  std::uint32_t destination_register,
                                  std::uint32_t base_register,
                                  std::int32_t offset,
                                  c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  append_le32(fragment.bytes,
              encode_i_type(0x07,
                            destination_register,
                            funct3,
                            base_register,
                            offset));
  return true;
}

void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source) {
  append_rv64_prepared_move(fragment, destination, source);
}

void append_rv64_add_registers(RiscvEncodedFragment& fragment,
                               std::uint32_t destination,
                               std::uint32_t lhs,
                               std::uint32_t rhs) {
  append_rv64_prepared_add_registers(fragment, destination, lhs, rhs);
}

void append_rv64_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate) {
  append_rv64_prepared_load_immediate(fragment, destination, immediate);
}

bool append_rv64_stack_pointer_adjustment(RiscvEncodedFragment& fragment,
                                          std::int64_t byte_delta) {
  return append_rv64_prepared_stack_pointer_adjustment(fragment, byte_delta);
}

bool append_rv64_stack_offset_address_to_register(RiscvEncodedFragment& fragment,
                                                  std::uint32_t destination,
                                                  std::size_t offset) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              destination,
                              0,
                              2,
                              static_cast<std::int32_t>(
                                  signed_offset)));  // addi rd, sp, off
    return true;
  }
  append_rv64_load_immediate(fragment, destination, signed_offset);
  append_rv64_add_registers(fragment, destination, 2, destination);
  return true;
}

bool append_rv64_store_register_to_stack_offset(RiscvEncodedFragment& fragment,
                                                std::uint32_t source_register,
                                                std::size_t offset,
                                                std::size_t size_bytes) {
  return append_rv64_prepared_store_register_to_stack_offset(
      fragment, source_register, offset, size_bytes);
}

bool append_rv64_load_stack_offset_to_register(RiscvEncodedFragment& fragment,
                                               std::uint32_t destination_register,
                                               std::size_t offset,
                                               std::size_t size_bytes) {
  return append_rv64_prepared_load_stack_offset_to_register(
      fragment, destination_register, offset, size_bytes);
}

bool append_rv64_store_fpr_to_stack_offset(RiscvEncodedFragment& fragment,
                                           std::uint32_t source_register,
                                           std::size_t offset,
                                           c4c::backend::bir::TypeKind type) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_store_fpr_to_stack(
        fragment,
        source_register,
        static_cast<std::int32_t>(signed_offset),
        type);
  }
  append_rv64_load_immediate(
      fragment, kRv64StackFrameScratchRegister, signed_offset);
  append_rv64_add_registers(fragment,
                            kRv64StackFrameScratchRegister,
                            2,
                            kRv64StackFrameScratchRegister);
  return append_rv64_store_fpr_to_base(
      fragment, source_register, kRv64StackFrameScratchRegister, 0, type);
}

bool append_rv64_load_stack_offset_to_fpr(RiscvEncodedFragment& fragment,
                                          std::uint32_t destination_register,
                                          std::size_t offset,
                                          c4c::backend::bir::TypeKind type) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_load_stack_to_fpr(
        fragment,
        destination_register,
        static_cast<std::int32_t>(signed_offset),
        type);
  }
  append_rv64_load_immediate(
      fragment, kRv64StackFrameScratchRegister, signed_offset);
  append_rv64_add_registers(fragment,
                            kRv64StackFrameScratchRegister,
                            2,
                            kRv64StackFrameScratchRegister);
  return append_rv64_load_base_to_fpr(
      fragment, destination_register, kRv64StackFrameScratchRegister, 0, type);
}

std::optional<RiscvEncodedFragment>
fragment_for_rv64_variadic_incoming_gpr_publications(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedVariadicEntryPlanFunction* entry_plan,
    std::size_t stack_frame_bytes) {
  if (entry_plan == nullptr ||
      entry_plan->helper_resources.required_helpers.empty()) {
    return RiscvEncodedFragment{};
  }
  if (auto diagnostic = rv64_variadic_incoming_gpr_publications_diagnostic(
          stack_layout,
          *entry_plan,
          stack_frame_bytes)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  for (const auto& publication :
       entry_plan->rv64_incoming_variadic_gpr_publications) {
    const auto source_register =
        rv64_register_number(publication.source_register_name);
    if (!source_register.has_value() ||
        !append_rv64_store_register_to_stack(
            fragment,
            *source_register,
            static_cast<std::int32_t>(
                publication.destination_stack_offset_bytes),
            publication.size_bytes)) {
      return std::nullopt;
    }
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_variadic_va_start(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::CallInst& call,
    std::size_t stack_frame_bytes) {
  const auto helper = prepare::prepared_variadic_entry_helper_kind_for_call(call);
  if (!helper.has_value() ||
      *helper != prepare::PreparedVariadicEntryHelperKind::VaStart) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr) {
    return std::nullopt;
  }
  const auto* homes = prepare::find_prepared_variadic_entry_helper_operand_homes(
      *entry_plan,
      block_index,
      instruction_index);
  const auto* payload =
      homes != nullptr ? prepare::find_prepared_variadic_va_start_operand_homes(*homes)
                       : nullptr;
  if (homes == nullptr || homes->helper != *helper ||
      payload == nullptr ||
      rv64_variadic_va_start_materialization_diagnostic(prepared.stack_layout,
                                                        *entry_plan,
                                                        *homes,
                                                        stack_frame_bytes)) {
    return std::nullopt;
  }

  const auto* overflow_field =
      rv64_variadic_va_list_overflow_arg_area_field(*entry_plan);
  const auto destination =
      gpr_register_number_for_home(payload->destination_va_list_address);
  const auto destination_offset =
      prepared_stack_slot_home_offset(prepared.stack_layout,
                                      payload->destination_va_list,
                                      stack_frame_bytes,
                                      *entry_plan->va_list_layout.size_bytes);
  const auto destination_register =
      destination.value_or(kRv64VaStartDestinationScratch);
  if (overflow_field == nullptr || !destination_offset.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13, destination_register, 0, 2, *destination_offset));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            kRv64VaStartOverflowAreaScratch,
                            0,
                            2,
                            static_cast<std::int32_t>(
                                *entry_plan->overflow_area.base_stack_offset_bytes)));
  if (!append_rv64_store_register_to_base(
          fragment,
          kRv64VaStartOverflowAreaScratch,
          destination_register,
          static_cast<std::int32_t>(overflow_field->offset_bytes),
          overflow_field->size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_variadic_va_arg_aggregate(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::CallInst& call,
    std::size_t stack_frame_bytes) {
  const auto helper = prepare::prepared_variadic_entry_helper_kind_for_call(call);
  if (!helper.has_value() ||
      *helper != prepare::PreparedVariadicEntryHelperKind::VaArgAggregate) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr) {
    return std::nullopt;
  }
  const auto* homes = prepare::find_prepared_variadic_entry_helper_operand_homes(
      *entry_plan,
      block_index,
      instruction_index);
  const auto* payload =
      homes != nullptr
          ? prepare::find_prepared_variadic_aggregate_va_arg_operand_homes(*homes)
          : nullptr;
  if (homes == nullptr || homes->helper != *helper ||
      payload == nullptr ||
      rv64_variadic_va_arg_aggregate_materialization_diagnostic(
          prepared.stack_layout,
          *entry_plan,
          *homes,
          stack_frame_bytes)) {
    return std::nullopt;
  }

  const auto* overflow_field =
      rv64_variadic_va_list_overflow_arg_area_field(*entry_plan);
  const auto va_list_base = gpr_register_number_for_home(payload->source_va_list);
  if (overflow_field == nullptr || !va_list_base.has_value()) {
    return std::nullopt;
  }
  const auto& plan = payload->aggregate_access_plan;
  const auto& write_address = *plan.payload_write_address;

  constexpr std::array<std::uint32_t, 3> scratch_candidates = {6, 7, 28};
  std::optional<std::uint32_t> overflow_pointer;
  std::optional<std::uint32_t> copy_scratch;
  for (const auto candidate : scratch_candidates) {
    if (candidate == *va_list_base) {
      continue;
    }
    if (!overflow_pointer.has_value()) {
      overflow_pointer = candidate;
      continue;
    }
    copy_scratch = candidate;
    break;
  }
  if (!overflow_pointer.has_value() || !copy_scratch.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_load_base_to_register(
          fragment,
          *overflow_pointer,
          *va_list_base,
          static_cast<std::int32_t>(overflow_field->offset_bytes),
          overflow_field->size_bytes)) {
    return std::nullopt;
  }

  std::size_t byte_offset = 0;
  while (byte_offset < *plan.copy_size_bytes) {
    std::size_t width = 1;
    const std::size_t remaining = *plan.copy_size_bytes - byte_offset;
    const std::size_t source_offset =
        *plan.source_payload_offset_bytes + byte_offset;
    const std::size_t destination_offset =
        write_address.stack_offset_bytes + byte_offset;
    if (remaining >= 8 && source_offset % 8 == 0 &&
        destination_offset % 8 == 0) {
      width = 8;
    } else if (remaining >= 4 && source_offset % 4 == 0 &&
               destination_offset % 4 == 0) {
      width = 4;
    } else if (remaining >= 2 && source_offset % 2 == 0 &&
               destination_offset % 2 == 0) {
      width = 2;
    }
    if (source_offset >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
        destination_offset >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
        !append_rv64_load_base_to_register(fragment,
                                           *copy_scratch,
                                           *overflow_pointer,
                                           static_cast<std::int32_t>(source_offset),
                                           width) ||
        !append_rv64_store_register_to_stack(
            fragment,
            *copy_scratch,
            static_cast<std::int32_t>(destination_offset),
            width)) {
      return std::nullopt;
    }
    byte_offset += width;
  }

  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            *overflow_pointer,
                            0,
                            *overflow_pointer,
                            static_cast<std::int32_t>(
                                *plan.progression_stride_bytes)));
  if (!append_rv64_store_register_to_base(
          fragment,
          *overflow_pointer,
          *va_list_base,
          static_cast<std::int32_t>(overflow_field->offset_bytes),
          overflow_field->size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_variadic_va_end(
    const c4c::backend::prepare::PreparedCallPlan* call_plan,
    const c4c::backend::bir::CallInst& call) {
  namespace prepare = c4c::backend::prepare;

  if (call.callee != "llvm.va_end.p0" || call_plan == nullptr ||
      call.is_indirect || call.callee_value.has_value() ||
      call_plan->is_indirect || call_plan->indirect_callee.has_value() ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value() ||
      call_plan->wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      !call_plan->direct_callee_name.has_value() ||
      *call_plan->direct_callee_name != call.callee ||
      call.return_type != c4c::backend::bir::TypeKind::Void ||
      call.args.size() != 1 || call.arg_types.size() != 1 ||
      call.arg_types[0] != c4c::backend::bir::TypeKind::Ptr ||
      call.args[0].type != c4c::backend::bir::TypeKind::Ptr ||
      call_plan->arguments.size() != 1) {
    return std::nullopt;
  }
  return RiscvEncodedFragment{};
}

bool is_rv64_variadic_va_end_call(
    const c4c::backend::bir::CallInst& call) {
  return call.callee == "llvm.va_end.p0";
}

const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for_id(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::prepare::PreparedValueId value_id) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->value_homes.homes_by_id.find(value_id);
  return it == lookups->value_homes.homes_by_id.end() ? nullptr : it->second;
}

bool prepared_bir_value_has_name(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Value& value,
    c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return false;
  }
  return value.kind == c4c::backend::bir::Value::Kind::Named &&
         !value.name.empty() && names.value_names.find(value.name) == value_name;
}

bool prepared_bir_values_have_same_name(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  if (lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      rhs.kind != c4c::backend::bir::Value::Kind::Named ||
      lhs.name.empty() || rhs.name.empty()) {
    return false;
  }
  const auto lhs_name = names.value_names.find(lhs.name);
  const auto rhs_name = names.value_names.find(rhs.name);
  if (lhs_name != c4c::kInvalidValueName &&
      rhs_name != c4c::kInvalidValueName) {
    return lhs_name == rhs_name;
  }
  return lhs.name == rhs.name;
}

std::optional<c4c::backend::bir::TypeKind> prepared_bir_value_type_for_name(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (names.value_names.find(param.name) == value_name) {
      return param.type;
    }
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (prepared_bir_value_has_name(names, binary->result, value_name)) {
          return binary->result.type;
        }
      } else if (const auto* select =
                     std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
        if (prepared_bir_value_has_name(names, select->result, value_name)) {
          return select->result.type;
        }
      } else if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
        if (prepared_bir_value_has_name(names, cast->result, value_name)) {
          return cast->result.type;
        }
      } else if (const auto* phi = std::get_if<c4c::backend::bir::PhiInst>(&inst)) {
        if (prepared_bir_value_has_name(names, phi->result, value_name)) {
          return phi->result.type;
        }
      } else if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
        if (call->result.has_value() &&
            prepared_bir_value_has_name(names, *call->result, value_name)) {
          return call->result->type;
        }
        for (const auto& lane : call->result_lanes) {
          if (prepared_bir_value_has_name(names, lane, value_name)) {
            return lane.type;
          }
        }
      } else if (const auto* load =
                     std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
        if (prepared_bir_value_has_name(names, load->result, value_name)) {
          return load->result.type;
        }
      } else if (const auto* load =
                     std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst)) {
        if (prepared_bir_value_has_name(names, load->result, value_name)) {
          return load->result.type;
        }
      }
    }
    if (block.terminator.value.has_value() &&
        prepared_bir_value_has_name(names, *block.terminator.value, value_name)) {
      return block.terminator.value->type;
    }
    for (const auto& lane : block.terminator.return_lanes) {
      if (prepared_bir_value_has_name(names, lane, value_name)) {
        return lane.type;
      }
    }
  }
  return std::nullopt;
}

std::optional<prepare::PreparedValueId> prepared_value_id_for_named_value(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const bir::Value& value);

bool prepared_join_transfer_edge_copies_are_published(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer);

const c4c::backend::bir::Block* find_prepared_bir_block_by_prepared_label(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label);

const c4c::backend::bir::BinaryInst* find_prepared_binary_producer_in_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Value& value);

const c4c::backend::bir::BinaryInst*
find_prepared_fp_zero_compare_consumer_in_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block,
    c4c::backend::bir::TypeKind source_type);

std::optional<RiscvEncodedFragment> fragment_for_prepared_fp_compare_publication(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary);

std::optional<std::uint32_t> rv64_fp_compare_funct7(
    c4c::backend::bir::TypeKind type);

bool is_rv64_zero_floating_immediate(const c4c::backend::bir::Value& value);

const c4c::backend::prepare::PreparedParallelCopyBundle*
find_prepared_parallel_copy_bundle_for_edge(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label);

struct PreparedSelectEdgeSourceProducerFragment {
  bool matched = false;
  std::optional<RiscvEncodedFragment> fragment;
};

PreparedSelectEdgeSourceProducerFragment
fragment_for_prepared_block_entry_select_edge_source_producer(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes);

PreparedSelectEdgeSourceProducerFragment fragment_for_prepared_select_edge_source_producer(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes);

bool prepared_move_bundle_is_authorized_cast_dependency_stack_publication(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle);

bool prepared_move_bundle_is_authorized_select_edge_source_producer_suppression(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle);

bool prepared_before_instruction_move_bundle_requires_suppression_authority(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle);

bool prepared_move_has_matching_stack_destination_register_fan_in_authority(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    const c4c::backend::prepare::PreparedStackDestinationFanInAuthorityFact*
        authority);

std::optional<RiscvEncodedFragment>
fragment_for_predecessor_select_publication_immediate_to_gpr(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle);

bool prepared_predecessor_select_publication_bundle_is_immediate_to_gpr_materialized(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle);

bool prepared_move_bundle_is_out_of_ssa_move_packet_family(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  namespace prepare = c4c::backend::prepare;

  if (move_bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
      move_bundle.authority_kind !=
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy) {
    return false;
  }
  return std::any_of(
      move_bundle.moves.begin(),
      move_bundle.moves.end(),
      [](const prepare::PreparedMoveResolution& move) {
        return move.reason == "phi_join_register_to_register" ||
               move.reason == "phi_join_immediate_materialization" ||
               move.reason.rfind("edge_consumer_preservation", 0) == 0 ||
               move.source_parallel_copy_step_index.has_value() ||
               move.uses_cycle_temp_source ||
               move.op_kind ==
                   prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
      });
}

bool is_scalar_integer_immediate(const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  switch (value.type) {
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return true;
    default:
      return false;
  }
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_edge_preserved_fp_zero_compare(
    const c4c::backend::bir::BinaryInst& binary,
    std::uint32_t source_fpr,
    std::uint32_t destination_gpr) {
  namespace bir = c4c::backend::bir;

  if (binary.result.type != bir::TypeKind::I32 ||
      binary.operand_type != binary.lhs.type ||
      binary.operand_type != binary.rhs.type ||
      (binary.opcode != bir::BinaryOpcode::Eq &&
       binary.opcode != bir::BinaryOpcode::Ne)) {
    return std::nullopt;
  }
  const auto funct7 = rv64_fp_compare_funct7(binary.operand_type);
  if (!funct7.has_value()) {
    return std::nullopt;
  }

  const bool lhs_is_zero = is_rv64_zero_floating_immediate(binary.lhs);
  const bool rhs_is_zero = is_rv64_zero_floating_immediate(binary.rhs);
  if (lhs_is_zero == rhs_is_zero) {
    return std::nullopt;
  }

  constexpr std::array<std::uint32_t, 3> scratch_fpr_candidates = {31, 30, 29};
  const auto scratch_it =
      std::find_if(scratch_fpr_candidates.begin(),
                   scratch_fpr_candidates.end(),
                   [&](std::uint32_t candidate) {
                     return candidate != source_fpr;
                   });
  if (scratch_it == scratch_fpr_candidates.end()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_gpr_to_fpr_move(fragment,
                                  *scratch_it,
                                  0,
                                  binary.operand_type)) {
    return std::nullopt;
  }
  const std::uint32_t lhs = lhs_is_zero ? *scratch_it : source_fpr;
  const std::uint32_t rhs = rhs_is_zero ? *scratch_it : source_fpr;
  append_le32(fragment.bytes,
              encode_r_type(0x53, destination_gpr, 2, lhs, rhs, *funct7));
  if (binary.opcode == bir::BinaryOpcode::Ne) {
    append_le32(fragment.bytes,
                encode_i_type(0x13, destination_gpr, 4, destination_gpr, 1));
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_out_of_ssa_moves(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    std::size_t stack_frame_bytes,
    prepare::PreparedObjectTraversalEventKind event_kind,
    const c4c::backend::prepare::PreparedParallelCopyBundle& parallel_copy_bundle,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  namespace prepare = c4c::backend::prepare;

  if (event_kind != prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies ||
      move_bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
      move_bundle.authority_kind !=
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      parallel_copy_bundle.execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      move_bundle.source_parallel_copy_predecessor_label !=
          parallel_copy_bundle.predecessor_label ||
      move_bundle.source_parallel_copy_successor_label !=
          parallel_copy_bundle.successor_label ||
      parallel_copy_bundle.has_cycle) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  std::size_t next_step_index = 0;
  for (const auto& move : move_bundle.moves) {
    const bool phi_join_move =
        move.reason == "phi_join_register_to_register";
    const bool phi_join_immediate_move =
        move.reason == "phi_join_immediate_materialization";
    const bool preservation_move =
        move.reason == "edge_consumer_preservation_register_to_register";
    const bool stack_preservation_move =
        move.reason == "edge_consumer_preservation_register_to_stack";
    if (!phi_join_move && !phi_join_immediate_move && !preservation_move &&
        !stack_preservation_move) {
      return std::nullopt;
    }
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_contiguous_width != 1 ||
        move.destination_occupied_register_names.size() > 1 ||
        (move.source_immediate_i32.has_value() != phi_join_immediate_move) ||
        move.uses_cycle_temp_source ||
        move.authority_kind !=
            prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        move.source_parallel_copy_predecessor_label !=
            parallel_copy_bundle.predecessor_label ||
        move.source_parallel_copy_successor_label !=
            parallel_copy_bundle.successor_label) {
      return std::nullopt;
    }

    if (phi_join_move) {
      if (!move.source_parallel_copy_step_index.has_value() ||
          *move.source_parallel_copy_step_index != next_step_index ||
          next_step_index >= parallel_copy_bundle.steps.size()) {
        return std::nullopt;
      }
      const auto& step = parallel_copy_bundle.steps[next_step_index];
      if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
          step.uses_cycle_temp_source) {
        return std::nullopt;
      }
      ++next_step_index;
    } else if (phi_join_immediate_move) {
      if (!move.source_parallel_copy_step_index.has_value() ||
          *move.source_parallel_copy_step_index != next_step_index ||
          next_step_index >= parallel_copy_bundle.steps.size()) {
        return std::nullopt;
      }
      const auto& step = parallel_copy_bundle.steps[next_step_index];
      if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
          step.uses_cycle_temp_source) {
        return std::nullopt;
      }
      const auto* parallel_copy_move =
          prepare::find_prepared_parallel_copy_move_for_step(parallel_copy_bundle,
                                                             step);
      if (parallel_copy_move == nullptr ||
          !is_scalar_integer_immediate(parallel_copy_move->source_value) ||
          parallel_copy_move->source_value.immediate != *move.source_immediate_i32) {
        return std::nullopt;
      }
      ++next_step_index;
    } else if (move.source_parallel_copy_step_index.has_value()) {
      return std::nullopt;
    }

    const auto* destination_home =
        prepared_value_home_for_id(lookups, move.to_value_id);
    if (destination_home == nullptr) {
      return std::nullopt;
    }
    if (stack_preservation_move) {
      const auto* source_home =
          prepared_value_home_for_id(lookups, move.from_value_id);
      if (source_home == nullptr) {
        return std::nullopt;
      }
      const auto source = gpr_register_number_for_home(*source_home);
      if (!source.has_value()) {
        return std::nullopt;
      }
      if (move.destination_storage_kind !=
              prepare::PreparedMoveStorageKind::StackSlot ||
          move.destination_stack_offset_bytes.has_value()) {
        return std::nullopt;
      }
      const auto destination_type = prepared_bir_value_type_for_name(
          names, function, destination_home->value_name);
      const auto destination_size_bytes =
          destination_type.has_value()
              ? rv64_scalar_memory_size_for_type(*destination_type)
              : destination_home->size_bytes;
      if (!destination_size_bytes.has_value()) {
        return std::nullopt;
      }
      const auto stack_offset = prepared_stack_slot_home_absolute_offset(
          stack_layout, *destination_home, stack_frame_bytes,
          *destination_size_bytes);
      if (!stack_offset.has_value() ||
          !append_rv64_store_register_to_stack_offset(fragment,
                                                     *source,
                                                     *stack_offset,
                                                     *destination_size_bytes)) {
        return std::nullopt;
      }
      continue;
    }

    if (move.destination_storage_kind !=
            prepare::PreparedMoveStorageKind::Register ||
        move.destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const auto destination = gpr_register_number_for_home(*destination_home);
    std::optional<std::uint32_t> fpr_destination;
    if (!destination.has_value() &&
        move.destination_register_placement.has_value() &&
        move.destination_register_placement->bank ==
            prepare::PreparedRegisterBank::Fpr &&
        move.destination_register_placement->contiguous_width == 1 &&
        destination_home->register_name.has_value()) {
      fpr_destination = rv64_fpr_register_number(*destination_home->register_name);
    }
    if (!destination.has_value() && !fpr_destination.has_value()) {
      return std::nullopt;
    }
    if (phi_join_immediate_move) {
      if (!destination.has_value()) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment,
                                 *destination,
                                 *move.source_immediate_i32);
      continue;
    }
    const auto* source_home =
        prepared_value_home_for_id(lookups, move.from_value_id);
    if (phi_join_move && destination.has_value() && source_home != nullptr &&
        source_home->kind == prepare::PreparedValueHomeKind::Register) {
      const auto source = gpr_register_number_for_home(*source_home);
      if (!source.has_value()) {
        return std::nullopt;
      }
      append_rv64_move(fragment, *destination, *source);
      continue;
    }
    if (phi_join_move && destination.has_value()) {
      auto producer_fragment =
          fragment_for_prepared_block_entry_select_edge_source_producer(
              stack_layout,
              names,
              control_flow,
              function,
              lookups,
              dependency_operand_authorities,
              carrier_alias_authorities,
              move_bundle,
              move,
              *destination,
              stack_frame_bytes);
      if (producer_fragment.matched) {
        if (!producer_fragment.fragment.has_value()) {
          return std::nullopt;
        }
        append_fragment(fragment, std::move(*producer_fragment.fragment));
        continue;
      }
    }
    if (source_home == nullptr) {
      return std::nullopt;
    }
    if (fpr_destination.has_value()) {
      const auto source =
          source_home->register_name.has_value()
              ? rv64_fpr_register_number(*source_home->register_name)
              : fpr_register_number_for_home(*source_home);
      const auto type =
          prepared_bir_value_type_for_name(names, function, source_home->value_name);
      if (!source.has_value() || !type.has_value() ||
          !append_rv64_fpr_move(fragment, *fpr_destination, *source, *type)) {
        return std::nullopt;
      }
      continue;
    }
    if (phi_join_move &&
        source_home->kind ==
            prepare::PreparedValueHomeKind::RematerializableImmediate) {
      const auto report =
          prepare::verify_prepared_rematerializable_integer_immediate_contract(
              source_home);
      const auto fact =
          prepare::as_rematerializable_integer_immediate_fact(*source_home);
      if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent ||
          !fact.has_value()) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment, *destination, fact->signed_value);
      continue;
    }
    if (preservation_move) {
      const auto source_fpr =
          source_home->register_name.has_value()
              ? rv64_fpr_register_number(*source_home->register_name)
              : fpr_register_number_for_home(*source_home);
      const auto source_type =
          prepared_bir_value_type_for_name(names, function, source_home->value_name);
      const auto destination_type =
          prepared_bir_value_type_for_name(names,
                                           function,
                                           destination_home->value_name);
      if (source_fpr.has_value() && source_type.has_value() &&
          destination_type == c4c::backend::bir::TypeKind::I32 &&
          (*source_type == c4c::backend::bir::TypeKind::F32 ||
           *source_type == c4c::backend::bir::TypeKind::F64) &&
          move_bundle.source_parallel_copy_successor_label.has_value()) {
        const auto* successor_block = find_prepared_bir_block_by_prepared_label(
            names, function, *move_bundle.source_parallel_copy_successor_label);
        if ((successor_block == nullptr || successor_block->insts.empty()) &&
            move_bundle.block_index < function.blocks.size()) {
          const auto& predecessor_block =
              function.blocks.at(move_bundle.block_index);
          if (predecessor_block.terminator.kind ==
                  c4c::backend::bir::TerminatorKind::Branch &&
              !predecessor_block.terminator.target_label.empty()) {
            const auto successor_it =
                std::find_if(function.blocks.begin(),
                             function.blocks.end(),
                             [&](const c4c::backend::bir::Block& block) {
                               return block.label ==
                                      predecessor_block.terminator.target_label;
                             });
            if (successor_it != function.blocks.end()) {
              successor_block = &*successor_it;
            }
          }
        }
        const auto* binary =
            successor_block == nullptr
                ? nullptr
                : find_prepared_fp_zero_compare_consumer_in_block(
                      names,
                      *successor_block,
                      *source_type);
        if (binary == nullptr) {
          for (const auto& block : function.blocks) {
            const auto* candidate =
                find_prepared_fp_zero_compare_consumer_in_block(names,
                                                                block,
                                                                *source_type);
            if (candidate == nullptr) {
              continue;
            }
            if (binary != nullptr) {
              binary = nullptr;
              break;
            }
            binary = candidate;
          }
        }
        std::optional<c4c::backend::bir::BinaryInst> branch_condition_binary;
        if (binary == nullptr) {
          const c4c::backend::prepare::PreparedBranchCondition* selected = nullptr;
          for (const auto& branch_condition : control_flow.branch_conditions) {
            if (branch_condition.kind !=
                    c4c::backend::prepare::PreparedBranchConditionKind::
                        FusedCompare ||
                !branch_condition.predicate.has_value() ||
                !branch_condition.compare_type.has_value() ||
                !branch_condition.lhs.has_value() ||
                !branch_condition.rhs.has_value() ||
                *branch_condition.compare_type != *source_type ||
                branch_condition.condition_value.type !=
                    c4c::backend::bir::TypeKind::I32 ||
                (*branch_condition.predicate !=
                     c4c::backend::bir::BinaryOpcode::Eq &&
                 *branch_condition.predicate !=
                     c4c::backend::bir::BinaryOpcode::Ne)) {
              continue;
            }
            const bool lhs_is_zero =
                is_rv64_zero_floating_immediate(*branch_condition.lhs);
            const bool rhs_is_zero =
                is_rv64_zero_floating_immediate(*branch_condition.rhs);
            if (lhs_is_zero == rhs_is_zero) {
              continue;
            }
            if (selected != nullptr) {
              selected = nullptr;
              break;
            }
            selected = &branch_condition;
          }
          if (selected != nullptr) {
            branch_condition_binary = c4c::backend::bir::BinaryInst{
                .opcode = *selected->predicate,
                .result = selected->condition_value,
                .operand_type = *selected->compare_type,
                .lhs = *selected->lhs,
                .rhs = *selected->rhs,
            };
            binary = &*branch_condition_binary;
          }
        }
        if (binary != nullptr &&
            binary->result.type == *destination_type) {
          auto compare_fragment =
              fragment_for_prepared_edge_preserved_fp_zero_compare(
                  *binary,
                  *source_fpr,
                  *destination);
          if (!compare_fragment.has_value()) {
            return std::nullopt;
          }
          append_fragment(fragment, std::move(*compare_fragment));
          continue;
        }
      }
    }
    const auto source = gpr_register_number_for_home(*source_home);
    if (!source.has_value()) {
      return std::nullopt;
    }
    append_rv64_move(fragment, *destination, *source);
  }
  if (next_step_index != parallel_copy_bundle.steps.size()) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_before_return_stack_to_register_abi_move(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::size_t stack_frame_bytes,
    prepare::PreparedObjectTraversalEventKind event_kind,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (event_kind != prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies ||
      move_bundle.moves.size() != 1) {
    return std::nullopt;
  }
  const auto& move = move_bundle.moves.front();
  if (!prepared_move_is_before_return_stack_to_register_abi_move(move_bundle,
                                                                  move)) {
    return std::nullopt;
  }
  if (move.function_return_authority_kind !=
          prepare::PreparedMoveAuthorityKind::FunctionReturnDestinationHome ||
      move.authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      move.block_index != move_bundle.block_index ||
      move.instruction_index != move_bundle.instruction_index ||
      !move.destination_target_register_identity.has_value() ||
      move.destination_occupied_register_names.size() != 1 ||
      move.destination_occupied_register_names.front() !=
          *move.destination_register_name) {
    return std::nullopt;
  }
  const auto placement_identity =
      prepare::target_register_identity_for_abi_register_placement(
          target_profile,
          *move.destination_register_placement);
  const auto placement_register = placement_identity.has_value()
                                      ? gpr_register_number_for_target_identity(
                                            *placement_identity)
                                      : std::optional<std::uint32_t>{};
  const auto identity_register =
      gpr_register_number_for_target_identity(*move.destination_target_register_identity);
  const auto named_register = rv64_register_number(*move.destination_register_name);
  if (!placement_identity.has_value() || !placement_register.has_value() ||
      !identity_register.has_value() || !named_register.has_value() ||
      *placement_register != *identity_register ||
      *placement_register != *named_register) {
    return std::nullopt;
  }
  const auto* source_home = prepared_value_home_for_id(lookups, move.from_value_id);
  if (source_home == nullptr ||
      source_home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  const auto source_type =
      prepared_bir_value_type_for_name(names, function, source_home->value_name);
  if (!source_type.has_value()) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(*source_type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto stack_offset =
      prepared_stack_slot_home_absolute_offset(stack_layout,
                                               *source_home,
                                               stack_frame_bytes,
                                               *size_bytes);
  RiscvEncodedFragment fragment;
  if (!stack_offset.has_value() ||
      !append_rv64_load_stack_offset_to_register(fragment,
                                                *identity_register,
                                                *stack_offset,
                                                *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_authorized_pointer_return_already_loaded(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::bir::Terminator& terminator,
    std::size_t block_index,
    const std::unordered_set<PreparedBeforeReturnStackToRegisterKey,
                             PreparedBeforeReturnStackToRegisterKeyHash>*
        prepared_before_return_stack_to_register_values,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.value.has_value() ||
      terminator.value->type != c4c::backend::bir::TypeKind::Ptr ||
      terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      terminator.value->name.empty() || lookups == nullptr ||
      prepared_before_return_stack_to_register_values == nullptr) {
    return std::nullopt;
  }
  const auto terminator_value_name =
      names.value_names.find(terminator.value->name);
  const auto terminator_value_id =
      terminator_value_name == c4c::kInvalidValueName
          ? lookups->value_homes.value_ids.end()
          : lookups->value_homes.value_ids.find(terminator_value_name);
  if (terminator_value_name == c4c::kInvalidValueName ||
      terminator_value_id == lookups->value_homes.value_ids.end() ||
      prepared_before_return_stack_to_register_values->count(
          PreparedBeforeReturnStackToRegisterKey{
              .block_index = block_index,
              .value_id = terminator_value_id->second,
          }) == 0) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  if (restore_return_address) {
    if (!append_rv64_prepared_call_frame_epilogue(
            fragment,
            frame_plan,
            stack_frame_bytes)) {
      return std::nullopt;
    }
  } else if (!append_rv64_prepared_stack_frame_epilogue(fragment,
                                                       frame_plan,
                                                       stack_frame_bytes)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_stack_slot_to_stack_slot_move(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan,
    std::size_t stack_frame_bytes,
    const c4c::backend::prepare::PreparedParallelCopyBundle* parallel_copy_bundle,
    const c4c::backend::prepare::PreparedStackDestinationFanInAuthorityFact*
        stack_destination_fan_in_authority,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    const c4c::backend::prepare::PreparedValueHome& source_home,
    const c4c::backend::prepare::PreparedValueHome& destination_home);

const prepare::PreparedStoragePlanValue* prepared_storage_plan_value_for_id(
    const prepare::PreparedStoragePlanFunction* storage_plan,
    prepare::PreparedValueId value_id) {
  if (storage_plan == nullptr) {
    return nullptr;
  }
  for (const auto& value : storage_plan->values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

bool prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot(
    const prepare::PreparedStoragePlanFunction* storage_plan,
    prepare::PreparedValueId value_id) {
  const auto* value = prepared_storage_plan_value_for_id(storage_plan, value_id);
  if (value == nullptr) {
    return true;
  }
  return value->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
         value->bank == prepare::PreparedRegisterBank::Gpr &&
         value->contiguous_width == 1 && value->slot_id.has_value() &&
         value->stack_offset_bytes.has_value();
}

bool prepared_storage_plan_endpoint_is_coherent_gpr_register(
    const prepare::PreparedStoragePlanFunction* storage_plan,
    prepare::PreparedValueId value_id,
    std::uint32_t expected_register) {
  const auto* value = prepared_storage_plan_value_for_id(storage_plan, value_id);
  if (value == nullptr) {
    return true;
  }
  if (value->encoding != prepare::PreparedStorageEncodingKind::Register ||
      value->bank != prepare::PreparedRegisterBank::Gpr ||
      value->contiguous_width != 1) {
    return false;
  }
  if (value->register_name.has_value()) {
    const auto register_number = rv64_register_number(*value->register_name);
    if (!register_number.has_value() ||
        *register_number != expected_register) {
      return false;
    }
  }
  return true;
}

std::optional<c4c::backend::prepare::PreparedTargetRegisterIdentity>
rv64_gpr_target_identity_for_placement(
    const c4c::TargetProfile& target_profile,
    const prepare::PreparedRegisterPlacement& placement) {
  if (target_profile.arch != c4c::TargetArch::Riscv64 ||
      placement.bank != prepare::PreparedRegisterBank::Gpr ||
      placement.contiguous_width != 1) {
    return std::nullopt;
  }
  if (placement.pool == prepare::PreparedRegisterSlotPool::CallArgument ||
      placement.pool == prepare::PreparedRegisterSlotPool::CallResult) {
    return prepare::target_register_identity_for_abi_register_placement(
        target_profile,
        placement);
  }

  std::optional<std::size_t> physical_index;
  switch (placement.pool) {
    case prepare::PreparedRegisterSlotPool::CallerSaved:
      if (placement.slot_index == 0) {
        physical_index = 5;  // t0
      }
      break;
    case prepare::PreparedRegisterSlotPool::CalleeSaved:
      if (placement.slot_index == 0) {
        physical_index = 9;  // s1
      } else if (placement.slot_index == 1) {
        physical_index = 18;  // s2
      }
      break;
    case prepare::PreparedRegisterSlotPool::None:
    case prepare::PreparedRegisterSlotPool::CallArgument:
    case prepare::PreparedRegisterSlotPool::CallResult:
    case prepare::PreparedRegisterSlotPool::ReservedScratch:
      break;
  }
  if (!physical_index.has_value()) {
    return std::nullopt;
  }
  return c4c::backend::prepare::PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Riscv64,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .physical_index = *physical_index,
  };
}

std::optional<std::uint32_t> rv64_gpr_formal_register_for_home(
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (target_profile.arch != c4c::TargetArch::Riscv64 ||
      function == nullptr ||
      home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  std::size_t gpr_index = 0;
  for (const auto& param : function->params) {
    if (!param.abi.has_value() || !param.abi->passed_in_register ||
        param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer) {
      continue;
    }
    const auto param_name = names.value_names.find(param.name);
    if (param_name == home.value_name) {
      if (gpr_index >= 8) {
        return std::nullopt;
      }
      return static_cast<std::uint32_t>(10 + gpr_index);
    }
    ++gpr_index;
  }
  return std::nullopt;
}

std::optional<std::uint32_t> explicit_gpr_register_for_home(
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function* function,
    const prepare::PreparedStoragePlanFunction* storage_plan,
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register) {
    return std::nullopt;
  }
  std::optional<c4c::backend::prepare::PreparedTargetRegisterIdentity> identity;
  if (home.target_register_identity.has_value()) {
    identity = home.target_register_identity;
  } else if (const auto* storage =
                 prepared_storage_plan_value_for_id(storage_plan, home.value_id);
             storage != nullptr &&
             storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
             storage->bank == prepare::PreparedRegisterBank::Gpr &&
             storage->contiguous_width == 1 &&
             storage->register_placement.has_value()) {
    identity = rv64_gpr_target_identity_for_placement(
        target_profile,
        *storage->register_placement);
    if (storage->register_name.has_value()) {
      const auto spelled = rv64_register_number(*storage->register_name);
      if (!spelled.has_value() ||
          (identity.has_value() && *spelled != identity->physical_index)) {
        return std::nullopt;
      }
    }
  } else if (const auto formal_register =
                 rv64_gpr_formal_register_for_home(target_profile,
                                                   names,
                                                   function,
                                                   home);
             formal_register.has_value()) {
    identity = c4c::backend::prepare::PreparedTargetRegisterIdentity{
        .target_arch = c4c::TargetArch::Riscv64,
        .bank = prepare::PreparedRegisterBank::Gpr,
        .register_class = prepare::PreparedRegisterClass::General,
        .physical_index = *formal_register,
    };
  }
  if (!identity.has_value() ||
      identity->target_arch != c4c::TargetArch::Riscv64 ||
      identity->bank != c4c::backend::prepare::PreparedRegisterBank::Gpr ||
      identity->register_class !=
          c4c::backend::prepare::PreparedRegisterClass::General ||
      identity->physical_index > 31) {
    return std::nullopt;
  }
  const auto register_number =
      static_cast<std::uint32_t>(identity->physical_index);
  if (home.register_name.has_value()) {
    const auto spelled = rv64_register_number(*home.register_name);
    if (!spelled.has_value() || *spelled != register_number) {
      return std::nullopt;
    }
  }
  return register_number;
}

std::optional<std::size_t> prepared_gpr_stack_home_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedStoragePlanFunction* storage_plan,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  if (const auto* storage =
          prepared_storage_plan_value_for_id(storage_plan, home.value_id);
      storage != nullptr) {
    if (storage->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
        storage->bank != prepare::PreparedRegisterBank::Gpr ||
        storage->contiguous_width != 1 ||
        !storage->stack_offset_bytes.has_value() ||
        (home.slot_id.has_value() && storage->slot_id.has_value() &&
         *home.slot_id != *storage->slot_id)) {
      return std::nullopt;
    }
    const auto offset = *storage->stack_offset_bytes;
    if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes) {
      return std::nullopt;
    }
    return offset;
  }
  return rv64_prepared_stack_slot_home_absolute_offset(stack_layout,
                                                      home,
                                                      stack_frame_bytes,
                                                      size_bytes);
}

bool append_rv64_move_value_to_register_with_formal_stack_home(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedValueLocationFunction* value_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Value& value,
    std::size_t incoming_stack_base_bytes,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;
  if (lookups != nullptr && value_locations != nullptr &&
      value.kind == c4c::backend::bir::Value::Kind::Named &&
      !value.name.empty()) {
    const auto size_bytes = rv64_scalar_memory_size_for_type(value.type);
    if (size_bytes.has_value()) {
      std::optional<std::size_t> selected_incoming_offset;
      for (std::size_t formal_index = 0; formal_index < function.params.size();
           ++formal_index) {
        const auto& formal = function.params[formal_index];
        if (formal.name != value.name || formal.type != value.type ||
            formal.size_bytes != *size_bytes) {
          continue;
        }
        if (!formal.abi.has_value() || !formal.abi->passed_on_stack) {
          break;
        }
        const auto plan = prepare::plan_prepared_formal_publication(
            prepare::PreparedFormalPublicationInputs{
                .names = &names,
                .function = &function,
                .value_locations = value_locations,
                .value_home_lookups = &lookups->value_homes,
            },
            formal_index);
        if (!prepare::prepared_formal_publication_available(plan) ||
            plan.action != prepare::PreparedFormalPublicationAction::IncomingStackToHome ||
            !plan.incoming_stack_offset_bytes.has_value() ||
            *plan.incoming_stack_offset_bytes >
                std::numeric_limits<std::size_t>::max() -
                    incoming_stack_base_bytes) {
          return false;
        }
        if (selected_incoming_offset.has_value()) {
          return false;
        }
        selected_incoming_offset = plan.incoming_stack_offset_bytes;
      }
      if (selected_incoming_offset.has_value()) {
        const auto incoming_absolute_offset =
            incoming_stack_base_bytes + *selected_incoming_offset;
        return append_rv64_load_stack_offset_to_register(fragment,
                                                        destination,
                                                        incoming_absolute_offset,
                                                        *size_bytes);
      }
    }
  }
  return append_rv64_move_value_to_register(fragment,
                                           destination,
                                           stack_layout,
                                           names,
                                           lookups,
                                           value,
                                           stack_frame_bytes);
}

std::optional<std::size_t> prepared_stack_slot_home_size_bytes(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  if (home.size_bytes.has_value()) {
    return home.size_bytes;
  }
  if (!home.slot_id.has_value()) {
    return std::nullopt;
  }
  const auto* slot = prepare::find_frame_slot_by_id(stack_layout, *home.slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return slot->size_bytes;
}

bool rv64_prepared_move_bundle_has_register_fan_in_stack_destination(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move_bundle.moves.size() < 2) {
    return false;
  }
  for (std::size_t lhs_index = 0; lhs_index < move_bundle.moves.size();
       ++lhs_index) {
    const auto& lhs = move_bundle.moves[lhs_index];
    if (lhs.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        lhs.destination_storage_kind !=
            prepare::PreparedMoveStorageKind::StackSlot ||
        lhs.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        lhs.uses_cycle_temp_source || lhs.source_immediate_i32.has_value()) {
      continue;
    }
    const auto* lhs_source_home =
        prepared_value_home_for_id(lookups, lhs.from_value_id);
    const auto* lhs_destination_home =
        prepared_value_home_for_id(lookups, lhs.to_value_id);
    if (lhs_source_home == nullptr || lhs_destination_home == nullptr ||
        lhs_source_home->kind != prepare::PreparedValueHomeKind::Register ||
        lhs_destination_home->kind !=
            prepare::PreparedValueHomeKind::StackSlot) {
      continue;
    }
    for (std::size_t rhs_index = lhs_index + 1;
         rhs_index < move_bundle.moves.size();
         ++rhs_index) {
      const auto& rhs = move_bundle.moves[rhs_index];
      if (rhs.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          rhs.destination_storage_kind !=
              prepare::PreparedMoveStorageKind::StackSlot ||
          rhs.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          rhs.uses_cycle_temp_source || rhs.source_immediate_i32.has_value()) {
        continue;
      }
      const auto* rhs_source_home =
          prepared_value_home_for_id(lookups, rhs.from_value_id);
      const auto* rhs_destination_home =
          prepared_value_home_for_id(lookups, rhs.to_value_id);
      if (rhs_source_home == nullptr || rhs_destination_home == nullptr ||
          rhs_source_home->kind != prepare::PreparedValueHomeKind::Register ||
          rhs_destination_home->kind !=
              prepare::PreparedValueHomeKind::StackSlot) {
        continue;
      }
      if (lhs.to_value_id == rhs.to_value_id ||
          (lhs_destination_home->slot_id.has_value() &&
           rhs_destination_home->slot_id.has_value() &&
           lhs_destination_home->slot_id == rhs_destination_home->slot_id) ||
          (lhs_destination_home->offset_bytes.has_value() &&
           rhs_destination_home->offset_bytes.has_value() &&
           lhs_destination_home->offset_bytes ==
               rhs_destination_home->offset_bytes)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_move_bundle(
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements,
    std::size_t stack_frame_bytes,
    prepare::PreparedObjectTraversalEventKind event_kind,
    const c4c::backend::prepare::PreparedParallelCopyBundle* parallel_copy_bundle,
    const c4c::backend::prepare::PreparedStackDestinationFanInAuthorityFact*
        stack_destination_fan_in_authority,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (prepared_move_bundle_is_authorized_select_edge_source_producer_suppression(
          control_flow,
          control_flow.function_name,
          select_edge_source_producer_placements,
          move_bundle)) {
    return RiscvEncodedFragment{};
  }
  if (prepared_move_bundle_is_authorized_cast_dependency_stack_publication(
          names,
          control_flow,
          control_flow.function_name,
          function,
          dependency_operand_authorities,
          move_bundle)) {
    return RiscvEncodedFragment{};
  }
  if (prepared_before_instruction_move_bundle_requires_suppression_authority(
          move_bundle)) {
    return std::nullopt;
  }
  if (move_bundle.authority_kind !=
          prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn &&
      rv64_prepared_move_bundle_has_register_fan_in_stack_destination(
          lookups, move_bundle)) {
    return std::nullopt;
  }
  if (parallel_copy_bundle != nullptr) {
    if (prepared_move_bundle_is_out_of_ssa_move_packet_family(
            move_bundle)) {
      return fragment_for_prepared_out_of_ssa_moves(
          stack_layout,
          names,
          control_flow,
          function,
          lookups,
          dependency_operand_authorities,
          carrier_alias_authorities,
          stack_frame_bytes,
          event_kind,
          *parallel_copy_bundle,
          move_bundle);
    }
  }
  if (move_bundle.phase == prepare::PreparedMovePhase::BeforeReturn &&
      move_bundle.authority_kind == prepare::PreparedMoveAuthorityKind::None) {
    if (auto fragment =
            fragment_for_prepared_before_return_stack_to_register_abi_move(
                stack_layout,
                names,
                target_profile,
                function,
                lookups,
                stack_frame_bytes,
                event_kind,
                move_bundle)) {
      return fragment;
    }
  }
  if (std::any_of(move_bundle.moves.begin(),
                  move_bundle.moves.end(),
                  [&](const prepare::PreparedMoveResolution& move) {
                    if (move.reason != "return_stack_to_register") {
                      return false;
                    }
                    const auto* source_home =
                        prepared_value_home_for_id(lookups, move.from_value_id);
                    const auto source_type =
                        source_home == nullptr
                            ? std::optional<c4c::backend::bir::TypeKind>{}
                            : prepared_bir_value_type_for_name(
                                  names, function, source_home->value_name);
                    return !(source_home != nullptr &&
                             source_home->kind ==
                                 prepare::PreparedValueHomeKind::Register &&
                             source_type == c4c::backend::bir::TypeKind::Ptr &&
                             move.destination_kind ==
                                 prepare::PreparedMoveDestinationKind::
                                     FunctionReturnAbi &&
                             move.destination_storage_kind ==
                                 prepare::PreparedMoveStorageKind::Register);
                  })) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  for (const auto& move : move_bundle.moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.uses_cycle_temp_source || move.destination_contiguous_width != 1 ||
        move.destination_occupied_register_names.size() > 1 ||
        move.destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }

    if (move.destination_storage_kind ==
        prepare::PreparedMoveStorageKind::StackSlot) {
      const bool stack_destination_register_fan_in_authorized =
          prepared_move_has_matching_stack_destination_register_fan_in_authority(
              move_bundle, move, stack_destination_fan_in_authority);
      if (move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
          (move_bundle.authority_kind != prepare::PreparedMoveAuthorityKind::None &&
           move_bundle.authority_kind !=
               prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion &&
           move_bundle.authority_kind !=
               prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn) ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          (move.reason != "consumer_register_to_stack" &&
           move.reason != "consumer_stack_to_stack") ||
          move.source_immediate_i32.has_value()) {
        return std::nullopt;
      }

      const auto* source_home =
          prepared_value_home_for_id(lookups, move.from_value_id);
      const auto* destination_home =
          prepared_value_home_for_id(lookups, move.to_value_id);
      if (source_home == nullptr || destination_home == nullptr) {
        return std::nullopt;
      }
      const auto destination_type = prepared_bir_value_type_for_name(
          names, function, destination_home->value_name);
      if (!destination_type.has_value()) {
        return std::nullopt;
      }
      const auto destination_size_bytes =
          rv64_scalar_memory_size_for_type(*destination_type);
      if (!destination_size_bytes.has_value()) {
        return std::nullopt;
      }
      const auto stack_offset = prepared_stack_slot_home_absolute_offset(
          stack_layout,
          *destination_home,
          stack_frame_bytes,
          *destination_size_bytes);
      if (!stack_offset.has_value()) {
        return std::nullopt;
      }
      if (move.reason == "consumer_register_to_stack" &&
          source_home->kind == prepare::PreparedValueHomeKind::Register) {
        if ((!stack_destination_register_fan_in_authorized &&
             move.authority_kind != prepare::PreparedMoveAuthorityKind::None) ||
            move.destination_register_name.has_value() ||
            !move.destination_occupied_register_names.empty() ||
            move.destination_register_placement.has_value() ||
            move.source_parallel_copy_step_index.has_value() ||
            move.source_parallel_copy_predecessor_label.has_value() ||
            move.source_parallel_copy_successor_label.has_value() ||
            destination_home->kind != prepare::PreparedValueHomeKind::StackSlot) {
          return std::nullopt;
        }
        if (source_home->target_register_identity.has_value() &&
            (source_home->target_register_identity->target_arch !=
                 c4c::TargetArch::Riscv64 ||
             source_home->target_register_identity->bank !=
                 prepare::PreparedRegisterBank::Gpr ||
             source_home->target_register_identity->register_class !=
                 prepare::PreparedRegisterClass::General)) {
          return std::nullopt;
        }
        const auto source = gpr_register_number_for_home(*source_home);
        if (!source.has_value() ||
            !prepared_storage_plan_endpoint_is_coherent_gpr_register(
                storage_plan,
                move.from_value_id,
                *source) ||
            !prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot(
                storage_plan,
                move.to_value_id)) {
          return std::nullopt;
        }
        const auto source_type = prepared_bir_value_type_for_name(
            names, function, source_home->value_name);
        const auto source_size_bytes =
            source_type.has_value()
                ? rv64_scalar_memory_size_for_type(*source_type)
                : source_home->size_bytes;
        if (!source_size_bytes.has_value()) {
          return std::nullopt;
        }
        if (!append_rv64_store_register_to_stack_offset(fragment,
                                                       *source,
                                                       *stack_offset,
                                                       *destination_size_bytes)) {
          return std::nullopt;
        }
        continue;
      }

      if (source_home->kind ==
          prepare::PreparedValueHomeKind::RematerializableImmediate) {
        const auto report =
            prepare::verify_prepared_rematerializable_integer_immediate_contract(
                source_home);
        const auto fact =
            prepare::as_rematerializable_integer_immediate_fact(*source_home);
        const auto scratch = rv64_unoccupied_temporary_gpr(lookups);
        if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent ||
            !fact.has_value() || !scratch.has_value() ||
            !prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot(
                storage_plan,
                move.to_value_id)) {
          return std::nullopt;
        }
        append_rv64_load_immediate(fragment, *scratch, fact->signed_value);
        if (!append_rv64_store_register_to_stack_offset(fragment,
                                                       *scratch,
                                                       *stack_offset,
                                                       *destination_size_bytes)) {
          return std::nullopt;
        }
        continue;
      }

      auto stack_to_stack_fragment =
          fragment_for_prepared_stack_slot_to_stack_slot_move(stack_layout,
                                                              names,
                                                              function,
                                                              lookups,
                                                              storage_plan,
                                                              stack_frame_bytes,
                                                              parallel_copy_bundle,
                                                              stack_destination_fan_in_authority,
                                                              move_bundle,
                                                              move,
                                                              *source_home,
                                                              *destination_home);
      if (!stack_to_stack_fragment.has_value()) {
        return std::nullopt;
      }
      append_rv64_fragment(fragment, std::move(*stack_to_stack_fragment));
      continue;
    }

    if (move.destination_storage_kind !=
        prepare::PreparedMoveStorageKind::Register) {
      return std::nullopt;
    }

    std::optional<std::uint32_t> destination;
    std::optional<std::uint32_t> fpr_destination;
    if (move.destination_register_name.has_value()) {
      destination = rv64_register_number(*move.destination_register_name);
    }
    if (!destination.has_value() &&
        move.destination_kind ==
            prepare::PreparedMoveDestinationKind::Value) {
      const auto* destination_home =
          prepared_value_home_for_id(lookups, move.to_value_id);
      if (destination_home != nullptr) {
        destination = gpr_register_number_for_home(*destination_home);
        fpr_destination = fpr_register_number_for_home(*destination_home);
      }
    }
    if (!destination.has_value() && !fpr_destination.has_value() &&
        move.destination_register_placement.has_value() &&
        move.destination_register_placement->bank ==
            prepare::PreparedRegisterBank::Fpr &&
        move.destination_register_placement->contiguous_width == 1) {
      fpr_destination = fpr_register_number_for_abi_placement(
          target_profile, *move.destination_register_placement);
    }
    if (!destination.has_value() && !fpr_destination.has_value()) {
      return std::nullopt;
    }

    if (move.source_immediate_i32.has_value()) {
      if (!destination.has_value()) {
        return std::nullopt;
      }
      if (!fits_signed_12_bit_immediate(*move.source_immediate_i32)) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment, *destination, *move.source_immediate_i32);
      continue;
    }

    if (destination.has_value()) {
      auto producer_fragment = fragment_for_prepared_select_edge_source_producer(
          stack_layout,
          names,
          control_flow,
          function,
          lookups,
          dependency_operand_authorities,
          carrier_alias_authorities,
          move_bundle,
          move,
          *destination,
          stack_frame_bytes);
      if (producer_fragment.matched) {
        if (!producer_fragment.fragment.has_value()) {
          return std::nullopt;
        }
        append_fragment(fragment, std::move(*producer_fragment.fragment));
        continue;
      }
    }

    const auto* source_home =
        prepared_value_home_for_id(lookups, move.from_value_id);
    if (source_home == nullptr) {
      return std::nullopt;
    }
    if (source_home->kind ==
        prepare::PreparedValueHomeKind::RematerializableImmediate) {
      const auto report =
          prepare::verify_prepared_rematerializable_integer_immediate_contract(
              source_home);
      if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
        return std::nullopt;
      }
      const auto fact =
          prepare::as_rematerializable_integer_immediate_fact(*source_home);
      if (!fact.has_value() || !fact->fits_signed_12_bit_immediate) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment, *destination, fact->signed_value);
      continue;
    }

    if (fpr_destination.has_value()) {
      const auto source = fpr_register_number_for_home(*source_home);
      const auto type =
          prepared_bir_value_type_for_name(names, function, source_home->value_name);
      if (!source.has_value() || !type.has_value() ||
          !append_rv64_fpr_move(fragment, *fpr_destination, *source, *type)) {
        return std::nullopt;
      }
      continue;
    }

    const auto source = gpr_register_number_for_home(*source_home);
    if (!source.has_value() &&
        source_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      const auto type =
          prepared_bir_value_type_for_name(names, function, source_home->value_name);
      if (!type.has_value()) {
        return std::nullopt;
      }
      if (*type == c4c::backend::bir::TypeKind::Ptr) {
        return std::nullopt;
      }
      const auto size_bytes = rv64_scalar_memory_size_for_type(*type);
      if (!size_bytes.has_value()) {
        return std::nullopt;
      }
      const auto stack_offset =
          prepared_stack_slot_home_absolute_offset(stack_layout,
                                                   *source_home,
                                                   stack_frame_bytes,
                                                   *size_bytes);
      if (!stack_offset.has_value() ||
          !append_rv64_load_stack_offset_to_register(fragment,
                                                    *destination,
                                                    *stack_offset,
                                                    *size_bytes)) {
        return std::nullopt;
      }
      continue;
    }
    if (!source.has_value()) {
      return std::nullopt;
    }
    append_rv64_move(fragment, *destination, *source);
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_stack_slot_to_stack_slot_move(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan,
    std::size_t stack_frame_bytes,
    const c4c::backend::prepare::PreparedParallelCopyBundle* parallel_copy_bundle,
    const c4c::backend::prepare::PreparedStackDestinationFanInAuthorityFact*
        stack_destination_fan_in_authority,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    const c4c::backend::prepare::PreparedValueHome& source_home,
    const c4c::backend::prepare::PreparedValueHome& destination_home) {
  if (parallel_copy_bundle != nullptr ||
      move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move.reason != "consumer_stack_to_stack" ||
      move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      move.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      move.destination_register_name.has_value() ||
      !move.destination_occupied_register_names.empty() ||
      move.destination_register_placement.has_value() ||
      move.destination_contiguous_width != 1 ||
      move.destination_stack_offset_bytes.has_value() ||
      move.uses_cycle_temp_source || move.source_parallel_copy_step_index.has_value() ||
      move.source_parallel_copy_predecessor_label.has_value() ||
      move.source_parallel_copy_successor_label.has_value() ||
      move.source_immediate_i32.has_value() ||
      move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
      source_home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      destination_home.kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  const bool explicit_widening_authority =
      move_bundle.authority_kind ==
          prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion &&
      move.authority_kind ==
          prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion;
  const bool plain_stack_copy_authority =
      move_bundle.authority_kind == prepare::PreparedMoveAuthorityKind::None &&
      move.authority_kind == prepare::PreparedMoveAuthorityKind::None;
  const bool stack_destination_register_fan_in_authority =
      prepared_move_has_matching_stack_destination_register_fan_in_authority(
          move_bundle, move, stack_destination_fan_in_authority);
  if (!explicit_widening_authority && !plain_stack_copy_authority &&
      !stack_destination_register_fan_in_authority) {
    return std::nullopt;
  }
  if (!prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot(
          storage_plan,
          move.from_value_id) ||
      !prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot(
          storage_plan,
          move.to_value_id)) {
    return std::nullopt;
  }

  const auto source_type =
      prepared_bir_value_type_for_name(names, function, source_home.value_name);
  const auto destination_type =
      prepared_bir_value_type_for_name(names, function, destination_home.value_name);
  if (!destination_type.has_value()) {
    return std::nullopt;
  }
  if (source_home.target_register_identity.has_value() &&
      (source_home.target_register_identity->target_arch != c4c::TargetArch::Riscv64 ||
       source_home.target_register_identity->bank != prepare::PreparedRegisterBank::Gpr ||
       source_home.target_register_identity->register_class !=
           prepare::PreparedRegisterClass::General)) {
    return std::nullopt;
  }
  if (destination_home.target_register_identity.has_value() &&
      (destination_home.target_register_identity->target_arch !=
           c4c::TargetArch::Riscv64 ||
       destination_home.target_register_identity->bank !=
           prepare::PreparedRegisterBank::Gpr ||
       destination_home.target_register_identity->register_class !=
           prepare::PreparedRegisterClass::General)) {
    return std::nullopt;
  }

  const auto source_size_bytes =
      source_type.has_value()
          ? rv64_scalar_memory_size_for_type(*source_type)
          : prepared_stack_slot_home_size_bytes(stack_layout, source_home);
  const auto destination_size_bytes =
      rv64_scalar_memory_size_for_type(*destination_type);
  if (!source_size_bytes.has_value() || !destination_size_bytes.has_value()) {
    return std::nullopt;
  }
  const bool widening_stack_conversion =
      explicit_widening_authority &&
      source_type.has_value() &&
      rv64_fixed_integer_type(*source_type) &&
      rv64_fixed_integer_type(*destination_type) &&
      *source_size_bytes < *destination_size_bytes;
  if (!widening_stack_conversion &&
      ((!plain_stack_copy_authority &&
        !stack_destination_register_fan_in_authority) ||
       *source_size_bytes < *destination_size_bytes)) {
    return std::nullopt;
  }
  const auto source_stack_offset = prepared_stack_slot_home_absolute_offset(
      stack_layout, source_home, stack_frame_bytes, *source_size_bytes);
  const auto destination_stack_offset = prepared_stack_slot_home_absolute_offset(
      stack_layout, destination_home, stack_frame_bytes, *destination_size_bytes);
  const auto scratch = rv64_unoccupied_temporary_gpr(lookups);
  if (!source_stack_offset.has_value() || !destination_stack_offset.has_value() ||
      !scratch.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_load_stack_offset_to_register(fragment,
                                                *scratch,
                                                *source_stack_offset,
                                                widening_stack_conversion
                                                    ? *source_size_bytes
                                                    : *destination_size_bytes) ||
      !append_rv64_store_register_to_stack_offset(fragment,
                                                 *scratch,
                                                 *destination_stack_offset,
                                                 *destination_size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}


std::optional<std::uint32_t> append_rv64_prepare_floating_value_for_store(
    RiscvEncodedFragment& fragment,
    std::uint32_t scratch_fpr,
    std::uint32_t scratch_gpr,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (!rv64_floating_type(value.type)) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    if (value.immediate_bits >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return std::nullopt;
    }
    append_rv64_load_immediate(
        fragment,
        scratch_gpr,
        static_cast<std::int64_t>(value.immediate_bits));
    if (!append_rv64_gpr_to_fpr_move(
            fragment, scratch_fpr, scratch_gpr, value.type)) {
      return std::nullopt;
    }
    return scratch_fpr;
  }
  return fpr_register_number_for_value(names, lookups, value);
}

std::optional<std::uint32_t> rv64_branch_funct3(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq: return 0;
    case c4c::backend::bir::BinaryOpcode::Ne: return 1;
    case c4c::backend::bir::BinaryOpcode::Slt: return 4;
    case c4c::backend::bir::BinaryOpcode::Sge: return 5;
    case c4c::backend::bir::BinaryOpcode::Ult: return 6;
    case c4c::backend::bir::BinaryOpcode::Uge: return 7;
    default: return std::nullopt;
  }
}

struct Rv64NormalizedBranchPredicate {
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

std::optional<Rv64NormalizedBranchPredicate> normalize_rv64_branch_predicate(
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  if (lhs.type == c4c::backend::bir::TypeKind::Ptr ||
      rhs.type == c4c::backend::bir::TypeKind::Ptr) {
    if (opcode == c4c::backend::bir::BinaryOpcode::Ne &&
        lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        lhs.type == c4c::backend::bir::TypeKind::Ptr &&
        is_rv64_null_pointer_value(rhs)) {
      return Rv64NormalizedBranchPredicate{
          .opcode = opcode,
          .lhs = lhs,
          .rhs = rhs,
      };
    }
    return std::nullopt;
  }
  if (rv64_branch_funct3(opcode).has_value()) {
    return Rv64NormalizedBranchPredicate{
        .opcode = opcode,
        .lhs = lhs,
        .rhs = rhs,
    };
  }
  const auto is_rv64_gpr_integer = [](c4c::backend::bir::TypeKind type) {
    switch (type) {
      case c4c::backend::bir::TypeKind::I1:
      case c4c::backend::bir::TypeKind::I8:
      case c4c::backend::bir::TypeKind::I16:
      case c4c::backend::bir::TypeKind::I32:
      case c4c::backend::bir::TypeKind::I64:
        return true;
      default:
        return false;
    }
  };
  const bool matching_scalar_integer_operands =
      lhs.type == rhs.type && is_rv64_gpr_integer(lhs.type);
  if (matching_scalar_integer_operands) {
    switch (opcode) {
      case c4c::backend::bir::BinaryOpcode::Sgt:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Slt,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Sle:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Sge,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Ugt:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Ult,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Ule:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Uge,
            .lhs = rhs,
            .rhs = lhs,
        };
      default:
        break;
    }
  }
  return std::nullopt;
}

void append_rv64_local_jump(RiscvEncodedFragment& fragment,
                            std::string target_label) {
  const auto offset = fragment.bytes.size();
  append_le32(fragment.bytes, encode_j_type(0x6f, 0, 0));  // jal zero, target
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = offset,
      .kind = RiscvObjectFixupKind::Jal,
      .symbol_name = std::move(target_label),
      .addend = 0,
  });
}

void append_rv64_local_branch(RiscvEncodedFragment& fragment,
                              std::uint32_t funct3,
                              std::uint32_t lhs_register,
                              std::uint32_t rhs_register,
                              std::string target_label) {
  const auto offset = fragment.bytes.size();
  append_le32(fragment.bytes,
              encode_b_type(0x63, funct3, lhs_register, rhs_register, 0));
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = offset,
      .kind = RiscvObjectFixupKind::Branch,
      .symbol_name = std::move(target_label),
      .addend = 0,
  });
}

std::optional<RiscvEncodedFragment> make_rv64_block_label_fragment(
    std::string label_name) {
  if (label_name.empty()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = 0,
      .name = std::move(label_name),
  });
  return fragment;
}

std::string_view trim_ascii(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t' ||
                          text.front() == '\n' || text.front() == '\r')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t' ||
                          text.back() == '\n' || text.back() == '\r')) {
    text.remove_suffix(1);
  }
  return text;
}

std::optional<std::uint32_t> parse_rv64_insn_u32(std::string_view text,
                                                 std::uint32_t max_value) {
  text = trim_ascii(text);
  if (text.empty()) {
    return std::nullopt;
  }
  int base = 10;
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text.remove_prefix(2);
    base = 16;
  }
  std::uint32_t value = 0;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, base);
  if (ec != std::errc{} || ptr != end || value > max_value) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint32_t> rv64_register_number_for_inline_asm_operand(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier,
    std::size_t operand_index) {
  if (operand_index >= carrier.operands.size()) {
    return std::nullopt;
  }

  const auto& operand = carrier.operands[operand_index];
  const auto constraint_is_decimal_index = [](std::string_view constraint) {
    return !constraint.empty() &&
           std::all_of(constraint.begin(), constraint.end(), [](unsigned char ch) {
             return std::isdigit(ch) != 0;
           });
  };
  switch (operand.kind) {
    case c4c::backend::bir::InlineAsmOperandKind::RegisterOutput:
      if (operand.constraint != "=r" && operand.constraint != "+r") {
        return std::nullopt;
      }
      if (!operand.output_index.has_value() || *operand.output_index != 0 ||
          !carrier.result_home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*carrier.result_home);
    case c4c::backend::bir::InlineAsmOperandKind::RegisterInput:
      if (operand.constraint != "r") {
        return std::nullopt;
      }
      if (!operand.arg_index.has_value() || !operand.home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*operand.home);
    case c4c::backend::bir::InlineAsmOperandKind::TiedInput:
      if (!constraint_is_decimal_index(operand.constraint)) {
        return std::nullopt;
      }
      if (!operand.arg_index.has_value() || !operand.home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*operand.home);
    case c4c::backend::bir::InlineAsmOperandKind::Unsupported:
    case c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput:
    case c4c::backend::bir::InlineAsmOperandKind::MemoryInput:
    case c4c::backend::bir::InlineAsmOperandKind::AddressInput:
    case c4c::backend::bir::InlineAsmOperandKind::Clobber:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::uint32_t> rv64_register_number_for_inline_asm_operand_token(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier,
    std::string_view token) {
  token = trim_ascii(token);
  if (token.size() < 2 || (token.front() != '%' && token.front() != '$')) {
    return std::nullopt;
  }
  token.remove_prefix(1);
  std::size_t operand_index = 0;
  const char* const begin = token.data();
  const char* const end = token.data() + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, operand_index);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return rv64_register_number_for_inline_asm_operand(carrier, operand_index);
}

bool prepared_register_identities_match(
    const c4c::backend::prepare::PreparedTargetRegisterIdentity& lhs,
    const c4c::backend::prepare::PreparedTargetRegisterIdentity& rhs) {
  return lhs.target_arch == rhs.target_arch && lhs.bank == rhs.bank &&
         lhs.register_class == rhs.register_class &&
         lhs.physical_index == rhs.physical_index;
}

bool inline_asm_type_is_scalar_gpr_object_value(
    c4c::backend::bir::TypeKind type) {
  namespace bir = c4c::backend::bir;
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return true;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
    case bir::TypeKind::Vrm1:
    case bir::TypeKind::Vrm2:
    case bir::TypeKind::Vrm4:
    case bir::TypeKind::Vrm8:
      return false;
  }
  return false;
}

std::optional<RiscvEncodedFragment> fragment_for_empty_tied_scalar_gpr_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier* carrier,
    const c4c::backend::bir::CallInst& call) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;
  if (carrier == nullptr ||
      carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !call.inline_asm.has_value() || call.callee != "llvm.inline_asm" ||
      call.is_indirect || call.callee_value.has_value() ||
      carrier->has_named_operand_references || carrier->has_template_modifiers ||
      !carrier->clobbers.empty() || !carrier->missing_required_facts.empty() ||
      !trim_ascii(carrier->asm_text).empty() ||
      !trim_ascii(call.inline_asm->asm_text).empty() ||
      carrier->constraints != "=r,0" || carrier->operands.size() != 2 ||
      !carrier->result.has_value() || !carrier->result_home.has_value() ||
      !inline_asm_type_is_scalar_gpr_object_value(carrier->result->type)) {
    return std::nullopt;
  }

  const auto& output = carrier->operands[0];
  const auto& input = carrier->operands[1];
  if (output.kind != bir::InlineAsmOperandKind::RegisterOutput ||
      output.constraint_index != 0 || output.constraint != "=r" ||
      output.output_index != std::optional<std::size_t>{0} ||
      output.register_class != bir::InlineAsmRegisterClass::General ||
      output.register_group_width != 1 ||
      input.kind != bir::InlineAsmOperandKind::TiedInput ||
      input.constraint_index != 1 || input.constraint != "0" ||
      !input.arg_index.has_value() ||
      input.tied_output_index != std::optional<std::size_t>{0} ||
      !input.home.has_value() || !input.tied_home_authority.has_value() ||
      input.tied_home_authority->tied_output_index != 0) {
    return std::nullopt;
  }
  if (input.value.has_value() &&
      !inline_asm_type_is_scalar_gpr_object_value(input.value->type)) {
    return std::nullopt;
  }

  const auto destination = gpr_register_number_for_home(*carrier->result_home);
  const auto source = gpr_register_number_for_home(*input.home);
  if (!destination.has_value() || !source.has_value() ||
      !carrier->result_home->target_register_identity.has_value() ||
      !input.home->target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& result_identity = *carrier->result_home->target_register_identity;
  const auto& input_identity = *input.home->target_register_identity;
  if (!prepared_register_identities_match(result_identity, input_identity) ||
      !prepared_register_identities_match(
          result_identity,
          input.tied_home_authority->shared_register)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_rv64_move(fragment, *destination, *source);
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_no_result_side_effect_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier* carrier,
    const c4c::backend::bir::CallInst& call) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;
  if (carrier == nullptr ||
      carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !call.inline_asm.has_value() || call.callee != "llvm.inline_asm" ||
      call.is_indirect || call.callee_value.has_value() ||
      !call.args.empty() || !call.arg_types.empty() ||
      call.return_type != bir::TypeKind::Void || call.result.has_value() ||
      carrier->result.has_value() || carrier->result_home.has_value() ||
      carrier->has_named_operand_references || carrier->has_template_modifiers ||
      !carrier->missing_required_facts.empty() ||
      !trim_ascii(carrier->asm_text).empty() ||
      !trim_ascii(call.inline_asm->asm_text).empty() ||
      !carrier->side_effects || !call.inline_asm->side_effects ||
      carrier->constraints != "~{memory}" || carrier->operands.size() != 1 ||
      carrier->clobbers.size() != 1 || carrier->clobbers[0] != "memory") {
    return std::nullopt;
  }

  const auto& operand = carrier->operands[0];
  if (operand.kind != bir::InlineAsmOperandKind::Clobber ||
      operand.constraint_index != 0 || operand.constraint != "~{memory}" ||
      operand.name != std::optional<std::string>{"memory"} ||
      operand.arg_index.has_value() || operand.output_index.has_value() ||
      operand.tied_output_index.has_value()) {
    return std::nullopt;
  }

  return RiscvEncodedFragment{};
}

std::optional<std::vector<std::string_view>> split_rv64_insn_fields(
    std::string_view text,
    std::size_t expected_count) {
  std::vector<std::string_view> fields;
  while (true) {
    const std::size_t comma = text.find(',');
    const bool last = comma == std::string_view::npos;
    fields.push_back(trim_ascii(last ? text : text.substr(0, comma)));
    if (last) {
      break;
    }
    text.remove_prefix(comma + 1);
    if (fields.size() == expected_count) {
      return std::nullopt;
    }
  }
  if (fields.size() != expected_count ||
      std::any_of(fields.begin(), fields.end(), [](std::string_view field) {
        return field.empty();
      })) {
    return std::nullopt;
  }
  return fields;
}

std::optional<RiscvEncodedFragment> fragment_for_rv64_insn_r_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier* carrier,
    const c4c::backend::bir::CallInst& call) {
  namespace prepare = c4c::backend::prepare;
  if (carrier == nullptr ||
      carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !call.inline_asm.has_value() || call.callee != "llvm.inline_asm" ||
      call.is_indirect || call.callee_value.has_value() ||
      carrier->has_named_operand_references || carrier->has_template_modifiers ||
      !carrier->clobbers.empty() || !carrier->missing_required_facts.empty()) {
    return std::nullopt;
  }

  if (call.inline_asm->insn_r.has_value()) {
    const auto& insn = *call.inline_asm->insn_r;
    const auto rd =
        rv64_register_number_for_inline_asm_operand(*carrier, insn.operand_indices[0]);
    const auto rs1 =
        rv64_register_number_for_inline_asm_operand(*carrier, insn.operand_indices[1]);
    const auto rs2 =
        rv64_register_number_for_inline_asm_operand(*carrier, insn.operand_indices[2]);
    if (!rd.has_value() || !rs1.has_value() || !rs2.has_value()) {
      return std::nullopt;
    }
    RiscvEncodedFragment fragment;
    append_le32(fragment.bytes,
                encode_r_type(insn.opcode, *rd, insn.funct3, *rs1, *rs2, insn.funct7));
    return fragment;
  }

  std::string_view text = trim_ascii(call.inline_asm->asm_text);
  constexpr std::string_view prefix = ".insn r";
  if (text.size() < prefix.size() || text.substr(0, prefix.size()) != prefix ||
      (text.size() > prefix.size() && text[prefix.size()] != ' ' &&
       text[prefix.size()] != '\t')) {
    return std::nullopt;
  }
  text.remove_prefix(prefix.size());
  text = trim_ascii(text);

  const auto fields = split_rv64_insn_fields(text, 6);
  if (!fields.has_value()) {
    return std::nullopt;
  }

  const auto opcode = parse_rv64_insn_u32((*fields)[0], 0x7f);
  const auto funct3 = parse_rv64_insn_u32((*fields)[1], 0x7);
  const auto funct7 = parse_rv64_insn_u32((*fields)[2], 0x7f);
  const auto rd = rv64_register_number_for_inline_asm_operand_token(*carrier, (*fields)[3]);
  const auto rs1 = rv64_register_number_for_inline_asm_operand_token(*carrier, (*fields)[4]);
  const auto rs2 = rv64_register_number_for_inline_asm_operand_token(*carrier, (*fields)[5]);
  if (!opcode.has_value() || !funct3.has_value() || !funct7.has_value() ||
      !rd.has_value() || !rs1.has_value() || !rs2.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_r_type(*opcode, *rd, *funct3, *rs1, *rs2, *funct7));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_rv64_insn_d_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier* carrier,
    const c4c::backend::bir::CallInst& call) {
  if (carrier == nullptr || !call.inline_asm.has_value() ||
      call.callee != "llvm.inline_asm" || call.is_indirect ||
      call.callee_value.has_value()) {
    return std::nullopt;
  }
  const auto substituted = substitute_prepared_riscv_inline_asm_operands(*carrier);
  if (!substituted.has_value()) {
    return std::nullopt;
  }
  const auto parsed = parse_rv64_asm_line(*substituted);
  if (!parsed.has_value() ||
      !std::holds_alternative<Rv64InsnDLine>(*parsed)) {
    return std::nullopt;
  }
  const auto encoded = encode_rv64_asm_line(*parsed);
  if (!encoded.has_value()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  fragment.bytes = *encoded;
  return fragment;
}

RiscvEncodedFragment make_rv64_return_immediate_fragment(std::int64_t immediate) {
  RiscvEncodedFragment fragment;
  append_rv64_load_immediate(fragment, 10, immediate);
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
}

std::optional<std::string> diagnose_unsupported_prepared_saved_register_bank(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan) {
  if (frame_plan == nullptr) {
    return std::nullopt;
  }
  for (const auto& saved : frame_plan->saved_callee_registers) {
    if (frame_plan->has_dynamic_stack &&
        saved.bank != prepare::PreparedRegisterBank::Gpr) {
      return "unsupported_stack_frame: RV64 object route requires producer-"
             "published GPR callee-saved save-slot placement for dynamic "
             "stack frames (" +
             std::string(prepare::prepared_register_bank_name(saved.bank)) +
             ":" + saved.register_name + ")";
    }
    if (saved.bank != prepare::PreparedRegisterBank::Gpr &&
        saved.bank != prepare::PreparedRegisterBank::Fpr) {
      return "unsupported_stack_frame: RV64 object route does not support "
             "unsupported prepared callee-saved register save slots (" +
             std::string(prepare::prepared_register_bank_name(saved.bank)) +
             ":" + saved.register_name + ")";
    }
    const auto expected_bank = saved.bank;
    if (saved.placement.has_value() &&
        saved.placement->bank != expected_bank) {
      return "unsupported_stack_frame: RV64 object route does not support "
             "mismatched prepared callee-saved register placements (" +
             std::string(
                 prepare::prepared_register_bank_name(saved.placement->bank)) +
             ":" + saved.register_name + ")";
    }
    if (saved.slot_placement.has_value() &&
        saved.slot_placement->bank != expected_bank) {
      return "unsupported_stack_frame: RV64 object route does not support "
             "mismatched prepared callee-saved register save-slot placements (" +
             std::string(prepare::prepared_register_bank_name(
                 saved.slot_placement->bank)) +
             ":" + saved.register_name + ")";
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> rv64_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout) {
  return rv64_prepared_object_stack_frame_size(
      addressing, frame_plan, stack_layout);
}

const c4c::backend::bir::Value* pure_instruction_result(
    const c4c::backend::bir::Inst& inst) {
  if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
    return &binary->result;
  }
  if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
    return &select->result;
  }
  if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
    return &cast->result;
  }
  return nullptr;
}

bool prepared_pure_instruction_is_rematerialized_immediate(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Inst& inst) {
  const auto* result = pure_instruction_result(inst);
  if (result == nullptr) {
    return false;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, *result);
  return immediate.has_value() && fits_signed_12_bit_immediate(*immediate);
}

std::optional<std::int32_t> prepared_frame_slot_address_call_argument_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || frame_plan == nullptr || frame_plan->has_dynamic_stack ||
      frame_plan->uses_frame_pointer_for_fixed_slots ||
      (argument.value_bank != prepare::PreparedRegisterBank::Gpr &&
       argument.value_bank != prepare::PreparedRegisterBank::AggregateAddress) ||
      !argument.source_selection.has_value()) {
    return std::nullopt;
  }

  const auto& selection = *argument.source_selection;
  const bool local_frame_address_selection =
      selection.kind ==
      prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization;
  const bool frame_slot_address_selection =
      selection.kind ==
      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress;
  const auto local_route =
      local_frame_address_selection
          ? prepare::as_local_frame_address_materialization_route(selection)
          : std::nullopt;
  if ((!local_frame_address_selection && !frame_slot_address_selection) ||
      (local_frame_address_selection &&
       argument.source_encoding != prepare::PreparedStorageEncodingKind::Register &&
       argument.source_encoding !=
           prepare::PreparedStorageEncodingKind::ComputedAddress) ||
      (local_frame_address_selection && !local_route.has_value()) ||
      (frame_slot_address_selection &&
       (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
        !argument.source_value_id.has_value() ||
        !argument.source_slot_id.has_value() ||
        !selection.source_slot_id.has_value() ||
        selection.source_home_kind !=
            prepare::PreparedValueHomeKind::StackSlot ||
        !selection.source_value_id.has_value() ||
        *selection.source_value_id != *argument.source_value_id)) ||
      !selection.address_materialization_block_label.has_value() ||
      !selection.address_materialization_inst_index.has_value() ||
      !selection.address_materialization_frame_slot_id.has_value() ||
      !selection.address_materialization_byte_offset.has_value() ||
      *selection.address_materialization_byte_offset < 0 ||
      (frame_slot_address_selection &&
       *selection.source_slot_id != *selection.address_materialization_frame_slot_id) ||
      (argument.source_value_id.has_value() && selection.source_value_id.has_value() &&
       *argument.source_value_id != *selection.source_value_id)) {
    return std::nullopt;
  }

  const auto source_slot_id =
      local_route.has_value() ? local_route->source_slot_id
                              : *selection.source_slot_id;
  const std::optional<std::size_t> source_stack_offset_bytes =
      local_route.has_value()
          ? std::optional<std::size_t>{local_route->source_stack_offset_bytes}
          : selection.source_stack_offset_bytes;
  const auto materialization_block_label =
      local_route.has_value() ? local_route->address_materialization_block_label
                              : *selection.address_materialization_block_label;
  const auto materialization_inst_index =
      local_route.has_value() ? local_route->address_materialization_inst_index
                              : *selection.address_materialization_inst_index;
  const auto materialization_frame_slot_id =
      local_route.has_value() ? local_route->address_materialization_frame_slot_id
                              : *selection.address_materialization_frame_slot_id;
  const auto materialization_byte_offset =
      local_route.has_value() ? local_route->address_materialization_byte_offset
                              : *selection.address_materialization_byte_offset;

  const auto materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups->address_materializations,
          materialization_block_label);
  if (materializations == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != materialization_inst_index ||
        materialization->kind !=
            prepare::PreparedAddressMaterializationKind::FrameSlot) {
      continue;
    }
    const bool same_selected_value =
        (!selection.source_value_id.has_value() &&
         !selection.source_value_name.has_value()) ||
        (selection.source_value_id.has_value() &&
         materialization->result_value_id == selection.source_value_id) ||
        (selection.source_value_name.has_value() &&
         materialization->result_value_name == selection.source_value_name);
    if (same_selected_value &&
        (materialization->frame_slot_id !=
             materialization_frame_slot_id ||
         materialization->byte_offset !=
             materialization_byte_offset)) {
      return std::nullopt;
    }
    if (materialization->frame_slot_id !=
            materialization_frame_slot_id ||
        materialization->byte_offset != materialization_byte_offset) {
      continue;
    }
    if (selected != nullptr) {
      continue;
    }
    selected = materialization;
  }
  if (selected == nullptr || selected->address_space != bir::AddressSpace::Default ||
      selected->is_thread_local || selected->has_tls_address_space ||
      selected->tls_model != prepare::PreparedTlsMaterializationModel::None ||
      selected->tls_thread_pointer_register !=
          prepare::PreparedTlsThreadPointerRegister::None ||
      selected->tls_high_relocation != prepare::PreparedTlsRelocationKind::None ||
      selected->tls_low_relocation != prepare::PreparedTlsRelocationKind::None) {
    return std::nullopt;
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == source_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end()) {
    return std::nullopt;
  }

  const auto offset = static_cast<std::size_t>(materialization_byte_offset);
  const bool local_pointer_base_plus_offset =
      local_route.has_value() &&
      local_route->source_home_kind ==
          prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  if (local_pointer_base_plus_offset &&
      (!local_route->source_base_value_id.has_value() ||
       offset < slot_it->offset_bytes ||
       local_route->source_pointer_byte_delta < 0 ||
       static_cast<std::size_t>(local_route->source_pointer_byte_delta) !=
           offset - slot_it->offset_bytes)) {
    return std::nullopt;
  }
  if ((source_stack_offset_bytes.has_value() &&
       *source_stack_offset_bytes != offset) ||
      (!local_pointer_base_plus_offset &&
       (offset < slot_it->offset_bytes ||
        offset > slot_it->offset_bytes + slot_it->size_bytes)) ||
      offset > stack_frame_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }

  return static_cast<std::int32_t>(offset);
}

std::optional<std::int32_t> prepared_frame_slot_value_home_call_argument_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.source_value_id.has_value() ||
      !argument.source_slot_id.has_value() ||
      !argument.source_selection.has_value()) {
    return std::nullopt;
  }

  const auto& selection = *argument.source_selection;
  const auto route_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(&selection);
  const auto route = prepare::as_frame_slot_value_source_route(selection);
  if (route_report.fail_closed || !route.has_value() ||
      route->source_value_id != *argument.source_value_id) {
    return std::nullopt;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type(argument_type);
  const auto* source_home =
      prepared_value_home_for_id(lookups, *argument.source_value_id);
  if (!size_bytes.has_value() ||
      route->source_size_bytes != *size_bytes ||
      route->source_align_bytes > *size_bytes ||
      source_home == nullptr ||
      source_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !source_home->slot_id.has_value() ||
      *source_home->slot_id != *argument.source_slot_id ||
      !source_home->offset_bytes.has_value() ||
      (argument.source_stack_offset_bytes.has_value() &&
       *argument.source_stack_offset_bytes != *source_home->offset_bytes)) {
    return std::nullopt;
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *source_home->slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      slot_it->offset_bytes != *source_home->offset_bytes ||
      slot_it->size_bytes < *size_bytes ||
      slot_it->align_bytes > *size_bytes) {
    return std::nullopt;
  }

  const auto offset = *source_home->offset_bytes;
  if (offset > stack_frame_bytes ||
      stack_frame_bytes - offset < *size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<std::int32_t> prepared_explicit_scalar_frame_slot_call_argument_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
      argument.source_register_name.has_value() ||
      argument.source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.source_value_id.has_value() ||
      !argument.source_slot_id.has_value() ||
      !argument.source_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type(argument_type);
  const auto* source_home =
      prepared_value_home_for_id(lookups, *argument.source_value_id);
  if (!size_bytes.has_value() || source_home == nullptr ||
      source_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !source_home->slot_id.has_value() ||
      *source_home->slot_id != *argument.source_slot_id ||
      !source_home->offset_bytes.has_value() ||
      *source_home->offset_bytes != *argument.source_stack_offset_bytes) {
    return std::nullopt;
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *source_home->slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      slot_it->offset_bytes != *source_home->offset_bytes ||
      slot_it->size_bytes < *size_bytes ||
      slot_it->align_bytes > *size_bytes) {
    return std::nullopt;
  }

  const auto offset = *source_home->offset_bytes;
  if (offset > stack_frame_bytes ||
      stack_frame_bytes - offset < *size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<std::int32_t> prepared_sret_memory_return_argument_address_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedCallPlan& call_plan,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      !call_plan.memory_return.has_value() ||
      !call_plan.memory_return->sret_arg_index.has_value() ||
      *call_plan.memory_return->sret_arg_index != argument.arg_index ||
      call_plan.memory_return->encoding !=
          prepare::PreparedStorageEncodingKind::FrameSlot ||
      !call_plan.memory_return->slot_id.has_value() ||
      !call_plan.memory_return->stack_offset_bytes.has_value() ||
      call_plan.memory_return->size_bytes == 0 ||
      call_plan.memory_return->align_bytes == 0 ||
      argument.value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      argument.source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      argument.destination_register_bank.has_value() ||
      argument.destination_register_name.has_value() ||
      argument.destination_stack_offset_bytes.has_value() ||
      argument.destination_stack_size_bytes.has_value() ||
      !argument.source_selection.has_value()) {
    return std::nullopt;
  }

  const auto& selection = *argument.source_selection;
  const auto route =
      prepare::as_local_frame_address_materialization_route(selection);
  if (!route.has_value() ||
      route->source_slot_id != *call_plan.memory_return->slot_id ||
      route->source_stack_offset_bytes !=
          *call_plan.memory_return->stack_offset_bytes ||
      route->source_size_bytes < call_plan.memory_return->size_bytes ||
      route->source_align_bytes < call_plan.memory_return->align_bytes) {
    return std::nullopt;
  }

  const auto offset =
      prepared_frame_slot_address_call_argument_offset(stack_layout,
                                                       lookups,
                                                       frame_plan,
                                                       argument,
                                                       stack_frame_bytes);
  if (!offset.has_value() ||
      static_cast<std::size_t>(*offset) !=
          *call_plan.memory_return->stack_offset_bytes) {
    return std::nullopt;
  }
  return offset;
}

struct PreparedFrameSlotAddressArgumentPublication {
  std::int32_t address_offset = 0;
  std::int32_t destination_offset = 0;
  std::optional<std::int32_t> payload_address_offset;
  std::optional<bir::Value> payload_value;
};

[[nodiscard]] const prepare::PreparedStoreSourcePublicationRecord*
find_matching_call_argument_value_publication_store_source(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const prepare::PreparedCallArgumentValuePublicationFact& fact) {
  const prepare::PreparedStoreSourcePublicationRecord* selected = nullptr;
  for (const auto& record : prepared.store_source_publications.records) {
    const auto& plan = record.plan;
    if (record.function_name != fact.function_name ||
        record.block_label != fact.source_store_block_label ||
        record.instruction_index != fact.source_store_instruction_index ||
        !prepare::prepared_store_source_publication_available(plan) ||
        plan.intent !=
            prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication ||
        plan.duplicate_publication ||
        plan.source_value_id != fact.payload_value_id ||
        plan.source_value_name != fact.payload_value_name ||
        plan.source_value != fact.payload_value ||
        plan.destination_frame_slot_id != fact.destination_frame_slot_id ||
        plan.destination_stack_offset_bytes != fact.destination_stack_offset_bytes ||
        plan.destination_size_bytes != fact.destination_size_bytes) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &record;
  }
  return selected;
}

[[nodiscard]] const prepare::PreparedCallArgumentValuePublicationFact*
find_unique_prepared_call_argument_value_publication_fact(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    FunctionNameId function_id,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
  if (!argument.source_selection.has_value() ||
      !argument.source_value_id.has_value() ||
      !argument.source_selection->source_value_name.has_value() ||
      !argument.source_selection->source_slot_id.has_value() ||
      !argument.source_selection->source_stack_offset_bytes.has_value() ||
      !argument.source_selection->source_size_bytes.has_value()) {
    return nullptr;
  }

  const prepare::PreparedCallArgumentValuePublicationFact* selected = nullptr;
  for (const auto& fact : prepared.call_argument_value_publications.facts) {
    if (fact.function_name != function_id ||
        fact.call_instruction_index != argument.instruction_index ||
        fact.arg_index != argument.arg_index ||
        fact.argument_value_id != *argument.source_value_id ||
        fact.argument_value_name != *argument.source_selection->source_value_name ||
        fact.argument_object_slot_id != *argument.source_selection->source_slot_id ||
        fact.argument_object_stack_offset_bytes !=
            *argument.source_selection->source_stack_offset_bytes ||
        fact.argument_object_size_bytes !=
            *argument.source_selection->source_size_bytes) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &fact;
  }
  return selected;
}

std::optional<PreparedFrameSlotAddressArgumentPublication>
prepared_frame_slot_address_call_argument_publication(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::string_view function_name,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (!argument.source_selection.has_value() ||
      argument.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
    return std::nullopt;
  }
  const auto address_offset =
      prepared_frame_slot_address_call_argument_offset(stack_layout,
                                                       lookups,
                                                       frame_plan,
                                                       argument,
                                                       stack_frame_bytes);
  if (!address_offset.has_value()) {
    return std::nullopt;
  }
  const auto need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (!need.available ||
      need.kind !=
          prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
              FrameSlotAddress ||
      need.source_selection != &*argument.source_selection ||
      need.source_value_id != argument.source_value_id ||
      !need.source_materializes_address ||
      need.may_emit_local_aggregate_address_payload) {
    return std::nullopt;
  }

  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName ||
      !argument.source_selection->source_slot_id.has_value() ||
      !argument.source_selection->source_stack_offset_bytes.has_value() ||
      !argument.source_selection->source_size_bytes.has_value() ||
      *argument.source_selection->source_size_bytes != 8) {
    return std::nullopt;
  }

  const auto* fact =
      find_unique_prepared_call_argument_value_publication_fact(
          prepared, function_id, argument);
  if (fact == nullptr ||
      fact->call_block_label !=
          *argument.source_selection->address_materialization_block_label ||
      fact->destination_frame_slot_id != *argument.source_selection->source_slot_id ||
      fact->destination_stack_offset_bytes !=
          *argument.source_selection->source_stack_offset_bytes ||
      fact->destination_size_bytes != 8 ||
      fact->payload_value_id == need.source_value_id ||
      fact->payload_value_name == *argument.source_selection->source_value_name ||
      fact->payload_value.type != bir::TypeKind::Ptr ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(fact->destination_stack_offset_bytes))) {
    return std::nullopt;
  }

  const auto* selected =
      find_matching_call_argument_value_publication_store_source(prepared, *fact);
  if (selected == nullptr) {
    return std::nullopt;
  }

  const auto& plan = selected->plan;
  if (fact->payload_value.kind != bir::Value::Kind::Named ||
      fact->payload_value.type != bir::TypeKind::Ptr ||
      fact->destination_stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      fact->payload_value_name == *argument.source_selection->source_value_name) {
    return std::nullopt;
  }

  std::optional<std::int32_t> payload_address_offset;
  std::optional<bir::Value> payload_value;
  const bool has_any_payload_route =
      fact->payload_frame_slot_id.has_value() ||
      fact->payload_stack_offset_bytes.has_value() ||
      fact->payload_size_bytes.has_value() ||
      fact->payload_align_bytes.has_value();
  const bool has_complete_payload_route =
      fact->payload_frame_slot_id.has_value() &&
      fact->payload_stack_offset_bytes.has_value() &&
      fact->payload_size_bytes.has_value() &&
      fact->payload_align_bytes.has_value();
  if (has_any_payload_route && !has_complete_payload_route) {
    return std::nullopt;
  }
  if (has_complete_payload_route) {
    if (!fits_signed_12_bit_immediate(
            static_cast<std::int64_t>(*fact->payload_stack_offset_bytes))) {
      return std::nullopt;
    }
    const auto* payload_slot =
        prepare::find_frame_slot_by_id(stack_layout, *fact->payload_frame_slot_id);
    if (payload_slot == nullptr ||
        payload_slot->function_name != function_id ||
        *fact->payload_stack_offset_bytes < payload_slot->offset_bytes ||
        *fact->payload_stack_offset_bytes >
            payload_slot->offset_bytes + payload_slot->size_bytes ||
        payload_slot->size_bytes != *fact->payload_size_bytes ||
        payload_slot->align_bytes != *fact->payload_align_bytes) {
      return std::nullopt;
    }
    payload_address_offset =
        static_cast<std::int32_t>(*fact->payload_stack_offset_bytes);
  } else if (plan.source_load_local != nullptr) {
    if (plan.source_load_local->result.type != bir::TypeKind::Ptr ||
        plan.source_load_local->result != fact->payload_value) {
      return std::nullopt;
    }
    payload_value = fact->payload_value;
  } else {
    return std::nullopt;
  }

  if (plan.source_value_id != fact->payload_value_id ||
      plan.source_value_name != fact->payload_value_name ||
      plan.source_value != fact->payload_value) {
    return std::nullopt;
  }

  return PreparedFrameSlotAddressArgumentPublication{
      .address_offset = *address_offset,
      .destination_offset =
          static_cast<std::int32_t>(fact->destination_stack_offset_bytes),
      .payload_address_offset = payload_address_offset,
      .payload_value = std::move(payload_value),
  };
}

const prepare::PreparedCallPreservedValue* exact_prior_preserved_value_for_selection(
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedCallArgumentSourceSelection& selection) {
  if (lookups == nullptr || !selection.source_value_id.has_value() ||
      !selection.preserved_call_block_index.has_value() ||
      !selection.preserved_call_instruction_index.has_value() ||
      *selection.source_value_id >=
          lookups->call_plans.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries =
      lookups->call_plans.prior_preserved_by_value[*selection.source_value_id];
  const auto it = std::find_if(
      entries.begin(),
      entries.end(),
      [&](const prepare::PreparedPriorPreservedValueEntry& entry) {
        return entry.block_index == *selection.preserved_call_block_index &&
               entry.instruction_index ==
                   *selection.preserved_call_instruction_index &&
               entry.preserved != nullptr;
      });
  return it == entries.end() ? nullptr : it->preserved;
}

bool ensure_rv64_prepared_call_outgoing_stack_area(
    RiscvEncodedFragment& fragment,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t* active_call_stack_adjustment) {
  if (active_call_stack_adjustment == nullptr ||
      !call_plan.outgoing_stack_argument_area.has_value() ||
      call_plan.outgoing_stack_argument_area->size_bytes == 0) {
    return false;
  }
  const std::size_t size_bytes =
      call_plan.outgoing_stack_argument_area->size_bytes;
  if (size_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !fits_signed_12_bit_immediate(-static_cast<std::int32_t>(size_bytes))) {
    return false;
  }
  if (*active_call_stack_adjustment == size_bytes) {
    return true;
  }
  if (*active_call_stack_adjustment != 0) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            -static_cast<std::int32_t>(size_bytes)));
  *active_call_stack_adjustment = size_bytes;
  return true;
}

bool prepared_scalar_stack_call_argument_destination_is_valid(
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type) {
  if (argument.destination_register_bank.has_value() ||
      argument.destination_register_name.has_value() ||
      argument.destination_register_placement.has_value() ||
      argument.aggregate_transport.has_value() ||
      !argument.destination_stack_offset_bytes.has_value() ||
      !argument.destination_stack_size_bytes.has_value() ||
      !call_plan.outgoing_stack_argument_area.has_value()) {
    return false;
  }
  const auto floating_size =
      argument_type == c4c::backend::bir::TypeKind::F32
          ? std::optional<std::size_t>{4}
          : argument_type == c4c::backend::bir::TypeKind::F64
                ? std::optional<std::size_t>{8}
                : std::optional<std::size_t>{};
  const auto integer_size = rv64_scalar_memory_size_for_type(argument_type);
  const std::size_t offset = *argument.destination_stack_offset_bytes;
  const std::size_t size = *argument.destination_stack_size_bytes;
  const std::size_t area_size = call_plan.outgoing_stack_argument_area->size_bytes;
  if (offset > area_size || size > area_size - offset ||
      offset >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return false;
  }
  if (rv64_floating_type(argument_type)) {
    return floating_size.has_value() && size == *floating_size &&
           argument.value_bank == prepare::PreparedRegisterBank::Fpr;
  }
  return integer_size.has_value() && size >= *integer_size && size <= 8 &&
         rv64_load_store_funct3_for_size(size).has_value() &&
         (argument.value_bank == prepare::PreparedRegisterBank::Gpr ||
          argument.value_bank == prepare::PreparedRegisterBank::None);
}

std::optional<std::int32_t> add_active_call_stack_adjustment_to_offset(
    std::int32_t offset,
    std::size_t active_call_stack_adjustment) {
  if (active_call_stack_adjustment >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      offset > std::numeric_limits<std::int32_t>::max() -
                   static_cast<std::int32_t>(active_call_stack_adjustment)) {
    return std::nullopt;
  }
  const auto adjusted =
      offset + static_cast<std::int32_t>(active_call_stack_adjustment);
  if (!fits_signed_12_bit_immediate(adjusted)) {
    return std::nullopt;
  }
  return adjusted;
}

bool append_rv64_prepared_local_frame_address_call_argument_source(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes,
    std::size_t active_call_stack_adjustment) {
  const auto offset =
      prepared_frame_slot_address_call_argument_offset(stack_layout,
                                                       lookups,
                                                       frame_plan,
                                                       argument,
                                                       stack_frame_bytes);
  const auto adjusted_offset =
      offset.has_value()
          ? add_active_call_stack_adjustment_to_offset(
                *offset, active_call_stack_adjustment)
          : std::optional<std::int32_t>{};
  if (!adjusted_offset.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination_register,
                            0,
                            2,
                            *adjusted_offset));  // addi rd, sp, off
  return true;
}

bool append_rv64_prepared_scalar_stack_call_argument(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedCallPlan& call_plan,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes,
    std::size_t* active_call_stack_adjustment) {
  namespace prepare = c4c::backend::prepare;

  if (!prepared_scalar_stack_call_argument_destination_is_valid(call_plan,
                                                               argument,
                                                               argument_type) ||
      !ensure_rv64_prepared_call_outgoing_stack_area(
          fragment, call_plan, active_call_stack_adjustment)) {
    return false;
  }

  const std::size_t destination_offset =
      *argument.destination_stack_offset_bytes;
  const std::size_t destination_size = *argument.destination_stack_size_bytes;
  if (rv64_floating_type(argument_type)) {
    constexpr std::uint32_t scratch_fpr = 30;  // ft10
    constexpr std::uint32_t scratch_gpr = 28;  // t3
    if (argument.value_bank != prepare::PreparedRegisterBank::Fpr) {
      return false;
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
        argument.source_literal.has_value()) {
      if (argument.source_literal->kind != c4c::backend::bir::Value::Kind::Immediate ||
          argument.source_literal->type != argument_type ||
          argument.source_literal->immediate_bits >
              static_cast<std::uint64_t>(
                  std::numeric_limits<std::int64_t>::max())) {
        return false;
      }
      append_rv64_load_immediate(
          fragment,
          scratch_gpr,
          static_cast<std::int64_t>(argument.source_literal->immediate_bits));
      if (!append_rv64_gpr_to_fpr_move(fragment,
                                       scratch_fpr,
                                       scratch_gpr,
                                       argument_type)) {
        return false;
      }
    } else if (argument.source_encoding ==
                   prepare::PreparedStorageEncodingKind::Register &&
               argument.source_register_bank == prepare::PreparedRegisterBank::Fpr &&
               argument.source_register_name.has_value()) {
      const auto source = rv64_fpr_register_number(*argument.source_register_name);
      if (!source.has_value() ||
          !append_rv64_fpr_move(fragment, scratch_fpr, *source, argument_type)) {
        return false;
      }
    } else {
      return false;
    }
    return append_rv64_store_fpr_to_stack(fragment,
                                         scratch_fpr,
                                         static_cast<std::int32_t>(
                                             destination_offset),
                                         argument_type);
  }

  constexpr std::uint32_t scratch_gpr = 28;  // t3
  if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument.source_literal.has_value()) {
    const auto immediate =
        integer_immediate_for_value(prepared.names, lookups, *argument.source_literal);
    if (immediate.has_value()) {
      append_rv64_load_immediate(fragment, scratch_gpr, *immediate);
    } else if (is_rv64_null_pointer_value(*argument.source_literal)) {
      append_rv64_load_immediate(fragment, scratch_gpr, 0);
    } else {
      return false;
    }
  } else if (argument.source_encoding ==
                 prepare::PreparedStorageEncodingKind::Register &&
             argument.source_register_bank == prepare::PreparedRegisterBank::Gpr &&
             argument.source_register_name.has_value()) {
    const auto source = rv64_register_number(*argument.source_register_name);
    if (!source.has_value()) {
      return false;
    }
    if (argument.source_selection.has_value() &&
        argument.source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization &&
        !append_rv64_prepared_local_frame_address_call_argument_source(
            fragment,
            stack_layout,
            lookups,
            frame_plan,
            argument,
            *source,
            stack_frame_bytes,
            *active_call_stack_adjustment)) {
      return false;
    }
    append_rv64_move(fragment, scratch_gpr, *source);
  } else if (argument.source_encoding ==
                 prepare::PreparedStorageEncodingKind::FrameSlot) {
    auto offset = prepared_frame_slot_call_argument_offset(stack_layout,
                                                          lookups,
                                                          argument,
                                                          argument_type,
                                                          stack_frame_bytes);
    if (!offset.has_value()) {
      offset = prepared_frame_slot_value_home_call_argument_offset(
          stack_layout, lookups, argument, argument_type, stack_frame_bytes);
    }
    if (!offset.has_value()) {
      offset = prepared_explicit_scalar_frame_slot_call_argument_offset(
          stack_layout, lookups, argument, argument_type, stack_frame_bytes);
    }
    const auto adjusted_offset =
        offset.has_value()
            ? add_active_call_stack_adjustment_to_offset(
                  *offset, *active_call_stack_adjustment)
            : std::optional<std::int32_t>{};
    const auto source_size = rv64_scalar_memory_size_for_type(argument_type);
    if (!adjusted_offset.has_value() || !source_size.has_value() ||
        !append_rv64_load_stack_to_register(fragment,
                                           scratch_gpr,
                                           *adjusted_offset,
                                           *source_size)) {
      return false;
    }
  } else {
    return false;
  }
  return append_rv64_store_register_to_stack(fragment,
                                            scratch_gpr,
                                            static_cast<std::int32_t>(
                                                destination_offset),
                                            destination_size);
}

bool prepared_call_argument_uses_gpr_source_register(
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    std::uint32_t source_register) {
  namespace prepare = c4c::backend::prepare;

  if (argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      argument.source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.source_register_name.has_value()) {
    return false;
  }
  const auto source = rv64_register_number(*argument.source_register_name);
  return source.has_value() && *source == source_register;
}

bool append_pending_rv64_prepared_scalar_stack_call_arguments_using_gpr_source(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedCallPlan& call_plan,
    const c4c::backend::bir::CallInst& call,
    c4c::backend::bir::TypeKind current_argument_type,
    std::size_t current_arg_index,
    std::uint32_t source_register,
    std::size_t stack_frame_bytes,
    std::size_t* active_call_stack_adjustment,
    std::vector<bool>& emitted_stack_arguments) {
  for (std::size_t arg_index = 0; arg_index < call_plan.arguments.size();
       ++arg_index) {
    if (arg_index < emitted_stack_arguments.size() &&
        emitted_stack_arguments[arg_index]) {
      continue;
    }
    const auto& argument = call_plan.arguments[arg_index];
    if (!argument.destination_stack_offset_bytes.has_value() &&
        !argument.destination_stack_size_bytes.has_value()) {
      continue;
    }
    if (!prepared_call_argument_uses_gpr_source_register(argument,
                                                        source_register)) {
      continue;
    }
    if (argument.arg_index != arg_index ||
        argument.destination_contiguous_width != 1 ||
        arg_index >= call.arg_types.size() ||
        (arg_index == current_arg_index &&
         call.arg_types[arg_index] != current_argument_type) ||
        !append_rv64_prepared_scalar_stack_call_argument(
            fragment,
            prepared,
            stack_layout,
            lookups,
            frame_plan,
            call_plan,
            argument,
            call.arg_types[arg_index],
            stack_frame_bytes,
            active_call_stack_adjustment)) {
      return false;
    }
    if (arg_index < emitted_stack_arguments.size()) {
      emitted_stack_arguments[arg_index] = true;
    }
  }
  return true;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_call(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::string_view function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedCallPlan* call_plan,
    const c4c::backend::prepare::PreparedInlineAsmCarrier* inline_asm_carrier,
    const c4c::backend::bir::CallInst& call,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (call.inline_asm.has_value()) {
    if (auto fragment =
            fragment_for_no_result_side_effect_inline_asm(inline_asm_carrier, call);
        fragment.has_value()) {
      return fragment;
    }
    if (auto fragment =
            fragment_for_empty_tied_scalar_gpr_inline_asm(inline_asm_carrier, call);
        fragment.has_value()) {
      return fragment;
    }
    if (auto fragment = fragment_for_rv64_insn_d_inline_asm(inline_asm_carrier, call);
        fragment.has_value()) {
      return fragment;
    }
    return fragment_for_rv64_insn_r_inline_asm(inline_asm_carrier, call);
  }

  if (call_plan == nullptr || call.is_indirect || call.callee_value.has_value() ||
      call_plan->is_indirect || call_plan->indirect_callee.has_value()) {
    return std::nullopt;
  }
  switch (call_plan->wrapper_kind) {
    case prepare::PreparedCallWrapperKind::SameModule:
    case prepare::PreparedCallWrapperKind::DirectExternFixedArity:
      break;
    case prepare::PreparedCallWrapperKind::DirectExternVariadic:
    case prepare::PreparedCallWrapperKind::Indirect:
      return std::nullopt;
  }
  if (!call_plan->direct_callee_name.has_value() ||
      call_plan->direct_callee_name->empty()) {
    return std::nullopt;
  }
  if (!call.callee.empty() && call.callee != *call_plan->direct_callee_name) {
    return std::nullopt;
  }

  if (call.args.size() != call_plan->arguments.size()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  const auto* before_call_bundle = prepare::find_indexed_prepared_move_bundle(
      lookups == nullptr ? nullptr : &lookups->move_bundles,
      nullptr,
      prepare::PreparedMovePhase::BeforeCall,
      block_index,
      instruction_index);
  const auto before_call_effects =
      prepare::plan_prepared_call_boundary_effects(
          *call_plan,
          before_call_bundle,
          nullptr);
  for (const auto& effect : before_call_effects) {
    if (effect.effect_kind !=
        prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation) {
      continue;
    }
    if (!append_rv64_callee_saved_gpr_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall) &&
        !append_rv64_callee_saved_fpr_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall) &&
        !append_rv64_register_source_stack_slot_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall) &&
        !append_rv64_stack_slot_source_register_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall) &&
        !rv64_noop_stack_slot_preservation_effect(
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall)) {
      return std::nullopt;
    }
  }

  std::size_t active_call_stack_adjustment = 0;
  bool emitted_memory_return_argument = false;
  std::vector<bool> emitted_stack_arguments(call_plan->arguments.size(), false);
  for (std::size_t arg_index = 0; arg_index < call_plan->arguments.size();
       ++arg_index) {
    const auto& argument = call_plan->arguments[arg_index];
    if (argument.arg_index != arg_index ||
        argument.destination_contiguous_width != 1) {
      return std::nullopt;
    }
    const auto* selection =
        argument.source_selection.has_value() ? &*argument.source_selection : nullptr;
    const auto local_materialization_route =
        selection != nullptr
            ? prepare::as_local_frame_address_materialization_route(*selection)
            : std::nullopt;
    const auto* transport = argument.aggregate_transport.has_value()
                                ? &*argument.aggregate_transport
                                : nullptr;
    if (call_plan->memory_return.has_value() &&
        call_plan->memory_return->sret_arg_index ==
            std::optional<std::size_t>{arg_index}) {
      if (active_call_stack_adjustment != 0) {
        return std::nullopt;
      }
      const auto offset =
          prepared_sret_memory_return_argument_address_offset(stack_layout,
                                                              lookups,
                                                              frame_plan,
                                                              *call_plan,
                                                              argument,
                                                              stack_frame_bytes);
      const auto destination = rv64_register_number("a0");
      if (!offset.has_value() || !destination.has_value()) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                *destination,
                                0,
                                2,
                                *offset));  // addi a0, sp, off
      emitted_memory_return_argument = true;
      continue;
    }
    if (argument.value_bank == prepare::PreparedRegisterBank::AggregateAddress ||
        transport != nullptr) {
      if (arg_index >= call.arg_abi.size() ||
          argument.value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
          argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
          argument.source_register_bank !=
              prepare::PreparedRegisterBank::Gpr ||
          argument.destination_register_bank.has_value() ||
          argument.destination_register_name.has_value() ||
          argument.destination_stack_offset_bytes !=
              std::optional<std::size_t>{0} ||
          !argument.destination_stack_size_bytes.has_value() ||
          selection == nullptr ||
          selection->kind != prepare::PreparedCallArgumentSourceSelectionKind::
                                 LocalFrameAddressMaterialization ||
          !local_materialization_route.has_value() ||
          transport == nullptr ||
          transport->kind != prepare::PreparedAggregateTransportKind::StackCopy ||
          !transport->source_stack_offset_bytes.has_value() ||
          *transport->source_stack_offset_bytes !=
              local_materialization_route->source_stack_offset_bytes ||
          transport->destination_stack_offset_bytes !=
              argument.destination_stack_offset_bytes ||
          transport->destination_stack_size_bytes !=
              argument.destination_stack_size_bytes ||
          transport->payload_size_bytes == 0 ||
          transport->copy_size_bytes == 0 ||
          transport->copy_align_bytes == 0 ||
          transport->payload_align_bytes == 0 ||
          transport->copy_size_bytes > transport->payload_size_bytes ||
          transport->copy_size_bytes > 128 ||
          !transport->lanes.empty()) {
        return std::nullopt;
      }
      const auto& abi = call.arg_abi[arg_index];
      if (abi.type != bir::TypeKind::Ptr ||
          !abi.byval_copy ||
          abi.primary_class != bir::AbiValueClass::Memory ||
          abi.size_bytes != transport->copy_size_bytes ||
          abi.align_bytes != transport->copy_align_bytes ||
          *argument.destination_stack_size_bytes != transport->copy_size_bytes ||
          !call_plan->outgoing_stack_argument_area.has_value() ||
          call_plan->outgoing_stack_argument_area->size_bytes !=
              *argument.destination_stack_size_bytes) {
        return std::nullopt;
      }
      const std::size_t stack_copy_size =
          align_riscv_stack_slot(transport->copy_size_bytes, 16);
      if (stack_copy_size == 0 ||
          stack_copy_size >
              static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
          active_call_stack_adjustment >
              std::numeric_limits<std::size_t>::max() - stack_copy_size ||
          !fits_signed_12_bit_immediate(
              -static_cast<std::int32_t>(stack_copy_size))) {
        return std::nullopt;
      }
      const std::size_t pending_stack_adjustment =
          active_call_stack_adjustment + stack_copy_size;
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                2,
                                0,
                                2,
                                -static_cast<std::int32_t>(stack_copy_size)));
      std::vector<bool> covered(transport->copy_size_bytes, false);
      for (const auto& chunk : transport->chunks) {
        if (*transport->source_stack_offset_bytes >
            std::numeric_limits<std::size_t>::max() -
                transport->payload_size_bytes) {
          return std::nullopt;
        }
        if (chunk.kind !=
                prepare::PreparedAggregateTransportChunkKind::RequiredPayload ||
            chunk.size_bytes == 0 ||
            chunk.payload_offset_bytes > transport->copy_size_bytes ||
            chunk.destination_offset_bytes > transport->copy_size_bytes ||
            chunk.size_bytes >
                transport->copy_size_bytes - chunk.payload_offset_bytes ||
            chunk.size_bytes >
                transport->copy_size_bytes - chunk.destination_offset_bytes ||
            chunk.source_offset_bytes >
                std::numeric_limits<std::size_t>::max() -
                    pending_stack_adjustment) {
          return std::nullopt;
        }
        std::size_t byte_offset = 0;
        while (byte_offset < chunk.size_bytes) {
          std::size_t width = 1;
          const std::size_t remaining = chunk.size_bytes - byte_offset;
          const std::size_t source_offset =
              chunk.source_offset_bytes + pending_stack_adjustment + byte_offset;
          const std::size_t destination_offset =
              chunk.destination_offset_bytes + byte_offset;
          if (remaining >= 8 && source_offset % 8 == 0 &&
              destination_offset % 8 == 0) {
            width = 8;
          } else if (remaining >= 4 && source_offset % 4 == 0 &&
                     destination_offset % 4 == 0) {
            width = 4;
          } else if (remaining >= 2 && source_offset % 2 == 0 &&
                     destination_offset % 2 == 0) {
            width = 2;
          }
          if (source_offset >
                  static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
              destination_offset >
                  static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
              !append_rv64_load_stack_to_register(fragment,
                                                  28,
                                                  static_cast<std::int32_t>(
                                                      source_offset),
                                                  width) ||
              !append_rv64_store_register_to_stack(fragment,
                                                   28,
                                                   static_cast<std::int32_t>(
                                                       destination_offset),
                                                   width)) {
            return std::nullopt;
          }
          for (std::size_t byte = 0; byte < width; ++byte) {
            const std::size_t destination_byte = destination_offset + byte;
            if (covered[destination_byte]) {
              return std::nullopt;
            }
            covered[destination_byte] = true;
          }
          byte_offset += width;
        }
      }
      for (bool byte_covered : covered) {
        if (!byte_covered) {
          return std::nullopt;
        }
      }
      const auto destination = rv64_register_number("a0");
      if (!destination.has_value()) {
        return std::nullopt;
      }
      append_rv64_move(fragment, *destination, 2);
      active_call_stack_adjustment = pending_stack_adjustment;
      continue;
    }
    if (argument.destination_stack_offset_bytes.has_value() ||
        argument.destination_stack_size_bytes.has_value()) {
      if (arg_index < emitted_stack_arguments.size() &&
          emitted_stack_arguments[arg_index]) {
        continue;
      }
      if (arg_index >= call.arg_types.size() ||
          !append_rv64_prepared_scalar_stack_call_argument(
              fragment,
              prepared,
              stack_layout,
              lookups,
              frame_plan,
              *call_plan,
              argument,
              call.arg_types[arg_index],
              stack_frame_bytes,
              &active_call_stack_adjustment)) {
        return std::nullopt;
      }
      if (arg_index < emitted_stack_arguments.size()) {
        emitted_stack_arguments[arg_index] = true;
      }
      continue;
    }
    if (!argument.destination_register_bank.has_value()) {
      return std::nullopt;
    }
    if (*argument.destination_register_bank == prepare::PreparedRegisterBank::Fpr) {
      if (arg_index < call.arg_types.size()) {
        RiscvEncodedFragment fpr_immediate_probe;
        if (append_rv64_fpr_immediate_call_argument(fpr_immediate_probe,
                                                    prepared.target_profile,
                                                    argument,
                                                    call.arg_types[arg_index])) {
          if (!append_pending_rv64_prepared_scalar_stack_call_arguments_using_gpr_source(
                  fragment,
                  prepared,
                  stack_layout,
                  lookups,
                  frame_plan,
                  *call_plan,
                  call,
                  call.arg_types[arg_index],
                  arg_index,
                  kRv64FprImmediateCallArgumentScratchGpr,
                  stack_frame_bytes,
                  &active_call_stack_adjustment,
                  emitted_stack_arguments) ||
              !append_rv64_fpr_immediate_call_argument(fragment,
                                                       prepared.target_profile,
                                                       argument,
                                                       call.arg_types[arg_index])) {
            return std::nullopt;
          }
          continue;
        }
      }
      if (argument.value_bank != prepare::PreparedRegisterBank::Fpr ||
          argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
          argument.source_register_bank != prepare::PreparedRegisterBank::Fpr ||
          !argument.source_value_id.has_value() ||
          !argument.destination_register_placement.has_value() ||
          argument.destination_register_placement->bank !=
              prepare::PreparedRegisterBank::Fpr ||
          argument.destination_register_placement->contiguous_width != 1) {
        return std::nullopt;
      }
      const auto* source_home =
          prepared_value_home_for_id(lookups, *argument.source_value_id);
      const auto source =
          source_home == nullptr ? std::nullopt : fpr_register_number_for_home(*source_home);
      const auto destination = fpr_register_number_for_abi_placement(
          prepared.target_profile, *argument.destination_register_placement);
      if (!source.has_value() || !destination.has_value() ||
          arg_index >= call.arg_types.size() ||
          !append_rv64_fpr_move(fragment,
                                *destination,
                                *source,
                                call.arg_types[arg_index])) {
        return std::nullopt;
      }
      continue;
    }
    if (*argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        !argument.destination_register_name.has_value()) {
      return std::nullopt;
    }
    const auto destination = rv64_register_number(*argument.destination_register_name);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    const auto* source_relationship =
        bir::find_call_argument_source_relationship(call, arg_index);
    const bool relationship_matches_argument_value =
        source_relationship != nullptr &&
        (!source_relationship->source_value_name.has_value() ||
         (arg_index < call.args.size() &&
          call.args[arg_index].kind == bir::Value::Kind::Named &&
          call.args[arg_index].name == *source_relationship->source_value_name));
    if (relationship_matches_argument_value &&
        source_relationship->source_encoding ==
            bir::CallArgumentSourceEncodingKind::ComputedAddress) {
      const auto symbol =
          prepared_call_relationship_object_symbol(prepared, *source_relationship);
      if (symbol.has_value()) {
        const std::string auipc_label = ".Lpcrel_call_arg_" +
                                        std::string{function_name} + "_" +
                                        std::to_string(block_index) + "_" +
                                        std::to_string(instruction_index) + "_" +
                                        std::to_string(arg_index);
        append_rv64_fragment(
            fragment,
            make_rv64_pcrel_address_fragment(
                *destination,
                std::move(symbol->first),
                auipc_label,
                symbol->second,
                *source_relationship->source_pointer_byte_delta));
        continue;
      }
    }
    if (argument.source_selection.has_value() &&
        argument.source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization) {
      if (!append_rv64_prepared_local_frame_address_call_argument_source(
              fragment,
              stack_layout,
              lookups,
              frame_plan,
              argument,
              *destination,
              stack_frame_bytes,
              active_call_stack_adjustment)) {
        return std::nullopt;
      }
      continue;
    }
    const bool scalar_gpr_frame_slot_argument =
        argument.value_bank == prepare::PreparedRegisterBank::Gpr &&
        argument.source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot;
    if (argument.source_selection.has_value() &&
        argument.source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
      const auto publication =
          prepared_frame_slot_address_call_argument_publication(prepared,
                                                                stack_layout,
                                                                lookups,
                                                                frame_plan,
                                                                function_name,
                                                                argument,
                                                                stack_frame_bytes);
      if (!publication.has_value() && scalar_gpr_frame_slot_argument) {
        // Fall through to explicit scalar frame-slot source handling below.
      } else if (!publication.has_value()) {
        return std::nullopt;
      } else {
        const std::uint32_t scratch = rv64_temporary_gpr_avoiding(*destination);
        if (publication->payload_address_offset.has_value()) {
          append_le32(fragment.bytes,
                      encode_i_type(0x13,
                                    scratch,
                                    0,
                                    2,
                                    *publication->payload_address_offset));  // addi rd, sp, off
        } else if (publication->payload_value.has_value()) {
          if (!append_rv64_move_value_to_register(fragment,
                                                  scratch,
                                                  stack_layout,
                                                  prepared.names,
                                                  lookups,
                                                  *publication->payload_value,
                                                  stack_frame_bytes)) {
            return std::nullopt;
          }
        } else {
          return std::nullopt;
        }
        if (!append_rv64_store_register_to_stack(fragment,
                                                 scratch,
                                                 publication->destination_offset,
                                                 8)) {
          return std::nullopt;
        }
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  *destination,
                                  0,
                                  2,
                                  publication->address_offset));  // addi rd, sp, off
        continue;
      }
    }
    const bool symbolic_call_argument =
        argument.source_encoding ==
            prepare::PreparedStorageEncodingKind::SymbolAddress &&
        (argument.source_symbol_name.has_value() ||
         argument.source_symbol_name_id.has_value());
    const bool computed_global_call_argument =
        argument.source_encoding ==
            prepare::PreparedStorageEncodingKind::ComputedAddress &&
        argument.source_base_value_id.has_value() &&
        argument.source_base_value_name.has_value() &&
        argument.source_pointer_byte_delta.has_value() &&
        (argument.source_symbol_name.has_value() ||
         argument.source_symbol_name_id.has_value());
    if (symbolic_call_argument || computed_global_call_argument) {
      auto symbol = prepared_call_argument_object_symbol(prepared, argument);
      if (!symbol.has_value()) {
        return std::nullopt;
      }
      const std::string auipc_label = ".Lpcrel_call_arg_" +
                                      std::string{function_name} + "_" +
                                      std::to_string(block_index) + "_" +
                                      std::to_string(instruction_index) + "_" +
                                      std::to_string(arg_index);
      append_rv64_fragment(
          fragment,
          make_rv64_pcrel_address_fragment(
              *destination,
              std::move(symbol->first),
              auipc_label,
              symbol->second,
              argument.source_pointer_byte_delta.value_or(0)));
      continue;
    }
    if (argument.source_selection.has_value()) {
      switch (argument.source_selection->kind) {
        case prepare::PreparedCallArgumentSourceSelectionKind::None:
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue:
          if (argument.source_encoding !=
              prepare::PreparedStorageEncodingKind::FrameSlot) {
            return std::nullopt;
          }
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation: {
          const auto* prior_preserved =
              exact_prior_preserved_value_for_selection(
                  lookups,
                  *argument.source_selection);
          const auto source = gpr_register_number_for_prior_preserved_selection(
              *argument.source_selection,
              prior_preserved);
          if (source.has_value()) {
            append_rv64_move(fragment, *destination, *source);
            continue;
          }
          if (argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
              arg_index >= call.arg_types.size()) {
            return std::nullopt;
          }
          const auto offset = stack_slot_offset_for_prior_preserved_gpr_selection(
              stack_layout,
              *argument.source_selection,
              prior_preserved,
              call.arg_types[arg_index],
              stack_frame_bytes);
          const auto size_bytes =
              offset.has_value()
                  ? rv64_scalar_memory_size_for_type(call.arg_types[arg_index])
                  : std::optional<std::size_t>{};
          if (!offset.has_value() || !size_bytes.has_value() ||
              !append_rv64_load_stack_to_register(fragment,
                                                 *destination,
                                                 *offset,
                                                 *size_bytes)) {
            return std::nullopt;
          }
          continue;
        }
        case prepare::PreparedCallArgumentSourceSelectionKind::
            LocalFrameAddressMaterialization:
          return std::nullopt;
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
          if (scalar_gpr_frame_slot_argument) {
            break;
          }
          return std::nullopt;
        case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
          return std::nullopt;
      }
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
        argument.source_literal.has_value()) {
      const auto immediate =
          integer_immediate_for_value({}, nullptr, *argument.source_literal);
      if (immediate.has_value()) {
        append_rv64_load_immediate(fragment, *destination, *immediate);
        continue;
      }
      if (is_rv64_null_pointer_value(*argument.source_literal)) {
        append_rv64_load_immediate(fragment, *destination, 0);
        continue;
      }
      return std::nullopt;
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Register &&
        argument.source_register_bank == prepare::PreparedRegisterBank::Gpr &&
        argument.source_register_name.has_value()) {
      const auto source = rv64_register_number(*argument.source_register_name);
      if (!source.has_value()) {
        return std::nullopt;
      }
      append_rv64_move(fragment, *destination, *source);
      continue;
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        arg_index < call.arg_types.size()) {
      auto offset =
          prepared_frame_slot_call_argument_offset(stack_layout,
                                                   lookups,
                                                   argument,
                                                   call.arg_types[arg_index],
                                                   stack_frame_bytes);
      if (!offset.has_value()) {
        offset = prepared_frame_slot_value_home_call_argument_offset(
            stack_layout,
            lookups,
            argument,
            call.arg_types[arg_index],
            stack_frame_bytes);
      }
      if (!offset.has_value() && scalar_gpr_frame_slot_argument) {
        offset = prepared_explicit_scalar_frame_slot_call_argument_offset(
            stack_layout,
            lookups,
            argument,
            call.arg_types[arg_index],
            stack_frame_bytes);
      }
      if (!offset.has_value() ||
          !append_rv64_load_stack_to_register(fragment,
                                             *destination,
                                             *offset,
                                             *rv64_scalar_memory_size_for_type(
                                                 call.arg_types[arg_index]))) {
        return std::nullopt;
      }
      continue;
    }
    return std::nullopt;
  }
  if (call_plan->memory_return.has_value() && !emitted_memory_return_argument) {
    return std::nullopt;
  }

  const auto call_offset = fragment.bytes.size();
  append_le32(fragment.bytes, encode_u_type(0x17, 1, 0));       // auipc ra, 0
  append_le32(fragment.bytes, encode_i_type(0x67, 1, 0, 1, 0));  // jalr ra, 0(ra)
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = call_offset,
      .kind = RiscvObjectFixupKind::CallPlt,
      .symbol_name = *call_plan->direct_callee_name,
      .addend = 0,
  });

  if (active_call_stack_adjustment != 0) {
    if (active_call_stack_adjustment >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
        !fits_signed_12_bit_immediate(
            static_cast<std::int32_t>(active_call_stack_adjustment))) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              2,
                              0,
                              2,
                              static_cast<std::int32_t>(
                                  active_call_stack_adjustment)));
  }

  if (call.result.has_value() != call_plan->result.has_value()) {
    return std::nullopt;
  }
  if (call_plan->result.has_value()) {
    const auto& result = *call_plan->result;
    if (result.source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        result.source_contiguous_width != 1 ||
        result.destination_contiguous_width != 1) {
      return std::nullopt;
    }
    if (result.source_register_bank == prepare::PreparedRegisterBank::Fpr ||
        result.destination_register_bank == prepare::PreparedRegisterBank::Fpr) {
      if (result.value_bank != prepare::PreparedRegisterBank::Fpr ||
          result.source_register_bank != prepare::PreparedRegisterBank::Fpr ||
          result.destination_register_bank != prepare::PreparedRegisterBank::Fpr ||
          !result.source_register_placement.has_value() ||
          result.source_register_placement->bank != prepare::PreparedRegisterBank::Fpr ||
          result.source_register_placement->contiguous_width != 1 ||
          !result.destination_value_id.has_value() || !call.result.has_value()) {
        return std::nullopt;
      }
      const auto source = fpr_register_number_for_abi_placement(
          prepared.target_profile, *result.source_register_placement);
      const auto* destination_home =
          prepared_value_home_for_id(lookups, *result.destination_value_id);
      const auto destination = destination_home == nullptr
                                   ? std::nullopt
                                   : fpr_register_number_for_home(*destination_home);
      if (!source.has_value() || !destination.has_value() ||
          !append_rv64_fpr_move(fragment, *destination, *source, call.result->type)) {
        return std::nullopt;
      }
      return fragment;
    }
    if (result.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) {
      const auto late_publication =
          prepare::find_prepared_call_result_late_publication(result);
      const bool scalar_gpr_value_bank =
          result.value_bank == prepare::PreparedRegisterBank::None ||
          result.value_bank == prepare::PreparedRegisterBank::Gpr;
      const bool scalar_gpr_source_bank =
          !result.source_register_bank.has_value() ||
          result.source_register_bank == prepare::PreparedRegisterBank::None ||
          result.source_register_bank == prepare::PreparedRegisterBank::Gpr;
      const bool source_register_publication_available =
          late_publication.source_register_publication_available ||
          (result.source_storage_kind == prepare::PreparedMoveStorageKind::Register &&
           result.source_register_name.has_value());
      if (!source_register_publication_available || !scalar_gpr_value_bank ||
          !scalar_gpr_source_bank ||
          !result.source_register_name.has_value() ||
          !result.destination_value_id.has_value() ||
          !result.destination_slot_id.has_value() ||
          !result.destination_stack_offset_bytes.has_value() ||
          !call.result.has_value()) {
        return std::nullopt;
      }
      const auto source = rv64_register_number(*result.source_register_name);
      const auto* destination_home =
          prepared_value_home_for_id(lookups, *result.destination_value_id);
      const bool pointer_result_type =
          call.result->type == c4c::backend::bir::TypeKind::Ptr;
      const bool pointer_return_type =
          call.return_type == c4c::backend::bir::TypeKind::Ptr;
      if (pointer_result_type != pointer_return_type) {
        return std::nullopt;
      }
      const bool pointer_stack_result = pointer_result_type && pointer_return_type;
      if (pointer_stack_result &&
          (!result.source_register_placement.has_value() ||
           !prepare::has_prepared_register_placement(
               *result.source_register_placement) ||
           result.source_register_placement->bank !=
               prepare::PreparedRegisterBank::Gpr ||
           result.source_register_placement->pool !=
               prepare::PreparedRegisterSlotPool::CallResult ||
           result.source_register_placement->contiguous_width != 1)) {
        return std::nullopt;
      }
      const auto size_bytes =
          pointer_stack_result
              ? std::optional<std::size_t>{8}
              : rv64_scalar_memory_size_for_type(call.result->type);
      if (!source.has_value() || destination_home == nullptr ||
          !size_bytes.has_value() ||
          destination_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
          destination_home->slot_id != result.destination_slot_id ||
          destination_home->offset_bytes != result.destination_stack_offset_bytes) {
        return std::nullopt;
      }
      const auto destination_offset = prepared_stack_slot_home_absolute_offset(
          stack_layout, *destination_home, stack_frame_bytes, *size_bytes);
      if (!destination_offset.has_value() ||
          *destination_offset != *result.destination_stack_offset_bytes ||
          !append_rv64_store_register_to_stack_offset(fragment,
                                                     *source,
                                                     *destination_offset,
                                                     *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    if (result.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
      return std::nullopt;
    }
    if (result.source_register_bank != prepare::PreparedRegisterBank::Gpr ||
        result.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        result.value_bank != prepare::PreparedRegisterBank::Gpr ||
        !result.source_register_name.has_value() ||
        !result.destination_register_name.has_value() ||
        !result.destination_value_id.has_value() ||
        !call.result.has_value() ||
        rv64_floating_type(call.result->type)) {
      return std::nullopt;
    }
    const auto source = rv64_register_number(*result.source_register_name);
    const auto plan_destination =
        rv64_register_number(*result.destination_register_name);
    const auto* destination_home =
        prepared_value_home_for_id(lookups, *result.destination_value_id);
    const auto destination =
        destination_home == nullptr ? std::nullopt
                                    : gpr_register_number_for_home(*destination_home);
    if (!source.has_value() || !plan_destination.has_value() ||
        !destination.has_value() || destination_home == nullptr ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register ||
        *destination != *plan_destination) {
      return std::nullopt;
    }
    append_rv64_move(fragment, *destination, *source);
  } else if (call.return_type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }

  const auto* after_call_bundle = prepare::find_indexed_prepared_move_bundle(
      lookups == nullptr ? nullptr : &lookups->move_bundles,
      nullptr,
      prepare::PreparedMovePhase::AfterCall,
      block_index,
      instruction_index);
  const auto after_call_effects =
      prepare::plan_prepared_call_boundary_effects(
          *call_plan,
          nullptr,
          after_call_bundle);
  for (const auto& effect : after_call_effects) {
    if (effect.effect_kind !=
        prepare::PreparedCallBoundaryEffectKind::PreservationRepublication) {
      continue;
    }
    if (!append_rv64_callee_saved_gpr_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall) &&
        !append_rv64_callee_saved_fpr_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall) &&
        !append_rv64_register_source_stack_slot_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall) &&
        !append_rv64_stack_slot_source_register_preservation_effect(
            fragment,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall) &&
        !rv64_noop_stack_slot_preservation_effect(
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall)) {
      return std::nullopt;
    }
  }

  return fragment;
}

const c4c::backend::prepare::PreparedInlineAsmCarrier* find_prepared_inline_asm_carrier(
    const c4c::backend::prepare::PreparedInlineAsmCarrierFunction* function_carriers,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == block_index && carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}


const c4c::backend::prepare::PreparedMemoryAccess*
prepared_memory_access_for_instruction(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (lookups == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
}

std::optional<c4c::ValueNameId> prepared_named_value_id(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

const c4c::backend::prepare::PreparedMemoryAccess*
unique_prepared_memory_access_for_value_in_block(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name) {
  if (lookups == nullptr || block_label == c4c::kInvalidBlockLabel ||
      (!result_value_name.has_value() && !stored_value_name.has_value())) {
    return nullptr;
  }

  const c4c::backend::prepare::PreparedMemoryAccess* selected = nullptr;
  for (const auto& entry : lookups->memory_accesses.accesses_by_position) {
    const auto* access = entry.second;
    if (access == nullptr || access->block_label != block_label ||
        access->result_value_name != result_value_name ||
        access->stored_value_name != stored_value_name) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = access;
  }
  return selected;
}

const c4c::backend::prepare::PreparedMemoryAccess*
prepared_memory_access_for_local_instruction(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::LoadLocalInst& load) {
  const auto* access =
      prepared_memory_access_for_instruction(lookups, block_label, instruction_index);
  if (access != nullptr) {
    return access;
  }
  return unique_prepared_memory_access_for_value_in_block(
      lookups,
      block_label,
      prepared_named_value_id(names, load.result),
      std::nullopt);
}

const c4c::backend::prepare::PreparedMemoryAccess*
prepared_memory_access_for_local_instruction(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::StoreLocalInst& store) {
  const auto* access =
      prepared_memory_access_for_instruction(lookups, block_label, instruction_index);
  if (access != nullptr) {
    return access;
  }
  return unique_prepared_memory_access_for_value_in_block(
      lookups,
      block_label,
      std::nullopt,
      prepared_named_value_id(names, store.value));
}

std::optional<std::size_t> rv64_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return std::size_t{1};
    case c4c::backend::bir::TypeKind::I16:
      return std::size_t{2};
    case c4c::backend::bir::TypeKind::I32:
      return std::size_t{4};
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return std::size_t{8};
    default:
      return std::nullopt;
  }
}

std::optional<unsigned> rv64_integer_type_bits(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return 8U;
    case c4c::backend::bir::TypeKind::I16:
      return 16U;
    case c4c::backend::bir::TypeKind::I32:
      return 32U;
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return 64U;
    default:
      return std::nullopt;
  }
}

bool rv64_fixed_integer_type(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
      return true;
    default:
      return false;
  }
}

bool rv64_floating_type(c4c::backend::bir::TypeKind type) {
  return type == c4c::backend::bir::TypeKind::F32 ||
         type == c4c::backend::bir::TypeKind::F64;
}

std::optional<std::int32_t> rv64_va_start_destination_load_offset(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr || load.result.type != c4c::backend::bir::TypeKind::Ptr ||
      load.slot_name.empty() ||
      access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset != 0 ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes) {
    return std::nullopt;
  }

  const auto value_name = prepared.names.value_names.find(load.slot_name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr ||
      !rv64_variadic_helper_free_entry_contract_is_complete(*entry_plan)) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* selected = nullptr;
  const prepare::PreparedVariadicVaStartOperandHomes* selected_payload = nullptr;
  for (const auto& homes : entry_plan->helper_operand_homes) {
    const auto* payload =
        prepare::find_prepared_variadic_va_start_operand_homes(homes);
    if (homes.helper != prepare::PreparedVariadicEntryHelperKind::VaStart ||
        homes.block_index != block_index ||
        homes.instruction_index >= instruction_index ||
        payload == nullptr ||
        payload->destination_va_list_address.kind !=
            prepare::PreparedValueHomeKind::Register ||
        payload->destination_va_list_address.value_name != value_name ||
        payload->destination_va_list.kind != prepare::PreparedValueHomeKind::StackSlot ||
        !payload->destination_va_list.size_bytes.has_value() ||
        *payload->destination_va_list.size_bytes != size_bytes) {
      continue;
    }
    if (selected == nullptr ||
        homes.instruction_index > selected->instruction_index) {
      selected = &homes;
      selected_payload = payload;
    }
  }
  if (selected == nullptr || selected_payload == nullptr) {
    return std::nullopt;
  }
  return prepared_stack_slot_home_offset(prepared.stack_layout,
                                         selected_payload->destination_va_list,
                                         stack_frame_bytes,
                                         size_bytes);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_frame_address_materialization(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  if (binary.opcode != c4c::backend::bir::BinaryOpcode::Add ||
      binary.result.type != c4c::backend::bir::TypeKind::Ptr ||
      binary.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto destination = gpr_register_number_for_value(names, lookups, binary.result);
  if (!destination.has_value()) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(binary.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  if (lookups == nullptr) {
    return std::nullopt;
  }
  std::vector<const c4c::backend::prepare::PreparedAddressMaterialization*>
      materialization_candidates;
  if (const auto* materializations =
          c4c::backend::prepare::find_indexed_prepared_address_materializations(
              &lookups->address_materializations,
              block_label)) {
    materialization_candidates = *materializations;
  } else {
    for (const auto& entry : lookups->address_materializations.materializations_by_block) {
      materialization_candidates.insert(materialization_candidates.end(),
                                        entry.second.begin(),
                                        entry.second.end());
    }
  }
  const c4c::backend::prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : materialization_candidates) {
    if (materialization == nullptr ||
        materialization->inst_index != instruction_index ||
        materialization->kind !=
            c4c::backend::prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->result_value_name != result_value_name ||
        !materialization->frame_slot_id.has_value() ||
        materialization->byte_offset < 0 ||
        materialization->address_space != c4c::backend::bir::AddressSpace::Default) {
      continue;
    }
    if (selected != nullptr) {
      return std::nullopt;
    }
    selected = materialization;
  }
  if (selected == nullptr) {
    return std::nullopt;
  }
  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *selected->frame_slot_id &&
                            (selected->function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == selected->function_name);
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      selected->byte_offset < static_cast<std::int64_t>(slot_it->offset_bytes)) {
    return std::nullopt;
  }
  const auto offset = static_cast<std::size_t>(selected->byte_offset);
  if (slot_it->offset_bytes >
          std::numeric_limits<std::size_t>::max() - slot_it->size_bytes ||
      offset > slot_it->offset_bytes + slot_it->size_bytes ||
      offset > stack_frame_bytes) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  if (!append_rv64_stack_offset_address_to_register(
          fragment, *destination, offset)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<std::uint32_t> rv64_temporary_gpr_avoiding(
    std::uint32_t first,
    std::optional<std::uint32_t> second) {
  constexpr std::array<std::uint32_t, 3> candidates = {6, 7, 28};  // t1, t2, t3
  for (const auto candidate : candidates) {
    if (candidate != first && (!second.has_value() || candidate != *second)) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_pointer_result_frame_address_materialization(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;

  if (lookups == nullptr ||
      binary.opcode != bir::BinaryOpcode::Add ||
      binary.result.type != bir::TypeKind::Ptr ||
      binary.result.kind != bir::Value::Kind::Named ||
      binary.lhs.type != bir::TypeKind::Ptr ||
      binary.lhs.kind != bir::Value::Kind::Named ||
      binary.rhs.type == bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  const auto lhs_value_name = names.value_names.find(binary.lhs.name);
  if (lhs_value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* base_home = prepared_value_home_for(names, lookups, binary.lhs);
  const auto base_register =
      base_home == nullptr ? std::nullopt : gpr_register_number_for_home(*base_home);
  if (base_home == nullptr || !base_register.has_value()) {
    return std::nullopt;
  }
  const auto* result_home = prepared_value_home_for(names, lookups, binary.result);
  if (result_home == nullptr) {
    return std::nullopt;
  }

  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups->address_materializations,
          block_label);
  if (materializations == nullptr) {
    return std::nullopt;
  }
  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != instruction_index) {
      continue;
    }
    const bool identifies_base =
        materialization->result_value_name ==
            std::optional<c4c::ValueNameId>{lhs_value_name} ||
        (materialization->result_value_id.has_value() &&
         *materialization->result_value_id == base_home->value_id);
    if (!identifies_base) {
      continue;
    }
    if (materialization->kind !=
            prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->address_space != bir::AddressSpace::Default ||
        materialization->is_thread_local ||
        materialization->has_tls_address_space ||
        !materialization->frame_slot_id.has_value() ||
        materialization->byte_offset < 0 ||
        (materialization->result_home_kind.has_value() &&
         *materialization->result_home_kind != base_home->kind)) {
      return std::nullopt;
    }
    if (selected != nullptr) {
      return std::nullopt;
    }
    selected = materialization;
  }
  if (selected == nullptr) {
    return std::nullopt;
  }
  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *selected->frame_slot_id &&
                            (selected->function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == selected->function_name);
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      selected->byte_offset < static_cast<std::int64_t>(slot_it->offset_bytes)) {
    return std::nullopt;
  }
  const auto address_offset = static_cast<std::size_t>(selected->byte_offset);
  if (slot_it->offset_bytes >
          std::numeric_limits<std::size_t>::max() - slot_it->size_bytes ||
      address_offset > slot_it->offset_bytes + slot_it->size_bytes ||
      address_offset > stack_frame_bytes) {
    return std::nullopt;
  }

  const auto* offset_home = prepared_value_home_for(names, lookups, binary.rhs);
  const auto offset_register_home =
      offset_home == nullptr ? std::nullopt : gpr_register_number_for_home(*offset_home);
  const auto offset_stack_home =
      offset_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *offset_home,
                                                     stack_frame_bytes,
                                                     8);
  const auto offset_immediate = integer_immediate_for_value(names, lookups, binary.rhs);
  if (!offset_register_home.has_value() && !offset_stack_home.has_value() &&
      !offset_immediate.has_value()) {
    return std::nullopt;
  }

  const auto destination_register = gpr_register_number_for_home(*result_home);
  const auto destination_stack_offset =
      destination_register.has_value()
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *result_home,
                                                     stack_frame_bytes,
                                                     8);
  if (!destination_register.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  std::uint32_t offset_register = 0;
  if (offset_register_home.has_value()) {
    offset_register = *offset_register_home;
  } else {
    const auto scratch = rv64_temporary_gpr_avoiding(*base_register, destination_register);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    offset_register = *scratch;
    if (offset_stack_home.has_value()) {
      if (!append_rv64_load_stack_offset_to_register(
              fragment,
              offset_register,
              *offset_stack_home,
              8)) {
        return std::nullopt;
      }
    } else {
      append_rv64_load_immediate(fragment, offset_register, *offset_immediate);
    }
  }

  std::uint32_t result_register = 0;
  if (destination_register.has_value()) {
    result_register = *destination_register;
  } else {
    const auto scratch =
        rv64_temporary_gpr_avoiding(*base_register, std::optional{offset_register});
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = *scratch;
  }
  append_rv64_add_registers(
      fragment, result_register, *base_register, offset_register);
  if (destination_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack_offset(
          fragment,
          result_register,
          *destination_stack_offset,
          8)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_pointer_add(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;

  if (lookups == nullptr ||
      binary.opcode != bir::BinaryOpcode::Add ||
      binary.result.type != bir::TypeKind::Ptr ||
      binary.result.kind != bir::Value::Kind::Named ||
      binary.lhs.type != bir::TypeKind::Ptr ||
      binary.rhs.type == bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto* base_home = prepared_value_home_for(names, lookups, binary.lhs);
  const auto* offset_home = prepared_value_home_for(names, lookups, binary.rhs);
  const auto* result_home = prepared_value_home_for(names, lookups, binary.result);
  if (base_home == nullptr ||
      result_home == nullptr ||
      offset_home == nullptr) {
    return std::nullopt;
  }

  const auto result_register =
      explicit_gpr_register_for_home(target_profile,
                                     names,
                                     function,
                                     storage_plan,
                                     *result_home);
  const auto result_stack_offset =
      result_register.has_value()
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *result_home,
                                                     stack_frame_bytes,
                                                     8);
  if (!result_register.has_value() && !result_stack_offset.has_value()) {
    return std::nullopt;
  }

  const auto lhs_register =
      explicit_gpr_register_for_home(target_profile,
                                     names,
                                     function,
                                     storage_plan,
                                     *base_home);
  if (!lhs_register.has_value()) {
    return std::nullopt;
  }
  const auto rhs_register =
      explicit_gpr_register_for_home(target_profile,
                                     names,
                                     function,
                                     storage_plan,
                                     *offset_home);
  if ((base_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
       !lhs_register.has_value()) ||
      (offset_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
       !rhs_register.has_value()) ||
      (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
       !result_register.has_value())) {
    return std::nullopt;
  }
  const auto rhs_stack_offset =
      rhs_register.has_value()
          ? std::nullopt
          : prepared_gpr_stack_home_absolute_offset(stack_layout,
                                                    storage_plan,
                                                    *offset_home,
                                                    stack_frame_bytes,
                                                    8);
  if (lhs_register.has_value() && result_register.has_value() &&
      rhs_stack_offset.has_value() && *lhs_register != *result_register) {
    RiscvEncodedFragment fragment;
    if (!append_rv64_load_stack_offset_to_register(fragment,
                                                   *result_register,
                                                   *rhs_stack_offset,
                                                   8)) {
      return std::nullopt;
    }
    append_rv64_add_registers(fragment,
                              *result_register,
                              *lhs_register,
                              *result_register);
    return fragment;
  }

  auto destination_register = result_register;
  if (!destination_register.has_value()) {
    destination_register = rv64_unoccupied_temporary_gpr_avoiding(
        lookups,
        {lhs_register.value_or(32), rhs_register.value_or(32)});
  }
  if (!destination_register.has_value()) {
    return std::nullopt;
  }

  auto base_register = lhs_register;

  auto offset_register = rhs_register;
  if (!offset_register.has_value()) {
    offset_register = rv64_unoccupied_temporary_gpr_avoiding(
        lookups,
        {*destination_register, *base_register});
  }
  if (!offset_register.has_value()) {
    return std::nullopt;
  }

  if (destination_register == base_register || destination_register == offset_register ||
      base_register == offset_register) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!lhs_register.has_value() &&
      !append_rv64_move_value_to_register(fragment,
                                          *base_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.lhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  if (!rhs_register.has_value() &&
      !append_rv64_move_value_to_register(fragment,
                                          *offset_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }

  append_rv64_add_registers(fragment,
                            *destination_register,
                            *base_register,
                            *offset_register);
  if (result_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack_offset(fragment,
                                                 *destination_register,
                                                 *result_stack_offset,
                                                 8)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<unsigned> rv64_narrow_integer_bit_width(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return 8;
    case c4c::backend::bir::TypeKind::I16:
      return 16;
    default:
      return std::nullopt;
  }
}

void append_rv64_zero_extend_narrow_gpr(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination,
                                        std::uint32_t source,
                                        unsigned bit_width) {
  const auto shift = static_cast<std::int32_t>(64 - bit_width);
  append_le32(fragment.bytes,
              encode_i_type(0x13, destination, 1, source, shift));
  append_le32(fragment.bytes,
              encode_i_type(0x13, destination, 5, destination, shift));
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_narrow_bitfield_binary(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;

  const auto bit_width = rv64_narrow_integer_bit_width(binary.result.type);
  if (!bit_width.has_value() ||
      binary.operand_type != binary.result.type ||
      binary.lhs.type != binary.result.type ||
      (binary.opcode != bir::BinaryOpcode::Add &&
       binary.opcode != bir::BinaryOpcode::LShr &&
       binary.opcode != bir::BinaryOpcode::Shl &&
       binary.opcode != bir::BinaryOpcode::And &&
       binary.opcode != bir::BinaryOpcode::Or)) {
    return std::nullopt;
  }
  if ((binary.opcode == bir::BinaryOpcode::And ||
       binary.opcode == bir::BinaryOpcode::Or) &&
      binary.rhs.type != binary.result.type) {
    return std::nullopt;
  }

  const auto result_size_bytes =
      rv64_scalar_memory_size_for_type(binary.result.type);
  if (!result_size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home =
      prepared_value_home_for(names, lookups, binary.result);
  const auto destination =
      destination_home == nullptr
          ? std::nullopt
          : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *destination_home,
                                                     stack_frame_bytes,
                                                     *result_size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  const auto scratch =
      destination.has_value() ? destination : rv64_unoccupied_temporary_gpr(lookups);
  if (!scratch.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          *scratch,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.lhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_zero_extend_narrow_gpr(fragment, *scratch, *scratch, *bit_width);
  if (binary.opcode == bir::BinaryOpcode::Add) {
    const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);
    if (!rhs_immediate.has_value() ||
        (*rhs_immediate != 1 && *rhs_immediate != -1) ||
        binary.rhs.type != binary.result.type) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              *scratch,
                              0,
                              *scratch,
                              static_cast<std::int32_t>(*rhs_immediate)));
    append_rv64_zero_extend_narrow_gpr(fragment, *scratch, *scratch, *bit_width);
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   *scratch,
                                                   *destination_stack_offset,
                                                   *result_size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  if (binary.opcode == bir::BinaryOpcode::LShr ||
      binary.opcode == bir::BinaryOpcode::Shl) {
    const auto shift = integer_immediate_for_value(names, lookups, binary.rhs);
    if (!shift.has_value() || *shift < 0 ||
        *shift >= static_cast<std::int64_t>(*bit_width)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              *scratch,
                              binary.opcode == bir::BinaryOpcode::LShr ? 5 : 1,
                              *scratch,
                              static_cast<std::int32_t>(*shift)));
    append_rv64_zero_extend_narrow_gpr(fragment, *scratch, *scratch, *bit_width);
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   *scratch,
                                                   *destination_stack_offset,
                                                   *result_size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }

  const std::uint32_t funct3 =
      binary.opcode == bir::BinaryOpcode::And ? 7U : 6U;
  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);
  if (rhs_immediate.has_value() &&
      fits_signed_12_bit_immediate(*rhs_immediate)) {
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              *scratch,
                              funct3,
                              *scratch,
                              static_cast<std::int32_t>(*rhs_immediate)));
    append_rv64_zero_extend_narrow_gpr(fragment, *scratch, *scratch, *bit_width);
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   *scratch,
                                                   *destination_stack_offset,
                                                   *result_size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto rhs_scratch =
      rv64_unoccupied_temporary_gpr_avoiding(lookups, *scratch);
  if (!rhs_scratch.has_value() ||
      !append_rv64_move_value_to_register(fragment,
                                          *rhs_scratch,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_zero_extend_narrow_gpr(fragment,
                                     *rhs_scratch,
                                     *rhs_scratch,
                                     *bit_width);
  append_le32(fragment.bytes,
              encode_r_type(0x33, *scratch, funct3, *scratch, *rhs_scratch, 0));
  append_rv64_zero_extend_narrow_gpr(fragment, *scratch, *scratch, *bit_width);
  if (destination_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack_offset(fragment,
                                                 *scratch,
                                                 *destination_stack_offset,
                                                 *result_size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}


bool rv64_select_edge_binary_operand_is_register_or_immediate(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (is_rv64_null_pointer_value(value)) {
    return true;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, value);
  if (immediate.has_value()) {
    return fits_signed_12_bit_immediate(*immediate);
  }
  return gpr_register_number_for_value(names, lookups, value).has_value();
}

const prepare::PreparedDependencyOperandAuthorityRecord*
find_available_rv64_select_edge_cast_dependency_authority(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    c4c::backend::prepare::PreparedDependencyOperandRole role,
    const c4c::backend::bir::Value& dependency_operand) {
  if (dependency_operand_authorities == nullptr ||
      dependency_operand.kind != c4c::backend::bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto dependency_name = names.value_names.find(dependency_operand.name);
  if (dependency_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& record : dependency_operand_authorities->records) {
    const auto& authority = record.authority;
    if (record.function_name != function_name ||
        record.predecessor_label != publication.predecessor_label ||
        record.successor_label != publication.successor_label ||
        record.source_producer_kind !=
            prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
        record.source_producer_kind != publication.source_producer_kind ||
        record.source_producer_block_label != publication.source_producer_block_label ||
        record.source_producer_instruction_index !=
            publication.source_producer_instruction_index ||
        authority.policy !=
            prepare::PreparedDependencyOperandMaterializationPolicy::
                RematerializeCastFromSource ||
        !prepare::prepared_dependency_operand_authority_available(authority) ||
        authority.operand_role != role ||
        authority.destination_value_id != publication.destination_value_id ||
        authority.edge_source_value_id != publication.source_value_id ||
        authority.dependency_value_name != dependency_name ||
        !authority.cast_source_value_id.has_value() ||
        !record.cast_producer_block_label.has_value() ||
        !record.cast_producer_instruction_index.has_value()) {
      continue;
    }
    if (record.cast_source_home_kind !=
            prepare::PreparedValueHomeKind::Register &&
        record.cast_source_home_kind !=
            prepare::PreparedValueHomeKind::RematerializableImmediate) {
      continue;
    }
    return &record;
  }
  return nullptr;
}

bool rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    c4c::backend::prepare::PreparedDependencyOperandRole role,
    const c4c::backend::bir::Value& value) {
  return rv64_select_edge_binary_operand_is_register_or_immediate(names,
                                                                  lookups,
                                                                  value) ||
         find_available_rv64_select_edge_cast_dependency_authority(
             names,
             function_name,
             dependency_operand_authorities,
             publication,
             role,
             value) != nullptr;
}

bool rv64_select_edge_binary_has_available_cast_dependency_authority(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::bir::BinaryInst& binary) {
  return find_available_rv64_select_edge_cast_dependency_authority(
             names,
             function_name,
             dependency_operand_authorities,
             publication,
             prepare::PreparedDependencyOperandRole::Lhs,
             binary.lhs) != nullptr ||
         find_available_rv64_select_edge_cast_dependency_authority(
             names,
             function_name,
             dependency_operand_authorities,
             publication,
             prepare::PreparedDependencyOperandRole::Rhs,
             binary.rhs) != nullptr;
}

bool append_rv64_compare_registers_to_register(
    RiscvEncodedFragment& fragment,
    c4c::backend::bir::BinaryOpcode opcode,
    std::uint32_t destination_register,
    std::uint32_t lhs_register,
    std::uint32_t rhs_register) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                4,
                                lhs_register,
                                rhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                3,
                                destination_register,
                                1));
      return true;
    case c4c::backend::bir::BinaryOpcode::Ne:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                4,
                                lhs_register,
                                rhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                3,
                                0,
                                destination_register,
                                0));
      return true;
    case c4c::backend::bir::BinaryOpcode::Slt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                2,
                                lhs_register,
                                rhs_register,
                                0));
      return true;
    case c4c::backend::bir::BinaryOpcode::Sgt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                2,
                                rhs_register,
                                lhs_register,
                                0));
      return true;
    case c4c::backend::bir::BinaryOpcode::Sle:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                2,
                                rhs_register,
                                lhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return true;
    case c4c::backend::bir::BinaryOpcode::Sge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                2,
                                lhs_register,
                                rhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return true;
    case c4c::backend::bir::BinaryOpcode::Ult:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                3,
                                lhs_register,
                                rhs_register,
                                0));
      return true;
    case c4c::backend::bir::BinaryOpcode::Ugt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                3,
                                rhs_register,
                                lhs_register,
                                0));
      return true;
    case c4c::backend::bir::BinaryOpcode::Ule:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                3,
                                rhs_register,
                                lhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return true;
    case c4c::backend::bir::BinaryOpcode::Uge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                3,
                                lhs_register,
                                rhs_register,
                                0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return true;
    default:
      return false;
  }
}

std::optional<std::uint32_t> rv64_fp_compare_funct7(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x50;
    case c4c::backend::bir::TypeKind::F64:
      return 0x51;
    default:
      return std::nullopt;
  }
}

bool is_rv64_zero_floating_immediate(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         rv64_floating_type(value.type) &&
         value.immediate == 0 && value.immediate_bits == 0;
}

std::optional<std::int64_t> rv64_fp_immediate_bits_as_i64(
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      (value.type != c4c::backend::bir::TypeKind::F32 &&
       value.type != c4c::backend::bir::TypeKind::F64)) {
    return std::nullopt;
  }
  std::int64_t bits = 0;
  if (value.type == c4c::backend::bir::TypeKind::F32) {
    std::int32_t f32_bits = 0;
    const auto raw_bits = static_cast<std::uint32_t>(value.immediate_bits);
    std::memcpy(&f32_bits, &raw_bits, sizeof(f32_bits));
    bits = f32_bits;
  } else {
    std::memcpy(&bits, &value.immediate_bits, sizeof(bits));
  }
  return bits;
}

std::optional<std::uint32_t> rv64_fpr_compare_operand_existing_register(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (const auto reg = fpr_register_number_for_value(names, lookups, value);
      reg.has_value()) {
    return reg;
  }
  if (const auto* home = prepared_value_home_for(names, lookups, value);
      home != nullptr && home->register_name.has_value()) {
    if (const auto reg = rv64_fpr_register_number(*home->register_name);
        reg.has_value()) {
      return reg;
    }
  }
  return std::nullopt;
}

std::optional<std::uint32_t> rv64_fpr_compare_operand_register(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::uint32_t scratch_fpr,
    std::optional<std::uint32_t> scratch_gpr) {
  if (const auto reg = rv64_fpr_compare_operand_existing_register(names,
                                                                  lookups,
                                                                  value);
      reg.has_value()) {
    return reg;
  }
  const auto bits = rv64_fp_immediate_bits_as_i64(value);
  if (!bits.has_value()) {
    return std::nullopt;
  }
  if (*bits == 0 && !scratch_gpr.has_value()) {
    if (!append_rv64_gpr_to_fpr_move(fragment, scratch_fpr, 0, value.type)) {
      return std::nullopt;
    }
    return scratch_fpr;
  }
  if (!scratch_gpr.has_value()) {
    return std::nullopt;
  }
  append_rv64_load_immediate(fragment, *scratch_gpr, *bits);
  if (!append_rv64_gpr_to_fpr_move(fragment, scratch_fpr, *scratch_gpr, value.type)) {
    return std::nullopt;
  }
  return scratch_fpr;
}

std::optional<std::uint32_t> rv64_compare_scratch_fpr_avoiding(
    std::optional<std::uint32_t> lhs_home_reg,
    std::optional<std::uint32_t> rhs_home_reg,
    std::optional<std::uint32_t> first_scratch = std::nullopt) {
  constexpr std::array<std::uint32_t, 3> scratch_fpr_candidates = {31, 30, 29};
  const auto scratch_fpr_it =
      std::find_if(scratch_fpr_candidates.begin(),
                   scratch_fpr_candidates.end(),
                   [&](std::uint32_t candidate) {
                     return (!lhs_home_reg.has_value() || *lhs_home_reg != candidate) &&
                            (!rhs_home_reg.has_value() || *rhs_home_reg != candidate) &&
                            (!first_scratch.has_value() || *first_scratch != candidate);
                   });
  if (scratch_fpr_it == scratch_fpr_candidates.end()) {
    return std::nullopt;
  }
  return *scratch_fpr_it;
}

bool append_rv64_fp_compare_to_register(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::bir::BinaryOpcode opcode,
    c4c::backend::bir::TypeKind operand_type,
    const c4c::backend::bir::Value& lhs_value,
    const c4c::backend::bir::Value& rhs_value,
    std::uint32_t destination_register);

std::optional<RiscvEncodedFragment> fragment_for_prepared_fp_compare_publication(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary) {
  if (binary.result.type != c4c::backend::bir::TypeKind::I32 ||
      binary.operand_type != binary.lhs.type ||
      binary.operand_type != binary.rhs.type) {
    return std::nullopt;
  }

  const auto* destination_home = prepared_value_home_for(names, lookups, binary.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_fp_compare_to_register(fragment,
                                          names,
                                          lookups,
                                          binary.opcode,
                                          binary.operand_type,
                                          binary.lhs,
                                          binary.rhs,
                                          *destination)) {
    return std::nullopt;
  }
  return fragment;
}

bool append_rv64_fp_compare_to_register(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::bir::BinaryOpcode opcode,
    c4c::backend::bir::TypeKind operand_type,
    const c4c::backend::bir::Value& lhs_value,
    const c4c::backend::bir::Value& rhs_value,
    std::uint32_t destination_register) {
  if (operand_type != lhs_value.type || operand_type != rhs_value.type) {
    return false;
  }
  const auto funct7 = rv64_fp_compare_funct7(operand_type);
  if (!funct7.has_value()) {
    return false;
  }

  const auto lhs_home_reg =
      rv64_fpr_compare_operand_existing_register(names, lookups, lhs_value);
  const auto rhs_home_reg =
      rv64_fpr_compare_operand_existing_register(names, lookups, rhs_value);
  const bool lhs_needs_materialization =
      !lhs_home_reg.has_value() &&
      rv64_fp_immediate_bits_as_i64(lhs_value).has_value();
  const bool rhs_needs_materialization =
      !rhs_home_reg.has_value() &&
      rv64_fp_immediate_bits_as_i64(rhs_value).has_value();
  const auto scratch_gpr =
      (lhs_needs_materialization || rhs_needs_materialization)
          ? rv64_unoccupied_temporary_gpr(lookups)
          : std::optional<std::uint32_t>{};
  const auto lhs_scratch_fpr =
      rv64_compare_scratch_fpr_avoiding(lhs_home_reg, rhs_home_reg);
  if (!lhs_scratch_fpr.has_value()) {
    return false;
  }
  const auto rhs_scratch_fpr =
      (lhs_needs_materialization && rhs_needs_materialization)
          ? rv64_compare_scratch_fpr_avoiding(lhs_home_reg,
                                              rhs_home_reg,
                                              lhs_scratch_fpr)
          : lhs_scratch_fpr;
  if (!rhs_scratch_fpr.has_value()) {
    return false;
  }

  const auto lhs = rv64_fpr_compare_operand_register(
      fragment, names, lookups, lhs_value, *lhs_scratch_fpr, scratch_gpr);
  const auto rhs = rv64_fpr_compare_operand_register(
      fragment, names, lookups, rhs_value, *rhs_scratch_fpr, scratch_gpr);
  if (!lhs.has_value() || !rhs.has_value()) {
    return false;
  }

  auto append_compare = [&](std::uint32_t funct3,
                            std::uint32_t lhs,
                            std::uint32_t rhs) {
    append_le32(fragment.bytes,
                encode_r_type(0x53,
                              destination_register,
                              funct3,
                              lhs,
                              rhs,
                              *funct7));
  };

  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
      append_compare(2, *lhs, *rhs);
      return true;
    case c4c::backend::bir::BinaryOpcode::Ne:
      append_compare(2, *lhs, *rhs);
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return true;
    case c4c::backend::bir::BinaryOpcode::Slt:
      append_compare(1, *lhs, *rhs);
      return true;
    case c4c::backend::bir::BinaryOpcode::Sgt:
      append_compare(1, *rhs, *lhs);
      return true;
    case c4c::backend::bir::BinaryOpcode::Sle:
      append_compare(0, *lhs, *rhs);
      return true;
    case c4c::backend::bir::BinaryOpcode::Sge:
      append_compare(0, *rhs, *lhs);
      return true;
    default:
      return false;
  }
}

bool append_rv64_materialize_cast_dependency_authority(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord& record,
    std::uint32_t destination_register) {
  if (record.cast_source_home_kind ==
      prepare::PreparedValueHomeKind::RematerializableImmediate) {
    if (!record.cast_source_immediate_i32.has_value()) {
      return false;
    }
    append_rv64_load_immediate(fragment,
                               destination_register,
                               *record.cast_source_immediate_i32);
    return true;
  }
  if (record.cast_source_home_kind == prepare::PreparedValueHomeKind::Register &&
      record.cast_source_register_name.has_value()) {
    const auto source = rv64_register_number(*record.cast_source_register_name);
    if (!source.has_value()) {
      return false;
    }
    append_rv64_move(fragment, destination_register, *source);
    return true;
  }
  return false;
}

std::optional<std::uint32_t> rv64_cast_dependency_authority_source_register(
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord& record) {
  if (record.cast_source_home_kind != prepare::PreparedValueHomeKind::Register ||
      !record.cast_source_register_name.has_value()) {
    return std::nullopt;
  }
  return rv64_register_number(*record.cast_source_register_name);
}

std::optional<std::uint32_t>
rv64_select_edge_dependency_operand_current_source_register(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord*
        authority) {
  if (authority != nullptr) {
    return rv64_cast_dependency_authority_source_register(*authority);
  }
  return gpr_register_number_for_value(names, lookups, value);
}

bool prepared_cast_is_available_select_edge_dependency_authority_source(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords&
        dependency_operand_authorities,
    const c4c::backend::bir::CastInst& cast) {
  if (cast.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto result_name = names.value_names.find(cast.result.name);
  if (result_name == c4c::kInvalidValueName) {
    return false;
  }
  const auto matching_record = std::find_if(
      dependency_operand_authorities.records.begin(),
      dependency_operand_authorities.records.end(),
      [&](const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord&
              record) {
        const auto& authority = record.authority;
        return record.function_name == function_name &&
               record.cast_producer_block_label == block_label &&
               record.cast_producer_instruction_index == instruction_index &&
               authority.policy ==
                   prepare::PreparedDependencyOperandMaterializationPolicy::
                       RematerializeCastFromSource &&
               prepare::prepared_dependency_operand_authority_available(
                   authority) &&
               authority.dependency_value_name == result_name &&
               record.source_producer_kind ==
                   prepare::PreparedEdgePublicationSourceProducerKind::Binary;
      });
  if (matching_record == dependency_operand_authorities.records.end() ||
      !matching_record->source_producer_block_label.has_value() ||
      !matching_record->source_producer_instruction_index.has_value()) {
    return false;
  }

  const auto is_authorized_source_producer_operand =
      [&](c4c::BlockLabelId use_block_label,
          std::size_t use_instruction_index,
          prepare::PreparedDependencyOperandRole role) {
        return matching_record->source_producer_block_label == use_block_label &&
               matching_record->source_producer_instruction_index ==
                   use_instruction_index &&
               matching_record->authority.operand_role == role;
      };
  const auto uses_cast_result = [&](const c4c::backend::bir::Value& value) {
    return prepared_bir_value_has_name(names, value, result_name);
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size();
       ++block_index) {
    if (block_index >= control_flow.blocks.size()) {
      return false;
    }
    const auto& block = function.blocks.at(block_index);
    const auto use_block_label = control_flow.blocks.at(block_index).block_label;
    for (std::size_t i = 0; i < block.insts.size(); ++i) {
      const auto& inst = block.insts.at(i);
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (uses_cast_result(binary->lhs) &&
            !is_authorized_source_producer_operand(
                use_block_label,
                i,
                prepare::PreparedDependencyOperandRole::Lhs)) {
          return false;
        }
        if (uses_cast_result(binary->rhs) &&
            !is_authorized_source_producer_operand(
                use_block_label,
                i,
                prepare::PreparedDependencyOperandRole::Rhs)) {
          return false;
        }
        continue;
      }
      if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
        if (uses_cast_result(select->lhs) || uses_cast_result(select->rhs) ||
            uses_cast_result(select->true_value) ||
            uses_cast_result(select->false_value)) {
          return false;
        }
        continue;
      }
      if (const auto* cast_inst = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
        if (uses_cast_result(cast_inst->operand)) {
          return false;
        }
        continue;
      }
      if (const auto* phi = std::get_if<c4c::backend::bir::PhiInst>(&inst)) {
        for (const auto& incoming : phi->incomings) {
          if (uses_cast_result(incoming.value)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
        if (call->callee_value.has_value() &&
            uses_cast_result(*call->callee_value)) {
          return false;
        }
        for (const auto& arg : call->args) {
          if (uses_cast_result(arg)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
        if (uses_cast_result(store->value)) {
          return false;
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (uses_cast_result(store->value)) {
          return false;
        }
        continue;
      }
    }
    if (block.terminator.value.has_value() &&
        uses_cast_result(*block.terminator.value)) {
      return false;
    }
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch &&
        uses_cast_result(block.terminator.condition)) {
      return false;
    }
    for (const auto& lane : block.terminator.return_lanes) {
      if (uses_cast_result(lane)) {
        return false;
      }
    }
  }
  return true;
}

bool prepared_move_bundle_is_authorized_cast_dependency_stack_publication(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (dependency_operand_authorities == nullptr ||
      move_bundle.function_name != function_name ||
      move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move_bundle.authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      !move_bundle.abi_bindings.empty() || move_bundle.moves.size() != 1 ||
      move_bundle.block_index >= function.blocks.size() ||
      move_bundle.block_index >= control_flow.blocks.size()) {
    return false;
  }
  const auto& block = function.blocks.at(move_bundle.block_index);
  const auto prepared_block_label =
      control_flow.blocks.at(move_bundle.block_index).block_label;
  if (move_bundle.instruction_index >= block.insts.size()) {
    return false;
  }
  const auto* cast =
      std::get_if<c4c::backend::bir::CastInst>(
          &block.insts.at(move_bundle.instruction_index));
  if (cast == nullptr ||
      !prepared_cast_is_available_select_edge_dependency_authority_source(
          names,
          control_flow,
          function_name,
          function,
          prepared_block_label,
          move_bundle.instruction_index,
          *dependency_operand_authorities,
          *cast)) {
    return false;
  }
  if (cast->result.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto result_name = names.value_names.find(cast->result.name);
  if (result_name == c4c::kInvalidValueName) {
    return false;
  }
  const auto matching_record = std::find_if(
      dependency_operand_authorities->records.begin(),
      dependency_operand_authorities->records.end(),
      [&](const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord&
              record) {
        const auto& authority = record.authority;
        return record.function_name == function_name &&
               record.cast_producer_block_label == prepared_block_label &&
               record.cast_producer_instruction_index ==
                   move_bundle.instruction_index &&
               authority.policy ==
                   prepare::PreparedDependencyOperandMaterializationPolicy::
                       RematerializeCastFromSource &&
               prepare::prepared_dependency_operand_authority_available(
                   authority) &&
               authority.dependency_value_name == result_name &&
               authority.cast_source_value_id.has_value() &&
               record.source_producer_kind ==
                   prepare::PreparedEdgePublicationSourceProducerKind::Binary &&
               (record.cast_source_home_kind ==
                    prepare::PreparedValueHomeKind::Register ||
                record.cast_source_home_kind ==
                    prepare::PreparedValueHomeKind::RematerializableImmediate);
      });
  if (matching_record == dependency_operand_authorities->records.end()) {
    return false;
  }
  const auto& move = move_bundle.moves.front();
  return move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.destination_kind ==
             prepare::PreparedMoveDestinationKind::Value &&
         move.destination_storage_kind ==
             prepare::PreparedMoveStorageKind::StackSlot &&
         !move.uses_cycle_temp_source &&
         !move.source_immediate_i32.has_value() &&
         !move.destination_register_name.has_value() &&
         !move.destination_register_placement.has_value() &&
         move.destination_contiguous_width == 1 &&
         move.destination_occupied_register_names.empty() &&
         move.from_value_id == *matching_record->authority.cast_source_value_id &&
         move.to_value_id == matching_record->authority.dependency_value_id;
}

bool prepared_move_bundle_is_authorized_select_edge_source_producer_suppression(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (select_edge_source_producer_placements == nullptr ||
      move_bundle.block_index >= control_flow.blocks.size()) {
    return false;
  }
  const auto block_label = control_flow.blocks[move_bundle.block_index].block_label;
  for (const auto& record : select_edge_source_producer_placements->records) {
    if (prepare::prepared_select_edge_source_producer_placement_matches_move_bundle(
            record, function_name, block_label, move_bundle)) {
      return true;
    }
  }
  return false;
}

bool prepared_before_instruction_move_bundle_requires_suppression_authority(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle) {
  if (move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move_bundle.authority_kind != prepare::PreparedMoveAuthorityKind::None) {
    return false;
  }
  return std::any_of(
      move_bundle.moves.begin(),
      move_bundle.moves.end(),
      [](const c4c::backend::prepare::PreparedMoveResolution& move) {
        return move.destination_storage_kind ==
               prepare::PreparedMoveStorageKind::Register;
      });
}

bool prepared_move_has_matching_stack_destination_register_fan_in_authority(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    const c4c::backend::prepare::PreparedStackDestinationFanInAuthorityFact*
        authority) {
  if (move_bundle.authority_kind !=
          prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn ||
      move.authority_kind !=
          prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn ||
      authority == nullptr) {
    return false;
  }
  if (authority->authority_kind !=
          prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn ||
      authority->owner != "prepared_stack_destination_register_fan_in" ||
      authority->semantics != prepare::PreparedStackDestinationFanInSemantics::
                                  SelectMaterializationPreservedStackFallback ||
      authority->destination_value_id != move.to_value_id ||
      authority->destination_home_kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      authority->source_homes.size() != move_bundle.moves.size()) {
    return false;
  }

  std::size_t register_source_count = 0;
  bool has_stack_source = false;
  bool matched_move = false;
  for (std::size_t index = 0; index < move_bundle.moves.size(); ++index) {
    const auto& bundle_move = move_bundle.moves[index];
    const auto& source = authority->source_homes[index];
    if (source.candidate_order != index ||
        source.source_value_id != bundle_move.from_value_id ||
        bundle_move.to_value_id != authority->destination_value_id) {
      return false;
    }
    if (source.source_home_kind == prepare::PreparedValueHomeKind::Register) {
      ++register_source_count;
    } else if (source.source_home_kind ==
               prepare::PreparedValueHomeKind::StackSlot) {
      has_stack_source = true;
    } else {
      return false;
    }
    if (&bundle_move == &move) {
      matched_move = true;
    }
  }
  return matched_move && register_source_count >= 2 && has_stack_source;
}

std::optional<std::string>
rv64_prepared_move_bundle_classification_failure_diagnostic(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedObjectTraversalEvent& event,
    const c4c::backend::prepare::PreparedObjectMoveBundleConsumerClassification&
        classification,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  const auto* move_bundle = classification.move_bundle;
  if (move_bundle != nullptr && move_bundle->moves.size() == 2 &&
      classification.status == prepare::PreparedObjectMoveBundleConsumerStatus::
              AmbiguousNonParallelMultiSourceStackDestination &&
      classification.parallel_copy_bundle == nullptr &&
      move_bundle->phase == prepare::PreparedMovePhase::BeforeInstruction &&
      move_bundle->authority_kind == prepare::PreparedMoveAuthorityKind::None) {
    const auto& lhs = move_bundle->moves[0];
    const auto& rhs = move_bundle->moves[1];
    const auto stack_register_move =
        [](const prepare::PreparedMoveResolution& move) {
          return move.authority_kind == prepare::PreparedMoveAuthorityKind::None &&
                 !move.source_parallel_copy_step_index.has_value() &&
                 !move.source_parallel_copy_predecessor_label.has_value() &&
                 !move.source_parallel_copy_successor_label.has_value() &&
                 move.destination_kind ==
                     prepare::PreparedMoveDestinationKind::Value &&
                 move.destination_storage_kind ==
                     prepare::PreparedMoveStorageKind::StackSlot &&
                 move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
                 !move.uses_cycle_temp_source &&
                 !move.source_immediate_i32.has_value() &&
                 move.reason == "consumer_register_to_stack";
        };
    const auto* lhs_source_home =
        prepared_value_home_for_id(lookups, lhs.from_value_id);
    const auto* lhs_destination_home =
        prepared_value_home_for_id(lookups, lhs.to_value_id);
    const auto* rhs_source_home =
        prepared_value_home_for_id(lookups, rhs.from_value_id);
    const auto* rhs_destination_home =
        prepared_value_home_for_id(lookups, rhs.to_value_id);
    const bool same_stack_destination =
        lhs.to_value_id == rhs.to_value_id ||
        (lhs_destination_home != nullptr && rhs_destination_home != nullptr &&
         lhs_destination_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
         rhs_destination_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
         ((lhs_destination_home->slot_id.has_value() &&
           rhs_destination_home->slot_id.has_value() &&
           lhs_destination_home->slot_id == rhs_destination_home->slot_id) ||
          (lhs_destination_home->offset_bytes.has_value() &&
           rhs_destination_home->offset_bytes.has_value() &&
           lhs_destination_home->offset_bytes ==
               rhs_destination_home->offset_bytes)));
    if (stack_register_move(lhs) && stack_register_move(rhs) &&
        lhs_source_home != nullptr && rhs_source_home != nullptr &&
        lhs_destination_home != nullptr && rhs_destination_home != nullptr &&
        lhs_source_home->kind == prepare::PreparedValueHomeKind::Register &&
        rhs_source_home->kind == prepare::PreparedValueHomeKind::Register &&
        lhs_destination_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
        rhs_destination_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
        lhs.from_value_id != rhs.from_value_id && same_stack_destination) {
      std::ostringstream out;
      out << "unsupported_prepared_move_bundle_classification: "
             "non-parallel register-source fan-in to one stack destination "
             "has no ordering or mutually-exclusive authority";
      out << " event_kind="
          << prepare::prepared_object_traversal_event_kind_name(event.kind);
      out << " function="
          << rv64_prepared_function_name(names, control_flow.function_name);
      out << " block_index=" << event.block_index;
      if (event.prepared_block != nullptr) {
        out << " block_label="
            << rv64_prepared_block_label(names,
                                         event.prepared_block->block_label);
      }
      out << " instruction_index=" << event.instruction_index;
      out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
      out << " authority="
          << prepare::prepared_move_authority_kind_name(
                 move_bundle->authority_kind);
      out << " move_count=" << move_bundle->moves.size();
      out << " parallel_copy=no";
      out << " move[0].from_value_id=" << lhs.from_value_id;
      out << " move[0].to_value_id=" << lhs.to_value_id;
      out << " move[0].source_home_kind="
          << prepare::prepared_value_home_kind_name(lhs_source_home->kind);
      out << " move[0].destination_home_kind="
          << prepare::prepared_value_home_kind_name(lhs_destination_home->kind);
      out << " move[1].from_value_id=" << rhs.from_value_id;
      out << " move[1].to_value_id=" << rhs.to_value_id;
      out << " move[1].source_home_kind="
          << prepare::prepared_value_home_kind_name(rhs_source_home->kind);
      out << " move[1].destination_home_kind="
          << prepare::prepared_value_home_kind_name(rhs_destination_home->kind);
      out << " diagnostic_owner=rv64_prepared_move_bundle_consumer";
      out << " fragment_status="
             "producer_authority_missing_for_register_fan_in_stack_destination";
      return out.str();
    }
  }
  if (move_bundle == nullptr || move_bundle->moves.size() != 1) {
    return std::nullopt;
  }

  const auto& move = move_bundle->moves.front();
  if (move_bundle->phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move_bundle->authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      move.reason != "consumer_stack_to_stack" ||
      move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      move.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
      move.destination_contiguous_width != 1 || move.uses_cycle_temp_source ||
      move.source_immediate_i32.has_value()) {
    return std::nullopt;
  }

  const auto* source_home =
      prepared_value_home_for_id(lookups, move.from_value_id);
  const auto* destination_home =
      prepared_value_home_for_id(lookups, move.to_value_id);
  if (source_home == nullptr || destination_home == nullptr ||
      destination_home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }

  const auto source_type =
      prepared_bir_value_type_for_name(names, function, source_home->value_name);
  const auto destination_type = prepared_bir_value_type_for_name(
      names, function, destination_home->value_name);
  if (!source_type.has_value() || !destination_type.has_value()) {
    return std::nullopt;
  }
  const auto source_size = rv64_scalar_memory_size_for_type(*source_type);
  const auto destination_size =
      rv64_scalar_memory_size_for_type(*destination_type);
  if (source_home->kind == prepare::PreparedValueHomeKind::Register) {
    if (!source_size.has_value() || !destination_size.has_value() ||
        *source_size == *destination_size) {
      return std::nullopt;
    }
  } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    if (!source_size.has_value() || !destination_size.has_value() ||
        *source_type == *destination_type) {
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }

  const bool stack_source =
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot;
  std::ostringstream out;
  out << "unsupported_prepared_move_bundle_classification: "
      << (stack_source ? "stack-source stack-destination conversion-adjacent "
                       : "register-source stack-destination conversion ")
      << "move was classified as consumer_stack_to_stack";
  out << " event_kind="
      << prepare::prepared_object_traversal_event_kind_name(event.kind);
  out << " function="
      << rv64_prepared_function_name(names, control_flow.function_name);
  out << " block_index=" << event.block_index;
  if (event.prepared_block != nullptr) {
    out << " block_label="
        << rv64_prepared_block_label(names, event.prepared_block->block_label);
  }
  out << " instruction_index=" << event.instruction_index;
  out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
  out << " authority="
      << prepare::prepared_move_authority_kind_name(move_bundle->authority_kind);
  out << " move_count=" << move_bundle->moves.size();
  out << " parallel_copy="
      << (classification.parallel_copy_bundle == nullptr ? "no" : "yes");
  out << " move[0].from_value_id=" << move.from_value_id;
  out << " move[0].to_value_id=" << move.to_value_id;
  out << " move[0].reason=" << move.reason;
  out << " move[0].source_home_kind="
      << prepare::prepared_value_home_kind_name(source_home->kind);
  out << " move[0].destination_home_kind="
      << prepare::prepared_value_home_kind_name(destination_home->kind);
  out << " move[0].source_type=" << bir::render_type(*source_type);
  out << " move[0].destination_type=" << bir::render_type(*destination_type);
  if (source_size.has_value()) {
    out << " move[0].source_size_bytes=" << *source_size;
  }
  if (destination_size.has_value()) {
    out << " move[0].destination_size_bytes=" << *destination_size;
  }
  out << " diagnostic_owner=prepared_move_bundle_classifier";
  out << " fragment_status=";
  if (stack_source) {
    out << "producer_classification_rejected_stack_source_stack_destination_"
           "conversion_adjacent_move";
  } else {
    out << "producer_classification_rejected_register_source_stack_destination_"
           "conversion";
  }
  return out.str();
}

std::string rv64_prepared_move_bundle_fragment_failure_diagnostic(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedObjectTraversalEvent& event,
    const c4c::backend::prepare::PreparedObjectMoveBundleConsumerClassification&
        classification,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements) {
  if (auto classification_diagnostic =
          rv64_prepared_move_bundle_classification_failure_diagnostic(
              names, control_flow, function, event, classification, lookups)) {
    return *classification_diagnostic;
  }

  std::ostringstream out;
  const auto* move_bundle = classification.move_bundle;
  if (move_bundle != nullptr &&
      move_bundle->authority_kind == prepare::PreparedMoveAuthorityKind::None &&
      rv64_prepared_move_bundle_has_register_fan_in_stack_destination(
          lookups, *move_bundle)) {
    out << "unsupported_prepared_move_bundle_classification: "
           "stack-destination register fan-in requires explicit prepared "
           "stack_destination_fan_in_authority fact";
    out << " event_kind="
        << prepare::prepared_object_traversal_event_kind_name(event.kind);
    out << " function="
        << rv64_prepared_function_name(names, control_flow.function_name);
    out << " block_index=" << event.block_index;
    if (event.prepared_block != nullptr) {
      out << " block_label="
          << rv64_prepared_block_label(names, event.prepared_block->block_label);
    }
    out << " instruction_index=" << event.instruction_index;
    out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
    out << " authority="
        << prepare::prepared_move_authority_kind_name(move_bundle->authority_kind);
    out << " move_count=" << move_bundle->moves.size();
    out << " diagnostic_owner=rv64_prepared_move_bundle_consumer";
    out << " fragment_status=missing_stack_destination_fan_in_authority_fact";
    return out.str();
  }
  if (move_bundle != nullptr &&
      move_bundle->authority_kind ==
          prepare::PreparedMoveAuthorityKind::StackDestinationRegisterFanIn &&
      classification.status ==
          prepare::PreparedObjectMoveBundleConsumerStatus::Available) {
    const bool has_explicit_fact =
        classification.stack_destination_fan_in_authority.has_value();
    const auto* fact =
        has_explicit_fact ? &*classification.stack_destination_fan_in_authority
                          : nullptr;
    const bool all_moves_authorized =
        has_explicit_fact &&
        std::all_of(move_bundle->moves.begin(),
                    move_bundle->moves.end(),
                    [&](const prepare::PreparedMoveResolution& move) {
                      return prepared_move_has_matching_stack_destination_register_fan_in_authority(
                          *move_bundle, move, fact);
                    });
    if (!all_moves_authorized) {
      out << "unsupported_prepared_move_bundle_classification: "
             "stack-destination register fan-in authority requires explicit "
             "prepared stack_destination_fan_in_authority fact";
      out << " event_kind="
          << prepare::prepared_object_traversal_event_kind_name(event.kind);
      out << " function="
          << rv64_prepared_function_name(names, control_flow.function_name);
      out << " block_index=" << event.block_index;
      if (event.prepared_block != nullptr) {
        out << " block_label="
            << rv64_prepared_block_label(names,
                                         event.prepared_block->block_label);
      }
      out << " instruction_index=" << event.instruction_index;
      out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
      out << " authority="
          << prepare::prepared_move_authority_kind_name(
                 move_bundle->authority_kind);
      out << " move_count=" << move_bundle->moves.size();
      out << " diagnostic_owner=rv64_prepared_move_bundle_consumer";
      out << " fragment_status="
          << (has_explicit_fact
                  ? "malformed_stack_destination_fan_in_authority_fact"
                  : "missing_stack_destination_fan_in_authority_fact");
      return out.str();
    }
    out.str("");
    out.clear();
  }

  out << "unsupported_move_bundle_target_shape: prepared move bundle requires "
         "unsupported RV64 moves";
  out << " event_kind="
      << prepare::prepared_object_traversal_event_kind_name(event.kind);
  out << " function="
      << rv64_prepared_function_name(names, control_flow.function_name);
  out << " block_index=" << event.block_index;
  if (event.prepared_block != nullptr) {
    out << " block_label="
        << rv64_prepared_block_label(names, event.prepared_block->block_label);
  }
  out << " instruction_index=" << event.instruction_index;

  if (move_bundle == nullptr) {
    out << " fragment_status=missing_move_bundle";
    return out.str();
  }

  out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
  out << " bundle_block_index=" << move_bundle->block_index;
  out << " bundle_instruction_index=" << move_bundle->instruction_index;
  out << " authority="
      << prepare::prepared_move_authority_kind_name(move_bundle->authority_kind);
  out << " move_count=" << move_bundle->moves.size();

  if (classification.parallel_copy_bundle != nullptr) {
    const auto& bundle = *classification.parallel_copy_bundle;
    out << " parallel_copy=yes";
    out << " parallel_copy_predecessor="
        << rv64_prepared_block_label(names, bundle.predecessor_label);
    out << " parallel_copy_successor="
        << rv64_prepared_block_label(names, bundle.successor_label);
    out << " parallel_copy_execution_site="
        << prepare::prepared_parallel_copy_execution_site_name(bundle.execution_site);
  } else {
    out << " parallel_copy=no";
  }

  const bool select_edge_suppression_authorized =
      prepared_move_bundle_is_authorized_select_edge_source_producer_suppression(
          control_flow,
          control_flow.function_name,
          select_edge_source_producer_placements,
          *move_bundle);
  const bool cast_stack_publication_authorized =
      prepared_move_bundle_is_authorized_cast_dependency_stack_publication(
          names,
          control_flow,
          control_flow.function_name,
          function,
          dependency_operand_authorities,
          *move_bundle);
  out << " select_edge_suppression_authorized="
      << (select_edge_suppression_authorized ? "yes" : "no");
  out << " cast_dependency_stack_publication_authorized="
      << (cast_stack_publication_authorized ? "yes" : "no");

  for (std::size_t move_index = 0; move_index < move_bundle->moves.size();
       ++move_index) {
    const auto& move = move_bundle->moves[move_index];
    const std::string move_prefix =
        " move[" + std::to_string(move_index) + "]";
    out << move_prefix << ".from_value_id=" << move.from_value_id;
    out << move_prefix << ".to_value_id=" << move.to_value_id;
    out << move_prefix << ".destination_kind="
        << rv64_prepared_move_destination_kind_name(move.destination_kind);
    out << move_prefix << ".destination_storage="
        << rv64_prepared_move_storage_kind_name(move.destination_storage_kind);
    out << move_prefix << ".destination_width="
        << move.destination_contiguous_width;
    out << move_prefix << ".op_kind="
        << rv64_prepared_move_op_kind_name(move.op_kind);
    out << move_prefix << ".authority="
        << prepare::prepared_move_authority_kind_name(move.authority_kind);
    out << move_prefix << ".reason="
        << (move.reason.empty() ? "<none>" : move.reason);
    if (move.source_immediate_i32.has_value()) {
      out << move_prefix << ".source_imm_i32=" << *move.source_immediate_i32;
    }
    const auto* source_home =
        prepared_value_home_for_id(lookups, move.from_value_id);
    const auto* destination_home =
        prepared_value_home_for_id(lookups, move.to_value_id);
    if (source_home != nullptr) {
      out << move_prefix << ".source_home_kind="
          << prepare::prepared_value_home_kind_name(source_home->kind);
      const auto source_type = prepared_bir_value_type_for_name(
          names, function, source_home->value_name);
      if (source_type.has_value()) {
        out << move_prefix << ".source_type="
            << bir::render_type(*source_type);
      }
    } else {
      out << move_prefix << ".source_home_kind=<missing>";
    }
    if (destination_home != nullptr) {
      out << move_prefix << ".destination_home_kind="
          << prepare::prepared_value_home_kind_name(destination_home->kind);
      const auto destination_type = prepared_bir_value_type_for_name(
          names, function, destination_home->value_name);
      if (destination_type.has_value()) {
        out << move_prefix << ".destination_type="
            << bir::render_type(*destination_type);
      }
    } else {
      out << move_prefix << ".destination_home_kind=<missing>";
    }
  }
  out << " fragment_status=generic_move_bundle_materialization_failed";
  return out.str();
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_select_edge_binary_with_cast_dependencies(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::bir::BinaryInst& binary,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes) {
  const auto* lhs_authority =
      find_available_rv64_select_edge_cast_dependency_authority(
          names,
          function_name,
          dependency_operand_authorities,
          publication,
          prepare::PreparedDependencyOperandRole::Lhs,
          binary.lhs);
  const auto* rhs_authority =
      find_available_rv64_select_edge_cast_dependency_authority(
          names,
          function_name,
          dependency_operand_authorities,
          publication,
          prepare::PreparedDependencyOperandRole::Rhs,
          binary.rhs);
  if (lhs_authority == nullptr && rhs_authority == nullptr) {
    return std::nullopt;
  }
  const auto lhs_source_register =
      rv64_select_edge_dependency_operand_current_source_register(names,
                                                                  lookups,
                                                                  binary.lhs,
                                                                  lhs_authority);
  const auto rhs_source_register =
      rv64_select_edge_dependency_operand_current_source_register(names,
                                                                  lookups,
                                                                  binary.rhs,
                                                                  rhs_authority);
  if (rhs_source_register.has_value() && *rhs_source_register == 28 &&
      (!lhs_source_register.has_value() || *lhs_source_register != 28)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  const auto materialize_operand =
      [&](const c4c::backend::bir::Value& value,
          const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecord*
              authority,
          std::uint32_t register_number) -> bool {
    if (authority != nullptr) {
      return append_rv64_materialize_cast_dependency_authority(fragment,
                                                              *authority,
                                                              register_number);
    }
    return append_rv64_move_value_to_register(fragment,
                                             register_number,
                                             stack_layout,
                                             names,
                                             lookups,
                                             value,
                                             stack_frame_bytes);
  };

  if (!materialize_operand(binary.lhs, lhs_authority, 28) ||
      !materialize_operand(binary.rhs, rhs_authority, 29) ||
      !append_rv64_compare_registers_to_register(fragment,
                                                 binary.opcode,
                                                 destination_register,
                                                 28,
                                                 29)) {
    return std::nullopt;
  }
  return fragment;
}

bool prepared_select_edge_binary_source_has_only_carrier_uses(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::BinaryInst& producer,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer) {
  if (producer.result.kind != c4c::backend::bir::Value::Kind::Named ||
      producer.result.name.empty()) {
    return false;
  }
  const auto producer_name = names.value_names.find(producer.result.name);
  if (producer_name == c4c::kInvalidValueName) {
    return false;
  }

  auto uses_producer = [&](const c4c::backend::bir::Value& value) {
    return prepared_bir_value_has_name(names, value, producer_name);
  };

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (uses_producer(binary->lhs) || uses_producer(binary->rhs)) {
          return false;
        }
        continue;
      }
      if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
        const bool uses_as_value =
            uses_producer(select->true_value) || uses_producer(select->false_value);
        if (uses_producer(select->lhs) || uses_producer(select->rhs)) {
          return false;
        }
        if (!uses_as_value) {
          continue;
        }
        auto prepared_block_label = block.label_id;
        if (producer.opcode == c4c::backend::bir::BinaryOpcode::Ne) {
          const auto label_by_name = names.block_labels.find(block.label);
          if (label_by_name != c4c::kInvalidBlockLabel) {
            prepared_block_label = label_by_name;
          }
        }
        const auto classification =
            prepare::classify_prepared_object_select_consumer(&control_flow,
                                                              prepared_block_label,
                                                              inst);
        if (classification.kind !=
                prepare::PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier ||
            classification.join_transfer != &join_transfer ||
            !prepared_bir_values_have_same_name(names,
                                                select->result,
                                                join_transfer.result)) {
          return false;
        }
        continue;
      }
      if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
        if (uses_producer(cast->operand)) {
          return false;
        }
        continue;
      }
      if (const auto* phi = std::get_if<c4c::backend::bir::PhiInst>(&inst)) {
        for (const auto& incoming : phi->incomings) {
          if (uses_producer(incoming.value)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
        if (call->callee_value.has_value() && uses_producer(*call->callee_value)) {
          return false;
        }
        for (const auto& arg : call->args) {
          if (uses_producer(arg)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
        if (uses_producer(store->value)) {
          return false;
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (uses_producer(store->value)) {
          return false;
        }
        continue;
      }
    }
    if (block.terminator.value.has_value() &&
        uses_producer(*block.terminator.value)) {
      return false;
    }
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch &&
        uses_producer(block.terminator.condition)) {
      return false;
    }
    for (const auto& lane : block.terminator.return_lanes) {
      if (uses_producer(lane)) {
        return false;
      }
    }
  }
  return true;
}

bool prepared_select_edge_binary_source_has_authorized_consumers(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::bir::BinaryInst& producer,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer) {
  if (prepared_select_edge_binary_source_has_only_carrier_uses(names,
                                                              control_flow,
                                                              function,
                                                              producer,
                                                              join_transfer)) {
    return true;
  }
  const bool supported_carrier_alias_compare =
      producer.result.type == c4c::backend::bir::TypeKind::I32 &&
      (producer.opcode == c4c::backend::bir::BinaryOpcode::Ule ||
       producer.opcode == c4c::backend::bir::BinaryOpcode::Ne);
  if (supported_carrier_alias_compare &&
      prepared_select_edge_binary_source_has_carrier_alias_authority(
          function_name, carrier_alias_authorities, publication)) {
    return true;
  }

  if (lookups == nullptr ||
      !prepared_bir_values_have_same_name(names,
                                          publication.destination_value,
                                          join_transfer.result)) {
    return false;
  }
  const auto* join_block =
      find_prepared_bir_block_by_prepared_label(names,
                                                function,
                                                join_transfer.join_block_label);
  if (join_block == nullptr) {
    return false;
  }
  const auto select_chain =
      prepare::find_prepared_scalar_select_chain_materialization(
          names,
          &lookups->edge_publication_source_producers,
          join_transfer.join_block_label,
          join_block,
          publication.destination_value,
          join_block->insts.size());
  if (!select_chain.available ||
      !select_chain.root_is_select ||
      !select_chain.root_instruction_index.has_value() ||
      publication.destination_value.kind != c4c::backend::bir::Value::Kind::Named ||
      names.value_names.find(publication.destination_value.name) !=
          select_chain.root_value_name) {
    return false;
  }
  const auto select_uses_source_as_payload =
      [&](const c4c::backend::bir::SelectInst& select) {
        return prepared_bir_values_have_same_name(names,
                                                  select.true_value,
                                                  publication.source_value) ||
               prepared_bir_values_have_same_name(names,
                                                  select.false_value,
                                                  publication.source_value);
      };
  const auto select_uses_source_as_condition =
      [&](const c4c::backend::bir::SelectInst& select) {
        return prepared_bir_values_have_same_name(names,
                                                  select.lhs,
                                                  publication.source_value) ||
               prepared_bir_values_have_same_name(names,
                                                  select.rhs,
                                                  publication.source_value);
      };
  const auto select_result_feeds_final =
      [&](const c4c::backend::bir::SelectInst& select,
          const c4c::backend::bir::SelectInst& final_select) {
        const auto exact_named_match =
            [](const c4c::backend::bir::Value& lhs,
               const c4c::backend::bir::Value& rhs) {
              return lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                     rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                     lhs.name == rhs.name;
            };
        return exact_named_match(final_select.true_value, select.result) ||
               exact_named_match(final_select.false_value, select.result);
      };
  const c4c::backend::bir::SelectInst* final_select = nullptr;
  for (const auto& inst : join_block->insts) {
    const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
    if (select == nullptr ||
        !prepared_bir_values_have_same_name(names,
                                            select->result,
                                            publication.destination_value)) {
      continue;
    }
    if (final_select != nullptr) {
      return false;
    }
    final_select = select;
  }
  if (final_select == nullptr) {
    return false;
  }
  for (const auto& inst : join_block->insts) {
    const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
    if (select == nullptr || select == final_select) {
      continue;
    }
    if (select_uses_source_as_payload(*select) &&
        !select_uses_source_as_condition(*select) &&
        select_result_feeds_final(*select, *final_select)) {
      return false;
    }
  }
  const auto find_same_block_select_chain_producer =
      [&](const c4c::backend::bir::Value& value,
          std::size_t before_instruction_index)
      -> const c4c::backend::bir::Inst* {
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return nullptr;
    }
    const auto limit = std::min(before_instruction_index, join_block->insts.size());
    for (std::size_t i = 0; i < limit; ++i) {
      const auto& inst = join_block->insts.at(i);
      if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
          select != nullptr &&
          prepared_bir_values_have_same_name(names, select->result, value)) {
        return &inst;
      }
      if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
          cast != nullptr &&
          prepared_bir_values_have_same_name(names, cast->result, value)) {
        return &inst;
      }
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
          binary != nullptr &&
          prepared_bir_values_have_same_name(names, binary->result, value)) {
        return &inst;
      }
    }
    return nullptr;
  };
  const std::function<bool(const c4c::backend::bir::Value&,
                           std::size_t,
                           const std::function<bool(
                               const c4c::backend::bir::Inst&)>&,
                           unsigned)>
      select_chain_contains_producer =
          [&](const c4c::backend::bir::Value& value,
              std::size_t before_instruction_index,
              const std::function<bool(const c4c::backend::bir::Inst&)>& matches,
              unsigned depth) -> bool {
    if (depth > 64U || value.kind != c4c::backend::bir::Value::Kind::Named) {
      return false;
    }
    const auto* chain_producer =
        find_same_block_select_chain_producer(value, before_instruction_index);
    if (chain_producer == nullptr) {
      return false;
    }
    if (matches(*chain_producer)) {
      return true;
    }
    const auto nested_before =
        static_cast<std::size_t>(chain_producer - join_block->insts.data());
    if (const auto* select =
            std::get_if<c4c::backend::bir::SelectInst>(chain_producer)) {
      return select_chain_contains_producer(select->true_value,
                                            nested_before,
                                            matches,
                                            depth + 1U) ||
             select_chain_contains_producer(select->false_value,
                                            nested_before,
                                            matches,
                                            depth + 1U);
    }
    if (const auto* cast =
            std::get_if<c4c::backend::bir::CastInst>(chain_producer)) {
      return select_chain_contains_producer(cast->operand,
                                            nested_before,
                                            matches,
                                            depth + 1U);
    }
    if (const auto* binary =
            std::get_if<c4c::backend::bir::BinaryInst>(chain_producer)) {
      return select_chain_contains_producer(binary->lhs,
                                            nested_before,
                                            matches,
                                            depth + 1U) ||
             select_chain_contains_producer(binary->rhs,
                                            nested_before,
                                            matches,
                                            depth + 1U);
    }
    return false;
  };
  const auto producer_is_in_select_chain = select_chain_contains_producer(
      publication.destination_value,
      join_block->insts.size(),
      [&](const c4c::backend::bir::Inst& chain_producer) {
        const auto* binary =
            std::get_if<c4c::backend::bir::BinaryInst>(&chain_producer);
        return binary != nullptr &&
               prepared_bir_values_have_same_name(names,
                                                  binary->result,
                                                  producer.result);
      },
      0U);
  if (!producer_is_in_select_chain) {
    return false;
  }

  const auto producer_name = names.value_names.find(producer.result.name);
  if (producer_name == c4c::kInvalidValueName) {
    return false;
  }
  const auto uses_producer = [&](const c4c::backend::bir::Value& value) {
    return prepared_bir_value_has_name(names, value, producer_name);
  };
  const auto select_is_in_destination_chain =
      [&](const c4c::backend::bir::SelectInst& select) {
        return select_chain_contains_producer(
            publication.destination_value,
            join_block->insts.size(),
            [&](const c4c::backend::bir::Inst& chain_producer) {
              const auto* chain_select =
                  std::get_if<c4c::backend::bir::SelectInst>(&chain_producer);
              return chain_select != nullptr &&
                     prepared_bir_values_have_same_name(
                         names,
                         chain_select->result,
                         select.result);
            },
            0U);
      };

  for (const auto& block : function.blocks) {
    const bool is_join_block = &block == join_block;
    for (const auto& inst : block.insts) {
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (uses_producer(binary->lhs) || uses_producer(binary->rhs)) {
          return false;
        }
        continue;
      }
      if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
        if (uses_producer(select->lhs) || uses_producer(select->rhs)) {
          return false;
        }
        if (uses_producer(select->true_value) ||
            uses_producer(select->false_value)) {
          if (!is_join_block || !select_is_in_destination_chain(*select)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
        if (uses_producer(cast->operand)) {
          return false;
        }
        continue;
      }
      if (const auto* phi = std::get_if<c4c::backend::bir::PhiInst>(&inst)) {
        for (const auto& incoming : phi->incomings) {
          if (uses_producer(incoming.value)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
        if (call->callee_value.has_value() && uses_producer(*call->callee_value)) {
          return false;
        }
        for (const auto& arg : call->args) {
          if (uses_producer(arg)) {
            return false;
          }
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
        if (uses_producer(store->value)) {
          return false;
        }
        continue;
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (uses_producer(store->value)) {
          return false;
        }
      }
    }
    if (block.terminator.value.has_value() &&
        uses_producer(*block.terminator.value)) {
      return false;
    }
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch &&
        uses_producer(block.terminator.condition)) {
      return false;
    }
    for (const auto& lane : block.terminator.return_lanes) {
      if (uses_producer(lane)) {
        return false;
      }
    }
  }
  return true;
}

bool prepared_select_edge_publication_has_emittable_source_block(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer) {
  if (!publication.source_producer_block_label.has_value()) {
    return false;
  }
  if (*publication.source_producer_block_label == join_transfer.join_block_label) {
    return true;
  }
  return *publication.source_producer_block_label == publication.predecessor_label &&
         publication.successor_label == join_transfer.join_block_label &&
         publication.parallel_copy_execution_site ==
             c4c::backend::prepare::PreparedParallelCopyExecutionSite::
                 PredecessorTerminator &&
         publication.parallel_copy_execution_block_label ==
             std::optional<c4c::BlockLabelId>{publication.predecessor_label};
}

bool prepared_select_edge_publication_matches_move_resolution(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move) {
  if (publication.source_value_id == move.from_value_id) {
    return true;
  }
  if (!move.source_parallel_copy_step_index.has_value() ||
      !move_bundle.source_parallel_copy_predecessor_label.has_value() ||
      !move_bundle.source_parallel_copy_successor_label.has_value()) {
    return false;
  }
  const auto* bundle = find_prepared_parallel_copy_bundle_for_edge(
      control_flow,
      *move_bundle.source_parallel_copy_predecessor_label,
      *move_bundle.source_parallel_copy_successor_label);
  if (bundle == nullptr ||
      *move.source_parallel_copy_step_index >= bundle->steps.size()) {
    return false;
  }
  const auto& step = bundle->steps.at(*move.source_parallel_copy_step_index);
  if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
      step.uses_cycle_temp_source) {
    return false;
  }
  const auto* parallel_copy_move =
      prepare::find_prepared_parallel_copy_move_for_step(*bundle, step);
  if (parallel_copy_move == nullptr ||
      parallel_copy_move->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      parallel_copy_move->join_transfer_index >= control_flow.join_transfers.size()) {
    return false;
  }
  const auto& join_transfer =
      control_flow.join_transfers.at(parallel_copy_move->join_transfer_index);
  return publication.join_transfer == &join_transfer &&
         prepared_bir_values_have_same_name(names,
                                            publication.source_value,
                                            parallel_copy_move->source_value) &&
         prepared_bir_values_have_same_name(
             names,
             publication.destination_value,
             parallel_copy_move->destination_value);
}

const c4c::backend::prepare::PreparedEdgePublication*
find_prepared_select_edge_publication_for_parallel_copy_move(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    const c4c::backend::bir::Value& destination_value) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const c4c::backend::prepare::PreparedEdgePublication* match = nullptr;
  for (const auto& publication : lookups->edge_publications.publications) {
    if (publication.predecessor_label != predecessor_label ||
        publication.successor_label != successor_label ||
        publication.status !=
            prepare::PreparedEdgePublicationLookupStatus::Available ||
        !prepared_bir_values_have_same_name(names,
                                            publication.destination_value,
                                            destination_value)) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &publication;
  }
  return match;
}

bool prepared_value_names_match(const c4c::backend::prepare::PreparedNameTables& names,
                                const c4c::backend::bir::Value& lhs,
                                const c4c::backend::bir::Value& rhs) {
  return lhs.kind == c4c::backend::bir::Value::Kind::Named &&
         rhs.kind == c4c::backend::bir::Value::Kind::Named &&
         prepared_bir_values_have_same_name(names, lhs, rhs);
}

bool prepared_select_edge_source_value_needs_binary_producer(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Value& value,
    const std::vector<const c4c::backend::bir::BinaryInst*>& available_producers,
    const std::vector<const c4c::backend::bir::BinaryInst*>& emitted_producers) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto is_available = [&](const c4c::backend::bir::BinaryInst* producer) {
    return prepared_value_names_match(names, value, producer->result);
  };
  const auto is_emitted = [&](const c4c::backend::bir::BinaryInst* producer) {
    return prepared_value_names_match(names, value, producer->result);
  };
  return std::any_of(available_producers.begin(),
                     available_producers.end(),
                     is_available) &&
         !std::any_of(emitted_producers.begin(),
                      emitted_producers.end(),
                      is_emitted);
}

const c4c::backend::bir::Block* find_prepared_bir_block_by_prepared_label(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label) {
  if (block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  const auto matches_label = [&](const c4c::backend::bir::Block& block) {
    if (block_label != c4c::kInvalidBlockLabel &&
        block.label_id != c4c::kInvalidBlockLabel &&
        block.label_id == block_label) {
      return true;
    }
    return names.block_labels.find(block.label) == block_label;
  };
  const auto it = std::find_if(function.blocks.begin(),
                               function.blocks.end(),
                               matches_label);
  return it == function.blocks.end() ? nullptr : &*it;
}

const c4c::backend::bir::BinaryInst* find_prepared_binary_producer_in_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return nullptr;
  }
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary != nullptr &&
        prepared_value_names_match(names, value, binary->result)) {
      return binary;
    }
  }
  return nullptr;
}

const c4c::backend::bir::BinaryInst*
find_prepared_fp_zero_compare_consumer_in_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block,
    c4c::backend::bir::TypeKind source_type) {
  const c4c::backend::bir::BinaryInst* match = nullptr;
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        binary->operand_type != source_type ||
        binary->lhs.type != source_type ||
        binary->rhs.type != source_type ||
        (binary->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
         binary->opcode != c4c::backend::bir::BinaryOpcode::Ne)) {
      continue;
    }
    const bool lhs_is_zero = is_rv64_zero_floating_immediate(binary->lhs);
    const bool rhs_is_zero = is_rv64_zero_floating_immediate(binary->rhs);
    if (lhs_is_zero == rhs_is_zero) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = binary;
  }
  return match;
}

const c4c::backend::prepare::PreparedParallelCopyBundle*
find_prepared_parallel_copy_bundle_for_edge(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label) {
  for (const auto& bundle : control_flow.parallel_copy_bundles) {
    if (bundle.predecessor_label == predecessor_label &&
        bundle.successor_label == successor_label) {
      return &bundle;
    }
  }
  return nullptr;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_width_preserving_i32_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast);

std::optional<RiscvEncodedFragment>
fragment_for_prepared_select_edge_source_dependencies(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId source_block_label,
    const c4c::backend::bir::BinaryInst& source_binary,
    std::size_t stack_frame_bytes) {
  const auto* source_block =
      find_prepared_bir_block_by_prepared_label(names, function, source_block_label);
  if (source_block == nullptr) {
    return std::nullopt;
  }

  std::vector<const c4c::backend::bir::BinaryInst*> candidate_producers;
  std::vector<const c4c::backend::bir::CastInst*> candidate_casts;
  for (const auto& inst : source_block->insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary != nullptr) {
      if (binary == &source_binary ||
          prepared_value_names_match(names, binary->result, source_binary.result)) {
        break;
      }
      candidate_producers.push_back(binary);
      continue;
    }
    const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
    if (cast != nullptr) {
      candidate_casts.push_back(cast);
    }
  }

  std::vector<const c4c::backend::bir::BinaryInst*> emitted_producers;
  std::vector<const c4c::backend::bir::CastInst*> emitted_casts;
  RiscvEncodedFragment fragment;
  std::vector<const c4c::backend::bir::BinaryInst*> active_producers;
  std::vector<const c4c::backend::bir::CastInst*> active_casts;
  auto find_producer_for_value = [&](const c4c::backend::bir::Value& value)
      -> const c4c::backend::bir::BinaryInst* {
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return nullptr;
    }
    const auto producer_it =
        std::find_if(candidate_producers.begin(),
                     candidate_producers.end(),
                     [&](const c4c::backend::bir::BinaryInst* producer) {
                       return prepared_value_names_match(names,
                                                         value,
                                                         producer->result);
                     });
    return producer_it == candidate_producers.end() ? nullptr : *producer_it;
  };
  auto find_cast_for_value = [&](const c4c::backend::bir::Value& value)
      -> const c4c::backend::bir::CastInst* {
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return nullptr;
    }
    const auto cast_it =
        std::find_if(candidate_casts.begin(),
                     candidate_casts.end(),
                     [&](const c4c::backend::bir::CastInst* cast) {
                       return prepared_value_names_match(names,
                                                         value,
                                                         cast->result);
                     });
    return cast_it == candidate_casts.end() ? nullptr : *cast_it;
  };
  std::function<bool(const c4c::backend::bir::CastInst*)> emit_cast;
  std::function<bool(const c4c::backend::bir::BinaryInst*)> emit_producer =
      [&](const c4c::backend::bir::BinaryInst* producer) -> bool {
    if (producer == nullptr) {
      return true;
    }
    const auto already_emitted =
        std::any_of(emitted_producers.begin(),
                    emitted_producers.end(),
                    [&](const c4c::backend::bir::BinaryInst* emitted) {
                      return prepared_value_names_match(names,
                                                        producer->result,
                                                        emitted->result);
                    });
    if (already_emitted) {
      return true;
    }
    const auto is_active =
        std::any_of(active_producers.begin(),
                    active_producers.end(),
                    [&](const c4c::backend::bir::BinaryInst* active) {
                      return prepared_value_names_match(names,
                                                        producer->result,
                                                        active->result);
                    });
    if (is_active) {
      return false;
    }
    active_producers.push_back(producer);
    if (!emit_producer(find_producer_for_value(producer->lhs)) ||
        !emit_cast(find_cast_for_value(producer->lhs)) ||
        !emit_producer(find_producer_for_value(producer->rhs)) ||
        !emit_cast(find_cast_for_value(producer->rhs))) {
      return false;
    }
    active_producers.pop_back();

    auto producer_fragment = fragment_for_prepared_binary(stack_layout,
                                                          names,
                                                          lookups,
                                                          *producer,
                                                          stack_frame_bytes);
    if (!producer_fragment.has_value()) {
      return false;
    }
    append_fragment(fragment, std::move(*producer_fragment));
    emitted_producers.push_back(producer);
    return true;
  };
  emit_cast = [&](const c4c::backend::bir::CastInst* cast) -> bool {
    if (cast == nullptr) {
      return true;
    }
    const auto already_emitted =
        std::any_of(emitted_casts.begin(),
                    emitted_casts.end(),
                    [&](const c4c::backend::bir::CastInst* emitted) {
                      return prepared_value_names_match(names,
                                                        cast->result,
                                                        emitted->result);
                    });
    if (already_emitted) {
      return true;
    }
    const auto is_active =
        std::any_of(active_casts.begin(),
                    active_casts.end(),
                    [&](const c4c::backend::bir::CastInst* active) {
                      return prepared_value_names_match(names,
                                                        cast->result,
                                                        active->result);
                    });
    if (is_active) {
      return false;
    }
    active_casts.push_back(cast);
    if (!emit_producer(find_producer_for_value(cast->operand)) ||
        !emit_cast(find_cast_for_value(cast->operand))) {
      return false;
    }
    active_casts.pop_back();

    auto cast_fragment =
        fragment_for_prepared_width_preserving_i32_cast(names, lookups, *cast);
    if (!cast_fragment.has_value()) {
      cast_fragment = fragment_for_prepared_cast(stack_layout,
                                                 names,
                                                 lookups,
                                                 *cast,
                                                 stack_frame_bytes);
    }
    if (!cast_fragment.has_value()) {
      return false;
    }
    append_fragment(fragment, std::move(*cast_fragment));
    emitted_casts.push_back(cast);
    return true;
  };
  if (!emit_producer(find_producer_for_value(source_binary.lhs)) ||
      !emit_cast(find_cast_for_value(source_binary.lhs)) ||
      !emit_producer(find_producer_for_value(source_binary.rhs))) {
    return std::nullopt;
  }
  if (!emit_cast(find_cast_for_value(source_binary.rhs))) {
    return std::nullopt;
  }

  const auto operand_ready = [&](const c4c::backend::bir::Value& value) {
    const auto needs_binary =
        prepared_select_edge_source_value_needs_binary_producer(
            names,
            value,
            candidate_producers,
            emitted_producers);
    const auto* cast = find_cast_for_value(value);
    const auto needs_cast =
        cast != nullptr &&
        !std::any_of(emitted_casts.begin(),
                     emitted_casts.end(),
                     [&](const c4c::backend::bir::CastInst* emitted) {
                       return prepared_value_names_match(names,
                                                         cast->result,
                                                         emitted->result);
                     });
    return !needs_binary && !needs_cast;
  };
  if (!operand_ready(source_binary.lhs) || !operand_ready(source_binary.rhs)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_prepared_width_preserving_i32_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast) {
  if ((cast.opcode != c4c::backend::bir::CastOpcode::ZExt &&
       cast.opcode != c4c::backend::bir::CastOpcode::Trunc) ||
      cast.operand.type != c4c::backend::bir::TypeKind::I32 ||
      cast.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  const auto source =
      source_home == nullptr ? std::nullopt : gpr_register_number_for_home(*source_home);
  if (!destination.has_value() || !source.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_rv64_move(fragment, *destination, *source);
  return fragment;
}

bool prepared_width_preserving_i32_zext_is_materialized_by_move_bundle(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::CastInst& cast) {
  if (lookups == nullptr ||
      cast.opcode != c4c::backend::bir::CastOpcode::ZExt ||
      cast.operand.type != c4c::backend::bir::TypeKind::I32 ||
      cast.result.type != c4c::backend::bir::TypeKind::I32 ||
      cast.operand.kind != c4c::backend::bir::Value::Kind::Named ||
      cast.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  if (source_home == nullptr || destination_home == nullptr) {
    return false;
  }
  const auto is_supported_i32_gpr_home =
      [](const c4c::backend::prepare::PreparedValueHome& home) {
        return home.kind == prepare::PreparedValueHomeKind::Register ||
               home.kind == prepare::PreparedValueHomeKind::StackSlot;
      };
  if (!is_supported_i32_gpr_home(*source_home) ||
      !is_supported_i32_gpr_home(*destination_home)) {
    return false;
  }
  const auto* move_bundle = prepare::find_indexed_prepared_move_bundle(
      &lookups->move_bundles,
      nullptr,
      prepare::PreparedMovePhase::BeforeInstruction,
      block_index,
      instruction_index);
  if (move_bundle == nullptr ||
      move_bundle->phase != prepare::PreparedMovePhase::BeforeInstruction ||
      move_bundle->authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      !move_bundle->abi_bindings.empty() || move_bundle->moves.size() != 1) {
    return false;
  }
  const auto& move = move_bundle->moves.front();
  const auto expected_destination_storage =
      destination_home->kind == prepare::PreparedValueHomeKind::Register
          ? prepare::PreparedMoveStorageKind::Register
          : prepare::PreparedMoveStorageKind::StackSlot;
  const std::string_view expected_reason =
      source_home->kind == prepare::PreparedValueHomeKind::Register
          ? (destination_home->kind == prepare::PreparedValueHomeKind::Register
                 ? std::string_view{"consumer_register_to_register"}
                 : std::string_view{"consumer_register_to_stack"})
          : (destination_home->kind == prepare::PreparedValueHomeKind::Register
                 ? std::string_view{"consumer_stack_to_register"}
                 : std::string_view{"consumer_stack_to_stack"});
  return move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.authority_kind == prepare::PreparedMoveAuthorityKind::None &&
         move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
         move.destination_storage_kind == expected_destination_storage &&
         move.from_value_id == source_home->value_id &&
         move.to_value_id == destination_home->value_id &&
         move.reason == expected_reason &&
         !move.uses_cycle_temp_source &&
         !move.source_immediate_i32.has_value() &&
         !move.source_parallel_copy_step_index.has_value() &&
         !move.source_parallel_copy_predecessor_label.has_value() &&
         !move.source_parallel_copy_successor_label.has_value() &&
         move.destination_contiguous_width == 1;
}

PreparedSelectEdgeSourceProducerFragment
fragment_for_prepared_block_entry_select_edge_source_producer(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes) {
  if (!move.source_parallel_copy_step_index.has_value()) {
    return {};
  }
  if (!move_bundle.source_parallel_copy_predecessor_label.has_value() ||
      !move_bundle.source_parallel_copy_successor_label.has_value()) {
    return {};
  }
  const auto* bundle = find_prepared_parallel_copy_bundle_for_edge(
      control_flow,
      *move_bundle.source_parallel_copy_predecessor_label,
      *move_bundle.source_parallel_copy_successor_label);
  if (bundle == nullptr ||
      bundle->execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      bundle->execution_block_label !=
          move_bundle.source_parallel_copy_predecessor_label ||
      bundle->has_cycle ||
      *move.source_parallel_copy_step_index >= bundle->steps.size()) {
    return {};
  }

  const auto& step = bundle->steps.at(*move.source_parallel_copy_step_index);
  if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
      step.uses_cycle_temp_source) {
    return {};
  }
  const auto* parallel_copy_move =
      prepare::find_prepared_parallel_copy_move_for_step(*bundle, step);
  if (parallel_copy_move == nullptr ||
      parallel_copy_move->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
    return {};
  }
  if (parallel_copy_move->join_transfer_index >=
      control_flow.join_transfers.size()) {
    return {.matched = true};
  }

  const auto& join_transfer =
      control_flow.join_transfers.at(parallel_copy_move->join_transfer_index);
  if (!prepared_join_transfer_edge_copies_are_published(control_flow,
                                                        join_transfer) ||
      !prepared_bir_values_have_same_name(names,
                                          parallel_copy_move->destination_value,
                                          join_transfer.result)) {
    return {.matched = true};
  }

  std::optional<c4c::BlockLabelId> source_producer_block_label;
  const c4c::backend::bir::BinaryInst* binary = nullptr;
  auto find_binary_in_source_block = [&](c4c::BlockLabelId block_label) {
    if (binary != nullptr) {
      return;
    }
    const auto* source_block =
        find_prepared_bir_block_by_prepared_label(names, function, block_label);
    if (source_block == nullptr) {
      return;
    }
    binary = find_prepared_binary_producer_in_block(names,
                                                   *source_block,
                                                   parallel_copy_move->source_value);
    if (binary != nullptr) {
      source_producer_block_label = block_label;
    }
  };
  find_binary_in_source_block(*move_bundle.source_parallel_copy_predecessor_label);
  find_binary_in_source_block(join_transfer.join_block_label);
  if (binary == nullptr ||
      !c4c::backend::bir::is_compare_opcode(binary->opcode) ||
      binary->result.type != c4c::backend::bir::TypeKind::I32) {
    return {.matched = true};
  }
  auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      lookups == nullptr ? nullptr : &lookups->edge_publications,
      *move_bundle.source_parallel_copy_predecessor_label,
      *move_bundle.source_parallel_copy_successor_label,
      move.to_value_id);
  if (publication != nullptr &&
      (publication->join_transfer != &join_transfer ||
       !prepared_bir_values_have_same_name(names,
                                           publication->source_value,
                                           parallel_copy_move->source_value) ||
       !prepared_bir_values_have_same_name(
           names,
           publication->destination_value,
           parallel_copy_move->destination_value))) {
    publication = nullptr;
  }
  if (publication == nullptr) {
    publication = find_prepared_select_edge_publication_for_parallel_copy_move(
        names,
        lookups,
        *move_bundle.source_parallel_copy_predecessor_label,
        *move_bundle.source_parallel_copy_successor_label,
        parallel_copy_move->destination_value);
  }
  const bool has_authorized_consumers =
      publication != nullptr &&
      publication->status ==
          prepare::PreparedEdgePublicationLookupStatus::Available &&
      publication->carrier_kind ==
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization &&
      prepared_bir_values_have_same_name(names,
                                         publication->source_value,
                                         parallel_copy_move->source_value) &&
      publication->source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::Binary &&
      publication->source_binary == binary &&
      publication->join_transfer == &join_transfer &&
      prepared_select_edge_binary_source_has_authorized_consumers(
          names,
          control_flow,
          function,
          control_flow.function_name,
          lookups,
          carrier_alias_authorities,
          *binary,
          *publication,
          join_transfer);
  if (!has_authorized_consumers &&
      !prepared_select_edge_binary_source_has_only_carrier_uses(names,
                                                               control_flow,
                                                               function,
                                                               *binary,
                                                               join_transfer)) {
    return {.matched = true};
  }

  const auto* destination_home =
      prepared_value_home_for(names, lookups, parallel_copy_move->destination_value);
  const auto destination =
      destination_home == nullptr
          ? std::nullopt
          : gpr_register_number_for_home(*destination_home);
  if (!destination.has_value() || *destination != destination_register) {
    return {.matched = true};
  }

  auto fragment = fragment_for_prepared_select_edge_source_dependencies(
      stack_layout,
      names,
      function,
      lookups,
      *source_producer_block_label,
      *binary,
      stack_frame_bytes);
  if (!fragment.has_value()) {
    return {.matched = true};
  }
  auto edge_binary = *binary;
  edge_binary.result = parallel_copy_move->destination_value;
  std::optional<RiscvEncodedFragment> edge_fragment;
  if (publication != nullptr) {
    edge_fragment = fragment_for_prepared_select_edge_binary_with_cast_dependencies(
        stack_layout,
        names,
        lookups,
        control_flow.function_name,
        dependency_operand_authorities,
        *publication,
        edge_binary,
        destination_register,
        stack_frame_bytes);
    if (!edge_fragment.has_value()) {
      if (rv64_select_edge_binary_has_available_cast_dependency_authority(
              names,
              control_flow.function_name,
              dependency_operand_authorities,
              *publication,
              edge_binary)) {
        return {.matched = true};
      }
    }
  }
  if (!edge_fragment.has_value()) {
    edge_fragment = fragment_for_prepared_binary(stack_layout,
                                                 names,
                                                 lookups,
                                                 edge_binary,
                                                 stack_frame_bytes);
  }
  if (!edge_fragment.has_value()) {
    return {.matched = true};
  }
  append_fragment(*fragment, std::move(*edge_fragment));
  return {.matched = true, .fragment = std::move(fragment)};
}

PreparedSelectEdgeSourceProducerFragment fragment_for_prepared_select_edge_source_producer(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move,
    std::uint32_t destination_register,
    std::size_t stack_frame_bytes) {
  if (!move_bundle.source_parallel_copy_predecessor_label.has_value() ||
      !move_bundle.source_parallel_copy_successor_label.has_value()) {
    return {};
  }
  const auto intent = consume_edge_publication_move_intent(
      lookups,
      *move_bundle.source_parallel_copy_predecessor_label,
      *move_bundle.source_parallel_copy_successor_label,
      move.to_value_id);
  const auto* publication = intent.publication;
  if (publication == nullptr && lookups != nullptr) {
    publication = prepare::find_unique_indexed_prepared_edge_publication(
        &lookups->edge_publications,
        *move_bundle.source_parallel_copy_predecessor_label,
        *move_bundle.source_parallel_copy_successor_label,
        move.to_value_id);
  }
  if (publication == nullptr) {
    return fragment_for_prepared_block_entry_select_edge_source_producer(
        stack_layout,
        names,
        control_flow,
        function,
        lookups,
        dependency_operand_authorities,
        carrier_alias_authorities,
        move_bundle,
        move,
        destination_register,
        stack_frame_bytes);
  }
  if (publication != nullptr &&
      !prepared_select_edge_publication_matches_move_resolution(names,
                                                               control_flow,
                                                               *publication,
                                                               move_bundle,
                                                               move) &&
      move.source_parallel_copy_step_index.has_value() &&
      move_bundle.source_parallel_copy_predecessor_label.has_value() &&
      move_bundle.source_parallel_copy_successor_label.has_value()) {
    const auto* bundle = find_prepared_parallel_copy_bundle_for_edge(
        control_flow,
        *move_bundle.source_parallel_copy_predecessor_label,
        *move_bundle.source_parallel_copy_successor_label);
    if (bundle != nullptr &&
        *move.source_parallel_copy_step_index < bundle->steps.size()) {
      const auto& step = bundle->steps.at(*move.source_parallel_copy_step_index);
      const auto* parallel_copy_move =
          step.kind == prepare::PreparedParallelCopyStepKind::Move
              ? prepare::find_prepared_parallel_copy_move_for_step(*bundle, step)
              : nullptr;
      if (parallel_copy_move != nullptr) {
        publication = find_prepared_select_edge_publication_for_parallel_copy_move(
            names,
            lookups,
            *move_bundle.source_parallel_copy_predecessor_label,
            *move_bundle.source_parallel_copy_successor_label,
            parallel_copy_move->destination_value);
      }
    }
  }
  if (publication == nullptr ||
      publication->status != prepare::PreparedEdgePublicationLookupStatus::Available ||
      publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) {
    return {};
  }
  if (!prepared_select_edge_publication_matches_move_resolution(names,
                                                               control_flow,
                                                               *publication,
                                                               move_bundle,
                                                               move)) {
    return {};
  }
  const auto* join_transfer = publication->join_transfer;
  const auto* binary = publication->source_binary;
  if (join_transfer == nullptr || binary == nullptr ||
      !prepared_join_transfer_edge_copies_are_published(control_flow,
                                                        *join_transfer) ||
      !prepared_select_edge_publication_has_emittable_source_block(
          *publication,
          *join_transfer)) {
    return {.matched = true};
  }
  if (!c4c::backend::bir::is_compare_opcode(binary->opcode) ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
          names,
          lookups,
          control_flow.function_name,
          dependency_operand_authorities,
          *publication,
          prepare::PreparedDependencyOperandRole::Lhs,
          binary->lhs) ||
      !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
          names,
          lookups,
          control_flow.function_name,
          dependency_operand_authorities,
          *publication,
          prepare::PreparedDependencyOperandRole::Rhs,
          binary->rhs) ||
      !prepared_select_edge_binary_source_has_authorized_consumers(
          names,
          control_flow,
          function,
          control_flow.function_name,
          lookups,
          carrier_alias_authorities,
          *binary,
          *publication,
          *join_transfer)) {
    return {.matched = true};
  }
  const auto* destination_home = prepared_value_home_for_id(lookups, move.to_value_id);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  if (!destination.has_value() || *destination != destination_register ||
      !prepared_bir_values_have_same_name(names,
                                          publication->destination_value,
                                          join_transfer->result)) {
    return {.matched = true};
  }

  auto edge_binary = *binary;
  edge_binary.result = publication->destination_value;
  auto fragment = fragment_for_prepared_select_edge_source_dependencies(
      stack_layout,
      names,
      function,
      lookups,
      *publication->source_producer_block_label,
      *binary,
      stack_frame_bytes);
  if (!fragment.has_value()) {
    return {.matched = true};
  }
  auto edge_fragment =
      fragment_for_prepared_select_edge_binary_with_cast_dependencies(
          stack_layout,
          names,
          lookups,
          control_flow.function_name,
          dependency_operand_authorities,
          *publication,
          edge_binary,
          destination_register,
          stack_frame_bytes);
  if (!edge_fragment.has_value()) {
    if (rv64_select_edge_binary_has_available_cast_dependency_authority(
            names,
            control_flow.function_name,
            dependency_operand_authorities,
            *publication,
            edge_binary)) {
      return {.matched = true};
    }
    edge_fragment = fragment_for_prepared_binary(stack_layout,
                                                 names,
                                                 lookups,
                                                 edge_binary,
                                                 stack_frame_bytes);
    if (!edge_fragment.has_value()) {
      return {.matched = true};
    }
  }
  append_fragment(*fragment, std::move(*edge_fragment));
  return {.matched = true, .fragment = std::move(fragment)};
}

bool prepared_binary_is_select_edge_owned_source(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::bir::BinaryInst& binary) {
  if (lookups == nullptr || binary.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto source_value_id =
      prepared_value_id_for_named_value(names, lookups, binary.result);
  if (!source_value_id.has_value()) {
    return false;
  }
  for (const auto& publication : lookups->edge_publications.publications) {
    if (publication.source_value_id != source_value_id ||
        publication.source_binary != &binary ||
        publication.source_producer_kind !=
            prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
        publication.carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
        publication.join_transfer == nullptr ||
        !prepared_join_transfer_edge_copies_are_published(control_flow,
                                                          *publication.join_transfer) ||
        !prepared_select_edge_publication_has_emittable_source_block(
            publication,
            *publication.join_transfer) ||
        !c4c::backend::bir::is_compare_opcode(binary.opcode) ||
        binary.result.type != c4c::backend::bir::TypeKind::I32 ||
        !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
            names,
            lookups,
            control_flow.function_name,
            dependency_operand_authorities,
            publication,
            prepare::PreparedDependencyOperandRole::Lhs,
            binary.lhs) ||
        !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
            names,
            lookups,
            control_flow.function_name,
            dependency_operand_authorities,
            publication,
            prepare::PreparedDependencyOperandRole::Rhs,
            binary.rhs) ||
        !prepared_select_edge_binary_source_has_authorized_consumers(
            names,
            control_flow,
            function,
            control_flow.function_name,
            lookups,
            carrier_alias_authorities,
            binary,
            publication,
            *publication.join_transfer)) {
      continue;
    }
    return true;
  }
  return false;
}

std::string rv64_select_local_label(std::string_view function_name,
                                    std::string_view block_label,
                                    std::size_t instruction_index,
                                    std::string_view suffix) {
  return riscv_local_block_label(function_name,
                                 std::string(block_label) + "_select_" +
                                     std::to_string(instruction_index) + "_" +
                                     std::string(suffix));
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_select(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view function_name,
    std::string_view block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::SelectInst& select,
    std::size_t stack_frame_bytes,
    const c4c::backend::bir::Block* block,
    std::optional<std::uint32_t> forced_destination_register,
    std::size_t recursion_depth);

const c4c::backend::bir::SelectInst* find_same_block_scalar_select_producer(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block* block,
    const c4c::backend::bir::Value& value,
    std::size_t before_instruction_index,
    std::size_t* producer_instruction_index) {
  if (block == nullptr || value.kind != c4c::backend::bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto limit = std::min(before_instruction_index, block->insts.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto* select =
        std::get_if<c4c::backend::bir::SelectInst>(&block->insts.at(index));
    if (select == nullptr ||
        !prepared_bir_values_have_same_name(names, select->result, value)) {
      continue;
    }
    if (producer_instruction_index != nullptr) {
      *producer_instruction_index = index;
    }
    return select;
  }
  return nullptr;
}

bool select_inst_uses_value(const c4c::backend::prepare::PreparedNameTables& names,
                            const c4c::backend::bir::SelectInst& select,
                            const c4c::backend::bir::Value& value) {
  return prepared_bir_values_have_same_name(names, select.lhs, value) ||
         prepared_bir_values_have_same_name(names, select.rhs, value) ||
         prepared_bir_values_have_same_name(names, select.true_value, value) ||
         prepared_bir_values_have_same_name(names, select.false_value, value);
}

std::optional<std::size_t> unique_later_same_block_select_consumer_index(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block* block,
    const c4c::backend::bir::SelectInst& select,
    std::size_t instruction_index) {
  if (block == nullptr ||
      select.result.kind != c4c::backend::bir::Value::Kind::Named ||
      instruction_index >= block->insts.size()) {
    return std::nullopt;
  }
  std::optional<std::size_t> consumer_index;
  for (std::size_t index = instruction_index + 1U; index < block->insts.size();
       ++index) {
    const auto* consumer =
        std::get_if<c4c::backend::bir::SelectInst>(&block->insts.at(index));
    if (consumer == nullptr) {
      continue;
    }
    if (!select_inst_uses_value(names, *consumer, select.result)) {
      continue;
    }
    if (consumer_index.has_value()) {
      return std::nullopt;
    }
    consumer_index = index;
  }
  return consumer_index;
}

bool append_rv64_move_value_or_same_block_select_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view function_name,
    std::string_view block_label,
    const c4c::backend::bir::Block* block,
    std::size_t before_instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes,
    std::size_t recursion_depth) {
  if (append_rv64_move_value_to_register(fragment,
                                         destination,
                                         stack_layout,
                                         names,
                                         lookups,
                                         value,
                                         stack_frame_bytes)) {
    return true;
  }
  if (recursion_depth > 16U) {
    return false;
  }
  std::size_t producer_instruction_index = 0;
  const auto* nested_select = find_same_block_scalar_select_producer(
      names, block, value, before_instruction_index, &producer_instruction_index);
  if (nested_select == nullptr) {
    return false;
  }
  const auto unique_consumer = unique_later_same_block_select_consumer_index(
      names, block, *nested_select, producer_instruction_index);
  if (!unique_consumer.has_value() || *unique_consumer != before_instruction_index) {
    return false;
  }
  auto nested_fragment = fragment_for_prepared_select(stack_layout,
                                                      names,
                                                      lookups,
                                                      function_name,
                                                      block_label,
                                                      producer_instruction_index,
                                                      *nested_select,
                                                      stack_frame_bytes,
                                                      block,
                                                      destination,
                                                      recursion_depth + 1U);
  if (!nested_fragment.has_value()) {
    return false;
  }
  append_fragment(fragment, std::move(*nested_fragment));
  return true;
}

std::optional<std::uint32_t> rv64_scratch_fpr_avoiding(
    std::optional<std::uint32_t> first,
    std::optional<std::uint32_t> second,
    std::optional<std::uint32_t> third) {
  constexpr std::array<std::uint32_t, 3> candidates = {30, 31, 29};
  const auto conflicts = [&](std::uint32_t candidate) {
    return (first.has_value() && *first == candidate) ||
           (second.has_value() && *second == candidate) ||
           (third.has_value() && *third == candidate);
  };
  const auto it =
      std::find_if(candidates.begin(), candidates.end(), [&](std::uint32_t candidate) {
        return !conflicts(candidate);
      });
  return it == candidates.end() ? std::nullopt
                                : std::optional<std::uint32_t>{*it};
}

bool append_rv64_move_floating_value_to_fpr(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (!rv64_floating_type(value.type)) {
    return false;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    if (value.immediate_bits >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return false;
    }
    const auto scratch_gpr = rv64_unoccupied_temporary_gpr(lookups);
    if (!scratch_gpr.has_value()) {
      return false;
    }
    append_rv64_load_immediate(
        fragment, *scratch_gpr, static_cast<std::int64_t>(value.immediate_bits));
    return append_rv64_gpr_to_fpr_move(fragment,
                                       destination,
                                       *scratch_gpr,
                                       value.type);
  }
  if (const auto source = fpr_register_number_for_value(names, lookups, value);
      source.has_value()) {
    return append_rv64_fpr_move(fragment, destination, *source, value.type);
  }
  const auto* stack_home = prepared_value_home_for(names, lookups, value);
  const auto stack_offset =
      stack_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *stack_home,
                                                     stack_frame_bytes,
                                                     value.type == c4c::backend::bir::TypeKind::F32
                                                         ? std::size_t{4}
                                                         : std::size_t{8});
  if (stack_offset.has_value()) {
    return append_rv64_load_stack_offset_to_fpr(fragment,
                                               destination,
                                               *stack_offset,
                                               value.type);
  }
  return false;
}

bool append_rv64_move_floating_value_or_same_block_select_to_fpr(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view function_name,
    std::string_view block_label,
    const c4c::backend::bir::Block* block,
    std::size_t before_instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes,
    std::size_t recursion_depth) {
  if (append_rv64_move_floating_value_to_fpr(fragment,
                                            destination,
                                            stack_layout,
                                            names,
                                            lookups,
                                            value,
                                            stack_frame_bytes)) {
    return true;
  }
  if (recursion_depth > 16U) {
    return false;
  }
  std::size_t producer_instruction_index = 0;
  const auto* nested_select = find_same_block_scalar_select_producer(
      names, block, value, before_instruction_index, &producer_instruction_index);
  if (nested_select == nullptr || !rv64_floating_type(nested_select->result.type)) {
    return false;
  }
  const auto unique_consumer = unique_later_same_block_select_consumer_index(
      names, block, *nested_select, producer_instruction_index);
  if (!unique_consumer.has_value() || *unique_consumer != before_instruction_index) {
    return false;
  }
  auto nested_fragment = fragment_for_prepared_select(stack_layout,
                                                      names,
                                                      lookups,
                                                      function_name,
                                                      block_label,
                                                      producer_instruction_index,
                                                      *nested_select,
                                                      stack_frame_bytes,
                                                      block,
                                                      destination,
                                                      recursion_depth + 1U);
  if (!nested_fragment.has_value()) {
    return false;
  }
  append_fragment(fragment, std::move(*nested_fragment));
  return true;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_select(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view function_name,
    std::string_view block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::SelectInst& select,
    std::size_t stack_frame_bytes,
    const c4c::backend::bir::Block* block,
    std::optional<std::uint32_t> forced_destination_register,
    std::size_t recursion_depth) {
  const bool floating_result = rv64_floating_type(select.result.type);
  if (select.result.type != c4c::backend::bir::TypeKind::I8 &&
      select.result.type != c4c::backend::bir::TypeKind::I16 &&
      select.result.type != c4c::backend::bir::TypeKind::I32 &&
      select.result.type != c4c::backend::bir::TypeKind::I64 &&
      !floating_result) {
    return std::nullopt;
  }
  if (recursion_depth > 16U) {
    return std::nullopt;
  }
  const bool floating_condition = rv64_floating_type(select.compare_type);
  const auto normalized =
      floating_condition
          ? std::optional<Rv64NormalizedBranchPredicate>{}
          : normalize_rv64_branch_predicate(select.predicate, select.lhs, select.rhs);
  const auto funct3 =
      normalized.has_value() ? rv64_branch_funct3(normalized->opcode)
                             : std::optional<std::uint32_t>{};
  if ((!floating_condition &&
       (!normalized.has_value() || !funct3.has_value())) ||
      (floating_condition &&
       (select.compare_type != select.lhs.type ||
        select.compare_type != select.rhs.type))) {
    return std::nullopt;
  }
  const auto size_bytes =
      floating_result
          ? std::optional<std::size_t>{
                select.result.type == c4c::backend::bir::TypeKind::F32
                    ? std::size_t{4}
                    : std::size_t{8}}
          : rv64_scalar_memory_size_for_type(select.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, select.result);
  const auto destination =
      forced_destination_register.has_value()
          ? forced_destination_register
          : (destination_home == nullptr
                 ? std::nullopt
                 : (floating_result ? fpr_register_number_for_home(*destination_home)
                                    : gpr_register_number_for_home(*destination_home)));
  const auto destination_stack_offset =
      forced_destination_register.has_value() || destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *destination_home,
                                                     stack_frame_bytes,
                                                     *size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    if (unique_later_same_block_select_consumer_index(
            names, block, select, instruction_index)
            .has_value()) {
      return RiscvEncodedFragment{};
    }
    return std::nullopt;
  }
  const auto false_source_fpr =
      floating_result ? fpr_register_number_for_value(names, lookups, select.false_value)
                      : std::optional<std::uint32_t>{};
  const auto true_source_fpr =
      floating_result ? fpr_register_number_for_value(names, lookups, select.true_value)
                      : std::optional<std::uint32_t>{};
  const std::uint32_t destination_register =
      destination.value_or(floating_result
                               ? rv64_scratch_fpr_avoiding(false_source_fpr,
                                                           true_source_fpr,
                                                           std::nullopt)
                                     .value_or(30)
                               : 30);
  const std::string true_label =
      rv64_select_local_label(function_name, block_label, instruction_index, "true");
  const std::string end_label =
      rv64_select_local_label(function_name, block_label, instruction_index, "end");

  RiscvEncodedFragment fragment;
  if (floating_condition) {
    if (!append_rv64_fp_compare_to_register(fragment,
                                           names,
                                           lookups,
                                           select.predicate,
                                           select.compare_type,
                                           select.lhs,
                                           select.rhs,
                                           28)) {
      return std::nullopt;
    }
    append_rv64_local_branch(fragment, 1, 28, 0, true_label);
  } else {
    if (!append_rv64_move_value_or_same_block_select_to_register(fragment,
                                                                 28,
                                                                 stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 function_name,
                                                                 block_label,
                                                                 block,
                                                                 instruction_index,
                                                                 normalized->lhs,
                                                                 stack_frame_bytes,
                                                                 recursion_depth) ||
        !append_rv64_move_value_or_same_block_select_to_register(fragment,
                                                                 29,
                                                                 stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 function_name,
                                                                 block_label,
                                                                 block,
                                                                 instruction_index,
                                                                 normalized->rhs,
                                                                 stack_frame_bytes,
                                                                 recursion_depth)) {
      return std::nullopt;
    }
    append_rv64_local_branch(fragment, *funct3, 28, 29, true_label);
  }
  if (floating_result) {
    if (!append_rv64_move_floating_value_or_same_block_select_to_fpr(
            fragment,
            destination_register,
            stack_layout,
            names,
            lookups,
            function_name,
            block_label,
            block,
            instruction_index,
            select.false_value,
            stack_frame_bytes,
            recursion_depth)) {
      return std::nullopt;
    }
  } else {
    if (!append_rv64_move_value_or_same_block_select_to_register(fragment,
                                                                 destination_register,
                                                                 stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 function_name,
                                                                 block_label,
                                                                 block,
                                                                 instruction_index,
                                                                 select.false_value,
                                                                 stack_frame_bytes,
                                                                 recursion_depth)) {
      return std::nullopt;
    }
  }
  append_rv64_local_jump(fragment, end_label);
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = fragment.bytes.size(),
      .name = true_label,
  });
  if (floating_result) {
    if (!append_rv64_move_floating_value_or_same_block_select_to_fpr(
            fragment,
            destination_register,
            stack_layout,
            names,
            lookups,
            function_name,
            block_label,
            block,
            instruction_index,
            select.true_value,
            stack_frame_bytes,
            recursion_depth)) {
      return std::nullopt;
    }
  } else {
    if (!append_rv64_move_value_or_same_block_select_to_register(fragment,
                                                                 destination_register,
                                                                 stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 function_name,
                                                                 block_label,
                                                                 block,
                                                                 instruction_index,
                                                                 select.true_value,
                                                                 stack_frame_bytes,
                                                                 recursion_depth)) {
      return std::nullopt;
    }
  }
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = fragment.bytes.size(),
      .name = end_label,
  });
  if (destination_stack_offset.has_value()) {
    if (floating_result) {
      if (!append_rv64_store_fpr_to_stack_offset(fragment,
                                                destination_register,
                                                *destination_stack_offset,
                                                select.result.type)) {
        return std::nullopt;
      }
    } else if (!append_rv64_store_register_to_stack_offset(fragment,
                                                          destination_register,
                                                          *destination_stack_offset,
                                                          *size_bytes)) {
      return std::nullopt;
    }
  }
  return fragment;
}

bool prepared_join_transfer_edge_copies_are_published(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer) {
  if (join_transfer.edge_transfers.empty()) {
    return false;
  }
  return std::all_of(
      join_transfer.edge_transfers.begin(),
      join_transfer.edge_transfers.end(),
      [&](const c4c::backend::prepare::PreparedEdgeValueTransfer& edge_transfer) {
        return c4c::backend::prepare::
                   find_published_parallel_copy_bundle_for_edge_transfer(
                       control_flow,
                       edge_transfer) != nullptr;
      });
}

std::optional<prepare::PreparedValueId> prepared_value_id_for_named_value(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const bir::Value& value) {
  if (lookups == nullptr || value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  return value_id_it->second;
}

template <typename T>
void append_optional_value(std::ostringstream& out,
                           std::string_view name,
                           const std::optional<T>& value) {
  out << " " << name << "=";
  if (value.has_value()) {
    out << *value;
  } else {
    out << "<none>";
  }
}

void append_optional_block_label(const prepare::PreparedNameTables& names,
                                 std::ostringstream& out,
                                 std::string_view name,
                                 std::optional<BlockLabelId> label) {
  out << " " << name << "=";
  if (label.has_value()) {
    out << rv64_prepared_block_label(names, *label);
  } else {
    out << "<none>";
  }
}

bool prepared_select_publication_destination_is_stack_home(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle,
    const prepare::PreparedParallelCopyMove& move) {
  const auto destination_value_id =
      prepared_value_id_for_named_value(names, lookups, move.destination_value);
  if (!destination_value_id.has_value()) {
    return false;
  }
  const auto intent = consume_edge_publication_move_intent(
      lookups,
      bundle.predecessor_label,
      bundle.successor_label,
      *destination_value_id);
  const bool is_select_publication =
      move.carrier_kind ==
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      (intent.publication != nullptr &&
       intent.publication->carrier_kind ==
           prepare::PreparedJoinTransferCarrierKind::SelectMaterialization);
  if (!is_select_publication || intent.status != EdgePublicationMoveIntentStatus::Available ||
      intent.publication == nullptr || intent.publication->parallel_copy_bundle != &bundle ||
      intent.publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
    return false;
  }
  if (intent.destination_stack_size_bytes != std::optional<std::size_t>{4}) {
    return false;
  }
  const auto* destination_home =
      prepared_value_home_for_id(lookups, *destination_value_id);
  return destination_home != nullptr &&
         destination_home->kind == prepare::PreparedValueHomeKind::StackSlot;
}

bool prepared_select_publication_immediate_to_gpr_matches_bundle(
    const EdgePublicationMoveIntent& intent,
    const prepare::PreparedParallelCopyBundle& bundle) {
  return intent.status == EdgePublicationMoveIntentStatus::Available &&
         intent.publication != nullptr &&
         intent.publication->parallel_copy_bundle == &bundle &&
         intent.publication->carrier_kind ==
             prepare::PreparedJoinTransferCarrierKind::SelectMaterialization &&
         intent.publication->parallel_copy_execution_site ==
             prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator &&
         intent.publication->parallel_copy_execution_block_label ==
             std::optional<BlockLabelId>{bundle.predecessor_label} &&
         intent.publication->parallel_copy_step_kind ==
             prepare::PreparedParallelCopyStepKind::Move &&
         !intent.publication->parallel_copy_step_uses_cycle_temp_source &&
         !intent.publication->parallel_copy_bundle_has_cycle &&
         intent.source_type == bir::TypeKind::I32 &&
         intent.destination_type == bir::TypeKind::I32 &&
         intent.source_immediate_i32.has_value() &&
         intent.source_register.empty() &&
         !intent.source_stack_slot_id.has_value() &&
         !intent.source_stack_offset_bytes.has_value() &&
         !intent.source_memory_base_value_id.has_value() &&
         intent.source_memory_base_register.empty() &&
         !intent.source_memory_byte_offset.has_value() &&
         !intent.source_pointer_base_value_id.has_value() &&
         intent.source_pointer_base_register.empty() &&
         !intent.source_pointer_byte_delta.has_value() &&
         !intent.destination_register.empty() &&
         !intent.destination_stack_slot_id.has_value() &&
         !intent.destination_stack_offset_bytes.has_value() &&
         rv64_register_number(intent.destination_register).has_value();
}

bool prepared_predecessor_select_publication_bundle_is_immediate_to_gpr_materialized(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      bundle.execution_block_label != bundle.predecessor_label ||
      bundle.has_cycle || bundle.moves.empty() ||
      bundle.steps.size() != bundle.moves.size()) {
    return false;
  }

  bool matched = false;
  for (const auto& step : bundle.steps) {
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
        step.uses_cycle_temp_source) {
      return false;
    }
    const auto* move =
        prepare::find_prepared_parallel_copy_move_for_step(bundle, step);
    if (move == nullptr ||
        move->carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      return false;
    }
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    if (!destination_value_id.has_value()) {
      return false;
    }
    const auto intent = consume_edge_publication_move_intent(
        lookups,
        bundle.predecessor_label,
        bundle.successor_label,
        *destination_value_id);
    if (!prepared_select_publication_immediate_to_gpr_matches_bundle(intent,
                                                                     bundle)) {
      return false;
    }
    matched = true;
  }
  return matched;
}

void append_select_publication_intent_evidence(
    const prepare::PreparedNameTables& names,
    std::ostringstream& out,
    const EdgePublicationMoveIntent& intent) {
  out << " intent_status="
      << edge_publication_move_intent_status_name(intent.status);
  out << " intent_destination_value_id=" << intent.destination_value_id;
  append_optional_value(out, "intent_source_value_id", intent.source_value_id);
  out << " intent_source_type=" << static_cast<int>(intent.source_type);
  out << " intent_destination_type=" << static_cast<int>(intent.destination_type);
  out << " intent_source_register="
      << (intent.source_register.empty() ? "<none>" : intent.source_register);
  append_optional_value(out,
                        "intent_source_immediate_i32",
                        intent.source_immediate_i32);
  append_optional_value(out,
                        "intent_source_stack_slot_id",
                        intent.source_stack_slot_id);
  append_optional_value(out,
                        "intent_source_stack_offset",
                        intent.source_stack_offset_bytes);
  append_optional_value(out,
                        "intent_source_memory_base_value_id",
                        intent.source_memory_base_value_id);
  out << " intent_source_memory_base_register="
      << (intent.source_memory_base_register.empty()
              ? "<none>"
              : intent.source_memory_base_register);
  append_optional_value(out,
                        "intent_source_memory_byte_offset",
                        intent.source_memory_byte_offset);
  append_optional_value(out,
                        "intent_source_pointer_base_value_id",
                        intent.source_pointer_base_value_id);
  out << " intent_source_pointer_base_register="
      << (intent.source_pointer_base_register.empty()
              ? "<none>"
              : intent.source_pointer_base_register);
  append_optional_value(out,
                        "intent_source_pointer_byte_delta",
                        intent.source_pointer_byte_delta);
  out << " intent_destination_register="
      << (intent.destination_register.empty()
              ? "<none>"
              : intent.destination_register);
  append_optional_value(out,
                        "intent_destination_stack_slot_id",
                        intent.destination_stack_slot_id);
  append_optional_value(out,
                        "intent_destination_stack_offset",
                        intent.destination_stack_offset_bytes);
  if (intent.route5_edge_status.has_value()) {
    out << " intent_route5_edge_status="
        << static_cast<int>(*intent.route5_edge_status);
  } else {
    out << " intent_route5_edge_status=<none>";
  }
  out << " intent_route5_edge_source_agrees="
      << (intent.route5_edge_source_agrees ? "yes" : "no");
  out << " intent_route3_source_memory_agrees="
      << (intent.route3_source_memory_agrees ? "yes" : "no");

  const auto* publication = intent.publication;
  if (publication == nullptr) {
    out << " publication_present=no";
    return;
  }

  out << " publication_present=yes";
  out << " publication_status="
      << prepared_edge_publication_lookup_status_name(publication->status);
  out << " publication_predecessor="
      << rv64_prepared_block_label(names, publication->predecessor_label);
  out << " publication_successor="
      << rv64_prepared_block_label(names, publication->successor_label);
  out << " publication_destination_value_id="
      << publication->destination_value_id;
  out << " publication_destination_value_name="
      << (publication->destination_value_name == kInvalidValueName
              ? std::string{"<none>"}
              : names.value_names.spelling(publication->destination_value_name));
  append_optional_value(out,
                        "publication_source_value_id",
                        publication->source_value_id);
  out << " publication_source_value_name="
      << (publication->source_value_name == kInvalidValueName
              ? std::string{"<none>"}
              : names.value_names.spelling(publication->source_value_name));
  out << " publication_source_producer="
      << prepare::prepared_edge_publication_source_producer_kind_name(
             publication->source_producer_kind);
  append_optional_block_label(names,
                              out,
                              "publication_source_producer_block",
                              publication->source_producer_block_label);
  append_optional_value(out,
                        "publication_source_producer_inst",
                        publication->source_producer_instruction_index);
  out << " publication_source_memory_status="
      << prepare::prepared_edge_publication_source_memory_access_status_name(
             publication->source_memory_access_status);
  out << " publication_source_home_kind="
      << prepare::prepared_value_home_kind_name(publication->source_home_kind);
  out << " publication_destination_home_kind="
      << prepare::prepared_value_home_kind_name(
             publication->destination_home_kind);
  out << " publication_destination_storage="
      << rv64_prepared_move_storage_kind_name(
             publication->destination_storage_kind);
  out << " publication_phase="
      << prepare::prepared_move_phase_name(publication->phase);
  out << " publication_carrier="
      << prepare::prepared_join_transfer_carrier_kind_name(
             publication->carrier_kind);
  append_optional_value(out,
                        "publication_parallel_copy_step_index",
                        publication->parallel_copy_step_index);
  out << " publication_parallel_copy_step_kind="
      << prepare::prepared_parallel_copy_step_kind_name(
             publication->parallel_copy_step_kind);
  out << " publication_parallel_copy_step_uses_cycle_temp="
      << (publication->parallel_copy_step_uses_cycle_temp_source ? "yes" : "no");
  out << " publication_parallel_copy_bundle_has_cycle="
      << (publication->parallel_copy_bundle_has_cycle ? "yes" : "no");
  out << " publication_parallel_copy_execution_site="
      << prepare::prepared_parallel_copy_execution_site_name(
             publication->parallel_copy_execution_site);
  append_optional_block_label(names,
                              out,
                              "publication_parallel_copy_execution_block",
                              publication->parallel_copy_execution_block_label);
}

std::optional<std::string> rv64_select_publication_bundle_rejection_diagnostic(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedControlFlowFunction& control_flow,
    const prepare::PreparedObjectTraversalEvent& event,
    const prepare::PreparedMoveBundle* move_bundle,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return std::nullopt;
  }

  bool has_select_publication = false;
  for (const auto& move : bundle.moves) {
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move.destination_value);
    const auto intent =
        destination_value_id.has_value()
            ? consume_edge_publication_move_intent(lookups,
                                                   bundle.predecessor_label,
                                                   bundle.successor_label,
                                                   *destination_value_id)
            : EdgePublicationMoveIntent{};
    has_select_publication =
        has_select_publication ||
        move.carrier_kind ==
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
        (intent.publication != nullptr &&
         intent.publication->carrier_kind ==
             prepare::PreparedJoinTransferCarrierKind::SelectMaterialization);
  }
  if (!has_select_publication) {
    return std::nullopt;
  }

  std::ostringstream out;
  out << "unsupported_move_bundle_target_shape: prepared select publication "
         "move bundle requires unsupported RV64 moves";
  out << " select_publication_evidence=yes";
  out << " event_kind="
      << prepare::prepared_object_traversal_event_kind_name(event.kind);
  out << " function="
      << rv64_prepared_function_name(names, control_flow.function_name);
  out << " block_index=" << event.block_index;
  if (event.prepared_block != nullptr) {
    out << " block_label="
        << rv64_prepared_block_label(names, event.prepared_block->block_label);
  }
  out << " instruction_index=" << event.instruction_index;

  if (move_bundle != nullptr) {
    out << " phase=" << prepare::prepared_move_phase_name(move_bundle->phase);
    out << " bundle_block_index=" << move_bundle->block_index;
    out << " bundle_instruction_index=" << move_bundle->instruction_index;
    out << " authority="
        << prepare::prepared_move_authority_kind_name(
               move_bundle->authority_kind);
    out << " move_count=" << move_bundle->moves.size();
    append_optional_block_label(names,
                                out,
                                "move_bundle_parallel_copy_predecessor",
                                move_bundle->source_parallel_copy_predecessor_label);
    append_optional_block_label(names,
                                out,
                                "move_bundle_parallel_copy_successor",
                                move_bundle->source_parallel_copy_successor_label);
    for (std::size_t move_index = 0; move_index < move_bundle->moves.size();
         ++move_index) {
      const auto& move = move_bundle->moves[move_index];
      const std::string move_prefix =
          " move[" + std::to_string(move_index) + "]";
      out << move_prefix << ".from_value_id=" << move.from_value_id;
      out << move_prefix << ".to_value_id=" << move.to_value_id;
      out << move_prefix << ".destination_kind="
          << rv64_prepared_move_destination_kind_name(move.destination_kind);
      out << move_prefix << ".destination_storage="
          << rv64_prepared_move_storage_kind_name(move.destination_storage_kind);
      out << move_prefix << ".op_kind="
          << rv64_prepared_move_op_kind_name(move.op_kind);
      out << move_prefix << ".reason="
          << (move.reason.empty() ? "<none>" : move.reason);
      if (move.source_immediate_i32.has_value()) {
        out << move_prefix << ".source_imm_i32=" << *move.source_immediate_i32;
      }
    }
  } else {
    out << " phase=<missing>";
    out << " authority=<missing>";
    out << " move_count=<missing>";
  }

  out << " parallel_copy=yes";
  out << " parallel_copy_predecessor="
      << rv64_prepared_block_label(names, bundle.predecessor_label);
  out << " parallel_copy_successor="
      << rv64_prepared_block_label(names, bundle.successor_label);
  out << " parallel_copy_execution_site="
      << prepare::prepared_parallel_copy_execution_site_name(bundle.execution_site);
  append_optional_block_label(names,
                              out,
                              "parallel_copy_execution_block",
                              bundle.execution_block_label);
  out << " parallel_copy_has_cycle="
      << (bundle.has_cycle ? "yes" : "no");
  out << " parallel_copy_move_count=" << bundle.moves.size();
  out << " parallel_copy_step_count=" << bundle.steps.size();

  if (bundle.execution_block_label !=
      std::optional<BlockLabelId>{bundle.predecessor_label}) {
    out << " select_publication_rejection_reason="
        << "execution_block_not_predecessor";
    return out.str();
  }
  if (bundle.has_cycle) {
    out << " select_publication_rejection_reason=parallel_copy_cycle";
    return out.str();
  }

  for (std::size_t step_index = 0; step_index < bundle.steps.size();
       ++step_index) {
    const auto& step = bundle.steps[step_index];
    out << " selected_step_index=" << step_index;
    out << " selected_step_kind="
        << prepare::prepared_parallel_copy_step_kind_name(step.kind);
    out << " selected_step_uses_cycle_temp="
        << (step.uses_cycle_temp_source ? "yes" : "no");
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move) {
      out << " select_publication_rejection_reason=unsupported_step_kind";
      return out.str();
    }
    if (step.uses_cycle_temp_source) {
      out << " select_publication_rejection_reason=step_uses_cycle_temp";
      return out.str();
    }

    const auto* move =
        prepare::find_prepared_parallel_copy_move_for_step(bundle, step);
    if (move == nullptr) {
      out << " select_publication_rejection_reason=missing_parallel_copy_move";
      return out.str();
    }
    out << " selected_move_carrier="
        << prepare::prepared_join_transfer_carrier_kind_name(move->carrier_kind);
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    append_optional_value(out,
                          "selected_move_destination_value_id",
                          destination_value_id);
    if (!destination_value_id.has_value()) {
      if (move->carrier_kind ==
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
        out << " select_publication_rejection_reason="
            << "missing_destination_value_id";
        return out.str();
      }
      continue;
    }

    const auto intent = consume_edge_publication_move_intent(
        lookups,
        bundle.predecessor_label,
        bundle.successor_label,
        *destination_value_id);
    const bool is_select_publication =
        move->carrier_kind ==
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
        (intent.publication != nullptr &&
         intent.publication->carrier_kind ==
             prepare::PreparedJoinTransferCarrierKind::SelectMaterialization);
    if (!is_select_publication) {
      continue;
    }
    append_select_publication_intent_evidence(names, out, intent);
    if (move->carrier_kind !=
        prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      out << " select_publication_rejection_reason=move_carrier_mismatch";
      return out.str();
    }
    if (intent.publication == nullptr) {
      out << " select_publication_rejection_reason=missing_publication";
      return out.str();
    }
    if (intent.publication->parallel_copy_bundle != &bundle) {
      out << " select_publication_rejection_reason=publication_bundle_mismatch";
      return out.str();
    }
    if (intent.publication->carrier_kind !=
        prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      out << " select_publication_rejection_reason=publication_carrier_mismatch";
      return out.str();
    }
    if (!prepared_select_publication_move_is_rv64_object_admitted(intent) &&
        !prepared_select_publication_pointer_stack_source_to_gpr_matches_bundle(
            intent,
            bundle) &&
        !prepared_select_publication_stack_source_to_gpr_matches_bundle(intent,
                                                                        bundle) &&
        !prepared_select_publication_gpr_to_stack_destination_matches_bundle(
            intent,
            bundle) &&
        !prepared_select_publication_immediate_to_gpr_matches_bundle(intent,
                                                                     bundle) &&
        !prepared_select_publication_destination_is_stack_home(names,
                                                              lookups,
                                                              bundle,
                                                              *move)) {
      out << " select_publication_rejection_reason="
          << rv64_select_publication_move_rejection_reason(intent);
      return out.str();
    }
  }

  return std::nullopt;
}

std::optional<RiscvEncodedFragment>
fragment_for_predecessor_select_publication_immediate_to_gpr(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (!prepared_predecessor_select_publication_bundle_is_immediate_to_gpr_materialized(
          names,
          lookups,
          bundle)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  for (const auto& step : bundle.steps) {
    const auto* move =
        prepare::find_prepared_parallel_copy_move_for_step(bundle, step);
    if (move == nullptr) {
      return std::nullopt;
    }
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    if (!destination_value_id.has_value()) {
      return std::nullopt;
    }
    const auto intent = consume_edge_publication_move_intent(
        lookups,
        bundle.predecessor_label,
        bundle.successor_label,
        *destination_value_id);
    const auto destination = rv64_register_number(intent.destination_register);
    if (!destination.has_value() || !intent.source_immediate_i32.has_value()) {
      return std::nullopt;
    }
    append_rv64_load_immediate(fragment,
                               *destination,
                               *intent.source_immediate_i32);
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
fragment_for_predecessor_select_publication_pointer_stack_source_to_gpr(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      bundle.execution_block_label != bundle.predecessor_label ||
      bundle.has_cycle || bundle.moves.empty() ||
      bundle.steps.size() != bundle.moves.size()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  bool matched = false;
  for (const auto& step : bundle.steps) {
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
        step.uses_cycle_temp_source) {
      return std::nullopt;
    }
    const auto* move =
        prepare::find_prepared_parallel_copy_move_for_step(bundle, step);
    if (move == nullptr ||
        move->carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      return std::nullopt;
    }
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    if (!destination_value_id.has_value()) {
      return std::nullopt;
    }
    const auto intent = consume_edge_publication_move_intent(
        lookups,
        bundle.predecessor_label,
        bundle.successor_label,
        *destination_value_id);
    if (!prepared_select_publication_pointer_stack_source_to_gpr_matches_bundle(
            intent,
            bundle) &&
        !prepared_select_publication_stack_source_to_gpr_matches_bundle(
            intent,
            bundle)) {
      return std::nullopt;
    }
    const auto destination = rv64_register_number(intent.destination_register);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    if (!append_rv64_load_stack_offset_to_register(
            fragment,
            *destination,
            *intent.source_stack_offset_bytes,
            *intent.source_stack_size_bytes)) {
      return std::nullopt;
    }
    matched = true;
  }
  return matched ? std::optional<RiscvEncodedFragment>{std::move(fragment)}
                 : std::nullopt;
}

std::optional<RiscvEncodedFragment>
fragment_for_predecessor_select_publication_gpr_to_stack_destination(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      bundle.execution_block_label != bundle.predecessor_label ||
      bundle.has_cycle || bundle.moves.empty() ||
      bundle.steps.size() != bundle.moves.size()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  bool matched = false;
  for (const auto& step : bundle.steps) {
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
        step.uses_cycle_temp_source) {
      return std::nullopt;
    }
    const auto* move =
        prepare::find_prepared_parallel_copy_move_for_step(bundle, step);
    if (move == nullptr ||
        move->carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      return std::nullopt;
    }
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    if (!destination_value_id.has_value()) {
      return std::nullopt;
    }
    const auto intent = consume_edge_publication_move_intent(
        lookups,
        bundle.predecessor_label,
        bundle.successor_label,
        *destination_value_id);
    if (!prepared_select_publication_gpr_to_stack_destination_matches_bundle(
            intent,
            bundle)) {
      return std::nullopt;
    }
    const auto source = rv64_register_number(intent.source_register);
    if (!source.has_value() ||
        !append_rv64_store_register_to_stack(
            fragment,
            *source,
            static_cast<std::int32_t>(*intent.destination_stack_offset_bytes),
            *intent.destination_stack_size_bytes)) {
      return std::nullopt;
    }
    matched = true;
  }
  return matched ? std::optional<RiscvEncodedFragment>{std::move(fragment)}
                 : std::nullopt;
}

std::optional<std::string> diagnose_unsupported_prepared_param_homes(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedValueLocationFunction* value_locations,
    const c4c::backend::bir::Function& function,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  const auto has_supported_byval_param_stack_slot_home =
      [&](const c4c::backend::bir::Param& param,
          const prepare::PreparedValueHome& home) {
        if (!param.is_byval ||
            home.kind != prepare::PreparedValueHomeKind::StackSlot ||
            !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
            !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
            *home.size_bytes != param.size_bytes ||
            *home.align_bytes != param.align_bytes) {
          return false;
        }
        const auto frame_slot_it =
            std::find_if(stack_layout.frame_slots.begin(),
                         stack_layout.frame_slots.end(),
                         [&](const prepare::PreparedFrameSlot& slot) {
                           return slot.slot_id == *home.slot_id &&
                                  slot.function_name == home.function_name;
                         });
        if (frame_slot_it == stack_layout.frame_slots.end() ||
            frame_slot_it->offset_bytes != *home.offset_bytes ||
            frame_slot_it->size_bytes != param.size_bytes ||
            frame_slot_it->align_bytes != param.align_bytes) {
          return false;
        }
        const auto param_name = names.value_names.find(param.name);
        if (param_name == c4c::kInvalidValueName ||
            home.value_name != param_name) {
          return false;
        }
        const auto object_it =
            std::find_if(stack_layout.objects.begin(),
                         stack_layout.objects.end(),
                         [&](const prepare::PreparedStackObject& object) {
                           return object.object_id == frame_slot_it->object_id &&
                                  object.function_name == home.function_name;
                         });
        return object_it != stack_layout.objects.end() &&
               object_it->value_name == param_name &&
               object_it->source_kind == "byval_param" &&
               object_it->type == param.type &&
               object_it->size_bytes == param.size_bytes &&
               object_it->align_bytes == param.align_bytes &&
               object_it->address_exposed && object_it->requires_home_slot &&
               object_it->permanent_home_slot;
      };

  const auto has_supported_stack_passed_scalar_formal_home =
      [&](const c4c::backend::bir::Param& param,
          std::size_t param_index,
          const prepare::PreparedValueHome& home) {
        auto size_bytes = rv64_scalar_memory_size_for_type(param.type);
        if (!size_bytes.has_value()) {
          if (param.type == c4c::backend::bir::TypeKind::F32) {
            size_bytes = std::size_t{4};
          } else if (param.type == c4c::backend::bir::TypeKind::F64) {
            size_bytes = std::size_t{8};
          }
        }
        if (!param.abi.has_value() || !param.abi->passed_on_stack ||
            param.abi->passed_in_register ||
            (param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer &&
             param.abi->primary_class != c4c::backend::bir::AbiValueClass::Sse) ||
            param.abi->type != param.type || !size_bytes.has_value() ||
            home.kind != prepare::PreparedValueHomeKind::StackSlot ||
            !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
            !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
            *home.size_bytes != *size_bytes ||
            *home.align_bytes > *size_bytes) {
          return false;
        }
        const auto plan = prepare::plan_prepared_formal_publication(
            prepare::PreparedFormalPublicationInputs{
                .names = &names,
                .function = &function,
                .value_locations = value_locations,
                .value_home_lookups =
                    lookups == nullptr ? nullptr : &lookups->value_homes,
            },
            param_index);
        if (!prepare::prepared_formal_publication_available(plan) ||
            plan.action != prepare::PreparedFormalPublicationAction::IncomingStackToHome ||
            plan.home != &home || plan.value_name != home.value_name ||
            !plan.incoming_stack_offset_bytes.has_value() ||
            *plan.incoming_stack_offset_bytes >
                std::numeric_limits<std::size_t>::max() - stack_frame_bytes) {
          return false;
        }
        const auto incoming_absolute_offset =
            stack_frame_bytes + *plan.incoming_stack_offset_bytes;
        if (!fits_signed_12_bit_load_offset(incoming_absolute_offset)) {
          return false;
        }
        const auto frame_slot_it =
            std::find_if(stack_layout.frame_slots.begin(),
                         stack_layout.frame_slots.end(),
                         [&](const prepare::PreparedFrameSlot& slot) {
                           return slot.slot_id == *home.slot_id &&
                                  slot.function_name == home.function_name;
                         });
        if (frame_slot_it == stack_layout.frame_slots.end() ||
            frame_slot_it->offset_bytes != *home.offset_bytes ||
            frame_slot_it->size_bytes != *size_bytes ||
            frame_slot_it->align_bytes > *size_bytes) {
          return false;
        }
        const auto object_it =
            std::find_if(stack_layout.objects.begin(),
                         stack_layout.objects.end(),
                         [&](const prepare::PreparedStackObject& object) {
                           return object.object_id == frame_slot_it->object_id &&
                                  object.function_name == home.function_name;
                         });
        return object_it != stack_layout.objects.end() &&
               object_it->value_name == home.value_name &&
               object_it->source_kind == "regalloc.spill_slot" &&
               object_it->type == param.type &&
               object_it->size_bytes == *size_bytes &&
               object_it->align_bytes <= *size_bytes &&
               !object_it->address_exposed &&
               !object_it->requires_home_slot &&
               !object_it->permanent_home_slot;
      };

  const auto has_supported_scalar_gpr_param_stack_slot_home =
      [&](const c4c::backend::bir::Param& param,
          std::size_t param_index,
          const prepare::PreparedValueHome& home) {
        const auto size_bytes = rv64_scalar_memory_size_for_type(param.type);
        if (param.is_byval || param.is_sret || param.is_varargs ||
            !param.abi.has_value() || !param.abi->passed_in_register ||
            param.abi->passed_on_stack ||
            param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer ||
            param.abi->type != param.type ||
            param.abi->size_bytes != param.size_bytes ||
            param.abi->align_bytes != param.align_bytes ||
            !size_bytes.has_value() || *size_bytes != param.size_bytes ||
            home.kind != prepare::PreparedValueHomeKind::StackSlot ||
            !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
            !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
            *home.size_bytes != param.size_bytes ||
            *home.align_bytes != param.align_bytes ||
            !rv64_gpr_formal_argument_register_number(function,
                                                      param_index)
                 .has_value()) {
          return false;
        }
        const auto param_name = names.value_names.find(param.name);
        if (param_name == c4c::kInvalidValueName ||
            home.value_name != param_name) {
          return false;
        }
        const auto frame_slot_it =
            std::find_if(stack_layout.frame_slots.begin(),
                         stack_layout.frame_slots.end(),
                         [&](const prepare::PreparedFrameSlot& slot) {
                           return slot.slot_id == *home.slot_id &&
                                  slot.function_name == home.function_name;
                         });
        if (frame_slot_it == stack_layout.frame_slots.end() ||
            frame_slot_it->offset_bytes != *home.offset_bytes ||
            frame_slot_it->size_bytes != param.size_bytes ||
            frame_slot_it->align_bytes != param.align_bytes ||
            *home.offset_bytes > stack_frame_bytes ||
            stack_frame_bytes - *home.offset_bytes < param.size_bytes ||
            !fits_signed_12_bit_immediate(
                static_cast<std::int64_t>(*home.offset_bytes))) {
          return false;
        }
        const auto object_it =
            std::find_if(stack_layout.objects.begin(),
                         stack_layout.objects.end(),
                         [&](const prepare::PreparedStackObject& object) {
                           return object.object_id == frame_slot_it->object_id &&
                                  object.function_name == home.function_name;
                         });
        return object_it != stack_layout.objects.end() &&
               object_it->value_name == param_name &&
               object_it->source_kind == "regalloc.spill_slot" &&
               object_it->type == param.type &&
               object_it->size_bytes == param.size_bytes &&
               object_it->align_bytes == param.align_bytes &&
               !object_it->address_exposed && !object_it->requires_home_slot &&
               !object_it->permanent_home_slot;
      };

  const auto has_supported_sret_param_stack_slot_home =
      [&](const c4c::backend::bir::Param& param,
          std::size_t param_index,
          const prepare::PreparedValueHome& home) {
        constexpr std::size_t pointer_size = 8;
        if (!param.is_sret || param.is_byval || param.is_varargs ||
            param.type != c4c::backend::bir::TypeKind::Ptr ||
            !param.abi.has_value() || !param.abi->passed_in_register ||
            param.abi->passed_on_stack ||
            param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer ||
            param.abi->type != param.type ||
            home.kind != prepare::PreparedValueHomeKind::StackSlot ||
            !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
            !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
            *home.size_bytes != pointer_size ||
            *home.align_bytes > pointer_size) {
          return false;
        }
        const auto source_register =
            rv64_gpr_formal_argument_register_number(function, param_index);
        if (!source_register.has_value() || *source_register != rv64_register_number("a0")) {
          return false;
        }
        const auto param_name = names.value_names.find(param.name);
        if (param_name == c4c::kInvalidValueName ||
            home.value_name != param_name) {
          return false;
        }
        const auto frame_slot_it =
            std::find_if(stack_layout.frame_slots.begin(),
                         stack_layout.frame_slots.end(),
                         [&](const prepare::PreparedFrameSlot& slot) {
                           return slot.slot_id == *home.slot_id &&
                                  slot.function_name == home.function_name;
                         });
        if (frame_slot_it == stack_layout.frame_slots.end() ||
            frame_slot_it->offset_bytes != *home.offset_bytes ||
            frame_slot_it->size_bytes != pointer_size ||
            frame_slot_it->align_bytes > pointer_size ||
            *home.offset_bytes > stack_frame_bytes ||
            stack_frame_bytes - *home.offset_bytes < pointer_size ||
            !fits_signed_12_bit_immediate(
                static_cast<std::int64_t>(*home.offset_bytes))) {
          return false;
        }
        const auto object_it =
            std::find_if(stack_layout.objects.begin(),
                         stack_layout.objects.end(),
                         [&](const prepare::PreparedStackObject& object) {
                           return object.object_id == frame_slot_it->object_id &&
                                  object.function_name == home.function_name;
                         });
        return object_it != stack_layout.objects.end() &&
               object_it->value_name == param_name &&
               object_it->source_kind == "sret_param" &&
               object_it->type == c4c::backend::bir::TypeKind::Ptr &&
               object_it->size_bytes == pointer_size &&
               object_it->align_bytes <= pointer_size &&
               object_it->address_exposed && object_it->requires_home_slot &&
               object_it->permanent_home_slot;
      };

  for (std::size_t param_index = 0; param_index < function.params.size();
       ++param_index) {
    const auto& param = function.params[param_index];
    if (param.is_varargs) {
      continue;
    }
    const auto value = c4c::backend::bir::Value::named(param.type, param.name);
    const auto* home = prepared_value_home_for(names, lookups, value);
    if (param.is_sret) {
      if (home == nullptr) {
        continue;
      }
      if (home != nullptr &&
          has_supported_sret_param_stack_slot_home(param, param_index, *home)) {
        continue;
      }
      return std::string{
          "unsupported_sret_param_home: RV64 object route requires a pointer-sized permanent sret frame-slot home matching the incoming a0 formal"};
    }
    if (home != nullptr &&
        (gpr_register_number_for_home(*home).has_value() ||
         fpr_register_number_for_home(*home).has_value())) {
      continue;
    }
    if (param.is_byval && home != nullptr &&
        home->kind ==
            c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
      if (has_supported_byval_param_stack_slot_home(param, *home)) {
        continue;
      }
      return std::string{
          "unsupported_byval_param_home: RV64 object route requires a prepared permanent byval frame-slot home with matching size and alignment"};
    }
    if (!param.is_byval && home != nullptr &&
        home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      if (has_supported_scalar_gpr_param_stack_slot_home(param,
                                                        param_index,
                                                        *home)) {
        continue;
      }
      const auto size_bytes = rv64_scalar_memory_size_for_type(param.type);
      if (param.abi.has_value() && param.abi->passed_in_register &&
          param.abi->primary_class ==
              c4c::backend::bir::AbiValueClass::Integer &&
          size_bytes.has_value()) {
        return std::string{
            "unsupported_param_home: RV64 object route requires scalar GPR formal stack-slot homes to match prepared frame-slot facts"};
      }
      if (param.abi.has_value() && param.abi->passed_on_stack) {
        if (has_supported_stack_passed_scalar_formal_home(param,
                                                          param_index,
                                                          *home)) {
          continue;
        }
        return std::string{
            "unsupported_param_home: RV64 object route requires explicit prepared incoming stack formal authority before consuming stack-passed scalar formal homes"};
      }
    }
    return std::string{
        "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes"};
  }
  return std::nullopt;
}

std::optional<RiscvEncodedFragment> make_rv64_formal_entry_home_fragment(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  RiscvEncodedFragment fragment;
  for (std::size_t param_index = 0; param_index < function.params.size();
       ++param_index) {
    const auto& param = function.params[param_index];
    if (param.is_byval || param.name.empty() || !param.abi.has_value() ||
        !param.abi->passed_in_register ||
        param.abi->primary_class != c4c::backend::bir::AbiValueClass::Integer) {
      continue;
    }
    const auto size_bytes = rv64_formal_entry_home_store_size(param);
    if (!size_bytes.has_value() ||
        (!param.is_sret && *size_bytes != param.size_bytes)) {
      continue;
    }
    const auto value = c4c::backend::bir::Value::named(param.type, param.name);
    const auto* home = prepared_value_home_for(names, lookups, value);
    if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::StackSlot) {
      continue;
    }
    const auto source_register =
        rv64_gpr_formal_argument_register_number(function, param_index);
    const auto stack_offset =
        prepared_stack_slot_home_absolute_offset(stack_layout,
                                                 *home,
                                                 stack_frame_bytes,
                                                 *size_bytes);
    if (!source_register.has_value() || !stack_offset.has_value() ||
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   *source_register,
                                                   *stack_offset,
                                                   *size_bytes)) {
      return std::nullopt;
    }
  }
  return fragment;
}

std::optional<c4c::BlockLabelId> prepared_block_label_id_for(
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const auto prepared_label = names.block_labels.find(structured_label);
      if (prepared_label != c4c::kInvalidBlockLabel) {
        return prepared_label;
      }
    }
  }
  const auto block_label = names.block_labels.find(block.label);
  if (block_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  return block_label;
}

const c4c::backend::prepare::PreparedBranchCondition* find_branch_condition_for_terminator(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Value& condition) {
  if (condition.kind != c4c::backend::bir::Value::Kind::Named ||
      condition.name.empty()) {
    return nullptr;
  }
  const auto* direct =
      c4c::backend::prepare::find_prepared_branch_condition(control_flow, block_label_id);
  if (direct != nullptr &&
      direct->condition_value.kind == c4c::backend::bir::Value::Kind::Named &&
      direct->condition_value.name == condition.name) {
    return direct;
  }

  const c4c::backend::prepare::PreparedBranchCondition* selected = nullptr;
  for (const auto& candidate : control_flow.branch_conditions) {
    if (candidate.condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
        candidate.condition_value.name != condition.name) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &candidate;
  }
  return selected;
}

bool terminator_uses_value_as_condition(
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Value& value) {
  return block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch &&
         value.kind == c4c::backend::bir::Value::Kind::Named &&
         block.terminator.condition.kind == c4c::backend::bir::Value::Kind::Named &&
         value.name == block.terminator.condition.name;
}


const c4c::backend::prepare::PreparedValueHome* prepared_pointer_branch_operand_home_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Named
             ? prepared_value_home_for(names, lookups, value)
             : nullptr;
}

bool prepared_pointer_branch_predicate_is_supported(
    c4c::backend::bir::BinaryOpcode predicate) {
  switch (predicate) {
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

std::optional<Rv64NormalizedBranchPredicate> normalize_prepared_pointer_branch_predicate(
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return Rv64NormalizedBranchPredicate{
          .opcode = opcode,
          .lhs = lhs,
          .rhs = rhs,
      };
    case c4c::backend::bir::BinaryOpcode::Ugt:
      return Rv64NormalizedBranchPredicate{
          .opcode = c4c::backend::bir::BinaryOpcode::Ult,
          .lhs = rhs,
          .rhs = lhs,
      };
    case c4c::backend::bir::BinaryOpcode::Ule:
      return Rv64NormalizedBranchPredicate{
          .opcode = c4c::backend::bir::BinaryOpcode::Uge,
          .lhs = rhs,
          .rhs = lhs,
      };
    default:
      return std::nullopt;
  }
}

bool prepared_branch_condition_is_supported_pointer_branch(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  return branch_condition.kind ==
             c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare &&
         branch_condition.predicate.has_value() &&
         prepared_pointer_branch_predicate_is_supported(*branch_condition.predicate) &&
         branch_condition.compare_type.has_value() &&
         *branch_condition.compare_type == c4c::backend::bir::TypeKind::Ptr &&
         branch_condition.lhs.has_value() &&
         branch_condition.rhs.has_value();
}

bool rv64_prepared_condition_type_is_gpr_truth_value(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return true;
    default:
      return false;
  }
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_register_condition_branch(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& condition,
    std::string true_label,
    std::string false_label) {
  if (condition.kind != c4c::backend::bir::Value::Kind::Named ||
      !rv64_prepared_condition_type_is_gpr_truth_value(condition.type)) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for(names, lookups, condition);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto condition_register = gpr_register_number_for_home(*home);
  if (!condition_register.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_rv64_local_branch(fragment, 1, *condition_register, 0, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_fused_integer_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedValueLocationFunction* value_locations,
    const c4c::backend::bir::Function& function,
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string true_label,
    std::string false_label,
    std::size_t incoming_stack_base_bytes,
    std::size_t stack_frame_bytes) {
  const auto is_rv64_gpr_integer = [](c4c::backend::bir::TypeKind type) {
    switch (type) {
      case c4c::backend::bir::TypeKind::I1:
      case c4c::backend::bir::TypeKind::I8:
      case c4c::backend::bir::TypeKind::I16:
      case c4c::backend::bir::TypeKind::I32:
      case c4c::backend::bir::TypeKind::I64:
        return true;
      default:
        return false;
    }
  };
  if (lhs.type != rhs.type || !is_rv64_gpr_integer(lhs.type)) {
    return std::nullopt;
  }

  const auto normalized = normalize_rv64_branch_predicate(opcode, lhs, rhs);
  if (!normalized.has_value()) {
    return std::nullopt;
  }
  const auto funct3 = rv64_branch_funct3(normalized->opcode);
  if (!funct3.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register_with_formal_stack_home(
          fragment,
          28,
          stack_layout,
          names,
          lookups,
          value_locations,
          function,
          normalized->lhs,
          incoming_stack_base_bytes,
          stack_frame_bytes) ||
      !append_rv64_move_value_to_register_with_formal_stack_home(
          fragment,
          29,
          stack_layout,
          names,
          lookups,
          value_locations,
          function,
          normalized->rhs,
          incoming_stack_base_bytes,
          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_fused_floating_branch(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    std::string true_label,
    std::string false_label) {
  if (branch_condition.kind !=
          c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value() ||
      !rv64_fp_compare_funct7(*branch_condition.compare_type).has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_fp_compare_to_register(fragment,
                                         names,
                                         lookups,
                                         *branch_condition.predicate,
                                         *branch_condition.compare_type,
                                         *branch_condition.lhs,
                                         *branch_condition.rhs,
                                         28)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, 1, 28, 0, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}

struct Rv64SelectedBranchStackLoadSourceFreshnessStatus {
  bool freshness_required = false;
  bool available = true;
  c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus authority_status =
      c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::Available;
  c4c::backend::prepare::PreparedValueFreshnessQueryStatus
      source_freshness_status =
          c4c::backend::prepare::PreparedValueFreshnessQueryStatus::Selected;
  std::size_t source_freshness_candidates = 0;
};

Rv64SelectedBranchStackLoadSourceFreshnessStatus
selected_branch_stack_load_source_freshness_status(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value* value,
    const c4c::backend::prepare::PreparedValueHome* home,
    c4c::backend::prepare::PreparedBranchStackLoadRole role,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  Rv64SelectedBranchStackLoadSourceFreshnessStatus status;
  if (lookups == nullptr || home == nullptr ||
      home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
    return status;
  }
  status.freshness_required = true;
  status.available = false;
  status.authority_status =
      c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
          MissingSourceFreshnessAuthority;
  status.source_freshness_status =
      c4c::backend::prepare::PreparedValueFreshnessQueryStatus::NoCandidate;
  if (value == nullptr ||
      value->kind != c4c::backend::bir::Value::Kind::Named ||
      value->name.empty()) {
    status.authority_status =
        c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
            UnsupportedBranchValue;
    status.source_freshness_status =
        c4c::backend::prepare::PreparedValueFreshnessQueryStatus::MissingValue;
    return status;
  }
  const auto value_name = names.value_names.find(value->name);
  if (value_name == c4c::kInvalidValueName || value_name != home->value_name ||
      home->value_id == c4c::backend::prepare::PreparedValueId{0}) {
    status.authority_status =
        c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
            HomeValueMismatch;
    status.source_freshness_status =
        c4c::backend::prepare::PreparedValueFreshnessQueryStatus::MissingValue;
    return status;
  }

  for (const auto& record : lookups->branch_stack_load_authorities.records) {
    const auto& authority = record.authority;
    if (record.role != role ||
        record.block_label != block_label_id ||
        authority.value_id != home->value_id ||
        authority.value_name != home->value_name ||
        authority.branch_block_index != block_index ||
        authority.branch_terminator_instruction_index !=
            terminator_instruction_index) {
      continue;
    }
    status.authority_status = authority.status;
    status.source_freshness_status = authority.source_freshness_status;
    status.source_freshness_candidates =
        authority.source_freshness_authorities.size();
    if (!c4c::backend::prepare::prepared_branch_stack_load_authority_available(
            authority) ||
        !authority.source_freshness_authority.has_value()) {
      return status;
    }
    const auto& freshness = *authority.source_freshness_authority;
    if (freshness.value_id == home->value_id &&
        freshness.value_name == home->value_name &&
        freshness.use_kind ==
            c4c::backend::prepare::PreparedValueFreshnessUseKind::
                BranchStackLoadSource &&
        freshness.source_kind ==
            c4c::backend::prepare::PreparedValueFreshnessSourceKind::
                BranchStackSlot &&
        freshness.proof_kind ==
            c4c::backend::prepare::PreparedValueFreshnessProofKind::
                BranchTerminatorOrdering &&
        freshness.rank ==
            c4c::backend::prepare::PreparedValueFreshnessSourceRank::
                BranchStackSlot &&
        freshness.reference.home == home &&
        freshness.reference.block_index == block_index &&
        freshness.reference.instruction_index ==
            terminator_instruction_index) {
      status.available = true;
      return status;
    }
    status.authority_status =
        c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
            UnsupportedSourceFreshnessAuthority;
  }
  return status;
}

Rv64SelectedBranchStackLoadSourceFreshnessStatus
selected_lhs_branch_stack_load_source_freshness_status(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* lhs_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_branch_stack_load_source_freshness_status(
      names,
      lookups,
      branch_condition.lhs.has_value() ? &*branch_condition.lhs : nullptr,
      lhs_home,
      c4c::backend::prepare::PreparedBranchStackLoadRole::Lhs,
      block_label_id,
      block_index,
      terminator_instruction_index);
}

Rv64SelectedBranchStackLoadSourceFreshnessStatus
selected_rhs_branch_stack_load_source_freshness_status(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* rhs_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_branch_stack_load_source_freshness_status(
      names,
      lookups,
      branch_condition.rhs.has_value() ? &*branch_condition.rhs : nullptr,
      rhs_home,
      c4c::backend::prepare::PreparedBranchStackLoadRole::Rhs,
      block_label_id,
      block_index,
      terminator_instruction_index);
}

Rv64SelectedBranchStackLoadSourceFreshnessStatus
selected_condition_branch_stack_load_source_freshness_status(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* condition_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_branch_stack_load_source_freshness_status(
      names,
      lookups,
      &branch_condition.condition_value,
      condition_home,
      c4c::backend::prepare::PreparedBranchStackLoadRole::Condition,
      block_label_id,
      block_index,
      terminator_instruction_index);
}

bool selected_lhs_branch_stack_load_source_freshness_available(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* lhs_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_lhs_branch_stack_load_source_freshness_status(
             names,
             lookups,
             branch_condition,
             lhs_home,
             block_label_id,
             block_index,
             terminator_instruction_index)
      .available;
}

bool selected_rhs_branch_stack_load_source_freshness_available(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* rhs_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_rhs_branch_stack_load_source_freshness_status(
             names,
             lookups,
             branch_condition,
             rhs_home,
             block_label_id,
             block_index,
             terminator_instruction_index)
      .available;
}

bool selected_condition_branch_stack_load_source_freshness_available(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* condition_home,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  return selected_condition_branch_stack_load_source_freshness_status(
             names,
             lookups,
             branch_condition,
             condition_home,
             block_label_id,
             block_index,
             terminator_instruction_index)
      .available;
}

bool selected_lhs_stack_branch_freshness_allows_pointer_publication(
    const c4c::backend::prepare::PreparedFusedPointerBranchPublication& publication,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* lhs_home,
    const c4c::backend::prepare::PreparedValueHome* rhs_home) {
  if (publication.status !=
          c4c::backend::prepare::PreparedFusedPointerBranchPublicationStatus::
              UnsupportedOperandHome ||
      lhs_home == nullptr ||
      lhs_home->kind !=
          c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !branch_condition.rhs.has_value()) {
    return false;
  }
  const auto& rhs = *branch_condition.rhs;
  if (rhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return rhs.type == c4c::backend::bir::TypeKind::Ptr &&
           rhs.immediate_bits == 0;
  }
  return rhs.kind == c4c::backend::bir::Value::Kind::Named &&
         rhs_home != nullptr &&
         gpr_register_number_for_home(*rhs_home).has_value();
}

bool selected_rhs_stack_branch_freshness_allows_pointer_publication(
    const c4c::backend::prepare::PreparedFusedPointerBranchPublication& publication,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* lhs_home,
    const c4c::backend::prepare::PreparedValueHome* rhs_home) {
  if (publication.status !=
          c4c::backend::prepare::PreparedFusedPointerBranchPublicationStatus::
              UnsupportedOperandHome ||
      rhs_home == nullptr ||
      rhs_home->kind !=
          c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !branch_condition.lhs.has_value()) {
    return false;
  }
  const auto& lhs = *branch_condition.lhs;
  if (lhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return lhs.type == c4c::backend::bir::TypeKind::Ptr &&
           lhs.immediate_bits == 0;
  }
  return lhs.kind == c4c::backend::bir::Value::Kind::Named &&
         lhs_home != nullptr &&
         gpr_register_number_for_home(*lhs_home).has_value();
}

bool selected_condition_and_single_operand_stack_branch_freshness_allows_pointer_publication(
    const c4c::backend::prepare::PreparedFusedPointerBranchPublication& publication,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedValueHome* condition_home,
    const c4c::backend::prepare::PreparedValueHome* lhs_home,
    const c4c::backend::prepare::PreparedValueHome* rhs_home) {
  if (publication.status !=
          c4c::backend::prepare::PreparedFusedPointerBranchPublicationStatus::
              UnsupportedConditionHome ||
      condition_home == nullptr ||
      condition_home->kind !=
          c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return false;
  }

  const bool lhs_stack =
      lhs_home != nullptr &&
      lhs_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot;
  const bool rhs_stack =
      rhs_home != nullptr &&
      rhs_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot;
  if (lhs_stack == rhs_stack) {
    return false;
  }

  const auto other_operand_is_supported = [](
      const c4c::backend::bir::Value& value,
      const c4c::backend::prepare::PreparedValueHome* home) {
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      return value.type == c4c::backend::bir::TypeKind::Ptr &&
             value.immediate_bits == 0;
    }
    return value.kind == c4c::backend::bir::Value::Kind::Named &&
           home != nullptr &&
           gpr_register_number_for_home(*home).has_value();
  };
  return lhs_stack
             ? other_operand_is_supported(*branch_condition.rhs, rhs_home)
             : other_operand_is_supported(*branch_condition.lhs, lhs_home);
}

bool rv64_branch_stack_load_status_is_source_freshness_failure(
    const Rv64SelectedBranchStackLoadSourceFreshnessStatus& status) {
  return status.authority_status ==
             c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
                 MissingSourceFreshnessAuthority ||
         status.authority_status ==
             c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
                 InvalidSourceFreshnessAuthority ||
         status.authority_status ==
             c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
                 AmbiguousSourceFreshnessAuthority ||
         status.authority_status ==
             c4c::backend::prepare::PreparedBranchStackLoadAuthorityStatus::
                 UnsupportedSourceFreshnessAuthority;
}

std::string rv64_branch_stack_load_freshness_diagnostic(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index,
    std::string_view role_display_name,
    std::string_view role_field_name,
    const c4c::backend::bir::Value* value,
    const Rv64SelectedBranchStackLoadSourceFreshnessStatus& status) {
  std::ostringstream out;
  if (rv64_branch_stack_load_status_is_source_freshness_failure(status)) {
    out << "unsupported_branch_stack_load_source_freshness: RV64 fused pointer "
        << role_display_name
        << " stack-load requires selected BranchStackLoadSource freshness";
  } else {
    out << "unsupported_branch_stack_load_authority: RV64 fused pointer "
        << role_display_name
        << " stack-load requires available branch stack-load authority";
  }
  out << "; function=" << rv64_prepared_function_name(names, function_name)
      << "; block=" << rv64_prepared_block_label(names, block_label_id)
      << "; block_index=" << block_index
      << "; terminator_instruction_index=" << terminator_instruction_index
      << "; role=" << role_field_name;
  if (value != nullptr &&
      value->kind == c4c::backend::bir::Value::Kind::Named &&
      !value->name.empty()) {
    out << "; value=" << value->name;
  }
  out << "; authority_status="
      << c4c::backend::prepare::prepared_branch_stack_load_authority_status_name(
             status.authority_status)
      << "; source_freshness_status="
      << c4c::backend::prepare::prepared_value_freshness_query_status_name(
             status.source_freshness_status)
      << "; source_freshness_candidates="
      << status.source_freshness_candidates;
  return out.str();
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_fused_pointer_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedValueLocationFunction* value_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::bir::Terminator& terminator,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::size_t terminator_instruction_index,
    std::string true_label,
    std::string false_label,
    std::size_t incoming_stack_base_bytes,
    std::size_t stack_frame_bytes) {
  if (!prepared_branch_condition_is_supported_pointer_branch(branch_condition)) {
    return std::nullopt;
  }

  const auto* condition_home =
      prepared_value_home_for(names, lookups, branch_condition.condition_value);
  const auto* lhs_home =
      prepared_pointer_branch_operand_home_for(names, lookups, *branch_condition.lhs);
  const auto* rhs_home =
      prepared_pointer_branch_operand_home_for(names, lookups, *branch_condition.rhs);
  if (!selected_condition_branch_stack_load_source_freshness_available(
          names,
          lookups,
          branch_condition,
          condition_home,
          block_label_id,
          block_index,
          terminator_instruction_index)) {
    return std::nullopt;
  }
  if (!selected_lhs_branch_stack_load_source_freshness_available(
          names,
          lookups,
          branch_condition,
          lhs_home,
          block_label_id,
          block_index,
          terminator_instruction_index)) {
    return std::nullopt;
  }
  if (!selected_rhs_branch_stack_load_source_freshness_available(
          names,
          lookups,
          branch_condition,
          rhs_home,
          block_label_id,
          block_index,
          terminator_instruction_index)) {
    return std::nullopt;
  }
  const auto publication =
      c4c::backend::prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .condition_home = condition_home,
          .lhs_home = lhs_home,
          .rhs_home = rhs_home,
      });
  if (!c4c::backend::prepare::prepared_fused_pointer_branch_publication_available(
          publication) &&
      !selected_lhs_stack_branch_freshness_allows_pointer_publication(
          publication, branch_condition, lhs_home, rhs_home) &&
      !selected_rhs_stack_branch_freshness_allows_pointer_publication(
          publication, branch_condition, lhs_home, rhs_home) &&
      !selected_condition_and_single_operand_stack_branch_freshness_allows_pointer_publication(
          publication, branch_condition, condition_home, lhs_home, rhs_home)) {
    return std::nullopt;
  }
  const auto normalized = normalize_prepared_pointer_branch_predicate(
      publication.predicate,
      *branch_condition.lhs,
      *branch_condition.rhs);
  if (!normalized.has_value()) {
    return std::nullopt;
  }
  const auto funct3 = rv64_branch_funct3(normalized->opcode);
  if (!funct3.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register_with_formal_stack_home(
          fragment,
          28,
          stack_layout,
          names,
          lookups,
          value_locations,
          function,
          normalized->lhs,
          incoming_stack_base_bytes,
          stack_frame_bytes) ||
      !append_rv64_move_value_to_register_with_formal_stack_home(
          fragment,
          29,
          stack_layout,
          names,
          lookups,
          value_locations,
          function,
          normalized->rhs,
          incoming_stack_base_bytes,
          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_terminator(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::string_view function_name,
    const std::unordered_map<std::string, PreparedObjectCompare>& compares,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const std::unordered_set<PreparedBeforeReturnStackToRegisterKey,
                             PreparedBeforeReturnStackToRegisterKeyHash>*
        prepared_before_return_stack_to_register_values,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  switch (block.terminator.kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return fragment_for_prepared_return(prepared.stack_layout,
                                          names,
                                          lookups,
                                          frame_plan,
                                          block.terminator,
                                          block_index,
                                          block.insts.size(),
                                          prepared_before_return_stack_to_register_values,
                                          restore_return_address,
                                          stack_frame_bytes);
    case c4c::backend::bir::TerminatorKind::Branch: {
      const std::string target = bir_target_label_spelling(
          prepared.module,
          block.terminator.target_label_id,
          block.terminator.target_label);
      RiscvEncodedFragment fragment;
      append_rv64_local_jump(
          fragment,
          riscv_local_block_label(function_name, target));
      return fragment;
    }
    case c4c::backend::bir::TerminatorKind::CondBranch: {
      const std::string true_label = bir_target_label_spelling(
          prepared.module,
          block.terminator.true_label_id,
          block.terminator.true_label);
      const std::string false_label = bir_target_label_spelling(
          prepared.module,
          block.terminator.false_label_id,
          block.terminator.false_label);
      const std::string true_asm_label = riscv_local_block_label(function_name, true_label);
      const std::string false_asm_label = riscv_local_block_label(function_name, false_label);

      if (const auto condition_imm = integer_immediate_for_value(
              names,
              lookups,
              block.terminator.condition)) {
        RiscvEncodedFragment fragment;
        append_rv64_local_jump(fragment,
                               *condition_imm != 0 ? true_asm_label : false_asm_label);
        return fragment;
      }

      const auto* branch_condition =
          find_branch_condition_for_terminator(
              control_flow,
              block_label_id,
              block.terminator.condition);
      const auto* value_locations =
          prepare::find_prepared_value_location_function(prepared,
                                                         control_flow.function_name);
      auto incoming_stack_base_bytes = stack_frame_bytes;
      if (restore_return_address) {
        const auto call_frame_size =
            rv64_prepared_call_frame_size(stack_frame_bytes);
        if (!call_frame_size.has_value()) {
          return std::nullopt;
        }
        incoming_stack_base_bytes = *call_frame_size;
      }
      if (branch_condition != nullptr &&
          branch_condition->kind ==
              c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare &&
          branch_condition->predicate.has_value() &&
          branch_condition->lhs.has_value() &&
          branch_condition->rhs.has_value()) {
        if (prepared_branch_condition_is_supported_pointer_branch(*branch_condition)) {
          return fragment_for_prepared_fused_pointer_branch(prepared.stack_layout,
                                                            names,
                                                            lookups,
                                                            value_locations,
                                                            function,
                                                            *branch_condition,
                                                            block.terminator,
                                                            block_label_id,
                                                            block_index,
                                                            block.insts.size(),
                                                            true_asm_label,
                                                            false_asm_label,
                                                            incoming_stack_base_bytes,
                                                            stack_frame_bytes);
        }
        if (auto fused_integer_branch =
                fragment_for_prepared_fused_integer_branch(prepared.stack_layout,
                                                           names,
                                                           lookups,
                                                           value_locations,
                                                           function,
                                                           *branch_condition->predicate,
                                                           *branch_condition->lhs,
                                                           *branch_condition->rhs,
                                                           true_asm_label,
                                                           false_asm_label,
                                                           incoming_stack_base_bytes,
                                                           stack_frame_bytes)) {
          return fused_integer_branch;
        }
        if (auto fused_floating_branch =
                fragment_for_prepared_fused_floating_branch(names,
                                                            lookups,
                                                            *branch_condition,
                                                            true_asm_label,
                                                            false_asm_label)) {
          return fused_floating_branch;
        }
        return fragment_for_prepared_compare_branch(prepared.stack_layout,
                                                    names,
                                                    lookups,
                                                    *branch_condition->predicate,
                                                    *branch_condition->lhs,
                                                    *branch_condition->rhs,
                                                    true_asm_label,
                                                    false_asm_label,
                                                    stack_frame_bytes);
      }

      if (block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto compare_it = compares.find(block.terminator.condition.name);
      if (compare_it == compares.end()) {
        return fragment_for_prepared_register_condition_branch(
            names,
            lookups,
            block.terminator.condition,
            true_asm_label,
            false_asm_label);
      }
      return fragment_for_prepared_compare_branch(prepared.stack_layout,
                                                  names,
                                                  lookups,
                                                  compare_it->second.opcode,
                                                  compare_it->second.lhs,
                                                  compare_it->second.rhs,
                                                  true_asm_label,
                                                  false_asm_label,
                                                  stack_frame_bytes);
    }
  }
  return std::nullopt;
}

std::optional<std::string>
diagnose_unsupported_prepared_terminator_fragment(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups& lookups,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Block& block,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index) {
  if (block.terminator.kind !=
      c4c::backend::bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition =
      find_branch_condition_for_terminator(control_flow,
                                           block_label_id,
                                           block.terminator.condition);
  if (branch_condition == nullptr ||
      !prepared_branch_condition_is_supported_pointer_branch(*branch_condition)) {
    return std::nullopt;
  }
  const auto* condition_home =
      prepared_value_home_for(names, &lookups, branch_condition->condition_value);
  const auto condition_status =
      selected_condition_branch_stack_load_source_freshness_status(
          names,
          &lookups,
          *branch_condition,
          condition_home,
          block_label_id,
          block_index,
          block.insts.size());
  if (condition_status.freshness_required && !condition_status.available) {
    return rv64_branch_stack_load_freshness_diagnostic(
        names,
        function_name,
        block_label_id,
        block_index,
        block.insts.size(),
        "Condition",
        "condition",
        &branch_condition->condition_value,
        condition_status);
  }

  const auto* lhs_home =
      prepared_pointer_branch_operand_home_for(names,
                                               &lookups,
                                               *branch_condition->lhs);
  const auto lhs_status =
      selected_lhs_branch_stack_load_source_freshness_status(
          names,
          &lookups,
          *branch_condition,
          lhs_home,
          block_label_id,
          block_index,
          block.insts.size());
  if (lhs_status.freshness_required && !lhs_status.available) {
    return rv64_branch_stack_load_freshness_diagnostic(
        names,
        function_name,
        block_label_id,
        block_index,
        block.insts.size(),
        "Lhs",
        "lhs",
        branch_condition->lhs.has_value() ? &*branch_condition->lhs : nullptr,
        lhs_status);
  }

  const auto* rhs_home =
      prepared_pointer_branch_operand_home_for(names,
                                               &lookups,
                                               *branch_condition->rhs);
  const auto rhs_status =
      selected_rhs_branch_stack_load_source_freshness_status(
          names,
          &lookups,
          *branch_condition,
          rhs_home,
          block_label_id,
          block_index,
          block.insts.size());
  if (rhs_status.freshness_required && !rhs_status.available) {
    return rv64_branch_stack_load_freshness_diagnostic(
        names,
        function_name,
        block_label_id,
        block_index,
        block.insts.size(),
        "Rhs",
        "rhs",
        branch_condition->rhs.has_value() ? &*branch_condition->rhs : nullptr,
        rhs_status);
  }
  return std::nullopt;
}

bool prepared_binary_result_is_rematerializable_i32_immediate(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary) {
  if (c4c::backend::bir::is_compare_opcode(binary.opcode) ||
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  const auto* home = prepared_value_home_for(names, lookups, binary.result);
  if (home == nullptr) {
    return false;
  }
  const auto report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(home);
  return report.owner_class == prepare::PreparedContractOwnerClass::Coherent &&
         prepare::as_rematerializable_integer_immediate_fact(*home).has_value();
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_instruction(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups& lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords&
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords&
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedInlineAsmCarrierFunction* inline_asm_carriers,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId prepared_block_label,
    std::string_view function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::Inst& inst,
    std::unordered_map<std::string, PreparedObjectCompare>& compares,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan,
    std::size_t incoming_stack_base_bytes,
    std::size_t stack_frame_bytes) {
  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr) {
    if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
      if (prepared_binary_is_select_edge_owned_source(prepared.names,
                                                      control_flow,
                                                      function,
                                                      &lookups,
                                                      &dependency_operand_authorities,
                                                      &carrier_alias_authorities,
                                                      *binary)) {
        return RiscvEncodedFragment{};
      }
      if (prepared_binary_result_is_rematerializable_i32_immediate(
              prepared.names,
              &lookups,
              *binary)) {
        return RiscvEncodedFragment{};
      }
    } else if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
      if (prepared_cast_is_available_select_edge_dependency_authority_source(
              prepared.names,
              control_flow,
              control_flow.function_name,
              function,
              prepared_block_label,
              instruction_index,
              dependency_operand_authorities,
              *cast)) {
        return RiscvEncodedFragment{};
      }
    }
    if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
      if (auto fragment =
              fragment_for_prepared_pointer_result_frame_address_materialization(
                  prepared.stack_layout,
                  prepared.names,
                  &lookups,
                  prepared_block_label,
                  instruction_index,
                  *binary,
                  stack_frame_bytes);
          fragment.has_value()) {
        return fragment;
      }
      if (auto fragment = fragment_for_prepared_frame_address_materialization(
              prepared.stack_layout,
              prepared.names,
              &lookups,
              prepared_block_label,
              instruction_index,
              *binary,
              stack_frame_bytes);
          fragment.has_value()) {
        return fragment;
      }
      if (auto fragment = fragment_for_prepared_symbol_address_materialization(
              prepared,
              prepared.names,
              &lookups,
              prepared_block_label,
              instruction_index,
              *binary);
          fragment.has_value()) {
        return fragment;
      }
      if (c4c::backend::bir::is_compare_opcode(binary->opcode) &&
          binary->result.kind == c4c::backend::bir::Value::Kind::Named) {
        compares[binary->result.name] = PreparedObjectCompare{
            .opcode = binary->opcode,
            .lhs = binary->lhs,
            .rhs = binary->rhs,
        };
        if (terminator_uses_value_as_condition(block, binary->result)) {
          return RiscvEncodedFragment{};
        }
        if (auto fragment = fragment_for_prepared_scalar_compare_trunc_source(
                prepared.stack_layout,
                prepared.names,
                &lookups,
                block,
                block_index,
                instruction_index,
                *binary,
                stack_frame_bytes);
            fragment.has_value()) {
          return fragment;
        }
        if (auto fragment = fragment_for_prepared_fp_compare_publication(
                prepared.names,
                &lookups,
                *binary);
            fragment.has_value()) {
          return fragment;
        }
      }
      if (auto fragment =
              fragment_for_prepared_narrow_bitfield_binary(prepared.stack_layout,
                                                           prepared.names,
                                                           &lookups,
                                                           *binary,
                                                           stack_frame_bytes);
          fragment.has_value()) {
        return fragment;
      }
      if (auto fragment = fragment_for_prepared_pointer_add(prepared.stack_layout,
                                                            prepared.target_profile,
                                                            prepared.names,
                                                            &lookups,
                                                            &function,
                                                            storage_plan,
                                                            *binary,
                                                            stack_frame_bytes);
          fragment.has_value()) {
        return fragment;
      }
      auto fragment = fragment_for_prepared_binary(prepared.stack_layout,
                                                   prepared.names,
                                                   &lookups,
                                                   *binary,
                                                   stack_frame_bytes,
                                                   block_index,
                                                   instruction_index);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
      if (prepared_select_is_authorized_carrier_alias(control_flow.function_name,
                                                      prepared.names,
                                                      &carrier_alias_authorities,
                                                      *select)) {
        return RiscvEncodedFragment{};
      }
      const auto classification =
          prepare::classify_prepared_object_select_consumer(&control_flow,
                                                            prepared_block_label,
                                                            inst);
      if (prepare::diagnose_prepared_object_consumer(classification).has_value()) {
        return std::nullopt;
      }
      if (classification.kind ==
              prepare::PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier &&
          classification.join_transfer != nullptr &&
          prepared_join_transfer_edge_copies_are_published(control_flow,
                                                           *classification.join_transfer)) {
        const auto* result_home =
            prepared_value_home_for(prepared.names, &lookups, select->result);
        if (result_home != nullptr &&
            (gpr_register_number_for_home(*result_home).has_value() ||
             fpr_register_number_for_home(*result_home).has_value())) {
          return RiscvEncodedFragment{};
        }
      }
      if (classification.kind ==
              prepare::PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier &&
          classification.join_transfer != nullptr &&
          prepared_join_transfer_edge_copies_are_published(control_flow,
                                                           *classification.join_transfer)) {
        const auto* result_home =
            prepared_value_home_for(prepared.names, &lookups, select->result);
        if (result_home == nullptr) {
          return std::nullopt;
        }
        if (result_home->kind != prepare::PreparedValueHomeKind::StackSlot) {
          return std::nullopt;
        }
      }
      if (classification.kind ==
              prepare::PreparedObjectSelectConsumerKind::OrdinarySelect ||
          classification.kind ==
              prepare::PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier) {
        auto fragment = fragment_for_prepared_select(prepared.stack_layout,
                                                     prepared.names,
                                                     &lookups,
                                                     function_name,
                                                     bir_block_label_spelling(prepared.module,
                                                                              block),
                                                     instruction_index,
                                                     *select,
                                                     stack_frame_bytes,
                                                     &block,
                                                     std::nullopt,
                                                     0);
        if (fragment.has_value()) {
          return fragment;
        }
      }
    }
    if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
      auto fragment =
          fragment_for_prepared_width_preserving_i32_cast(prepared.names,
                                                          &lookups,
                                                          *cast);
      if (!fragment.has_value() &&
          prepared_width_preserving_i32_zext_is_materialized_by_move_bundle(
              prepared.names,
              &lookups,
              block_index,
              instruction_index,
              *cast)) {
        return RiscvEncodedFragment{};
      }
      if (!fragment.has_value()) {
        fragment = fragment_for_prepared_cast(prepared.stack_layout,
                                              prepared.names,
                                              &lookups,
                                              *cast,
                                              stack_frame_bytes);
      }
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (c4c::backend::bir::is_vrm_register_type(store->value.type)) {
        return RiscvEncodedFragment{};
      }
      const auto* access = prepared_memory_access_for_local_instruction(
          prepared.names,
          &lookups,
          prepared_block_label,
          instruction_index,
          *store);
      if (auto fragment =
              fragment_for_prepared_16_byte_byte_storage_global_store_local_publication(
                  prepared,
                  prepared.stack_layout,
                  &lookups,
                  control_flow.function_name,
                  prepared_block_label,
                  instruction_index,
                  *store,
                  access,
                  stack_frame_bytes)) {
        return fragment;
      }
      auto fragment = fragment_for_prepared_store_local(
          prepared.stack_layout,
          prepared.names,
          &lookups,
          prepared_block_label,
          instruction_index,
          *store,
          access,
          stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      if (c4c::backend::bir::is_vrm_register_type(load->result.type)) {
        return RiscvEncodedFragment{};
      }
      const auto function_id = prepared.names.function_names.find(function_name);
      auto fragment = fragment_for_prepared_load_local(
          prepared,
          function_id,
          block_index,
          instruction_index,
          prepared.stack_layout,
          prepared.names,
          &lookups,
          *load,
          prepared_memory_access_for_local_instruction(
              prepared.names,
              &lookups,
              prepared_block_label,
              instruction_index,
              *load),
          incoming_stack_base_bytes,
          stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
      if (auto fragment =
              fragment_for_prepared_16_byte_byte_storage_global_store(
                  prepared,
                  prepared.stack_layout,
                  prepared.names,
                  &lookups,
                  *store,
                  prepared_memory_access_for_instruction(
                      &lookups,
                      prepared_block_label,
                      instruction_index),
                  stack_frame_bytes)) {
        return fragment;
      }
      auto fragment = fragment_for_prepared_store_global(
          prepared,
          prepared.stack_layout,
          prepared.names,
          &lookups,
          *store,
          prepared_memory_access_for_instruction(
              &lookups,
              prepared_block_label,
              instruction_index),
          stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst)) {
      if (auto fragment =
              fragment_for_prepared_f32_byte_storage_global_load(
                  prepared,
                  prepared.stack_layout,
                  prepared.names,
                  &lookups,
                  *load,
                  prepared_memory_access_for_instruction(
                      &lookups,
                      prepared_block_label,
                      instruction_index),
                  stack_frame_bytes)) {
        return fragment;
      }
      if (auto fragment =
              fragment_for_prepared_16_byte_byte_storage_global_load(
                  prepared,
                  prepared.stack_layout,
                  prepared.names,
                  &lookups,
                  *load,
                  prepared_memory_access_for_instruction(
                      &lookups,
                      prepared_block_label,
                      instruction_index),
                  stack_frame_bytes)) {
        return fragment;
      }
      auto fragment = fragment_for_prepared_load_global(
          prepared,
          prepared.stack_layout,
          prepared.names,
          &lookups,
          *load,
          prepared_memory_access_for_instruction(
              &lookups,
              prepared_block_label,
              instruction_index),
          stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (prepared_pure_instruction_is_rematerialized_immediate(prepared.names,
                                                             &lookups,
                                                             inst)) {
      return RiscvEncodedFragment{};
    }
    return std::nullopt;
  }

  if (auto fragment =
          fragment_for_prepared_variadic_va_start(prepared,
                                                  control_flow.function_name,
                                                  block_index,
                                                  instruction_index,
                                                  *call,
                                                  stack_frame_bytes)) {
    return fragment;
  }
  if (auto fragment =
          fragment_for_prepared_variadic_va_arg_aggregate(prepared,
                                                          control_flow.function_name,
                                                          block_index,
                                                          instruction_index,
                                                          *call,
                                                          stack_frame_bytes)) {
    return fragment;
  }

  const auto* call_plan = prepare::find_indexed_prepared_call_plan(
      &lookups.call_plans,
      prepare::find_prepared_call_plans(prepared, control_flow.function_name),
      block_index,
      instruction_index);
  if (auto fragment = fragment_for_prepared_variadic_va_end(call_plan, *call)) {
    return fragment;
  }
  if (is_rv64_variadic_va_end_call(*call)) {
    return std::nullopt;
  }
  const auto* inline_asm_carrier =
      find_prepared_inline_asm_carrier(inline_asm_carriers, block_index, instruction_index);
  return fragment_for_prepared_call(prepared,
                                    prepared.stack_layout,
                                    &lookups,
                                    frame_plan,
                                    function_name,
                                    block_index,
                                    instruction_index,
                                    call_plan,
                                    inline_asm_carrier,
                                    *call,
                                    stack_frame_bytes);
}

bool prepared_object_traversal_is_complete_bir_stream(
    const std::vector<prepare::PreparedObjectTraversalEvent>& traversal) {
  for (const auto& event : traversal) {
    if (event.bir_block == nullptr) {
      return false;
    }
    if (event.kind == prepare::PreparedObjectTraversalEventKind::Instruction &&
        event.instruction == nullptr) {
      return false;
    }
    if (event.kind == prepare::PreparedObjectTraversalEventKind::Terminator &&
        event.terminator == nullptr) {
      return false;
    }
  }
  return !traversal.empty();
}

std::optional<std::string> diagnose_unsupported_prepared_instruction_fragment(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups& lookups,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId prepared_block_label,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Inst& inst,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  const auto pointer_arithmetic_diagnostic =
      [&](const bir::BinaryInst& binary) -> std::optional<std::string> {
    const auto same_named_value = [](const bir::Value& lhs,
                                     const bir::Value& rhs) {
      return lhs.kind == bir::Value::Kind::Named &&
             rhs.kind == bir::Value::Kind::Named &&
             lhs.name == rhs.name &&
             lhs.type == rhs.type;
    };
    const auto value_is_loaded_pointer = [&](const bir::Value& value) {
      if (value.type != bir::TypeKind::Ptr) {
        return false;
      }
      for (std::size_t index = 0; index < instruction_index &&
                                  index < block.insts.size();
           ++index) {
        const auto* load = std::get_if<bir::LoadLocalInst>(&block.insts[index]);
        if (load != nullptr && same_named_value(load->result, value)) {
          return true;
        }
      }
      return false;
    };
    const auto value_is_scaled_integer_offset = [&](const bir::Value& value) {
      if (value.type == bir::TypeKind::Ptr) {
        return false;
      }
      if (value.kind == bir::Value::Kind::Immediate) {
        return true;
      }
      for (std::size_t index = 0; index < instruction_index &&
                                  index < block.insts.size();
           ++index) {
        const auto* producer =
            std::get_if<bir::BinaryInst>(&block.insts[index]);
        if (producer != nullptr &&
            producer->opcode == bir::BinaryOpcode::Mul &&
            same_named_value(producer->result, value)) {
          return true;
        }
      }
      return false;
    };

    const bool opcode_is_pointer_arithmetic =
        binary.opcode == bir::BinaryOpcode::Add ||
        binary.opcode == bir::BinaryOpcode::Sub;
    const bool lhs_is_pointer_offset =
        value_is_loaded_pointer(binary.lhs) &&
        value_is_scaled_integer_offset(binary.rhs);
    const bool rhs_is_pointer_offset =
        binary.opcode == bir::BinaryOpcode::Add &&
        value_is_loaded_pointer(binary.rhs) &&
        value_is_scaled_integer_offset(binary.lhs);
    if (!opcode_is_pointer_arithmetic ||
        binary.result.type != bir::TypeKind::Ptr ||
        (!lhs_is_pointer_offset && !rhs_is_pointer_offset)) {
      return std::nullopt;
    }
    if (fragment_for_prepared_pointer_result_frame_address_materialization(
            stack_layout,
            names,
            &lookups,
            prepared_block_label,
            instruction_index,
            binary,
            stack_frame_bytes)
            .has_value() ||
        fragment_for_prepared_frame_address_materialization(stack_layout,
                                                            names,
                                                            &lookups,
                                                            prepared_block_label,
                                                            instruction_index,
                                                            binary,
                                                            stack_frame_bytes)
            .has_value() ||
        fragment_for_prepared_binary(stack_layout,
                                     names,
                                     &lookups,
                                     binary,
                                     stack_frame_bytes)
            .has_value()) {
      return std::nullopt;
    }
    std::ostringstream out;
    out << "unsupported_pointer_arithmetic: RV64 object route requires prepared pointer arithmetic lowering for loaded pointer base plus scaled integer byte offset"
        << "; function=" << rv64_prepared_function_name(names, function_name)
        << "; block=" << rv64_prepared_block_label(names, prepared_block_label)
        << "; block_index=" << block_index
        << "; instruction_index=" << instruction_index
        << "; instruction_kind=BinaryInst"
        << "; owner=" << bir::render_type(binary.result.type);
    if (binary.result.kind == bir::Value::Kind::Named &&
        !binary.result.name.empty()) {
      out << " " << binary.result.name;
    } else if (binary.result.kind == bir::Value::Kind::Immediate) {
      out << " immediate";
    }
    return out.str();
  };

  const auto local_memory_diagnostic =
      [&](const std::optional<std::size_t>& size_bytes,
          const prepare::PreparedMemoryAccess* access,
          bool f64_memory = false,
          bool allow_string_constant_load = false) -> std::optional<std::string> {
    if (!size_bytes.has_value()) {
      return std::string{
          "unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses"};
    }
    if (f64_memory) {
      if (!prepared_frame_slot_absolute_byte_offset(stack_layout,
                                                    access,
                                                    stack_frame_bytes,
                                                    *size_bytes)
               .has_value() &&
          !prepared_pointer_value_base_offset(&lookups, access, *size_bytes)
               .has_value() &&
          rv64_large_selected_pointer_offset_materialization_status(
              &lookups,
              access,
              *size_bytes) !=
              Rv64LargeSelectedPointerOffsetMaterializationStatus::Available &&
          !prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                           &lookups,
                                                           access,
                                                           stack_frame_bytes,
                                                           *size_bytes)
               .has_value() &&
          !prepared_sret_stack_slot_pointer_access(stack_layout,
                                                   &lookups,
                                                   access,
                                                   stack_frame_bytes,
                                                   *size_bytes)
               .has_value() &&
          !prepared_pointer_value_stack_home_base_offset(stack_layout,
                                                        &lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes)
               .has_value()) {
        return std::string{
            "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing"};
      }
      return std::nullopt;
    }
    const auto string_constant_local_load_is_supported = [&]() {
      if (!allow_string_constant_load || *size_bytes != 8 || access == nullptr ||
          access->address_space != bir::AddressSpace::Default ||
          access->is_volatile ||
          access->address.base_kind !=
              prepare::PreparedAddressBaseKind::StringConstant ||
          access->address.size_bytes != 8 ||
          access->address.align_bytes != 8 ||
          !fits_signed_12_bit_immediate(access->address.byte_offset) ||
          !prepare::prepared_string_constant_label_pointer_has_authority(
              access->address)) {
        return false;
      }
      const std::string_view label =
          access->address.symbol_name.has_value()
              ? names.link_names.spelling(*access->address.symbol_name)
              : std::string_view{};
      return !label.empty();
    };
    const auto scalar_direct_global_local_access_is_supported = [&]() {
      if (access == nullptr ||
          access->address_space != bir::AddressSpace::Default ||
          access->is_volatile ||
          access->address.base_kind !=
              prepare::PreparedAddressBaseKind::GlobalSymbol ||
          access->address.global_address_materialization_policy !=
              bir::GlobalAddressMaterializationPolicy::Direct ||
          access->address.size_bytes != *size_bytes ||
          access->address.align_bytes > *size_bytes ||
          access->address.provenance.layout_authority !=
              bir::MemoryLayoutAuthorityKind::ScalarLayout ||
          !fits_signed_12_bit_immediate(access->address.byte_offset) ||
          !prepare::prepared_global_symbol_memory_has_publication_authority(
              access->address)) {
        return false;
      }
      const std::string_view label =
          access->address.symbol_name.has_value()
              ? names.link_names.spelling(*access->address.symbol_name)
              : std::string_view{};
      return !label.empty();
    };
    if (!prepared_frame_slot_absolute_byte_offset(stack_layout,
                                                  access,
                                                  stack_frame_bytes,
                                                  *size_bytes)
             .has_value() &&
        !prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                         &lookups,
                                                         access,
                                                         stack_frame_bytes,
                                                         *size_bytes)
             .has_value() &&
        !prepared_pointer_value_base_offset(&lookups, access, *size_bytes)
             .has_value() &&
        rv64_large_selected_pointer_offset_materialization_status(
            &lookups,
            access,
            *size_bytes) !=
            Rv64LargeSelectedPointerOffsetMaterializationStatus::Available &&
        !prepared_pointer_value_stack_home_base_offset(stack_layout,
                                                      &lookups,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes)
             .has_value() &&
        !scalar_direct_global_local_access_is_supported() &&
        !string_constant_local_load_is_supported()) {
      return std::string{
          "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing"};
    }
    return std::nullopt;
  };

  if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst)) {
    const auto size_bytes = rv64_local_memory_size_for_type(store->value.type);
    const auto* access = prepared_memory_access_for_local_instruction(names,
                                                                     &lookups,
                                                                     prepared_block_label,
                                                                     instruction_index,
                                                                     *store);
    if (size_bytes.has_value() &&
        prepared_sret_stack_slot_pointer_access(stack_layout,
                                                &lookups,
                                                access,
                                                stack_frame_bytes,
                                                *size_bytes)
            .has_value()) {
      return std::nullopt;
    }
    return local_memory_diagnostic(
        size_bytes, access, rv64_floating_type(store->value.type));
  }
  if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst)) {
    const auto* access = prepared_memory_access_for_local_instruction(names,
                                                                     &lookups,
                                                                     prepared_block_label,
                                                                     instruction_index,
                                                                     *load);
    const auto diagnostic = local_memory_diagnostic(
        rv64_local_memory_size_for_type(load->result.type),
        access,
        rv64_floating_type(load->result.type),
        load->result.type == bir::TypeKind::Ptr);
    if (load->address.has_value() &&
        load->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol &&
        diagnostic.has_value()) {
      return std::string{
          "unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes"};
    }
    return diagnostic;
  }
  if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst)) {
    const auto access = prepared_memory_access_for_instruction(&lookups,
                                                               prepared_block_label,
                                                               instruction_index);
    const auto size_bytes = rv64_global_scalar_memory_size_for_type(load->result.type);
    const auto floating_size_bytes =
        rv64_global_floating_memory_size_for_type(load->result.type);
    if (!size_bytes.has_value() && !floating_size_bytes.has_value()) {
      if (load->result.type == bir::TypeKind::F32) {
        std::optional<c4c::ValueNameId> result_value_name;
        if (load->result.kind == bir::Value::Kind::Named) {
          result_value_name = names.value_names.find(load->result.name);
        }
        if (result_value_name.has_value() &&
            *result_value_name != c4c::kInvalidValueName &&
            prepared_global_byte_storage_access_has_fact_authority(
                access, *result_value_name, std::nullopt, 4, 4)) {
          return std::nullopt;
        }
        return std::string{
            access == nullptr
                ? "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing"
                : "unsupported_global_data: RV64 object route requires complete 4-byte byte-storage aggregate global-symbol memory facts"};
      }
      if (access != nullptr && access->address.size_bytes == 16) {
        std::optional<c4c::ValueNameId> result_value_name;
        if (load->result.kind == bir::Value::Kind::Named) {
          result_value_name = names.value_names.find(load->result.name);
        }
        if (result_value_name.has_value() &&
            *result_value_name != c4c::kInvalidValueName &&
            prepared_global_16_byte_byte_storage_access_has_fact_authority(
                access, *result_value_name, std::nullopt)) {
          return std::nullopt;
        }
        return std::string{
            "unsupported_global_data: RV64 object route requires complete 16-byte byte-storage aggregate global-symbol memory facts"};
      }
      if (load->result.type == bir::TypeKind::I128 ||
          load->result.type == bir::TypeKind::F128) {
        std::optional<c4c::ValueNameId> result_value_name;
        if (load->result.kind == bir::Value::Kind::Named) {
          result_value_name = names.value_names.find(load->result.name);
        }
        if (result_value_name.has_value() &&
            *result_value_name != c4c::kInvalidValueName &&
            prepared_global_16_byte_byte_storage_access_has_fact_authority(
                access, *result_value_name, std::nullopt)) {
          return std::nullopt;
        }
        return std::string{
            access == nullptr
                ? "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing"
                : "unsupported_global_data: RV64 object route requires complete 16-byte byte-storage aggregate global-symbol memory facts"};
      }
      return std::string{
          "unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses"};
    }
    return std::string{
        access == nullptr
            ? "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing"
            : "unsupported_global_data: RV64 object route requires supported prepared global memory facts"};
  }
  if (const auto* store = std::get_if<bir::StoreGlobalInst>(&inst)) {
    const auto access = prepared_memory_access_for_instruction(&lookups,
                                                               prepared_block_label,
                                                               instruction_index);
    const auto size_bytes = rv64_global_scalar_memory_size_for_type(store->value.type);
    if (!size_bytes.has_value()) {
      if (store->value.type == bir::TypeKind::I128) {
        return std::string{
            access == nullptr
                ? "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing"
                : "unsupported_global_data: RV64 object route requires complete 16-byte byte-storage aggregate global-symbol memory facts"};
      }
      return std::string{
          "unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses"};
    }
    return std::string{
        access == nullptr
            ? "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing"
            : "unsupported_global_data: RV64 object route requires supported prepared global memory facts"};
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    if (rv64_floating_type(cast->operand.type) ||
        rv64_floating_type(cast->result.type)) {
      return std::string{
          "unsupported_floating_cast: RV64 object route supports only prepared FPR width casts, I32/I64-to-F32/F64 integer-to-floating casts, and FPR-register-source F32/F64-to-I32/I64 floating-to-integer casts"};
    }
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    if (auto diagnostic = pointer_arithmetic_diagnostic(*binary)) {
      return diagnostic;
    }
  }
  if (const auto* call = std::get_if<bir::CallInst>(&inst);
      call != nullptr && call->inline_asm.has_value()) {
    return std::string{
        "unsupported_inline_asm_fragment: RV64 object route requires a complete supported inline-asm carrier"};
  }
  if (const auto* call = std::get_if<bir::CallInst>(&inst);
      call != nullptr && !call->inline_asm.has_value()) {
    const auto* call_plan = prepare::find_indexed_prepared_call_plan(
        &lookups.call_plans, nullptr, block_index, instruction_index);
    if (call_plan != nullptr &&
        call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::SameModule) {
      std::ostringstream out;
      out << "unsupported_call_abi: RV64 object route requires supported ordinary same-module call ABI/result lowering"
          << "; function=" << rv64_prepared_function_name(names, function_name)
          << "; block=" << rv64_prepared_block_label(names, prepared_block_label)
          << "; block_index=" << block_index
          << "; instruction_index=" << instruction_index
          << "; callee="
          << (call_plan->direct_callee_name.has_value()
                  ? std::string_view{*call_plan->direct_callee_name}
                  : std::string_view{call->callee})
          << "; args=" << call->args.size()
          << "; planned_args=" << call_plan->arguments.size()
          << "; result=";
      if (call->result.has_value()) {
        out << bir::render_type(call->result->type);
        if (call->result->kind == bir::Value::Kind::Named &&
            !call->result->name.empty()) {
          out << " " << call->result->name;
        }
      } else {
        out << "none";
      }
      return out.str();
    }
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr && bir::is_compare_opcode(binary->opcode) &&
      !terminator_uses_value_as_condition(block, binary->result) &&
      !prepared_compare_feeds_supported_scalar_trunc_publication(stack_layout,
                                                                 names,
                                                                 &lookups,
                                                                 block,
                                                                 block_index,
                                                                 instruction_index,
                                                                 *binary,
                                                                 stack_frame_bytes) &&
      !fragment_for_prepared_fp_compare_publication(names,
                                                    &lookups,
                                                    *binary)
           .has_value() &&
      !fragment_for_prepared_binary(stack_layout,
                                    names,
                                    &lookups,
                                    *binary,
                                    stack_frame_bytes,
                                    block_index,
                                    instruction_index)
           .has_value()) {
    return std::string{
        "unsupported_scalar_compare_publication: RV64 object route requires prepared scalar compare result homes and materializable operands"};
  }
  return std::nullopt;
}

std::string rv64_prepared_instruction_kind_name(const bir::Inst& inst) {
  if (std::holds_alternative<bir::BinaryInst>(inst)) {
    return "BinaryInst";
  }
  if (std::holds_alternative<bir::SelectInst>(inst)) {
    return "SelectInst";
  }
  if (std::holds_alternative<bir::CastInst>(inst)) {
    return "CastInst";
  }
  if (std::holds_alternative<bir::PhiInst>(inst)) {
    return "PhiInst";
  }
  if (std::holds_alternative<bir::CallInst>(inst)) {
    return "CallInst";
  }
  if (std::holds_alternative<bir::LoadLocalInst>(inst)) {
    return "LoadLocalInst";
  }
  if (std::holds_alternative<bir::LoadGlobalInst>(inst)) {
    return "LoadGlobalInst";
  }
  if (std::holds_alternative<bir::StoreGlobalInst>(inst)) {
    return "StoreGlobalInst";
  }
  if (std::holds_alternative<bir::StoreLocalInst>(inst)) {
    return "StoreLocalInst";
  }
  return "UnknownInst";
}

std::optional<bir::Value> rv64_prepared_instruction_owner_value(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& concrete) -> std::optional<bir::Value> {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::PhiInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return concrete.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return concrete.result;
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst>) {
          return concrete.value;
        } else {
          return std::nullopt;
        }
      },
      inst);
}

std::string rv64_prepared_instruction_owner_context(
    const bir::Inst& inst) {
  const auto owner = rv64_prepared_instruction_owner_value(inst);
  if (!owner.has_value()) {
    return "none";
  }
  std::string context = bir::render_type(owner->type);
  if (owner->kind == bir::Value::Kind::Named && !owner->name.empty()) {
    context += " ";
    context += owner->name;
  } else if (owner->kind == bir::Value::Kind::Immediate) {
    context += " immediate";
  }
  return context;
}

std::string unsupported_prepared_instruction_fragment_diagnostic(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId prepared_block_label,
    std::size_t block_index,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  std::ostringstream out;
  out << "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering"
      << "; function=" << rv64_prepared_function_name(names, function_name)
      << "; block=" << rv64_prepared_block_label(names, prepared_block_label)
      << "; block_index=" << block_index
      << "; instruction_index=" << instruction_index
      << "; instruction_kind=" << rv64_prepared_instruction_kind_name(inst)
      << "; owner=" << rv64_prepared_instruction_owner_context(inst);
  return out.str();
}

RiscvPreparedObjectFunctionResult prepared_function_to_object_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow) {
  namespace prepare = c4c::backend::prepare;

  auto admission = prepare_rv64_object_function_admission_shell(
      prepared,
      control_flow,
      RiscvPreparedFunctionAdmissionCallbacks{
          .variadic_function_admission_diagnostic =
              rv64_variadic_function_admission_diagnostic,
          .stack_frame_size = rv64_object_stack_frame_size,
          .saved_register_bank_diagnostic =
              diagnose_unsupported_prepared_saved_register_bank,
          .param_homes_diagnostic = diagnose_unsupported_prepared_param_homes,
          .variadic_helper_diagnostic =
              diagnose_first_unsupported_prepared_variadic_helper,
      });
  if (!admission.diagnostic.empty()) {
    return make_rv64_prepared_function_rejection(std::move(admission.diagnostic));
  }
  const std::string& function_name = admission.function_name;
  const auto* function = admission.function;
  const auto& lookups = admission.lookups;
  const auto& dependency_operand_authorities =
      admission.dependency_operand_authorities;
  const auto& carrier_alias_authorities = admission.carrier_alias_authorities;
  const auto& select_edge_source_producer_placements =
      admission.select_edge_source_producer_placements;
  RiscvObjectFunction object_function{
      .name = function_name,
      .global = true,
  };
  const auto* addressing = admission.addressing;
  const auto* frame_plan = admission.frame_plan;
  const auto* storage_plan = admission.storage_plan;
  const auto* inline_asm_carriers = admission.inline_asm_carriers;
  const auto stack_frame_bytes = admission.stack_frame_bytes;
  const bool has_call = admission.has_call;
  std::size_t incoming_stack_base_bytes = *stack_frame_bytes;
  if (has_call) {
    const auto call_frame_size = rv64_prepared_call_frame_size(*stack_frame_bytes);
    if (!call_frame_size.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: RV64 object route requires supported prepared call frame size");
    }
    incoming_stack_base_bytes = *call_frame_size;
  }
  if (has_call) {
    auto prologue =
        make_rv64_prepared_call_frame_prologue_fragment(frame_plan,
                                                        *stack_frame_bytes);
    if (!prologue.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots");
    }
    object_function.fragments.push_back(std::move(*prologue));
  } else if (*stack_frame_bytes > 0) {
    auto prologue =
        make_rv64_prepared_stack_frame_prologue_fragment(frame_plan,
                                                         *stack_frame_bytes);
    if (!prologue.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots");
    }
    object_function.fragments.push_back(std::move(*prologue));
  }
  auto formal_entry_homes =
      make_rv64_formal_entry_home_fragment(prepared.stack_layout,
                                           prepared.names,
                                           &lookups,
                                           *function,
                                           *stack_frame_bytes);
  if (!formal_entry_homes.has_value()) {
    return make_rv64_prepared_function_rejection(
        "unsupported_param_home: RV64 object route requires encodable scalar GPR formal stack-slot home stores");
  }
  if (!formal_entry_homes->bytes.empty()) {
    object_function.fragments.push_back(std::move(*formal_entry_homes));
  }
  if (function->is_variadic) {
    const auto* entry_plan =
        prepare::find_prepared_variadic_entry_plan(prepared,
                                                   control_flow.function_name);
    auto incoming_gpr_publications =
        fragment_for_rv64_variadic_incoming_gpr_publications(
            prepared.stack_layout,
            entry_plan,
            *stack_frame_bytes);
    if (!incoming_gpr_publications.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_function_admission: RV64 variadic entry incoming GPR publications require encodable prologue stores");
    }
    if (!incoming_gpr_publications->bytes.empty()) {
      object_function.fragments.push_back(std::move(*incoming_gpr_publications));
    }
  }

  std::unordered_map<std::string, PreparedObjectCompare> compares;
  const auto* value_locations =
      prepare::find_prepared_value_location_function(prepared,
                                                     control_flow.function_name);
  std::unordered_set<PreparedBeforeReturnStackToRegisterKey,
                     PreparedBeforeReturnStackToRegisterKeyHash>
      prepared_before_return_stack_to_register_values;
  if (value_locations != nullptr) {
    for (const auto& move_bundle : value_locations->move_bundles) {
      if (move_bundle.moves.size() != 1) {
        continue;
      }
      const auto& move = move_bundle.moves.front();
      const auto* source_home =
          prepared_value_home_for_id(&lookups, move.from_value_id);
      const auto source_type =
          source_home == nullptr
              ? std::optional<c4c::backend::bir::TypeKind>{}
              : prepared_bir_value_type_for_name(
                    prepared.names, *function, source_home->value_name);
      const bool direct_global_pointer_return_shape =
          function->return_type == c4c::backend::bir::TypeKind::Ptr &&
          source_home != nullptr &&
          source_home->kind == prepare::PreparedValueHomeKind::Register &&
          source_type == c4c::backend::bir::TypeKind::Ptr &&
          move.destination_kind ==
              prepare::PreparedMoveDestinationKind::FunctionReturnAbi &&
          move.destination_storage_kind ==
              prepare::PreparedMoveStorageKind::Register;
      if (move.reason == "return_stack_to_register" &&
          !direct_global_pointer_return_shape &&
          !(prepared_move_is_before_return_stack_to_register_abi_move(
                move_bundle, move) &&
            source_home != nullptr &&
            source_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        return make_rv64_prepared_function_rejection(
            "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves");
      }
      if (prepared_move_is_before_return_stack_to_register_abi_move(move_bundle,
                                                                     move) &&
          source_home != nullptr &&
          source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
          move.function_return_authority_kind ==
              prepare::PreparedMoveAuthorityKind::FunctionReturnDestinationHome) {
        prepared_before_return_stack_to_register_values.insert(
            PreparedBeforeReturnStackToRegisterKey{
                .block_index = move_bundle.block_index,
                .value_id = move.from_value_id,
            });
      }
    }
  }
  const auto traversal = control_flow.blocks.empty()
                             ? std::vector<prepare::PreparedObjectTraversalEvent>{}
                             : prepare::make_prepared_object_function_traversal(
                                   control_flow,
                                   value_locations,
                                   function,
                                   &select_edge_source_producer_placements);
  if (prepared_object_traversal_is_complete_bir_stream(traversal)) {
    for (const auto& event : traversal) {
      const auto* block = event.bir_block;
      const auto block_label_id = prepared_block_label_id_for(
          prepared.module.names,
          prepared.names,
          *block);
      const auto prepared_block_label =
          block_label_id.value_or(c4c::kInvalidBlockLabel);

      switch (event.kind) {
        case prepare::PreparedObjectTraversalEventKind::Label: {
          const std::string block_label =
              bir_block_label_spelling(prepared.module, *block);
          auto label_fragment = make_rv64_block_label_fragment(
              riscv_local_block_label(function_name, block_label));
          if (!label_fragment.has_value()) {
            return make_rv64_prepared_function_rejection(
                "unsupported_function_admission: BIR block has no target label");
          }
          object_function.fragments.push_back(std::move(*label_fragment));
          break;
        }
        case prepare::PreparedObjectTraversalEventKind::BlockEntryCopies:
        case prepare::PreparedObjectTraversalEventKind::BeforeInstructionCopies:
        case prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies: {
          const auto classification =
              prepare::classify_prepared_object_move_bundle_consumer(
                  prepare::PreparedObjectMoveBundleConsumerQuery{
                      .event = &event,
                      .value_home_lookups = &lookups.value_homes,
                  });
          if (auto diagnostic =
                  prepare::diagnose_prepared_object_consumer(classification)) {
            if (auto classification_diagnostic =
                    rv64_prepared_move_bundle_classification_failure_diagnostic(
                        prepared.names,
                        control_flow,
                        *function,
                        event,
                        classification,
                        &lookups)) {
              return RiscvPreparedObjectFunctionResult{
                  .prepared_consumer_category = diagnostic->category,
                  .diagnostic = std::move(*classification_diagnostic),
              };
            }
            return RiscvPreparedObjectFunctionResult{
                .prepared_consumer_category = diagnostic->category,
                .diagnostic = std::move(diagnostic->message),
            };
          }
          if (classification.parallel_copy_bundle != nullptr) {
            auto select_publication_diagnostic =
                rv64_select_publication_bundle_rejection_diagnostic(
                    prepared.names,
                    control_flow,
                    event,
                    classification.move_bundle,
                    &lookups,
                    *classification.parallel_copy_bundle);
            if (select_publication_diagnostic.has_value() &&
                !prepared_predecessor_select_publication_bundle_is_rv64_object_admitted(
                    prepared.names,
                    &lookups,
                    *classification.parallel_copy_bundle,
                    prepared_select_publication_destination_is_stack_home) &&
                !prepared_predecessor_select_publication_bundle_is_immediate_to_gpr_materialized(
                    prepared.names,
                    &lookups,
                    *classification.parallel_copy_bundle)) {
              return make_rv64_prepared_function_rejection(
                  std::move(*select_publication_diagnostic));
            }
          }
          if (classification.parallel_copy_bundle != nullptr &&
              prepared_predecessor_select_publication_bundle_is_stack_join_materialized(
                  prepared.names,
                  &lookups,
                  *classification.parallel_copy_bundle,
                  prepared_select_publication_destination_is_stack_home)) {
            auto select_publication_fragment =
                fragment_for_predecessor_select_publication_gpr_to_stack_destination(
                    prepared.names,
                    &lookups,
                    *classification.parallel_copy_bundle);
            if (select_publication_fragment.has_value()) {
              object_function.fragments.push_back(
                  std::move(*select_publication_fragment));
              break;
            }
            object_function.fragments.push_back(RiscvEncodedFragment{});
            break;
          }
          if (classification.parallel_copy_bundle != nullptr) {
            auto select_publication_fragment =
                fragment_for_predecessor_select_publication_immediate_to_gpr(
                    prepared.names,
                    &lookups,
                    *classification.parallel_copy_bundle);
            if (select_publication_fragment.has_value()) {
              object_function.fragments.push_back(
                  std::move(*select_publication_fragment));
              break;
            }
          }
          if (classification.parallel_copy_bundle != nullptr) {
            auto select_publication_fragment =
                fragment_for_predecessor_select_publication_pointer_stack_source_to_gpr(
                    prepared.names,
                    &lookups,
                    *classification.parallel_copy_bundle);
            if (select_publication_fragment.has_value()) {
              object_function.fragments.push_back(
                  std::move(*select_publication_fragment));
              break;
            }
          }
          auto fragment =
              fragment_for_prepared_move_bundle(prepared.target_profile,
                                                prepared.stack_layout,
                                                prepared.names,
                                                control_flow,
                                                *function,
                                                &lookups,
                                                storage_plan,
                                                &dependency_operand_authorities,
                                                &carrier_alias_authorities,
                                                &select_edge_source_producer_placements,
                                                *stack_frame_bytes,
                                                event.kind,
                                                classification.parallel_copy_bundle,
                                                classification
                                                    .stack_destination_fan_in_authority
                                                    ? &*classification
                                                           .stack_destination_fan_in_authority
                                                    : nullptr,
                                                *classification.move_bundle);
          if (!fragment.has_value()) {
            return make_rv64_prepared_function_rejection(
                rv64_prepared_move_bundle_fragment_failure_diagnostic(
                    prepared.names,
                    control_flow,
                    *function,
                    event,
                    classification,
                    &lookups,
                    &dependency_operand_authorities,
                    &select_edge_source_producer_placements));
          }
          for (const auto& move : classification.move_bundle->moves) {
            const auto* source_home =
                prepared_value_home_for_id(&lookups, move.from_value_id);
            if (prepared_move_is_before_return_stack_to_register_abi_move(
                    *classification.move_bundle,
                    move) &&
                source_home != nullptr &&
                source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
                move.function_return_authority_kind ==
                    prepare::PreparedMoveAuthorityKind::
                        FunctionReturnDestinationHome) {
              prepared_before_return_stack_to_register_values.insert(
                  PreparedBeforeReturnStackToRegisterKey{
                      .block_index = event.block_index,
                      .value_id = move.from_value_id,
                  });
            }
          }
          object_function.fragments.push_back(std::move(*fragment));
          break;
        }
        case prepare::PreparedObjectTraversalEventKind::Instruction: {
          if (event.instruction == nullptr) {
            return make_rv64_prepared_function_rejection(
                "unsupported_instruction_fragment: prepared traversal instruction is missing");
          }
          const auto select_classification =
              prepare::classify_prepared_object_select_consumer(&control_flow,
                                                                prepared_block_label,
                                                                *event.instruction);
          if (auto diagnostic =
                  prepare::diagnose_prepared_object_consumer(select_classification)) {
            return RiscvPreparedObjectFunctionResult{
                .prepared_consumer_category = diagnostic->category,
                .diagnostic = std::move(diagnostic->message),
            };
          }
          auto fragment = fragment_for_prepared_instruction(prepared,
                                                           control_flow,
                                                           *function,
                                                           lookups,
                                                           dependency_operand_authorities,
                                                           carrier_alias_authorities,
                                                           inline_asm_carriers,
                                                           *block,
                                                           prepared_block_label,
                                                           function_name,
                                                           event.block_index,
                                                           event.instruction_index,
                                                           *event.instruction,
                                                           compares,
                                                           frame_plan,
                                                           storage_plan,
                                                           incoming_stack_base_bytes,
                                                           *stack_frame_bytes);
          if (!fragment.has_value()) {
            if (auto diagnostic =
                    diagnose_unsupported_prepared_variadic_helper_fragment(
                        prepared,
                        control_flow.function_name,
                        event.block_index,
                        event.instruction_index,
                        *event.instruction,
                        *stack_frame_bytes)) {
              return make_rv64_prepared_function_rejection(std::move(*diagnostic));
            }
            if (auto diagnostic =
                diagnose_unsupported_prepared_instruction_fragment(
                    prepared.stack_layout,
                    prepared.names,
                    lookups,
                    control_flow.function_name,
                    prepared_block_label,
                    event.block_index,
                    event.instruction_index,
                    *block,
                    *event.instruction,
                    *stack_frame_bytes)) {
              return make_rv64_prepared_function_rejection(std::move(*diagnostic));
            }
            return make_rv64_prepared_function_rejection(
                unsupported_prepared_instruction_fragment_diagnostic(
                    prepared.names,
                    control_flow.function_name,
                    prepared_block_label,
                    event.block_index,
                    event.instruction_index,
                    *event.instruction));
          }
          object_function.fragments.push_back(std::move(*fragment));
          break;
        }
        case prepare::PreparedObjectTraversalEventKind::Terminator: {
          auto terminator_fragment =
              fragment_for_prepared_authorized_pointer_return_already_loaded(
                  prepared.names,
                  &lookups,
                  frame_plan,
                  block->terminator,
                  event.block_index,
                  &prepared_before_return_stack_to_register_values,
                  has_call,
                  *stack_frame_bytes);
          if (!terminator_fragment.has_value()) {
            terminator_fragment =
                fragment_for_prepared_terminator(prepared,
                                                 control_flow,
                                                 prepared.names,
                                                 &lookups,
                                                 *function,
                                                 *block,
                                                 prepared_block_label,
                                                 event.block_index,
                                                 function_name,
                                                 compares,
                                                 frame_plan,
                                                 &prepared_before_return_stack_to_register_values,
                                                 has_call,
                                                 *stack_frame_bytes);
          }
          if (!terminator_fragment.has_value()) {
            if (auto diagnostic =
                    diagnose_unsupported_prepared_terminator_fragment(
                        prepared.names,
                        lookups,
                        control_flow,
                        *block,
                        control_flow.function_name,
                        prepared_block_label,
                        event.block_index)) {
              return make_rv64_prepared_function_rejection(
                  std::move(*diagnostic));
            }
            return make_rv64_prepared_function_rejection(
                "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering");
          }
          object_function.fragments.push_back(std::move(*terminator_fragment));
          break;
        }
      }
    }
    return RiscvPreparedObjectFunctionResult{
        .function = std::move(object_function),
    };
  }

  for (std::size_t block_index = 0; block_index < function->blocks.size(); ++block_index) {
    const auto& block = function->blocks[block_index];
    const auto block_label_id = prepared_block_label_id_for(
        prepared.module.names,
        prepared.names,
        block);
    const auto prepared_block_label =
        block_label_id.value_or(c4c::kInvalidBlockLabel);
    const std::string block_label = bir_block_label_spelling(prepared.module, block);
    auto label_fragment = make_rv64_block_label_fragment(
        riscv_local_block_label(function_name, block_label));
    if (!label_fragment.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_function_admission: BIR block has no target label");
    }
    object_function.fragments.push_back(std::move(*label_fragment));

    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      auto fragment = fragment_for_prepared_instruction(prepared,
                                                       control_flow,
                                                       *function,
                                                       lookups,
                                                       dependency_operand_authorities,
                                                       carrier_alias_authorities,
                                                       inline_asm_carriers,
                                                       block,
                                                       prepared_block_label,
                                                       function_name,
                                                       block_index,
                                                       instruction_index,
                                                       block.insts[instruction_index],
                                                       compares,
                                                       frame_plan,
                                                       storage_plan,
                                                       incoming_stack_base_bytes,
                                                       *stack_frame_bytes);
      if (!fragment.has_value()) {
        if (auto diagnostic =
                diagnose_unsupported_prepared_variadic_helper_fragment(
                    prepared,
                    control_flow.function_name,
                    block_index,
                    instruction_index,
                    block.insts[instruction_index],
                    *stack_frame_bytes)) {
          return make_rv64_prepared_function_rejection(std::move(*diagnostic));
        }
        if (auto diagnostic =
                diagnose_unsupported_prepared_instruction_fragment(
                    prepared.stack_layout,
                    prepared.names,
                    lookups,
                    control_flow.function_name,
                    prepared_block_label,
                    block_index,
                    instruction_index,
                    block,
                    block.insts[instruction_index],
                    *stack_frame_bytes)) {
          return make_rv64_prepared_function_rejection(std::move(*diagnostic));
        }
        return make_rv64_prepared_function_rejection(
            unsupported_prepared_instruction_fragment_diagnostic(
                prepared.names,
                control_flow.function_name,
                prepared_block_label,
                block_index,
                instruction_index,
                block.insts[instruction_index]));
      }
      object_function.fragments.push_back(std::move(*fragment));
    }

    auto terminator_fragment =
        fragment_for_prepared_terminator(prepared,
                                         control_flow,
                                         prepared.names,
                                         &lookups,
                                         *function,
                                         block,
                                         prepared_block_label,
                                         block_index,
                                         function_name,
                                         compares,
                                         frame_plan,
                                         nullptr,
                                         has_call,
                                         *stack_frame_bytes);
    if (!terminator_fragment.has_value()) {
      if (auto diagnostic =
              diagnose_unsupported_prepared_terminator_fragment(
                  prepared.names,
                  lookups,
                  control_flow,
                  block,
                  control_flow.function_name,
                  prepared_block_label,
                  block_index)) {
        return make_rv64_prepared_function_rejection(std::move(*diagnostic));
      }
      return make_rv64_prepared_function_rejection(
          "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering");
    }
    object_function.fragments.push_back(std::move(*terminator_fragment));
  }
  return RiscvPreparedObjectFunctionResult{
      .function = std::move(object_function),
  };
}

std::string_view strip_inline_asm_register_constraint(std::string_view constraint) {
  while (!constraint.empty()) {
    const char ch = constraint.front();
    if (ch == '=' || ch == '+' || ch == '&' || ch == '%') {
      constraint.remove_prefix(1);
      continue;
    }
    break;
  }
  return constraint;
}

bool is_decimal_inline_asm_tie(std::string_view constraint) {
  return !constraint.empty() &&
         std::all_of(constraint.begin(), constraint.end(), [](unsigned char ch) {
           return std::isdigit(ch) != 0;
         });
}

bool is_rv64_vector_substitution_constraint(std::string_view constraint) {
  return constraint == "VR" || constraint == "VRM1" || constraint == "VRM2" ||
         constraint == "VRM4" || constraint == "VRM8";
}

bool is_supported_rv64_substitution_register_constraint(std::string_view constraint) {
  constraint = strip_inline_asm_register_constraint(constraint);
  return constraint == "r" || is_rv64_vector_substitution_constraint(constraint);
}

const prepare::PreparedValueHome* substitution_home_for_operand(
    const prepare::PreparedInlineAsmCarrier& carrier,
    const prepare::PreparedInlineAsmOperand& operand) {
  switch (operand.kind) {
    case c4c::backend::bir::InlineAsmOperandKind::RegisterOutput:
      if (!is_supported_rv64_substitution_register_constraint(operand.constraint) ||
          !operand.output_index.has_value() || *operand.output_index != 0 ||
          !carrier.result_home.has_value()) {
        return nullptr;
      }
      return &*carrier.result_home;
    case c4c::backend::bir::InlineAsmOperandKind::RegisterInput:
      if (!is_supported_rv64_substitution_register_constraint(operand.constraint) ||
          !operand.arg_index.has_value() || !operand.home.has_value()) {
        return nullptr;
      }
      return &*operand.home;
    case c4c::backend::bir::InlineAsmOperandKind::TiedInput:
      if (!is_decimal_inline_asm_tie(operand.constraint) ||
          !operand.arg_index.has_value() || !operand.home.has_value()) {
        return nullptr;
      }
      return &*operand.home;
    case c4c::backend::bir::InlineAsmOperandKind::Unsupported:
    case c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput:
    case c4c::backend::bir::InlineAsmOperandKind::MemoryInput:
    case c4c::backend::bir::InlineAsmOperandKind::AddressInput:
    case c4c::backend::bir::InlineAsmOperandKind::Clobber:
      return nullptr;
  }
  return nullptr;
}

std::optional<std::string> register_name_for_inline_asm_substitution_home(
    const prepare::PreparedValueHome& home,
    c4c::backend::bir::InlineAsmRegisterClass register_class) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64) {
    return std::nullopt;
  }
  switch (register_class) {
    case c4c::backend::bir::InlineAsmRegisterClass::General:
      if (identity.bank != prepare::PreparedRegisterBank::Gpr ||
          identity.register_class != prepare::PreparedRegisterClass::General) {
        return std::nullopt;
      }
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::Vector:
      if (identity.bank != prepare::PreparedRegisterBank::Vreg ||
          identity.register_class != prepare::PreparedRegisterClass::Vector) {
        return std::nullopt;
      }
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::None:
      break;
  }
  return *home.register_name;
}

std::optional<std::string> substitute_positional_riscv_inline_asm_operands(
    std::string_view text,
    const std::vector<std::optional<std::string>>& gcc_operand_registers) {
  std::string result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if ((text[i] != '%' && text[i] != '$') || i + 1 >= text.size()) {
      result.push_back(text[i]);
      continue;
    }

    const char marker = text[i];
    ++i;
    if (marker == '%' && text[i] == '%') {
      result.push_back('%');
      continue;
    }

    bool braced = false;
    if (text[i] == '{') {
      braced = true;
      ++i;
      if (i >= text.size()) {
        return std::nullopt;
      }
    }

    if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
      std::size_t num = 0;
      while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
        num = num * 10 + static_cast<std::size_t>(text[i] - '0');
        ++i;
      }
      if (braced) {
        if (i >= text.size() || text[i] != '}') {
          return std::nullopt;
        }
      } else {
        --i;
      }
      if (num >= gcc_operand_registers.size() || !gcc_operand_registers[num].has_value()) {
        return std::nullopt;
      }
      result += *gcc_operand_registers[num];
      continue;
    }

    result.push_back(marker);
    if (braced) {
      result.push_back('{');
    }
    result.push_back(text[i]);
  }
  return result;
}

std::optional<std::size_t> parse_positional_inline_asm_placeholder(
    std::string_view token) {
  token = trim_ascii(token);
  if (token.size() < 2 || (token.front() != '%' && token.front() != '$')) {
    return std::nullopt;
  }
  token.remove_prefix(1);
  if (token.empty() || token.front() == '[' || token.front() == 'c') {
    return std::nullopt;
  }
  if (token.front() == '{') {
    if (token.size() < 3 || token.back() != '}') {
      return std::nullopt;
    }
    token.remove_prefix(1);
    token.remove_suffix(1);
  }
  std::size_t operand_index = 0;
  const char* const begin = token.data();
  const char* const end = token.data() + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, operand_index);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return operand_index;
}

const prepare::PreparedInlineAsmOperand* prepared_inline_asm_operand_by_constraint_index(
    const prepare::PreparedInlineAsmCarrier& carrier,
    std::size_t constraint_index) {
  const prepare::PreparedInlineAsmOperand* found = nullptr;
  for (const auto& operand : carrier.operands) {
    if (operand.constraint_index != constraint_index) {
      continue;
    }
    if (found != nullptr) {
      return nullptr;
    }
    found = &operand;
  }
  return found;
}

std::optional<std::int64_t> insn_d_immediate_operand_value(
    const prepare::PreparedInlineAsmCarrier& carrier,
    std::string_view token) {
  const auto operand_index = parse_positional_inline_asm_placeholder(token);
  if (!operand_index.has_value()) {
    return std::nullopt;
  }
  const auto* operand =
      prepared_inline_asm_operand_by_constraint_index(carrier, *operand_index);
  if (operand == nullptr ||
      operand->kind != c4c::backend::bir::InlineAsmOperandKind::
                           IntegerImmediateInput ||
      operand->constraint != "i" || !operand->arg_index.has_value() ||
      !operand->immediate_value.has_value()) {
    return std::nullopt;
  }
  return *operand->immediate_value;
}

std::optional<RiscvInsnDInlineAsmRegister> insn_d_register_from_home(
    const prepare::PreparedValueHome& home,
    c4c::backend::bir::InlineAsmRegisterClass register_class,
    std::size_t group_width) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  RiscvInsnDInlineAsmRegister result;
  switch (register_class) {
    case c4c::backend::bir::InlineAsmRegisterClass::General:
      if (identity.bank != prepare::PreparedRegisterBank::Gpr ||
          identity.register_class != prepare::PreparedRegisterClass::General) {
        return std::nullopt;
      }
      result.bank = RiscvInsnDInlineAsmRegisterBank::Gpr;
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::Vector:
      if (identity.bank != prepare::PreparedRegisterBank::Vreg ||
          identity.register_class != prepare::PreparedRegisterClass::Vector) {
        return std::nullopt;
      }
      result.bank = RiscvInsnDInlineAsmRegisterBank::Vector;
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::None:
      return std::nullopt;
  }
  result.physical_index = static_cast<std::uint32_t>(identity.physical_index);
  result.group_width = group_width == 0 ? 1 : group_width;
  return result;
}

std::optional<RiscvInsnDInlineAsmRegister> insn_d_register_operand(
    const prepare::PreparedInlineAsmCarrier& carrier,
    std::string_view token,
    bool output) {
  const auto operand_index = parse_positional_inline_asm_placeholder(token);
  if (!operand_index.has_value()) {
    return std::nullopt;
  }
  const auto* operand =
      prepared_inline_asm_operand_by_constraint_index(carrier, *operand_index);
  if (operand == nullptr) {
    return std::nullopt;
  }
  if (output) {
    if (operand->kind != c4c::backend::bir::InlineAsmOperandKind::RegisterOutput ||
        !operand->output_index.has_value() || *operand->output_index != 0 ||
        !carrier.result_home.has_value()) {
      return std::nullopt;
    }
    return insn_d_register_from_home(
        *carrier.result_home, operand->register_class, operand->register_group_width);
  }
  if (operand->kind != c4c::backend::bir::InlineAsmOperandKind::RegisterInput &&
      operand->kind != c4c::backend::bir::InlineAsmOperandKind::TiedInput) {
    return std::nullopt;
  }
  if (!operand->arg_index.has_value() || !operand->home.has_value()) {
    return std::nullopt;
  }
  if (operand->kind == c4c::backend::bir::InlineAsmOperandKind::TiedInput &&
      !is_decimal_inline_asm_tie(operand->constraint)) {
    return std::nullopt;
  }
  return insn_d_register_from_home(
      *operand->home, operand->register_class, operand->register_group_width);
}

}  // namespace

std::optional<std::string>
diagnose_rv64_prepared_terminator_fragment_for_authority_status(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups& lookups,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Block& block,
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index) {
  return diagnose_unsupported_prepared_terminator_fragment(names,
                                                           lookups,
                                                           control_flow,
                                                           block,
                                                           function_name,
                                                           block_label_id,
                                                           block_index);
}

std::optional<std::string> substitute_prepared_riscv_inline_asm_operands(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier) {
  namespace prepare = c4c::backend::prepare;
  if (carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      carrier.has_named_operand_references || carrier.has_template_modifiers ||
      !carrier.clobbers.empty() || !carrier.missing_required_facts.empty()) {
    return std::nullopt;
  }

  std::size_t operand_count = 0;
  for (const auto& operand : carrier.operands) {
    operand_count = std::max(operand_count, operand.constraint_index + 1);
  }

  std::vector<std::optional<std::string>> gcc_operands(operand_count);
  for (const auto& operand : carrier.operands) {
    if (operand.constraint_index >= gcc_operands.size() ||
        gcc_operands[operand.constraint_index].has_value()) {
      return std::nullopt;
    }
    if (operand.kind ==
        c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput) {
      if (!operand.immediate_value.has_value()) {
        return std::nullopt;
      }
      gcc_operands[operand.constraint_index] =
          std::to_string(*operand.immediate_value);
    } else {
      const auto* home = substitution_home_for_operand(carrier, operand);
      if (home == nullptr) {
        return std::nullopt;
      }
      auto register_name =
          register_name_for_inline_asm_substitution_home(*home, operand.register_class);
      if (!register_name.has_value()) {
        return std::nullopt;
      }
      gcc_operands[operand.constraint_index] = std::move(register_name);
    }
  }

  return substitute_positional_riscv_inline_asm_operands(
      carrier.asm_text,
      gcc_operands);
}

std::optional<RiscvInsnDInlineAsmShape> classify_prepared_rv64_insn_d_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier) {
  namespace prepare = c4c::backend::prepare;
  if (carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      carrier.has_named_operand_references || carrier.has_template_modifiers ||
      !carrier.clobbers.empty() || !carrier.missing_required_facts.empty()) {
    return std::nullopt;
  }

  std::string_view text = trim_ascii(carrier.asm_text);
  constexpr std::string_view prefix = ".insn.d";
  if (text.size() < prefix.size() || text.substr(0, prefix.size()) != prefix ||
      (text.size() > prefix.size() && text[prefix.size()] != ' ' &&
       text[prefix.size()] != '\t')) {
    return std::nullopt;
  }
  text.remove_prefix(prefix.size());
  text = trim_ascii(text);

  const auto fields = split_rv64_insn_fields(text, 7);
  if (!fields.has_value()) {
    return std::nullopt;
  }

  auto major = insn_d_immediate_operand_value(carrier, (*fields)[0]);
  auto operation = insn_d_immediate_operand_value(carrier, (*fields)[1]);
  auto destination = insn_d_register_operand(carrier, (*fields)[2], true);
  auto lhs = insn_d_register_operand(carrier, (*fields)[3], false);
  auto rhs = insn_d_register_operand(carrier, (*fields)[4], false);
  auto accumulator = insn_d_register_operand(carrier, (*fields)[5], false);
  auto dtype = insn_d_immediate_operand_value(carrier, (*fields)[6]);
  if (!major.has_value() || !operation.has_value() || !destination.has_value() ||
      !lhs.has_value() || !rhs.has_value() || !accumulator.has_value() ||
      !dtype.has_value()) {
    return std::nullopt;
  }

  return RiscvInsnDInlineAsmShape{
      .major = *major,
      .operation = *operation,
      .destination = *destination,
      .lhs = *lhs,
      .rhs = *rhs,
      .accumulator = *accumulator,
      .dtype = *dtype,
  };
}

std::optional<std::uint64_t> encode_rv64_ev_insn_d_inline_asm(
    const RiscvInsnDInlineAsmShape& shape) {
  const auto unsigned_field = [](std::int64_t value,
                                 std::uint64_t max) -> std::optional<std::uint64_t> {
    if (value < 0 || static_cast<std::uint64_t>(value) > max) {
      return std::nullopt;
    }
    return static_cast<std::uint64_t>(value);
  };
  const auto register_field =
      [](const RiscvInsnDInlineAsmRegister& reg) -> std::optional<std::uint64_t> {
    if (reg.physical_index > 31) {
      return std::nullopt;
    }
    return static_cast<std::uint64_t>(reg.physical_index);
  };

  const auto major7 = unsigned_field(shape.major, 0x7f);
  const auto evop8 = unsigned_field(shape.operation, 0xff);
  const auto dtype16 = unsigned_field(shape.dtype, 0xffff);
  const auto rd = register_field(shape.destination);
  const auto rs1 = register_field(shape.lhs);
  const auto rs2 = register_field(shape.rhs);
  const auto rs4 = register_field(shape.accumulator);
  if (!major7.has_value() || !evop8.has_value() || !dtype16.has_value() ||
      !rd.has_value() || !rs1.has_value() || !rs2.has_value() ||
      !rs4.has_value()) {
    return std::nullopt;
  }

  // First supported EV64 shape:
  // bits 6:0 prefix=0x3f, 11:7 rd, 14:12 reserved=0,
  // 19:15 rs1, 24:20 rs2, 31:25 major/subformat,
  // 39:32 EV operation, 44:40 rs4/mask, 47:45 reserved=0,
  // 63:48 dtype immediate.
  return std::uint64_t{0x3f} | (*rd << 7) | (*rs1 << 15) | (*rs2 << 20) |
         (*major7 << 25) | (*evop8 << 32) | (*rs4 << 40) |
         (*dtype16 << 48);
}

std::optional<std::uint32_t> rv64_elf_relocation_type(
    RiscvObjectFixupKind kind) {
  switch (kind) {
    case RiscvObjectFixupKind::CallPlt:
      return kRiscvRelocCallPlt;
    case RiscvObjectFixupKind::PcrelHi20:
      return kRiscvRelocPcrelHi20;
    case RiscvObjectFixupKind::PcrelLo12I:
      return kRiscvRelocPcrelLo12I;
    case RiscvObjectFixupKind::Branch:
      return kRiscvRelocBranch;
    case RiscvObjectFixupKind::Jal:
      return kRiscvRelocJal;
  }
  return std::nullopt;
}

object::RelocatableElfConfig rv64_relocatable_elf_config() {
  return object::RelocatableElfConfig{
      .elf_class = object::ElfClass::Elf64,
      .data_encoding = object::ElfDataEncoding::LittleEndian,
      .machine = kElfMachineRiscv,
      .flags = kRiscvElfFlagsRv64DoubleFloatAbi,
  };
}

RiscvEncodedFragment make_rv64_return_zero_fragment() {
  return make_rv64_return_immediate_fragment(0);
}

RiscvEncodedFragment make_rv64_direct_call_fragment(std::string callee_name) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes, encode_u_type(0x17, 1, 0));       // auipc ra, 0
  append_le32(fragment.bytes, encode_i_type(0x67, 1, 0, 1, 0));  // jalr ra, 0(ra)
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 0,
      .kind = RiscvObjectFixupKind::CallPlt,
      .symbol_name = std::move(callee_name),
      .addend = 0,
  });
  return fragment;
}

RiscvEncodedFragment make_rv64_pcrel_address_fragment(
    std::string symbol_name, std::string auipc_label_name) {
  return make_rv64_pcrel_address_fragment(
      5,
      std::move(symbol_name),
      std::move(auipc_label_name),
      RiscvObjectFixupTargetKind::Function);
}

RiscvEncodedFragment make_rv64_pcrel_address_fragment(
    std::uint32_t destination_register,
    std::string symbol_name,
    std::string auipc_label_name,
    RiscvObjectFixupTargetKind target_kind,
    std::int64_t addend) {
  RiscvEncodedFragment fragment;
  const std::int32_t encoded_low_addend =
      (addend >= -2048 && addend <= 2047) ? static_cast<std::int32_t>(addend) : 0;
  append_le32(fragment.bytes,
              encode_u_type(0x17, destination_register, 0));  // auipc rd, 0
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination_register,
                            0,
                            destination_register,
                            encoded_low_addend));  // addi rd, rd, lo
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = 0,
      .name = auipc_label_name,
  });
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 0,
      .kind = RiscvObjectFixupKind::PcrelHi20,
      .target_kind = target_kind,
      .symbol_name = std::move(symbol_name),
      .addend = addend,
  });
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 4,
      .kind = RiscvObjectFixupKind::PcrelLo12I,
      .target_kind = RiscvObjectFixupTargetKind::NoType,
      .symbol_name = std::move(auipc_label_name),
      .addend = 0,
  });
  return fragment;
}

bool publish_rv64_text_fragment_label(
    object::ObjectModule& module,
    object::SectionId text_section,
    std::unordered_map<std::string, object::SymbolId>& symbols_by_name,
    const RiscvObjectLabel& label,
    std::uint64_t fragment_section_offset,
    std::uint64_t fragment_size_bytes) {
  if (label.name.empty() || label.offset_bytes > fragment_size_bytes ||
      symbols_by_name.find(label.name) != symbols_by_name.end()) {
    return false;
  }
  const auto label_offset = fragment_section_offset + label.offset_bytes;
  object::bind_label(module, label.name, text_section, label_offset);
  auto& label_symbol = object::define_symbol(module,
                                             label.name,
                                             object::SymbolBinding::Local,
                                             object::SymbolKind::NoType,
                                             text_section,
                                             label_offset,
                                             0);
  symbols_by_name.emplace(label_symbol.name, label_symbol.id);
  return true;
}

bool attach_rv64_text_fixup(
    object::ObjectModule& module,
    object::SectionId text_section,
    std::unordered_map<std::string, object::SymbolId>& symbols_by_name,
    const RiscvObjectFixup& fixup,
    std::uint64_t fragment_section_offset,
    std::uint64_t fragment_size_bytes) {
  const auto reloc_type = rv64_elf_relocation_type(fixup.kind);
  if (!reloc_type.has_value() || fixup.symbol_name.empty() ||
      fixup.offset_bytes > fragment_size_bytes) {
    return false;
  }

  object::SymbolId target_symbol{};
  const auto existing = symbols_by_name.find(fixup.symbol_name);
  if (existing != symbols_by_name.end()) {
    target_symbol = existing->second;
  } else {
    auto& undefined = object::declare_undefined_symbol(
        module,
        fixup.symbol_name,
        object::SymbolBinding::Global,
        symbol_kind_for_fixup_target(fixup.target_kind));
    target_symbol = undefined.id;
    symbols_by_name.emplace(undefined.name, undefined.id);
  }

  object::attach_relocation(module,
                            text_section,
                            fragment_section_offset + fixup.offset_bytes,
                            *reloc_type,
                            target_symbol,
                            fixup.addend);
  return true;
}

std::optional<object::ObjectModule> build_rv64_text_object_module(
    const std::vector<RiscvObjectFunction>& functions) {
  object::ObjectModule module;
  auto& text = object::create_section(module,
                                      ".text",
                                      object::SectionKind::Text,
                                      4,
                                      true,
                                      true,
                                      false);

  std::unordered_map<std::string, object::SymbolId> symbols_by_name;
  for (const auto& function : functions) {
    if (function.name.empty()) {
      return std::nullopt;
    }
  }

  std::unordered_set<std::string> defined_function_names;
  for (const auto& function : functions) {
    if (!defined_function_names.insert(function.name).second) {
      return std::nullopt;
    }
  }

  for (const auto& function : functions) {
    const auto start_offset = text.size_bytes;
    std::vector<RiscvLaidOutFragment> laid_out_fragments;
    laid_out_fragments.reserve(function.fragments.size());
    for (const auto& fragment : function.fragments) {
      const auto fragment_offset = object::append_section_bytes(text, fragment.bytes);
      laid_out_fragments.push_back(RiscvLaidOutFragment{
          .fragment = &fragment,
          .section_offset = fragment_offset,
      });
      for (const auto& label : fragment.labels) {
        if (!publish_rv64_text_fragment_label(module,
                                              text.id,
                                              symbols_by_name,
                                              label,
                                              fragment_offset,
                                              fragment.bytes.size())) {
          return std::nullopt;
        }
      }
    }
    auto& symbol = object::define_symbol(module,
                                         function.name,
                                         binding_for_function(function),
                                         object::SymbolKind::Function,
                                         text.id,
                                         start_offset,
                                         text.size_bytes - start_offset);
    symbols_by_name[symbol.name] = symbol.id;

    for (const auto& laid_out : laid_out_fragments) {
      if (laid_out.fragment == nullptr) {
        return std::nullopt;
      }
      const auto& fragment = *laid_out.fragment;
      for (const auto& fixup : fragment.fixups) {
        if (!attach_rv64_text_fixup(module,
                                    text.id,
                                    symbols_by_name,
                                    fixup,
                                    laid_out.section_offset,
                                    fragment.bytes.size())) {
          return std::nullopt;
        }
      }
    }
  }

  return module;
}

std::string rv64_prepared_object_text_label(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::StringConstant& constant) {
  if (constant.name_id != c4c::kInvalidText) {
    const std::string_view spelling = module.names.texts.lookup(constant.name_id);
    if (!spelling.empty()) {
      return std::string{spelling};
    }
  }
  return constant.name;
}

prepare::PreparedSelectedObjectDataContractFacts
rv64_selected_object_data_contract_facts(
    const prepare::PreparedGlobalObjectData* object_data) {
  if (object_data == nullptr) {
    return prepare::PreparedSelectedObjectDataContractFacts{};
  }
  return prepare::PreparedSelectedObjectDataContractFacts{
      .object_label = object_data->object_label,
      .object_size_bytes = object_data->object_size_bytes,
      .emitted_byte_count = object_data->emitted_bytes.size(),
      .zero_fill_byte_count = object_data->zero_fill_byte_count,
      .relocation_slots = object_data->relocation_slots,
      .has_object_label = object_data->has_object_label,
      .has_publication_identity = object_data->has_publication_identity,
      .requires_emitted_bytes = object_data->requires_emitted_bytes,
      .has_emitted_bytes = object_data->has_emitted_bytes,
      .requires_zero_fill = object_data->requires_zero_fill,
      .has_zero_fill = object_data->has_zero_fill,
      .requires_relocation = object_data->requires_relocation,
      .has_relocation = object_data->has_relocation,
      .has_object_byte_range = object_data->has_object_byte_range,
      .requires_unsupported_marker = object_data->requires_unsupported_marker,
      .has_unsupported_marker = object_data->has_unsupported_marker,
      .conflicting_object_label = object_data->conflicting_object_label,
      .conflicting_publication_identity =
          object_data->conflicting_publication_identity,
      .conflicting_emitted_bytes = object_data->conflicting_emitted_bytes,
      .conflicting_zero_fill = object_data->conflicting_zero_fill,
      .conflicting_relocation = object_data->conflicting_relocation,
      .conflicting_object_byte_range =
          object_data->conflicting_object_byte_range,
      .conflicting_unsupported_marker =
          object_data->conflicting_unsupported_marker,
      .unsupported_but_coherent = object_data->unsupported_but_coherent,
      .invalid_pre_prepared_initializer_semantics =
          object_data->invalid_pre_prepared_initializer_semantics,
  };
}

std::optional<std::string> rv64_prepared_object_data_contract_diagnostic(
    const prepare::PreparedGlobalObjectData* object_data) {
  const auto report = prepare::verify_prepared_selected_object_data_contract(
      rv64_selected_object_data_contract_facts(object_data));
  if (report.owner_class == prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  return "unsupported_global_data: " + report.detail;
}

std::string rv64_prepared_object_data_label(
    const prepare::PreparedGlobalObjectData& object_data) {
  if (!object_data.object_label.has_value()) {
    return {};
  }
  return object_data.object_label_text;
}

std::string rv64_prepared_link_name_label(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId link_name) {
  if (link_name == c4c::kInvalidLinkName) {
    return {};
  }
  std::string label{prepared.names.link_names.spelling(link_name)};
  if (label.empty()) {
    label = std::string{prepared.module.names.link_names.spelling(link_name)};
  }
  return label;
}

object::SymbolKind rv64_prepared_link_symbol_kind(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId link_name) {
  const auto global_it = std::find_if(
      prepared.module.globals.begin(),
      prepared.module.globals.end(),
      [&](const c4c::backend::bir::Global& global) {
        return global.link_name_id == link_name;
      });
  if (global_it != prepared.module.globals.end()) {
    return object::SymbolKind::Object;
  }
  const auto function_it = std::find_if(
      prepared.module.functions.begin(),
      prepared.module.functions.end(),
      [&](const c4c::backend::bir::Function& function) {
        return function.link_name_id == link_name;
      });
  return function_it == prepared.module.functions.end()
             ? object::SymbolKind::NoType
             : object::SymbolKind::Function;
}

bool rv64_prepared_object_data_has_emission_identity(
    const prepare::PreparedGlobalObjectData& object_data) {
  return object_data.has_object_label && object_data.object_label.has_value() &&
         !object_data.object_label_text.empty() &&
         object_data.has_publication_identity &&
         object_data.has_object_byte_range &&
         object_data.object_byte_offset == 0 &&
         object_data.object_size_bytes != 0 && object_data.align_bytes != 0;
}

bool rv64_prepared_object_data_is_selected_fallback_candidate(
    const c4c::backend::bir::Global& global,
    const prepare::PreparedGlobalObjectData& object_data,
    prepare::PreparedSelectedObjectDataContractStatus status) {
  return status ==
             prepare::PreparedSelectedObjectDataContractStatus::
                 UnsupportedButCoherent &&
         rv64_prepared_object_data_has_emission_identity(object_data) &&
         !global.is_extern && !global.is_thread_local && !global.is_constant &&
         global.address_materialization_policy !=
             c4c::backend::bir::GlobalAddressMaterializationPolicy::GotRequired &&
         object_data.requires_unsupported_marker &&
         object_data.has_unsupported_marker &&
         !object_data.requires_emitted_bytes && !object_data.has_emitted_bytes &&
         object_data.emitted_bytes.empty() && !object_data.requires_zero_fill &&
         !object_data.has_zero_fill && object_data.zero_fill_byte_count == 0 &&
         !object_data.requires_relocation && !object_data.has_relocation;
}

bool rv64_is_selected_zero_fill_object_data(
    const c4c::backend::bir::Global& global,
    const prepare::PreparedGlobalObjectData& object_data,
    prepare::PreparedSelectedObjectDataContractStatus status) {
  const bool has_no_initializer =
      !global.initializer.has_value() &&
      !global.initializer_symbol_name.has_value() &&
      global.initializer_symbol_name_id == c4c::kInvalidLinkName &&
      global.initializer_elements.empty();
  const bool has_zero_pointer_initializer =
      global.type == c4c::backend::bir::TypeKind::Ptr &&
      global.initializer.has_value() &&
      global.initializer->kind == c4c::backend::bir::Value::Kind::Immediate &&
      global.initializer->type == c4c::backend::bir::TypeKind::Ptr &&
      global.initializer->immediate == 0 &&
      global.initializer->immediate_bits == 0 &&
      global.initializer_symbol_name_id == c4c::kInvalidLinkName &&
      !global.initializer_symbol_name.has_value() &&
      global.initializer_elements.empty();
  return rv64_prepared_object_data_is_selected_fallback_candidate(
             global, object_data, status) &&
         (has_no_initializer || has_zero_pointer_initializer) &&
         object_data.object_label == global.link_name_id &&
         object_data.object_size_bytes == global.size_bytes;
}

std::optional<c4c::LinkNameId> rv64_selected_pointer_initializer_target(
    const c4c::backend::bir::Global& global) {
  if (global.initializer_symbol_name_id != c4c::kInvalidLinkName) {
    return global.initializer_symbol_name_id;
  }
  if (global.initializer.has_value() &&
      global.initializer->kind == c4c::backend::bir::Value::Kind::Named &&
      global.initializer->type == c4c::backend::bir::TypeKind::Ptr &&
      global.initializer->pointer_symbol_link_name_id != c4c::kInvalidLinkName) {
    return global.initializer->pointer_symbol_link_name_id;
  }
  return std::nullopt;
}

std::optional<std::string> rv64_selected_symbol_pointer_initializer_label(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& global,
    const prepare::PreparedGlobalObjectData& object_data,
    prepare::PreparedSelectedObjectDataContractStatus status) {
  const auto initializer_target = rv64_selected_pointer_initializer_target(global);
  if (!rv64_prepared_object_data_is_selected_fallback_candidate(
          global, object_data, status) ||
      global.type != c4c::backend::bir::TypeKind::Ptr ||
      global.size_bytes != 8 ||
      global.align_bytes < 8 ||
      !initializer_target.has_value() ||
      !global.initializer_elements.empty() ||
      object_data.object_label != global.link_name_id ||
      object_data.object_size_bytes != 8 ||
      object_data.align_bytes < 8) {
    return std::nullopt;
  }
  auto label = rv64_prepared_link_name_label(prepared, *initializer_target);
  if (label.empty()) {
    return std::nullopt;
  }
  return label;
}

std::optional<std::string> rv64_prepared_object_data_admission_diagnostic(
    const prepare::PreparedGlobalObjectData& object_data) {
  if (!rv64_prepared_object_data_has_emission_identity(object_data)) {
    return "RV64 object route missing selected object-data identity, extent, or alignment authority";
  }
  if (object_data.requires_unsupported_marker ||
      object_data.has_unsupported_marker || object_data.unsupported_but_coherent) {
    return "RV64 object route cannot emit unsupported-marker selected object-data";
  }
  switch (object_data.section_kind) {
    case prepare::PreparedObjectDataSectionKind::Bss:
      if (object_data.requires_relocation || object_data.has_relocation) {
        return "RV64 object route cannot emit prepared relocations in bss object data";
      }
      if (!object_data.requires_zero_fill || !object_data.has_zero_fill ||
          object_data.zero_fill_byte_count != object_data.object_size_bytes ||
          object_data.has_emitted_bytes || !object_data.emitted_bytes.empty()) {
        return "RV64 object route missing zero-fill authority for prepared global";
      }
      return std::nullopt;
    case prepare::PreparedObjectDataSectionKind::ReadOnlyData:
    case prepare::PreparedObjectDataSectionKind::Data:
      if (object_data.requires_relocation || object_data.has_relocation) {
        if (!object_data.requires_relocation || !object_data.has_relocation ||
            object_data.relocation_slots.empty()) {
          return "RV64 object route missing prepared relocation records for object data";
        }
        if (object_data.requires_emitted_bytes) {
          if (!object_data.has_emitted_bytes ||
              object_data.emitted_bytes.size() != object_data.object_size_bytes) {
            return "RV64 object route missing emitted-byte authority for prepared relocation object data";
          }
        } else if (object_data.has_emitted_bytes ||
                   !object_data.emitted_bytes.empty()) {
          return "RV64 object route has conflicting emitted-byte authority for prepared relocation object data";
        }
        if (object_data.requires_zero_fill || object_data.has_zero_fill ||
            object_data.zero_fill_byte_count != 0) {
          return "RV64 object route has conflicting zero-fill authority for prepared relocation object data";
        }
        return std::nullopt;
      }
      if (!object_data.requires_emitted_bytes || !object_data.has_emitted_bytes ||
          object_data.emitted_bytes.size() != object_data.object_size_bytes ||
          object_data.has_zero_fill || object_data.zero_fill_byte_count != 0) {
        return "RV64 object route missing emitted-byte authority for prepared global";
      }
      return std::nullopt;
  }
  return "RV64 object route cannot classify prepared global section authority";
}

object::SymbolId rv64_find_or_declare_relocation_symbol(
    object::ObjectModule& object_module,
    std::string label,
    object::SymbolKind kind) {
  if (auto* existing = object::find_symbol(object_module, label)) {
    return existing->id;
  }
  return object::declare_undefined_symbol(object_module,
                                          std::move(label),
                                          object::SymbolBinding::Global,
                                          kind)
      .id;
}

object::SymbolBinding rv64_prepared_object_data_symbol_binding(
    const prepare::PreparedGlobalObjectData& object_data) {
  return object_data.public_symbol ? object::SymbolBinding::Global
                                   : object::SymbolBinding::Local;
}

std::optional<std::string> attach_rv64_prepared_object_data_relocations(
    object::ObjectModule& object_module,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    object::SectionId section,
    std::uint64_t object_offset,
    const prepare::PreparedGlobalObjectData& object_data) {
  for (const auto& slot : object_data.relocation_slots) {
    if (slot.size_bytes != 8) {
      return "RV64 object route cannot emit prepared object-data relocation width " +
             std::to_string(slot.size_bytes);
    }
    if (slot.target == c4c::kInvalidLinkName ||
        slot.byte_offset > object_data.object_size_bytes ||
        slot.size_bytes > object_data.object_size_bytes - slot.byte_offset) {
      return "RV64 object route cannot emit invalid prepared object-data relocation slot";
    }
    const auto target_label = rv64_prepared_link_name_label(prepared, slot.target);
    if (target_label.empty()) {
      return "RV64 object route cannot emit unnamed prepared object-data relocation target";
    }
    const auto target_symbol = rv64_find_or_declare_relocation_symbol(
        object_module,
        target_label,
        rv64_prepared_link_symbol_kind(prepared, slot.target));
    object::attach_relocation(object_module,
                              section,
                              object_offset + slot.byte_offset,
                              kRiscvReloc64,
                              target_symbol,
                              0);
  }
  return std::nullopt;
}

const prepare::PreparedGlobalObjectData*
rv64_find_prepared_global_object_data_for_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& global) {
  const auto* fallback = prepare::find_prepared_global_object_data(
      prepared.object_data, global.link_name_id);
  for (const auto& object_data : prepared.object_data.globals) {
    if (object_data.object_label == global.link_name_id &&
        object_data.object_size_bytes == global.size_bytes &&
        object_data.align_bytes == global.align_bytes) {
      return &object_data;
    }
  }
  return fallback;
}

bool rv64_has_concrete_prepared_object_data_for_label(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId label) {
  for (const auto& object_data : prepared.object_data.globals) {
    if (object_data.object_label == label &&
        !object_data.requires_unsupported_marker &&
        !object_data.has_unsupported_marker &&
        !object_data.unsupported_but_coherent &&
        rv64_prepared_object_data_has_emission_identity(object_data)) {
      return true;
    }
  }
  return false;
}

bool rv64_is_prepared_string_constant_object_symbol(
    const c4c::backend::bir::Module& module,
    std::string_view label,
    std::uint64_t size_bytes) {
  for (const auto& constant : module.string_constants) {
    if (rv64_prepared_object_text_label(module, constant) != label) {
      continue;
    }
    const auto emitted_size =
        constant.bytes.empty() || constant.bytes.back() != 0
            ? constant.bytes.size() + 1
            : constant.bytes.size();
    if (emitted_size == size_bytes) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> append_rv64_prepared_data_objects(
    object::ObjectModule& object_module,
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  for (const auto& constant : prepared.module.string_constants) {
    const auto label = rv64_prepared_object_text_label(prepared.module, constant);
    const auto* existing = object::find_symbol(object_module, label);
    if (label.empty() || constant.align_bytes == 0 ||
        (existing != nullptr && !object::is_undefined_symbol(*existing))) {
      return "unsupported_global_data: RV64 object route cannot emit prepared string constant symbol";
    }

    std::vector<std::uint8_t> bytes(constant.bytes.begin(),
                                    constant.bytes.end());
    if (bytes.empty() || bytes.back() != 0) {
      bytes.push_back(0);
    }

    auto& rodata = object::get_or_create_section(object_module,
                                                 ".rodata",
                                                 object::SectionKind::Data,
                                                 constant.align_bytes,
                                                 true,
                                                 false,
                                                 false);
    object::align_section(rodata, constant.align_bytes, 0);
    const auto offset = object::append_section_bytes(rodata, bytes);
    object::define_symbol(object_module,
                          label,
                          object::SymbolBinding::Local,
                          object::SymbolKind::Object,
                          rodata.id,
                          offset,
                          bytes.size());
  }

  for (const auto& global : prepared.module.globals) {
    if (global.is_extern && !global.initializer.has_value() &&
        !global.initializer_symbol_name.has_value() &&
        global.initializer_symbol_name_id == c4c::kInvalidLinkName &&
        global.initializer_elements.empty()) {
      continue;
    }

    const auto* object_data =
        rv64_find_prepared_global_object_data_for_global(prepared, global);
    const auto facts = rv64_selected_object_data_contract_facts(object_data);
    const auto status =
        prepare::classify_prepared_selected_object_data_contract(facts);
    const auto report =
        prepare::verify_prepared_selected_object_data_contract(facts);
    if (object_data != nullptr &&
        rv64_prepared_object_data_is_selected_fallback_candidate(
            global, *object_data, status) &&
        rv64_has_concrete_prepared_object_data_for_label(
            prepared, global.link_name_id)) {
      continue;
    }
    if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
      const bool supports_zero_fill =
          object_data != nullptr &&
          rv64_is_selected_zero_fill_object_data(global, *object_data, status);
      const auto symbol_pointer_label =
          object_data == nullptr
              ? std::optional<std::string>{}
              : rv64_selected_symbol_pointer_initializer_label(
                    prepared, global, *object_data, status);
      if (!supports_zero_fill && !symbol_pointer_label.has_value()) {
        return "unsupported_global_data: " + report.detail;
      }
    } else if (auto diagnostic =
                   rv64_prepared_object_data_admission_diagnostic(*object_data)) {
      return "unsupported_global_data: " + *diagnostic;
    }

    const auto label = rv64_prepared_object_data_label(*object_data);
    if (label.empty()) {
      return "unsupported_global_data: RV64 object route cannot emit unnamed prepared global";
    }

    const auto* existing = object::find_symbol(object_module, label);
    if (object_data->align_bytes == 0) {
      return "unsupported_global_data: RV64 object route cannot emit prepared global without alignment authority";
    }

    const bool selected_zero_fill =
        rv64_is_selected_zero_fill_object_data(global, *object_data, status);
    const auto section_kind =
        selected_zero_fill ? prepare::PreparedObjectDataSectionKind::Bss
                           : object_data->section_kind;
    object::SectionRecord* section = nullptr;
    switch (section_kind) {
      case prepare::PreparedObjectDataSectionKind::Bss:
        section = &object::get_or_create_section(object_module,
                                                 ".bss",
                                                 object::SectionKind::Bss,
                                                 object_data->align_bytes,
                                                 true,
                                                 false,
                                                 true);
        break;
      case prepare::PreparedObjectDataSectionKind::ReadOnlyData:
        section = &object::get_or_create_section(object_module,
                                                 ".rodata",
                                                 object::SectionKind::Data,
                                                 object_data->align_bytes,
                                                 true,
                                                 false,
                                                 false);
        break;
      case prepare::PreparedObjectDataSectionKind::Data:
        section = &object::get_or_create_section(object_module,
                                                 ".data",
                                                 object::SectionKind::Data,
                                                 object_data->align_bytes,
                                                 true,
                                                 false,
                                                 true);
        break;
    }

    if (existing != nullptr && !object::is_undefined_symbol(*existing)) {
      const auto binding = rv64_prepared_object_data_symbol_binding(*object_data);
      if (existing->binding == binding &&
          existing->kind == object::SymbolKind::Object &&
          existing->section.has_value() && *existing->section == section->id &&
          existing->size_bytes == object_data->object_size_bytes) {
        continue;
      }
      if (existing->binding == object::SymbolBinding::Local &&
          existing->kind == object::SymbolKind::Object &&
          existing->size_bytes == object_data->object_size_bytes &&
          rv64_is_prepared_string_constant_object_symbol(
              prepared.module, label, object_data->object_size_bytes)) {
        continue;
      }
      if (existing->binding != binding ||
          existing->kind != object::SymbolKind::Object) {
        return "unsupported_global_data: RV64 object route cannot emit duplicate prepared global symbol '" +
               label + "' existing_size=" + std::to_string(existing->size_bytes) +
               " prepared_size=" + std::to_string(object_data->object_size_bytes);
      }
    }

    object::align_section(*section, object_data->align_bytes, 0);
    const auto symbol_pointer_label =
        rv64_selected_symbol_pointer_initializer_label(
            prepared, global, *object_data, status);
    std::uint64_t offset = 0;
    if (selected_zero_fill) {
      offset = object::reserve_section_bytes(*section,
                                             object_data->object_size_bytes);
    } else if (object_data->requires_relocation || object_data->has_relocation) {
      offset = object_data->emitted_bytes.empty()
                   ? object::reserve_section_bytes(*section,
                                                   object_data->object_size_bytes)
                   : object::append_section_bytes(*section,
                                                  object_data->emitted_bytes);
      if (auto diagnostic = attach_rv64_prepared_object_data_relocations(
              object_module, prepared, section->id, offset, *object_data)) {
        return "unsupported_global_data: " + *diagnostic;
      }
    } else if (symbol_pointer_label.has_value()) {
      std::vector<std::uint8_t> bytes;
      append_le64(bytes, 0);
      offset = object::append_section_bytes(*section, bytes);
      const auto initializer_target =
          rv64_selected_pointer_initializer_target(global);
      if (!initializer_target.has_value()) {
        return "unsupported_global_data: RV64 object route cannot emit unresolved prepared pointer initializer";
      }
      const auto target_symbol = rv64_find_or_declare_relocation_symbol(
          object_module,
          *symbol_pointer_label,
          rv64_prepared_link_symbol_kind(prepared, *initializer_target));
      object::attach_relocation(object_module,
                                section->id,
                                offset,
                                kRiscvReloc64,
                                target_symbol,
                                0);
    } else {
      offset =
          section->kind == object::SectionKind::Bss
              ? object::reserve_section_bytes(*section,
                                              object_data->zero_fill_byte_count)
              : object::append_section_bytes(*section,
                                             object_data->emitted_bytes);
    }
    object::define_symbol(object_module,
                          label,
                          rv64_prepared_object_data_symbol_binding(*object_data),
                          object::SymbolKind::Object,
                          section->id,
                          offset,
                          object_data->object_size_bytes);
  }
  return std::nullopt;
}

RiscvPreparedObjectModuleResult
build_rv64_prepared_text_object_module_with_diagnostics(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  std::vector<RiscvObjectFunction> functions;
  functions.reserve(prepared.control_flow.functions.size());
  for (const auto& control_flow : prepared.control_flow.functions) {
    auto admission = admit_rv64_prepared_module_function(prepared, control_flow);
    if (admission.prepared_consumer_category.has_value()) {
      return RiscvPreparedObjectModuleResult{
          .prepared_consumer_category = admission.prepared_consumer_category,
          .diagnostic = std::move(admission.diagnostic),
      };
    }
    if (!admission.diagnostic.empty()) {
      return make_rv64_prepared_module_rejection(std::move(admission.diagnostic));
    }
    if (admission.skip) {
      continue;
    }
    auto function = prepared_function_to_object_function(prepared, control_flow);
    if (function.prepared_consumer_category.has_value()) {
      return RiscvPreparedObjectModuleResult{
          .prepared_consumer_category = function.prepared_consumer_category,
          .diagnostic = std::move(function.diagnostic),
      };
    }
    if (!function.function.has_value()) {
      return make_rv64_prepared_module_rejection(
          function.diagnostic.empty()
              ? "unsupported_function_admission: prepared function could not be emitted for RV64 object route"
              : std::move(function.diagnostic));
    }
    functions.push_back(std::move(*function.function));
  }
  if (functions.empty()) {
    return make_rv64_prepared_module_rejection(
        "unsupported_function_admission: no defined prepared functions were available for RV64 object emission");
  }
  auto module = build_rv64_text_object_module(functions);
  if (!module.has_value()) {
    return make_rv64_prepared_module_rejection(
        "object_module_or_elf_build_failed: RV64 object module construction failed");
  }
  if (auto diagnostic = append_rv64_prepared_data_objects(*module, prepared)) {
    return make_rv64_prepared_module_rejection(std::move(*diagnostic));
  }
  return RiscvPreparedObjectModuleResult{
      .module = std::move(*module),
  };
}

std::optional<object::ObjectModule> build_rv64_prepared_text_object_module(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  return build_rv64_prepared_text_object_module_with_diagnostics(prepared).module;
}

std::optional<object::RelocatableElfImage> write_rv64_relocatable_elf_object(
    const object::ObjectModule& module) {
  return object::write_relocatable_elf(module, rv64_relocatable_elf_config());
}

RiscvPreparedObjectImageResult
write_rv64_prepared_relocatable_elf_object_with_diagnostics(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  auto module = build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (module.prepared_consumer_category.has_value() || !module.diagnostic.empty()) {
    return RiscvPreparedObjectImageResult{
        .prepared_consumer_category = module.prepared_consumer_category,
        .diagnostic = std::move(module.diagnostic),
    };
  }
  if (!module.module.has_value()) {
    return make_rv64_prepared_image_rejection(
        "object_module_or_elf_build_failed: RV64 prepared object module construction failed without diagnostic detail");
  }
  auto image = write_rv64_relocatable_elf_object(*module.module);
  if (!image.has_value()) {
    return make_rv64_prepared_image_rejection(
        "object_module_or_elf_build_failed: RV64 relocatable ELF serialization failed");
  }
  return RiscvPreparedObjectImageResult{
      .image = std::move(*image),
  };
}

std::optional<object::RelocatableElfImage>
write_rv64_prepared_relocatable_elf_object(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  return write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared).image;
}

}  // namespace c4c::backend::riscv::codegen
