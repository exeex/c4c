#include "object_emission.hpp"

#include "../../../prealloc/prepared_contract_verifier.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/publication_plans.hpp"
#include "../../../prealloc/target_register_profile.hpp"
#include "emit.hpp"
#include "prepared_frame_emit.hpp"
#include "rv64_line_assembler.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
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
namespace bir = c4c::backend::bir;

constexpr std::uint16_t kElfMachineRiscv = 243;
constexpr std::uint32_t kRiscvElfFlagsRv64DoubleFloatAbi = 0x5;
constexpr std::uint32_t kRiscvReloc64 = 2;
constexpr std::uint32_t kRiscvRelocCallPlt = 19;
constexpr std::uint32_t kRiscvRelocPcrelHi20 = 23;
constexpr std::uint32_t kRiscvRelocPcrelLo12I = 24;
constexpr std::uint32_t kRiscvRelocBranch = 16;
constexpr std::uint32_t kRiscvRelocJal = 17;

constexpr std::uint32_t encode_u_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t imm20) {
  return ((imm20 & 0xfffffu) << 12) | ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_i_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::int32_t imm12) {
  return ((static_cast<std::uint32_t>(imm12) & 0xfffu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_s_type(std::uint32_t opcode, std::uint32_t funct3,
                                      std::uint32_t rs1, std::uint32_t rs2,
                                      std::int32_t imm12) {
  const auto imm = static_cast<std::uint32_t>(imm12) & 0xfffu;
  return ((imm >> 5) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((imm & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_r_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::uint32_t rs2, std::uint32_t funct7) {
  return ((funct7 & 0x7fu) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_b_type(std::uint32_t opcode, std::uint32_t funct3,
                                      std::uint32_t rs1, std::uint32_t rs2,
                                      std::int32_t imm13) {
  const auto imm = static_cast<std::uint32_t>(imm13);
  return (((imm >> 12) & 0x1u) << 31) | (((imm >> 5) & 0x3fu) << 25) |
         ((rs2 & 0x1fu) << 20) | ((rs1 & 0x1fu) << 15) |
         ((funct3 & 0x7u) << 12) | (((imm >> 1) & 0xfu) << 8) |
         (((imm >> 11) & 0x1u) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_j_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::int32_t imm21) {
  const auto imm = static_cast<std::uint32_t>(imm21);
  return (((imm >> 20) & 0x1u) << 31) | (((imm >> 1) & 0x3ffu) << 21) |
         (((imm >> 11) & 0x1u) << 20) | (((imm >> 12) & 0xffu) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
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
  bytes.push_back(static_cast<std::uint8_t>(word & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 8) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 16) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 24) & 0xffu));
}

void append_le64(std::vector<std::uint8_t>& bytes, std::uint64_t word) {
  for (int shift = 0; shift < 64; shift += 8) {
    bytes.push_back(static_cast<std::uint8_t>((word >> shift) & 0xffu));
  }
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

std::optional<std::uint32_t> gpr_register_number_for_prior_preserved_selection(
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection);

std::optional<std::size_t> rv64_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout);

const prepare::PreparedVariadicVaListField*
rv64_variadic_va_list_overflow_arg_area_field(
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan);

bool rv64_variadic_helper_free_entry_contract_is_complete(
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan);

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

const prepare::PreparedVariadicVaListField*
rv64_variadic_va_list_overflow_arg_area_field(
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  for (const auto& field : entry_plan.va_list_layout.fields) {
    if (field.kind == prepare::PreparedVariadicVaListFieldKind::OverflowArgArea) {
      return &field;
    }
  }
  return nullptr;
}

std::optional<std::int32_t> prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes);

bool rv64_variadic_helper_free_entry_contract_is_complete(
    const prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  const auto* overflow_arg_area =
      rv64_variadic_va_list_overflow_arg_area_field(entry_plan);
  return !entry_plan.register_save_area.required &&
         entry_plan.overflow_area.required &&
         entry_plan.overflow_area.align_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.required &&
         entry_plan.va_list_layout.size_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.align_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.fields.size() == 1 &&
         overflow_arg_area != nullptr && overflow_arg_area->offset_bytes == 0 &&
         overflow_arg_area->size_bytes == 8;
}

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
  if (payload->destination_va_list_address.kind !=
          prepare::PreparedValueHomeKind::Register ||
      !destination_address.has_value()) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home"};
  }
  if (*destination_address == kRv64VaStartOverflowAreaScratch) {
    return std::string{
        "unsupported_variadic_helper_lowering: RV64 va_start helper destination va_list address aliases the overflow-area scratch register"};
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

RiscvPreparedObjectModuleResult make_rv64_prepared_module_rejection(
    std::string diagnostic) {
  return RiscvPreparedObjectModuleResult{
      .diagnostic = std::move(diagnostic),
  };
}

RiscvPreparedObjectImageResult make_rv64_prepared_image_rejection(
    std::string diagnostic) {
  return RiscvPreparedObjectImageResult{
      .diagnostic = std::move(diagnostic),
  };
}

struct RiscvLaidOutFragment {
  const RiscvEncodedFragment* fragment = nullptr;
  std::uint64_t section_offset = 0;
};

std::optional<std::uint32_t> rv64_register_number(std::string_view name) {
  if (name.size() >= 2 && name.front() == 'x') {
    std::uint32_t value = 0;
    const char* const begin = name.data() + 1;
    const char* const end = name.data() + name.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc{} && ptr == end && value <= 31) {
      return value;
    }
    return std::nullopt;
  }
  if (name == "zero") return 0;
  if (name == "ra") return 1;
  if (name == "sp") return 2;
  if (name == "gp") return 3;
  if (name == "tp") return 4;
  if (name == "t0") return 5;
  if (name == "t1") return 6;
  if (name == "t2") return 7;
  if (name == "s0" || name == "fp") return 8;
  if (name == "s1") return 9;
  if (name == "a0") return 10;
  if (name == "a1") return 11;
  if (name == "a2") return 12;
  if (name == "a3") return 13;
  if (name == "a4") return 14;
  if (name == "a5") return 15;
  if (name == "a6") return 16;
  if (name == "a7") return 17;
  if (name == "s2") return 18;
  if (name == "s3") return 19;
  if (name == "s4") return 20;
  if (name == "s5") return 21;
  if (name == "s6") return 22;
  if (name == "s7") return 23;
  if (name == "s8") return 24;
  if (name == "s9") return 25;
  if (name == "s10") return 26;
  if (name == "s11") return 27;
  if (name == "t3") return 28;
  if (name == "t4") return 29;
  if (name == "t5") return 30;
  if (name == "t6") return 31;
  return std::nullopt;
}

std::optional<std::uint32_t> gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  if ((home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register &&
       home.kind !=
           c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset) ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }
  if (home.target_register_identity.has_value() &&
      home.target_register_identity->bank !=
          c4c::backend::prepare::PreparedRegisterBank::Gpr) {
    return std::nullopt;
  }
  return rv64_register_number(*home.register_name);
}

std::optional<std::uint32_t> gpr_register_number_for_prior_preserved_selection(
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection) {
  namespace prepare = c4c::backend::prepare;

  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation ||
      selection.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      selection.preserved_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !selection.preserved_register_name.has_value() ||
      !selection.preserved_register_contiguous_width.has_value() ||
      *selection.preserved_register_contiguous_width != 1 ||
      selection.preserved_occupied_register_names.empty() ||
      !selection.preserved_register_placement.has_value()) {
    return std::nullopt;
  }
  return rv64_register_number(*selection.preserved_register_name);
}

std::optional<std::int32_t> stack_slot_offset_for_prior_preserved_gpr_selection(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes);

void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source);

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

std::optional<std::int64_t> materializable_fpr_immediate_bits(
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  std::uint64_t bits = 0;
  switch (value.type) {
    case c4c::backend::bir::TypeKind::F32:
      bits = value.immediate_bits & 0xffffffffu;
      break;
    case c4c::backend::bir::TypeKind::F64:
      bits = value.immediate_bits;
      break;
    default:
      return std::nullopt;
  }
  if (bits > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  const auto immediate = static_cast<std::int64_t>(bits);
  return fits_signed_12_bit_immediate(immediate) ? std::optional{immediate}
                                                 : std::nullopt;
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

std::optional<std::size_t> rv64_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type);

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

std::optional<std::int32_t> prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      (home.size_bytes.has_value() && *home.size_bytes != size_bytes) ||
      (home.align_bytes.has_value() && *home.align_bytes > size_bytes)) {
    return std::nullopt;
  }
  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *home.slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      slot_it->size_bytes < size_bytes ||
      slot_it->align_bytes > size_bytes) {
    return std::nullopt;
  }
  const std::size_t offset = *home.offset_bytes;
  if (slot_it->offset_bytes != offset ||
      slot_it->size_bytes < size_bytes) {
    return std::nullopt;
  }
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<std::int32_t> prepared_stack_slot_home_offset_for_value(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  return prepared_stack_slot_home_offset(stack_layout,
                                         *home,
                                         stack_frame_bytes,
                                         *size_bytes);
}

std::optional<std::uint32_t> rv64_load_store_funct3_for_size(std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return std::nullopt;
  }
}

std::optional<std::uint32_t> rv64_global_load_funct3_for_size(std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return std::nullopt;
  }
}

std::optional<std::uint32_t> rv64_global_store_funct3_for_size(std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return std::nullopt;
  }
}

bool append_rv64_store_register_to_stack(RiscvEncodedFragment& fragment,
                                         std::uint32_t source_register,
                                         std::int32_t offset,
                                         std::size_t size_bytes = 4) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_s_type(0x23, *funct3, 2, source_register, offset));
  return true;
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

bool append_rv64_store_register_to_global_base(RiscvEncodedFragment& fragment,
                                               std::uint32_t source_register,
                                               std::uint32_t base_register,
                                               std::int32_t offset,
                                               std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_global_store_funct3_for_size(size_bytes);
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
                                        std::size_t size_bytes = 4) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x03, destination_register, *funct3, 2, offset));
  return true;
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

bool append_rv64_load_global_base_to_register(RiscvEncodedFragment& fragment,
                                              std::uint32_t destination_register,
                                              std::uint32_t base_register,
                                              std::int32_t offset,
                                              std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_global_load_funct3_for_size(size_bytes);
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

void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source) {
  if (destination == source) {
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13, destination, 0, source, 0));  // addi rd, rs, 0
}

void append_rv64_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate) {
  if (!fits_signed_12_bit_immediate(immediate)) {
    const std::uint64_t low_bits =
        static_cast<std::uint64_t>(immediate) & 0xfffU;
    const std::int64_t lo12 = low_bits >= 0x800U
                                  ? static_cast<std::int64_t>(low_bits) - 0x1000
                                  : static_cast<std::int64_t>(low_bits);
    const std::int64_t hi = (immediate - lo12) / 4096;
    append_rv64_load_immediate(fragment, destination, hi);
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              destination,
                              1,
                              destination,
                              12));  // slli rd, rd, 12
    if (lo12 != 0) {
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination,
                                0,
                                destination,
                                static_cast<std::int32_t>(lo12)));  // addi rd, rd, lo
    }
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            0,
                            0,
                            static_cast<std::int32_t>(immediate)));
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
  if (overflow_field == nullptr || !destination.has_value() ||
      !destination_offset.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13, *destination, 0, 2, *destination_offset));
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
          *destination,
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
  return value.kind == c4c::backend::bir::Value::Kind::Named &&
         !value.name.empty() && names.value_names.find(value.name) == value_name;
}

bool prepared_bir_values_have_same_name(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  return lhs.kind == c4c::backend::bir::Value::Kind::Named &&
         rhs.kind == c4c::backend::bir::Value::Kind::Named && !lhs.name.empty() &&
         !rhs.name.empty() && names.value_names.find(lhs.name) == names.value_names.find(rhs.name);
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

bool append_rv64_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes);

std::optional<RiscvEncodedFragment> fragment_for_prepared_binary(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes);

std::optional<prepare::PreparedValueId> prepared_value_id_for_named_value(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const bir::Value& value);

bool prepared_join_transfer_edge_copies_are_published(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedJoinTransfer& join_transfer);

struct PreparedSelectEdgeSourceProducerFragment {
  bool matched = false;
  std::optional<RiscvEncodedFragment> fragment;
};

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

std::optional<RiscvEncodedFragment> fragment_for_prepared_move_bundle(
    const c4c::TargetProfile& target_profile,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements,
    std::size_t stack_frame_bytes,
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
      if (move_bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
          move_bundle.authority_kind != prepare::PreparedMoveAuthorityKind::None ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.reason != "consumer_register_to_stack" ||
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
      const auto source = gpr_register_number_for_home(*source_home);
      const auto type = prepared_bir_value_type_for_name(
          names, function, destination_home->value_name);
      if (!source.has_value() || !type.has_value()) {
        return std::nullopt;
      }
      const auto size_bytes = rv64_scalar_memory_size_for_type(*type);
      if (!size_bytes.has_value()) {
        return std::nullopt;
      }
      const auto stack_offset = prepared_stack_slot_home_offset(
          stack_layout, *destination_home, stack_frame_bytes, *size_bytes);
      if (!stack_offset.has_value() ||
          !append_rv64_store_register_to_stack(
              fragment, *source, *stack_offset, *size_bytes)) {
        return std::nullopt;
      }
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
      const auto stack_offset = prepared_stack_slot_home_offset(stack_layout,
                                                               *source_home,
                                                               stack_frame_bytes,
                                                               *size_bytes);
      if (!stack_offset.has_value() ||
          !append_rv64_load_stack_to_register(fragment,
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

bool append_rv64_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (is_rv64_null_pointer_value(value)) {
    append_rv64_load_immediate(fragment, destination, 0);
    return true;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, value);
  if (immediate.has_value()) {
    append_rv64_load_immediate(fragment, destination, *immediate);
    return true;
  }
  const auto source = gpr_register_number_for_value(names, lookups, value);
  if (source.has_value()) {
    append_rv64_move(fragment, destination, *source);
    return true;
  }
  const auto stack_offset =
      prepared_stack_slot_home_offset_for_value(stack_layout,
                                                names,
                                                lookups,
                                                value,
                                                stack_frame_bytes);
  if (stack_offset.has_value()) {
    const auto size_bytes = rv64_scalar_memory_size_for_type(value.type);
    if (!size_bytes.has_value()) {
      return false;
    }
    return append_rv64_load_stack_to_register(fragment,
                                             destination,
                                             *stack_offset,
                                             *size_bytes);
  }
  return false;
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
  const bool matching_scalar_integer_operands =
      lhs.type == rhs.type &&
      lhs.type == c4c::backend::bir::TypeKind::I32;
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

std::size_t rv64_call_frame_size(std::size_t local_frame_bytes) {
  return local_frame_bytes + 16;
}

std::int32_t rv64_call_frame_ra_offset(std::size_t local_frame_bytes) {
  return static_cast<std::int32_t>(local_frame_bytes + 8);
}

bool rv64_is_callee_saved_gpr_register_name(std::string_view name) {
  return name == "s0" || name == "fp" || name == "s1" || name == "s2" ||
         name == "s3" || name == "s4" || name == "s5" || name == "s6" ||
         name == "s7" || name == "s8" || name == "s9" || name == "s10" ||
         name == "s11";
}

std::optional<std::int32_t> rv64_saved_callee_gpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (saved.bank != prepare::PreparedRegisterBank::Gpr ||
      saved.register_name.empty() ||
      !rv64_is_callee_saved_gpr_register_name(saved.register_name) ||
      !rv64_register_number(saved.register_name).has_value() ||
      saved.contiguous_width != 1 ||
      saved.occupied_register_names.size() != 1 ||
      saved.occupied_register_names.front() != saved.register_name ||
      !saved.placement.has_value() ||
      saved.placement->bank != prepare::PreparedRegisterBank::Gpr ||
      saved.placement->pool != prepare::PreparedRegisterSlotPool::CalleeSaved ||
      saved.placement->contiguous_width != 1 ||
      !saved.slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *saved.slot_placement)) {
    return std::nullopt;
  }

  const auto& slot = *saved.slot_placement;
  if (slot.bank != prepare::PreparedRegisterBank::Gpr ||
      slot.register_name != saved.register_name ||
      slot.contiguous_width != 1 ||
      slot.occupied_register_names.size() != 1 ||
      slot.occupied_register_names.front() != saved.register_name ||
      !slot.register_placement.has_value() ||
      slot.register_placement != saved.placement ||
      !slot.stack_offset_bytes.has_value() ||
      !slot.size_bytes.has_value() ||
      *slot.size_bytes != 8 ||
      slot.stack_offset_bytes > std::optional<std::size_t>{stack_frame_bytes} ||
      stack_frame_bytes - *slot.stack_offset_bytes < *slot.size_bytes ||
      *slot.stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(*slot.stack_offset_bytes))) {
    return std::nullopt;
  }

  return static_cast<std::int32_t>(*slot.stack_offset_bytes);
}

bool append_rv64_saved_callee_gpr_spills(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (frame_plan == nullptr) {
    return true;
  }
  for (const auto& saved : frame_plan->saved_callee_registers) {
    const auto source = rv64_register_number(saved.register_name);
    const auto offset =
        rv64_saved_callee_gpr_stack_offset(saved, stack_frame_bytes);
    if (!source.has_value() || !offset.has_value() ||
        !append_rv64_store_register_to_stack(fragment, *source, *offset, 8)) {
      return false;
    }
  }
  return true;
}

bool append_rv64_saved_callee_gpr_restores(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (frame_plan == nullptr) {
    return true;
  }
  for (auto it = frame_plan->saved_callee_registers.rbegin();
       it != frame_plan->saved_callee_registers.rend();
       ++it) {
    const auto destination = rv64_register_number(it->register_name);
    const auto offset = rv64_saved_callee_gpr_stack_offset(*it, stack_frame_bytes);
    if (!destination.has_value() || !offset.has_value() ||
        !append_rv64_load_stack_to_register(fragment, *destination, *offset, 8)) {
      return false;
    }
  }
  return true;
}

std::optional<RiscvEncodedFragment> make_rv64_call_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes) {
  RiscvEncodedFragment fragment;
  const auto frame_size = rv64_call_frame_size(local_frame_bytes);
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            -static_cast<std::int32_t>(frame_size)));
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            3,
                            2,
                            1,
                            rv64_call_frame_ra_offset(local_frame_bytes)));
  if (!append_rv64_saved_callee_gpr_spills(fragment,
                                          frame_plan,
                                          local_frame_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> make_rv64_stack_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            -static_cast<std::int32_t>(stack_frame_bytes)));
  if (!append_rv64_saved_callee_gpr_spills(fragment,
                                          frame_plan,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

bool append_rv64_call_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes) {
  const auto frame_size = rv64_call_frame_size(local_frame_bytes);
  if (!append_rv64_saved_callee_gpr_restores(fragment,
                                            frame_plan,
                                            local_frame_bytes)) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            1,
                            3,
                            2,
                            rv64_call_frame_ra_offset(local_frame_bytes)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            static_cast<std::int32_t>(frame_size)));
  return true;
}

bool append_rv64_stack_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (stack_frame_bytes == 0) {
    return frame_plan == nullptr || frame_plan->saved_callee_registers.empty();
  }
  if (!append_rv64_saved_callee_gpr_restores(fragment,
                                            frame_plan,
                                            stack_frame_bytes)) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            static_cast<std::int32_t>(stack_frame_bytes)));
  return true;
}

std::optional<std::size_t> rv64_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout) {
  const auto addressing_frame_size =
      addressing == nullptr ? std::size_t{0} : addressing->frame_size_bytes;
  auto prepared_frame_size =
      frame_plan == nullptr ? stack_layout.frame_size_bytes : frame_plan->frame_size_bytes;
  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!saved.slot_placement.has_value() ||
          !saved.slot_placement->stack_offset_bytes.has_value() ||
          !saved.slot_placement->size_bytes.has_value()) {
        continue;
      }
      if (*saved.slot_placement->stack_offset_bytes >
          std::numeric_limits<std::size_t>::max() -
              *saved.slot_placement->size_bytes) {
        return std::nullopt;
      }
      prepared_frame_size =
          std::max(prepared_frame_size,
                   *saved.slot_placement->stack_offset_bytes +
                       *saved.slot_placement->size_bytes);
    }
  }
  const auto frame_size = std::max(addressing_frame_size, prepared_frame_size);
  if (frame_size == 0) {
    return 0;
  }
  const auto aligned = ((frame_size + 15) / 16) * 16;
  return fits_signed_12_bit_immediate(static_cast<std::int64_t>(aligned))
             ? std::optional<std::size_t>{aligned}
             : std::nullopt;
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

std::optional<std::int32_t> prepared_frame_slot_call_argument_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.source_value_id.has_value() ||
      !argument.source_slot_id.has_value()) {
    return std::nullopt;
  }
  std::optional<prepare::PreparedCallArgumentFrameSlotValueRoute>
      frame_slot_value_route;
  if (argument.source_selection.has_value()) {
    const auto& selection = *argument.source_selection;
    const auto route_report =
        prepare::verify_prepared_frame_slot_value_source_route_contract(
            &selection);
    frame_slot_value_route =
        prepare::as_frame_slot_value_source_route(selection);
    if (route_report.fail_closed || !frame_slot_value_route.has_value() ||
        frame_slot_value_route->source_value_id != *argument.source_value_id ||
        frame_slot_value_route->source_slot_id != *argument.source_slot_id) {
      return std::nullopt;
    }
  }
  const auto* source_home =
      prepared_value_home_for_id(lookups, *argument.source_value_id);
  const auto size_bytes = rv64_scalar_memory_size_for_type(argument_type);
  const auto source_size_bytes =
      frame_slot_value_route.has_value()
          ? std::optional<std::size_t>{frame_slot_value_route->source_size_bytes}
          : source_home == nullptr ? std::optional<std::size_t>{}
                                   : source_home->size_bytes;
  const auto source_align_bytes =
      frame_slot_value_route.has_value()
          ? std::optional<std::size_t>{frame_slot_value_route->source_align_bytes}
          : source_home == nullptr ? std::optional<std::size_t>{}
                                   : source_home->align_bytes;
  if (source_home == nullptr ||
      source_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !source_home->slot_id.has_value() ||
      *source_home->slot_id !=
          (frame_slot_value_route.has_value()
               ? frame_slot_value_route->source_slot_id
               : *argument.source_slot_id) ||
      !source_home->offset_bytes.has_value() ||
      !size_bytes.has_value() ||
      !source_size_bytes.has_value() ||
      *source_size_bytes != *size_bytes ||
      (source_align_bytes.has_value() &&
       *source_align_bytes > *size_bytes)) {
    return std::nullopt;
  }
  if (argument.source_stack_offset_bytes.has_value() &&
      *argument.source_stack_offset_bytes != *source_home->offset_bytes) {
    return std::nullopt;
  }
  if (frame_slot_value_route.has_value() &&
      frame_slot_value_route->source_stack_offset_bytes !=
          *source_home->offset_bytes) {
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
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < *size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<std::int32_t> stack_slot_offset_for_prior_preserved_gpr_selection(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  const auto size_bytes = rv64_scalar_memory_size_for_type(argument_type);
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation ||
      selection.preservation_route != prepare::PreparedCallPreservationRoute::StackSlot ||
      selection.source_home_kind !=
          std::optional<prepare::PreparedValueHomeKind>{
              prepare::PreparedValueHomeKind::StackSlot} ||
      (selection.preserved_register_bank.has_value() &&
       *selection.preserved_register_bank != prepare::PreparedRegisterBank::None &&
       *selection.preserved_register_bank != prepare::PreparedRegisterBank::Gpr) ||
      !selection.source_slot_id.has_value() ||
      !selection.source_stack_offset_bytes.has_value() ||
      !selection.source_size_bytes.has_value() ||
      !selection.preserved_stack_slot_id.has_value() ||
      !selection.preserved_stack_offset_bytes.has_value() ||
      !selection.preserved_stack_size_bytes.has_value() ||
      !selection.preserved_stack_align_bytes.has_value() ||
      !size_bytes.has_value() ||
      *selection.source_slot_id != *selection.preserved_stack_slot_id ||
      *selection.source_stack_offset_bytes !=
          *selection.preserved_stack_offset_bytes ||
      *selection.source_size_bytes != *size_bytes ||
      *selection.preserved_stack_size_bytes != *size_bytes ||
      *selection.preserved_stack_align_bytes > *size_bytes) {
    return std::nullopt;
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *selection.preserved_stack_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      slot_it->offset_bytes != *selection.preserved_stack_offset_bytes ||
      slot_it->size_bytes < *size_bytes ||
      slot_it->align_bytes > *size_bytes) {
    return std::nullopt;
  }

  const std::size_t offset = *selection.preserved_stack_offset_bytes;
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < *size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
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
      return std::nullopt;
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
  if ((source_stack_offset_bytes.has_value() &&
       *source_stack_offset_bytes != offset) ||
      offset < slot_it->offset_bytes ||
      offset > slot_it->offset_bytes + slot_it->size_bytes ||
      offset > stack_frame_bytes ||
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
  bir::Value source_value;
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
  bir::Value source_value = fact->payload_value;
  if (plan.source_load_local != nullptr) {
    if (plan.source_load_local->result.type != bir::TypeKind::Ptr ||
        plan.source_load_local->result != fact->payload_value) {
      return std::nullopt;
    }
  }
  if (source_value.kind != bir::Value::Kind::Named ||
      source_value.type != bir::TypeKind::Ptr ||
      prepared_value_home_for(prepared.names, lookups, source_value) == nullptr ||
      fact->destination_stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      fact->payload_value_name == *argument.source_selection->source_value_name) {
    return std::nullopt;
  }

  return PreparedFrameSlotAddressArgumentPublication{
      .address_offset = *address_offset,
      .destination_offset =
          static_cast<std::int32_t>(fact->destination_stack_offset_bytes),
      .source_value = std::move(source_value),
  };
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
      call_plan->is_indirect || call_plan->indirect_callee.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value()) {
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
        !rv64_noop_stack_slot_preservation_effect(
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall)) {
      return std::nullopt;
    }
  }

  std::size_t active_call_stack_adjustment = 0;
  bool emitted_memory_return_argument = false;
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
          argument.destination_stack_offset_bytes.has_value() ||
          argument.destination_stack_size_bytes.has_value() ||
          selection == nullptr ||
          selection->kind != prepare::PreparedCallArgumentSourceSelectionKind::
                                 LocalFrameAddressMaterialization ||
          !local_materialization_route.has_value() ||
          transport == nullptr ||
          transport->kind != prepare::PreparedAggregateTransportKind::StackCopy ||
          !transport->source_stack_offset_bytes.has_value() ||
          *transport->source_stack_offset_bytes !=
              local_materialization_route->source_stack_offset_bytes ||
          transport->destination_stack_offset_bytes.has_value() ||
          transport->destination_stack_size_bytes.has_value() ||
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
          abi.align_bytes != transport->copy_align_bytes) {
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
    if (!argument.destination_register_bank.has_value()) {
      return std::nullopt;
    }
    if (*argument.destination_register_bank == prepare::PreparedRegisterBank::Fpr) {
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
    if (argument.source_selection.has_value() &&
        argument.source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization) {
      const auto offset =
          prepared_frame_slot_address_call_argument_offset(stack_layout,
                                                           lookups,
                                                           frame_plan,
                                                           argument,
                                                           stack_frame_bytes);
      if (!offset.has_value()) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                *destination,
                                0,
                                2,
                                *offset));  // addi rd, sp, off
      continue;
    }
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
      if (!publication.has_value()) {
        return std::nullopt;
      }
      const std::uint32_t scratch = rv64_temporary_gpr_avoiding(*destination);
      if (!append_rv64_move_value_to_register(fragment,
                                              scratch,
                                              stack_layout,
                                              prepared.names,
                                              lookups,
                                              publication->source_value,
                                              stack_frame_bytes) ||
          !append_rv64_store_register_to_stack(fragment,
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
          const auto source = gpr_register_number_for_prior_preserved_selection(
              *argument.source_selection);
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
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
        case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
          return std::nullopt;
      }
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
        argument.source_literal.has_value()) {
      const auto immediate =
          integer_immediate_for_value({}, nullptr, *argument.source_literal);
      if (!immediate.has_value()) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment, *destination, *immediate);
      continue;
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
      const auto offset =
          prepared_frame_slot_call_argument_offset(stack_layout,
                                                   lookups,
                                                   argument,
                                                   call.arg_types[arg_index],
                                                   stack_frame_bytes);
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
    if (argument.source_encoding ==
            prepare::PreparedStorageEncodingKind::SymbolAddress &&
        (argument.source_symbol_name.has_value() ||
         argument.source_symbol_name_id.has_value())) {
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
      if (call.result->type == c4c::backend::bir::TypeKind::Ptr) {
        return std::nullopt;
      }
      const auto size_bytes = rv64_scalar_memory_size_for_type(call.result->type);
      if (!source.has_value() || destination_home == nullptr ||
          !size_bytes.has_value() ||
          destination_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
          destination_home->slot_id != result.destination_slot_id ||
          destination_home->offset_bytes != result.destination_stack_offset_bytes) {
        return std::nullopt;
      }
      const auto destination_offset = prepared_stack_slot_home_offset(
          stack_layout, *destination_home, stack_frame_bytes, *size_bytes);
      if (!destination_offset.has_value() ||
          static_cast<std::size_t>(*destination_offset) !=
              *result.destination_stack_offset_bytes ||
          !append_rv64_store_register_to_stack(fragment,
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

std::optional<RiscvEncodedFragment> fragment_for_prepared_return(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::bir::Terminator& terminator,
    std::size_t block_index,
    std::size_t terminator_instruction_index,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.return_lanes.empty()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  if (!terminator.value.has_value()) {
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  if ((terminator.value->type == c4c::backend::bir::TypeKind::F32 ||
       terminator.value->type == c4c::backend::bir::TypeKind::F64) &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      !terminator.value->name.empty() && lookups != nullptr) {
    const auto value_name = names.value_names.find(terminator.value->name);
    const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
    if (value_name != c4c::kInvalidValueName &&
        value_id_it != lookups->value_homes.value_ids.end() &&
        prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
            &lookups->move_bundles,
            nullptr,
            block_index,
            value_id_it->second,
            prepare::PreparedRegisterBank::Fpr) != nullptr) {
      if (restore_return_address) {
        if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
          return std::nullopt;
        }
      } else if (!append_rv64_stack_frame_epilogue(
                     fragment,
                     frame_plan,
                     stack_frame_bytes)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
      return fragment;
    }
  }
  if (const auto bits = materializable_fpr_immediate_bits(*terminator.value)) {
    constexpr std::uint32_t scratch = 5;  // t0
    constexpr std::uint32_t return_fpr = 10;  // fa0
    append_rv64_load_immediate(fragment, scratch, *bits);
    if (!append_rv64_gpr_to_fpr_move(
            fragment, return_fpr, scratch, terminator.value->type)) {
      return std::nullopt;
    }
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  const bool direct_global_pointer_return_candidate =
      terminator.value->type == c4c::backend::bir::TypeKind::Ptr &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      (terminator.value->pointer_symbol_link_name_id != c4c::kInvalidLinkName ||
       (!terminator.value->name.empty() && terminator.value->name.front() == '@'));
  if (direct_global_pointer_return_candidate) {
    const auto* value_home = prepared_value_home_for(names, lookups, *terminator.value);
    const auto* before_return_move =
        value_home == nullptr
            ? nullptr
            : prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
                  lookups == nullptr ? nullptr : &lookups->move_bundles,
                  nullptr,
                  block_index,
                  value_home->value_id,
                  prepare::PreparedRegisterBank::Gpr);
    const auto authority =
        prepare::plan_prepared_direct_global_return_authority({
            .names = &names,
            .return_value = &*terminator.value,
            .value_home = value_home,
            .before_return_move = before_return_move,
            .block_index = block_index,
            .instruction_index = terminator_instruction_index,
        });
    if (!prepare::prepared_direct_global_return_authority_available(authority) ||
        authority.before_return_move == nullptr || authority.value_home == nullptr) {
      return std::nullopt;
    }
    const auto source = gpr_register_number_for_home(*authority.value_home);
    const auto destination =
        rv64_register_number(*authority.before_return_move->destination_register_name);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  if (!append_rv64_move_value_to_register(fragment,
                                          10,
                                          stack_layout,
                                          names,
                                          lookups,
                                          *terminator.value,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  if (restore_return_address) {
    if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
      return std::nullopt;
    }
  } else if (!append_rv64_stack_frame_epilogue(
                 fragment,
                 frame_plan,
                 stack_frame_bytes)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
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

std::optional<std::int32_t> prepared_frame_slot_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  if (access == nullptr || access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind !=
          c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      access->address.byte_offset < 0 ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  std::size_t slot_offset = 0;
  if (*access->address.frame_slot_id != 0 || !stack_layout.frame_slots.empty()) {
    const auto slot_it =
        std::find_if(stack_layout.frame_slots.begin(),
                     stack_layout.frame_slots.end(),
                     [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                       return slot.slot_id == *access->address.frame_slot_id;
                     });
    if (slot_it == stack_layout.frame_slots.end()) {
      return std::nullopt;
    }
    const auto slot_byte_offset =
        static_cast<std::size_t>(access->address.byte_offset);
    if (slot_byte_offset > slot_it->size_bytes ||
        slot_it->size_bytes - slot_byte_offset < size_bytes) {
      return std::nullopt;
    }
    slot_offset = slot_it->offset_bytes;
  }
  const auto offset =
      slot_offset + static_cast<std::size_t>(access->address.byte_offset);
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<std::pair<std::uint32_t, std::int32_t>>
prepared_pointer_value_base_offset(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t size_bytes) {
  if (access == nullptr ||
      access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind !=
          c4c::backend::prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto base_register =
      gpr_register_number_for_value_name(lookups, *access->address.pointer_value_name);
  if (!base_register.has_value()) {
    return std::nullopt;
  }
  return std::pair<std::uint32_t, std::int32_t>{
      *base_register,
      static_cast<std::int32_t>(access->address.byte_offset)};
}

std::optional<std::int32_t> prepared_byval_stack_slot_pointer_access_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || access == nullptr ||
      access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      access->address.byte_offset < 0 ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto value_id_it =
      lookups->value_homes.value_ids.find(*access->address.pointer_value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
      home.value_name != *access->address.pointer_value_name ||
      *home.align_bytes > size_bytes) {
    return std::nullopt;
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
      frame_slot_it->size_bytes != *home.size_bytes ||
      frame_slot_it->align_bytes != *home.align_bytes) {
    return std::nullopt;
  }
  const auto object_it =
      std::find_if(stack_layout.objects.begin(),
                   stack_layout.objects.end(),
                   [&](const prepare::PreparedStackObject& object) {
                     return object.object_id == frame_slot_it->object_id &&
                            object.function_name == home.function_name;
                   });
  if (object_it == stack_layout.objects.end() ||
      object_it->value_name != home.value_name ||
      object_it->source_kind != "byval_param" ||
      object_it->size_bytes != *home.size_bytes ||
      object_it->align_bytes != *home.align_bytes ||
      !object_it->address_exposed || !object_it->requires_home_slot ||
      !object_it->permanent_home_slot) {
    return std::nullopt;
  }
  const auto byte_offset =
      static_cast<std::size_t>(access->address.byte_offset);
  if (byte_offset > *home.size_bytes ||
      *home.size_bytes - byte_offset < size_bytes ||
      *home.offset_bytes > std::numeric_limits<std::size_t>::max() - byte_offset) {
    return std::nullopt;
  }
  const auto offset = *home.offset_bytes + byte_offset;
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

struct PreparedSretStackPointerAccess {
  std::int32_t pointer_home_offset = 0;
  std::int32_t pointee_offset = 0;
};

std::optional<PreparedSretStackPointerAccess>
prepared_sret_stack_slot_pointer_access(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || access == nullptr ||
      access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto value_id_it =
      lookups->value_homes.value_ids.find(*access->address.pointer_value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
      home.value_name != *access->address.pointer_value_name ||
      *home.align_bytes > 8) {
    return std::nullopt;
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
      frame_slot_it->size_bytes != *home.size_bytes ||
      frame_slot_it->align_bytes != *home.align_bytes) {
    return std::nullopt;
  }
  const auto object_it =
      std::find_if(stack_layout.objects.begin(),
                   stack_layout.objects.end(),
                   [&](const prepare::PreparedStackObject& object) {
                     return object.object_id == frame_slot_it->object_id &&
                            object.function_name == home.function_name;
                   });
  if (object_it == stack_layout.objects.end() ||
      object_it->value_name != home.value_name ||
      object_it->source_kind != "sret_param" ||
      object_it->type != c4c::backend::bir::TypeKind::Ptr ||
      !object_it->address_exposed || !object_it->requires_home_slot ||
      !object_it->permanent_home_slot) {
    return std::nullopt;
  }
  if (*home.offset_bytes > stack_frame_bytes ||
      stack_frame_bytes - *home.offset_bytes < 8 ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(*home.offset_bytes))) {
    return std::nullopt;
  }
  return PreparedSretStackPointerAccess{
      .pointer_home_offset = static_cast<std::int32_t>(*home.offset_bytes),
      .pointee_offset = static_cast<std::int32_t>(access->address.byte_offset),
  };
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

std::optional<std::size_t> rv64_local_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return std::size_t{4};
    case c4c::backend::bir::TypeKind::F64:
      return std::size_t{8};
    default:
      return rv64_scalar_memory_size_for_type(type);
  }
}

std::optional<std::size_t> rv64_global_scalar_memory_size_for_type(
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
    const c4c::backend::bir::BinaryInst& binary) {
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
                     return slot.slot_id == *selected->frame_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      selected->byte_offset < static_cast<std::int64_t>(slot_it->offset_bytes)) {
    return std::nullopt;
  }
  const auto offset = static_cast<std::size_t>(selected->byte_offset);
  if (!fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            *destination,
                            0,
                            2,
                            static_cast<std::int32_t>(offset)));  // addi rd, sp, off
  return fragment;
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_direct_global_materialization_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (materialization.kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      materialization.address_space != bir::AddressSpace::Default ||
      materialization.is_thread_local || materialization.has_tls_address_space ||
      materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      !materialization.symbol_name.has_value()) {
    return std::nullopt;
  }

  const std::string_view prepared_symbol =
      prepare::prepared_link_name(prepared.names, *materialization.symbol_name);
  if (prepared_symbol.empty()) {
    return std::nullopt;
  }

  for (const auto& global : prepared.module.globals) {
    std::string global_label = global.name;
    if (global.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view spelling =
          prepared.module.names.link_names.spelling(global.link_name_id);
      if (!spelling.empty()) {
        global_label = std::string{spelling};
      }
    }
    if (global_label == prepared_symbol) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Object};
    }
  }

  for (const auto& function : prepared.module.functions) {
    if (function.name == prepared_symbol) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Function};
    }
  }

  return std::pair<std::string, RiscvObjectFixupTargetKind>{
      std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Object};
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_address_materialization_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (materialization.kind ==
      prepare::PreparedAddressMaterializationKind::DirectGlobal) {
    return prepared_direct_global_materialization_symbol(prepared, materialization);
  }
  if (materialization.kind ==
          prepare::PreparedAddressMaterializationKind::StringConstant &&
      materialization.address_space == bir::AddressSpace::Default &&
      !materialization.is_thread_local && !materialization.has_tls_address_space &&
      materialization.text_name.has_value()) {
    const std::string_view label =
        prepared.names.texts.lookup(*materialization.text_name);
    if (!label.empty()) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{label}, RiscvObjectFixupTargetKind::Object};
    }
  }
  return std::nullopt;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_symbol_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary) {
  if (lookups == nullptr ||
      binary.opcode != c4c::backend::bir::BinaryOpcode::Add ||
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
        materialization->result_value_name != result_value_name) {
      continue;
    }
    if (!prepared_address_materialization_symbol(prepared, *materialization).has_value()) {
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
  auto symbol = prepared_address_materialization_symbol(prepared, *selected);
  if (!symbol.has_value()) {
    return std::nullopt;
  }

  const std::string auipc_label = ".Lpcrel_hi_" +
                                  std::to_string(selected->function_name) + "_" +
                                  std::to_string(selected->block_label) + "_" +
                                  std::to_string(selected->inst_index);
  return make_rv64_pcrel_address_fragment(*destination,
                                           std::move(symbol->first),
                                           auipc_label,
                                           symbol->second,
                                           selected->byte_offset);
}

std::optional<std::int32_t> prepared_frame_slot_address_materialization_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (lookups == nullptr || value.type != bir::TypeKind::Ptr ||
      value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
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
        materialization->inst_index != instruction_index ||
        materialization->kind !=
            prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->result_value_name != value_name ||
        !materialization->frame_slot_id.has_value() ||
        materialization->byte_offset < 0 ||
        materialization->address_space != bir::AddressSpace::Default ||
        materialization->is_thread_local ||
        materialization->has_tls_address_space ||
        materialization->tls_model !=
            prepare::PreparedTlsMaterializationModel::None ||
        materialization->tls_thread_pointer_register !=
            prepare::PreparedTlsThreadPointerRegister::None ||
        materialization->tls_high_relocation !=
            prepare::PreparedTlsRelocationKind::None ||
        materialization->tls_low_relocation !=
            prepare::PreparedTlsRelocationKind::None) {
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
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *selected->frame_slot_id;
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      selected->byte_offset < static_cast<std::int64_t>(slot_it->offset_bytes)) {
    return std::nullopt;
  }
  const auto offset = static_cast<std::size_t>(selected->byte_offset);
  if (offset > slot_it->offset_bytes + slot_it->size_bytes ||
      offset > stack_frame_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

bool append_rv64_materialize_or_move_store_value(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto frame_slot_address_offset =
      prepared_frame_slot_address_materialization_offset(stack_layout,
                                                         names,
                                                         lookups,
                                                         block_label,
                                                         instruction_index,
                                                         value,
                                                         stack_frame_bytes);
  if (frame_slot_address_offset.has_value()) {
    append_le32(fragment.bytes,
                encode_i_type(0x13,
                              destination,
                              0,
                              2,
                              *frame_slot_address_offset));  // addi rd, sp, off
    return true;
  }
  return append_rv64_move_value_to_register(fragment,
                                           destination,
                                           stack_layout,
                                           names,
                                           lookups,
                                           value,
                                           stack_frame_bytes);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_store_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::StoreLocalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (rv64_floating_type(store.value.type)) {
    constexpr std::uint32_t scratch_fpr = 0;  // ft0
    const auto size_bytes = rv64_local_memory_size_for_type(store.value.type);
    if (!size_bytes.has_value()) {
      return std::nullopt;
    }
    const auto offset =
        prepared_frame_slot_absolute_offset(stack_layout,
                                            access,
                                            stack_frame_bytes,
                                            *size_bytes);
    if (offset.has_value()) {
      RiscvEncodedFragment fragment;
      const auto source_fpr = append_rv64_prepare_floating_value_for_store(
          fragment, scratch_fpr, 6, names, lookups, store.value);
      if (!source_fpr.has_value() ||
          !append_rv64_store_fpr_to_stack(
              fragment, *source_fpr, *offset, store.value.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto pointer_base =
        prepared_pointer_value_base_offset(lookups, access, *size_bytes);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    RiscvEncodedFragment fragment;
    const auto source_fpr = append_rv64_prepare_floating_value_for_store(
        fragment,
        scratch_fpr,
        rv64_temporary_gpr_avoiding(pointer_base->first),
        names,
        lookups,
        store.value);
    if (!source_fpr.has_value() ||
        !append_rv64_store_fpr_to_base(fragment,
                                      *source_fpr,
                                      pointer_base->first,
                                      pointer_base->second,
                                      store.value.type)) {
      return std::nullopt;
    }
    return fragment;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type(store.value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto offset =
      prepared_frame_slot_absolute_offset(stack_layout,
                                          access,
                                          stack_frame_bytes,
                                          *size_bytes);
  if (!offset.has_value()) {
    const auto byval_offset =
        prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                        lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes);
    if (byval_offset.has_value()) {
      RiscvEncodedFragment fragment;
      if (!append_rv64_materialize_or_move_store_value(fragment,
                                                       6,
                                                       stack_layout,
                                                       names,
                                                       lookups,
                                                       block_label,
                                                       instruction_index,
                                                       store.value,
                                                       stack_frame_bytes) ||
          !append_rv64_store_register_to_stack(fragment,
                                               6,
                                               *byval_offset,
                                               *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto sret_pointer =
        prepared_sret_stack_slot_pointer_access(stack_layout,
                                                lookups,
                                                access,
                                                stack_frame_bytes,
                                                *size_bytes);
    if (sret_pointer.has_value()) {
      RiscvEncodedFragment fragment;
      if (!append_rv64_load_stack_to_register(fragment,
                                             7,
                                             sret_pointer->pointer_home_offset,
                                             8) ||
          !append_rv64_move_value_to_register(fragment,
                                              6,
                                              stack_layout,
                                              names,
                                              lookups,
                                              store.value,
                                              stack_frame_bytes) ||
          !append_rv64_store_register_to_base(fragment,
                                             6,
                                             7,
                                             sret_pointer->pointee_offset,
                                             *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto pointer_base =
        prepared_pointer_value_base_offset(lookups, access, *size_bytes);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    const std::uint32_t value_register =
        rv64_temporary_gpr_avoiding(pointer_base->first);
    RiscvEncodedFragment fragment;
    if (!append_rv64_materialize_or_move_store_value(fragment,
                                                     value_register,
                                                     stack_layout,
                                                     names,
                                                     lookups,
                                                     block_label,
                                                     instruction_index,
                                                     store.value,
                                                     stack_frame_bytes) ||
        !append_rv64_store_register_to_base(fragment,
                                           value_register,
                                           pointer_base->first,
                                           pointer_base->second,
                                           *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  RiscvEncodedFragment fragment;
  if (!append_rv64_materialize_or_move_store_value(fragment,
                                                   6,
                                                   stack_layout,
                                                   names,
                                                   lookups,
                                                   block_label,
                                                   instruction_index,
                                                   store.value,
                                                   stack_frame_bytes)) {
    return std::nullopt;
  }
  if (!append_rv64_store_register_to_stack(fragment, 6, *offset, *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (rv64_floating_type(load.result.type)) {
    const auto size_bytes = rv64_local_memory_size_for_type(load.result.type);
    if (!size_bytes.has_value()) {
      return std::nullopt;
    }
    const auto destination =
        fpr_register_number_for_value(names, lookups, load.result);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    const auto offset =
        prepared_frame_slot_absolute_offset(stack_layout,
                                            access,
                                            stack_frame_bytes,
                                            *size_bytes);
    RiscvEncodedFragment fragment;
    if (offset.has_value()) {
      if (!append_rv64_load_stack_to_fpr(
              fragment, *destination, *offset, load.result.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto pointer_base =
        prepared_pointer_value_base_offset(lookups, access, *size_bytes);
    if (!pointer_base.has_value() ||
        !append_rv64_load_base_to_fpr(fragment,
                                     *destination,
                                     pointer_base->first,
                                     pointer_base->second,
                                     load.result.type)) {
      return std::nullopt;
    }
    return fragment;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type(load.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto va_start_destination_offset =
      rv64_va_start_destination_load_offset(prepared,
                                            function_name,
                                            block_index,
                                            instruction_index,
                                            load,
                                            access,
                                            stack_frame_bytes,
                                            *size_bytes);
  const auto offset =
      va_start_destination_offset.has_value()
          ? va_start_destination_offset
          : prepared_frame_slot_absolute_offset(stack_layout,
                                                access,
                                                stack_frame_bytes,
                                                *size_bytes);
  if (!offset.has_value()) {
    const auto byval_offset =
        prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                        lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes);
    if (byval_offset.has_value()) {
      const auto destination =
          gpr_register_number_for_value(names, lookups, load.result);
      const auto destination_offset =
          prepared_stack_slot_home_offset_for_value(stack_layout,
                                                    names,
                                                    lookups,
                                                    load.result,
                                                    stack_frame_bytes);
      if (!destination.has_value() && !destination_offset.has_value()) {
        return std::nullopt;
      }
      const std::uint32_t destination_register = destination.value_or(6);
      RiscvEncodedFragment fragment;
      if (!append_rv64_load_stack_to_register(fragment,
                                             destination_register,
                                             *byval_offset,
                                             *size_bytes)) {
        return std::nullopt;
      }
      if (destination_offset.has_value() &&
          !append_rv64_store_register_to_stack(fragment,
                                              destination_register,
                                              *destination_offset,
                                              *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto pointer_base =
        prepared_pointer_value_base_offset(lookups, access, *size_bytes);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    const auto destination = gpr_register_number_for_value(names, lookups, load.result);
    const auto destination_offset =
        prepared_stack_slot_home_offset_for_value(stack_layout,
                                                  names,
                                                  lookups,
                                                  load.result,
                                                  stack_frame_bytes);
    if (!destination.has_value() && !destination_offset.has_value()) {
      return std::nullopt;
    }
    const std::uint32_t destination_register = destination.value_or(6);
    RiscvEncodedFragment fragment;
    if (!append_rv64_load_base_to_register(fragment,
                                          destination_register,
                                          pointer_base->first,
                                          pointer_base->second,
                                          *size_bytes)) {
      return std::nullopt;
    }
    if (destination_offset.has_value() &&
        !append_rv64_store_register_to_stack(fragment,
                                            destination_register,
                                            *destination_offset,
                                            *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto destination = gpr_register_number_for_value(names, lookups, load.result);
  RiscvEncodedFragment fragment;
  if (destination.has_value()) {
    if (!append_rv64_load_stack_to_register(fragment,
                                           *destination,
                                           *offset,
                                           *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto destination_offset =
      prepared_stack_slot_home_offset_for_value(stack_layout,
                                                names,
                                                lookups,
                                                load.result,
                                                stack_frame_bytes);
  if (!destination_offset.has_value() ||
      !append_rv64_load_stack_to_register(fragment, 6, *offset, *size_bytes) ||
      !append_rv64_store_register_to_stack(fragment, 6, *destination_offset)) {
    return std::nullopt;
  }
  return fragment;
}

const c4c::backend::bir::Global* prepared_global_for_access_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess& access) {
  if (!access.address.symbol_name.has_value()) {
    return nullptr;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access.address.symbol_name);
  if (symbol.empty()) {
    return nullptr;
  }
  for (const auto& global : prepared.module.globals) {
    std::string global_label = global.name;
    if (global.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view spelling =
          prepared.module.names.link_names.spelling(global.link_name_id);
      if (!spelling.empty()) {
        global_label = std::string{spelling};
      }
    }
    if (global_label == symbol) {
      return &global;
    }
  }
  return nullptr;
}

bool prepared_global_access_is_supported(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr ||
      access->result_value_name != result_value_name ||
      access->stored_value_name != stored_value_name ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes < size_bytes ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return false;
  }
  const auto policy =
      prepare::prepared_global_symbol_address_policy(access->address,
                                                     &prepared.target_profile);
  if (!policy.has_value() ||
      *policy != bir::GlobalAddressMaterializationPolicy::Direct) {
    return false;
  }
  const auto* global = prepared_global_for_access_symbol(prepared, *access);
  if (global == nullptr ||
      access->address.byte_offset < 0 ||
      static_cast<std::uint64_t>(access->address.byte_offset) > global->size_bytes ||
      size_bytes > global->size_bytes -
                       static_cast<std::uint64_t>(access->address.byte_offset)) {
    return false;
  }
  return true;
}

void append_rv64_zero_extend_register(RiscvEncodedFragment& fragment,
                                      std::uint32_t destination,
                                      std::uint32_t source,
                                      unsigned source_bits) {
  if (source_bits == 64U) {
    append_rv64_move(fragment, destination, source);
    return;
  }
  if (source_bits == 8U) {
    append_le32(fragment.bytes,
                encode_i_type(0x13, destination, 7, source, 0xff));
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            1,
                            source,
                            static_cast<std::int32_t>(64U - source_bits)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            5,
                            destination,
                            static_cast<std::int32_t>(64U - source_bits)));
}

void append_rv64_sign_extend_register(RiscvEncodedFragment& fragment,
                                      std::uint32_t destination,
                                      std::uint32_t source,
                                      unsigned source_bits) {
  if (source_bits == 64U) {
    append_rv64_move(fragment, destination, source);
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            1,
                            source,
                            static_cast<std::int32_t>(64U - source_bits)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            5,
                            destination,
                            static_cast<std::int32_t>(0x400U | (64U - source_bits))));
}

std::optional<std::uint32_t> rv64_int_to_fp_cast_rs2(
    c4c::backend::bir::CastOpcode opcode,
    c4c::backend::bir::TypeKind source_type) {
  switch (source_type) {
    case c4c::backend::bir::TypeKind::I32:
      if (opcode == c4c::backend::bir::CastOpcode::SIToFP) {
        return 0;  // fcvt.{s,d}.w
      }
      if (opcode == c4c::backend::bir::CastOpcode::UIToFP) {
        return 1;  // fcvt.{s,d}.wu
      }
      return std::nullopt;
    case c4c::backend::bir::TypeKind::I64:
      if (opcode == c4c::backend::bir::CastOpcode::SIToFP) {
        return 2;  // fcvt.{s,d}.l
      }
      if (opcode == c4c::backend::bir::CastOpcode::UIToFP) {
        return 3;  // fcvt.{s,d}.lu
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<std::uint32_t> rv64_int_to_fp_cast_funct7(
    c4c::backend::bir::TypeKind result_type) {
  switch (result_type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x68;
    case c4c::backend::bir::TypeKind::F64:
      return 0x69;
    default:
      return std::nullopt;
  }
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_floating_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast) {
  std::optional<std::uint32_t> rs2;
  std::optional<std::uint32_t> funct7;
  if (cast.opcode == c4c::backend::bir::CastOpcode::FPExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::F32 &&
      cast.result.type == c4c::backend::bir::TypeKind::F64) {
    rs2 = 0;
    funct7 = 0x21;  // fcvt.d.s fd, fs, rne
  } else if (cast.opcode == c4c::backend::bir::CastOpcode::FPTrunc &&
             cast.operand.type == c4c::backend::bir::TypeKind::F64 &&
             cast.result.type == c4c::backend::bir::TypeKind::F32) {
    rs2 = 1;
    funct7 = 0x20;  // fcvt.s.d fd, fs, rne
  } else {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  const auto destination =
      destination_home == nullptr ? std::nullopt : fpr_register_number_for_home(*destination_home);
  const auto source =
      source_home == nullptr ? std::nullopt : fpr_register_number_for_home(*source_home);
  if (!destination.has_value() || !source.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_r_type(0x53,
                            *destination,
                            0,
                            *source,
                            *rs2,
                            *funct7));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_int_to_fp_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes) {
  const auto rs2 = rv64_int_to_fp_cast_rs2(cast.opcode, cast.operand.type);
  const auto funct7 = rv64_int_to_fp_cast_funct7(cast.result.type);
  if (!rs2.has_value() || !funct7.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : fpr_register_number_for_home(*destination_home);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  const auto source = gpr_register_number_for_value(names, lookups, cast.operand);
  const std::uint32_t source_register = source.value_or(5);
  RiscvEncodedFragment fragment;
  if (!source.has_value() &&
      !append_rv64_move_value_to_register(fragment,
                                          source_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          cast.operand,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53,
                            *destination,
                            0,
                            source_register,
                            *rs2,
                            *funct7));
  return fragment;
}

bool rv64_prepared_pointer_cast_types_supported(
    c4c::backend::bir::CastOpcode opcode,
    c4c::backend::bir::TypeKind source_type,
    c4c::backend::bir::TypeKind result_type) {
  if (opcode == c4c::backend::bir::CastOpcode::IntToPtr) {
    return result_type == c4c::backend::bir::TypeKind::Ptr &&
           (source_type == c4c::backend::bir::TypeKind::I32 ||
            source_type == c4c::backend::bir::TypeKind::I64);
  }
  if (opcode == c4c::backend::bir::CastOpcode::PtrToInt) {
    return source_type == c4c::backend::bir::TypeKind::Ptr &&
           result_type == c4c::backend::bir::TypeKind::I64;
  }
  return false;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_pointer_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast) {
  if (!rv64_prepared_pointer_cast_types_supported(cast.opcode,
                                                  cast.operand.type,
                                                  cast.result.type)) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  if (destination_home == nullptr ||
      destination_home->kind != prepare::PreparedValueHomeKind::Register) {
    return std::nullopt;
  }
  const auto destination = gpr_register_number_for_home(*destination_home);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (cast.operand.kind == c4c::backend::bir::Value::Kind::Immediate) {
    const auto immediate = integer_immediate_for_value(names, lookups, cast.operand);
    if (!immediate.has_value()) {
      return std::nullopt;
    }
    append_rv64_load_immediate(fragment, *destination, *immediate);
    return fragment;
  }

  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  if (source_home == nullptr) {
    return std::nullopt;
  }
  if (source_home->kind == prepare::PreparedValueHomeKind::RematerializableImmediate) {
    const auto report =
        prepare::verify_prepared_rematerializable_integer_immediate_contract(
            source_home);
    if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
      return std::nullopt;
    }
    const auto fact = prepare::as_rematerializable_integer_immediate_fact(*source_home);
    if (!fact.has_value()) {
      return std::nullopt;
    }
    append_rv64_load_immediate(fragment, *destination, fact->signed_value);
    return fragment;
  }
  if (source_home->kind != prepare::PreparedValueHomeKind::Register) {
    return std::nullopt;
  }
  const auto source = gpr_register_number_for_home(*source_home);
  if (!source.has_value()) {
    return std::nullopt;
  }
  append_rv64_move(fragment, *destination, *source);
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes) {
  if (auto fragment = fragment_for_prepared_floating_cast(names, lookups, cast)) {
    return fragment;
  }
  if (auto fragment = fragment_for_prepared_int_to_fp_cast(stack_layout,
                                                           names,
                                                           lookups,
                                                           cast,
                                                           stack_frame_bytes)) {
    return fragment;
  }
  if (auto fragment = fragment_for_prepared_pointer_cast(names, lookups, cast)) {
    return fragment;
  }

  const auto source_bits = rv64_integer_type_bits(cast.operand.type);
  const auto result_bits = rv64_integer_type_bits(cast.result.type);
  if (!source_bits.has_value() || !result_bits.has_value()) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(cast.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_offset(stack_layout,
                                            *destination_home,
                                            stack_frame_bytes,
                                            *size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }

  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      *result_bits == *source_bits) {
    if (!rv64_fixed_integer_type(cast.operand.type) ||
        !rv64_fixed_integer_type(cast.result.type) ||
        !destination.has_value()) {
      return std::nullopt;
    }
    const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
    const auto source =
        source_home == nullptr ? std::nullopt : gpr_register_number_for_home(*source_home);
    if (!source.has_value()) {
      return std::nullopt;
    }
    RiscvEncodedFragment fragment;
    append_rv64_move(fragment, *destination, *source);
    return fragment;
  }

  const std::uint32_t destination_register = destination.value_or(30);
  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          destination_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          cast.operand,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  switch (cast.opcode) {
    case c4c::backend::bir::CastOpcode::ZExt:
      if (*result_bits <= *source_bits) {
        return std::nullopt;
      }
      append_rv64_zero_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *source_bits);
      break;
    case c4c::backend::bir::CastOpcode::SExt:
      if (*result_bits <= *source_bits) {
        return std::nullopt;
      }
      append_rv64_sign_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *source_bits);
      break;
    case c4c::backend::bir::CastOpcode::Trunc:
      if (*result_bits >= *source_bits) {
        return std::nullopt;
      }
      append_rv64_zero_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *result_bits);
      break;
    case c4c::backend::bir::CastOpcode::Bitcast:
      if (*result_bits != *source_bits) {
        return std::nullopt;
      }
      break;
    default:
      return std::nullopt;
  }
  if (destination_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack(fragment,
                                          destination_register,
                                          *destination_stack_offset,
                                          *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadGlobalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (load.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(load.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName ||
      !prepared_global_access_is_supported(prepared,
                                           access,
                                           result_value_name,
                                           std::nullopt,
                                           *size_bytes)) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access->address.symbol_name);
  const auto destination = gpr_register_number_for_value(names, lookups, load.result);
  const auto destination_offset =
      prepared_stack_slot_home_offset_for_value(stack_layout,
                                                names,
                                                lookups,
                                                load.result,
                                                stack_frame_bytes);
  if (!destination.has_value() && !destination_offset.has_value()) {
    return std::nullopt;
  }
  const std::uint32_t destination_register = destination.value_or(6);
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      destination_register,
      std::string{symbol},
      ".Lpcrel_hi_global_load_" + std::to_string(access->function_name) + "_" +
          std::to_string(access->block_label) + "_" +
          std::to_string(access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  if (!append_rv64_load_global_base_to_register(fragment,
                                               destination_register,
                                               destination_register,
                                               static_cast<std::int32_t>(
                                                   access->address.byte_offset),
                                               *size_bytes)) {
    return std::nullopt;
  }
  if (destination_offset.has_value() &&
      !append_rv64_store_register_to_stack(fragment,
                                          destination_register,
                                          *destination_offset,
                                          *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::StoreGlobalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(store.value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  std::optional<c4c::ValueNameId> stored_value_name;
  if (store.value.kind == c4c::backend::bir::Value::Kind::Named) {
    stored_value_name = names.value_names.find(store.value.name);
    if (*stored_value_name == c4c::kInvalidValueName) {
      return std::nullopt;
    }
  }
  if (!prepared_global_access_is_supported(prepared,
                                           access,
                                           std::nullopt,
                                           stored_value_name,
                                           *size_bytes)) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access->address.symbol_name);
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      5,
      std::string{symbol},
      ".Lpcrel_hi_global_store_" + std::to_string(access->function_name) + "_" +
          std::to_string(access->block_label) + "_" +
          std::to_string(access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  if (!append_rv64_move_value_to_register(fragment,
                                          6,
                                          stack_layout,
                                          names,
                                          lookups,
                                          store.value,
                                          stack_frame_bytes) ||
      !append_rv64_store_register_to_global_base(
          fragment,
          6,
          5,
          static_cast<std::int32_t>(access->address.byte_offset),
          *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_binary(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  if (binary.result.type != c4c::backend::bir::TypeKind::I32 &&
      binary.result.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  const auto result_size_bytes =
      rv64_scalar_memory_size_for_type(binary.result.type);
  if (!result_size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto destination_home = prepared_value_home_for(names, lookups, binary.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_offset(stack_layout,
                                            *destination_home,
                                            stack_frame_bytes,
                                            *result_size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  const std::uint32_t destination_register = destination.value_or(30);

  const auto lhs_register = gpr_register_number_for_value(names, lookups, binary.lhs);
  const auto rhs_register = gpr_register_number_for_value(names, lookups, binary.rhs);
  const auto lhs_immediate = integer_immediate_for_value(names, lookups, binary.lhs);
  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);

  RiscvEncodedFragment fragment;
  auto finish = [&]() -> std::optional<RiscvEncodedFragment> {
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack(fragment,
                                            destination_register,
                                            *destination_stack_offset,
                                            *result_size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  };
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  0,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  0,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  0,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Sub:
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          *rhs_immediate != std::numeric_limits<std::int64_t>::min()) {
        const auto negated = -*rhs_immediate;
        if (fits_signed_12_bit_immediate(negated)) {
          append_le32(fragment.bytes,
                      encode_i_type(0x13,
                                    destination_register,
                                    0,
                                    *lhs_register,
                                    static_cast<std::int32_t>(negated)));
          return finish();
        }
      }
      break;
    case c4c::backend::bir::BinaryOpcode::And:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  7,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  7,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  7,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Xor:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  4,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  4,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  4,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
      if (!append_rv64_move_value_to_register(fragment,
                                             28,
                                             stack_layout,
                                             names,
                                             lookups,
                                             binary.lhs,
                                             stack_frame_bytes) ||
          !append_rv64_move_value_to_register(fragment,
                                             29,
                                             stack_layout,
                                             names,
                                             lookups,
                                             binary.rhs,
                                             stack_frame_bytes)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 4, 28, 29, 0));
      if (binary.opcode == c4c::backend::bir::BinaryOpcode::Eq) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  3,
                                  destination_register,
                                  1));
      } else {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  3,
                                  0,
                                  destination_register,
                                  0));
      }
      return finish();
    default:
      break;
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr &&
      rhs_immediate.has_value()) {
    const std::int64_t shift_width =
        binary.result.type == c4c::backend::bir::TypeKind::I32 ? 32 : 64;
    if (*rhs_immediate < 0 || *rhs_immediate >= shift_width) {
      return std::nullopt;
    }
    if (!append_rv64_move_value_to_register(fragment,
                                            28,
                                            stack_layout,
                                            names,
                                            lookups,
                                            binary.lhs,
                                            stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_i_type(binary.result.type == c4c::backend::bir::TypeKind::I32
                                  ? 0x1b
                                  : 0x13,
                              destination_register,
                              5,
                              28,
                              static_cast<std::int32_t>(0x400 | *rhs_immediate)));
    return finish();
  }
  if (!append_rv64_move_value_to_register(fragment,
                                          28,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.lhs,
                                          stack_frame_bytes) ||
      !append_rv64_move_value_to_register(fragment,
                                          29,
                                          stack_layout,
                                          names,
                                          lookups,
                                          binary.rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sub:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 0x20));
      return finish();
    case c4c::backend::bir::BinaryOpcode::And:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 7, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Or:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 6, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Xor:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 4, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Shl:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 1, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::LShr:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 5, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::AShr:
      append_le32(fragment.bytes,
                  encode_r_type(binary.result.type == c4c::backend::bir::TypeKind::I32
                                    ? 0x3b
                                    : 0x33,
                                destination_register,
                                5,
                                28,
                                29,
                                0x20));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Mul:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::SDiv: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 4, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::UDiv: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 5, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::SRem: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 6, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::URem: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 7, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::Slt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sgt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 29, 28, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sle:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 29, 28, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 28, 29, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ult:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ugt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 29, 28, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ule:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 29, 28, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Uge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 28, 29, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    default:
      return std::nullopt;
  }
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

std::string rv64_prepared_move_bundle_fragment_failure_diagnostic(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedObjectTraversalEvent& event,
    const c4c::backend::prepare::PreparedObjectMoveBundleConsumerClassification&
        classification,
    const c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords*
        dependency_operand_authorities,
    const c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements) {
  std::ostringstream out;
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

  const auto* move_bundle = classification.move_bundle;
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
    out << move_prefix << ".op_kind="
        << rv64_prepared_move_op_kind_name(move.op_kind);
    out << move_prefix << ".reason="
        << (move.reason.empty() ? "<none>" : move.reason);
    if (move.source_immediate_i32.has_value()) {
      out << move_prefix << ".source_imm_i32=" << *move.source_immediate_i32;
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
        const auto classification =
            prepare::classify_prepared_object_select_consumer(&control_flow,
                                                              block.label_id,
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

bool prepared_select_edge_binary_source_has_carrier_alias_authority(
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::prepare::PreparedEdgePublication& publication) {
  if (carrier_alias_authorities == nullptr || !publication.source_value_id.has_value() ||
      !publication.source_producer_block_label.has_value() ||
      !publication.source_producer_instruction_index.has_value()) {
    return false;
  }
  for (const auto& record : carrier_alias_authorities->records) {
    const auto& authority = record.authority;
    if (record.function_name == function_name &&
        prepare::prepared_select_carrier_alias_authority_available(authority) &&
        authority.source_use_closure_proven &&
        !authority.carrier_aliases.empty() &&
        authority.predecessor_label == publication.predecessor_label &&
        authority.successor_label == publication.successor_label &&
        authority.destination_value_id == publication.destination_value_id &&
        authority.destination_value_name == publication.destination_value_name &&
        authority.source_value_id == publication.source_value_id &&
        authority.source_value_name == publication.source_value_name &&
        authority.source_producer_kind == publication.source_producer_kind &&
        authority.source_producer_block_label ==
            publication.source_producer_block_label &&
        authority.source_producer_instruction_index ==
            publication.source_producer_instruction_index) {
      return true;
    }
  }
  return false;
}

bool prepared_select_edge_binary_source_has_authorized_consumers(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::FunctionNameId function_name,
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
  return producer.opcode == c4c::backend::bir::BinaryOpcode::Ule &&
         producer.operand_type == c4c::backend::bir::TypeKind::Ptr &&
         prepared_select_edge_binary_source_has_carrier_alias_authority(
             function_name, carrier_alias_authorities, publication);
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

std::optional<RiscvEncodedFragment>
fragment_for_prepared_select_edge_source_dependencies(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId source_block_label,
    const c4c::backend::bir::BinaryInst& source_binary,
    std::size_t stack_frame_bytes) {
  const auto block_it = std::find_if(
      function.blocks.begin(),
      function.blocks.end(),
      [&](const c4c::backend::bir::Block& block) {
        return block.label_id == source_block_label;
      });
  if (block_it == function.blocks.end()) {
    return std::nullopt;
  }

  std::vector<const c4c::backend::bir::BinaryInst*> candidate_producers;
  for (const auto& inst : block_it->insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr) {
      continue;
    }
    if (binary == &source_binary ||
        prepared_value_names_match(names, binary->result, source_binary.result)) {
      break;
    }
    candidate_producers.push_back(binary);
  }

  std::vector<const c4c::backend::bir::BinaryInst*> emitted_producers;
  RiscvEncodedFragment fragment;
  std::vector<const c4c::backend::bir::BinaryInst*> active_producers;
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
        !emit_producer(find_producer_for_value(producer->rhs))) {
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
  if (!emit_producer(find_producer_for_value(source_binary.lhs)) ||
      !emit_producer(find_producer_for_value(source_binary.rhs))) {
    return std::nullopt;
  }

  const auto operand_ready = [&](const c4c::backend::bir::Value& value) {
    return !prepared_select_edge_source_value_needs_binary_producer(
        names,
        value,
        candidate_producers,
        emitted_producers);
  };
  if (!operand_ready(source_binary.lhs) || !operand_ready(source_binary.rhs)) {
    return std::nullopt;
  }
  return fragment;
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
  if (intent.status != EdgePublicationMoveIntentStatus::Available ||
      intent.publication == nullptr ||
      intent.publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      intent.publication->source_value_id != move.from_value_id ||
      intent.publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) {
    return {};
  }
  const auto* join_transfer = intent.publication->join_transfer;
  const auto* binary = intent.publication->source_binary;
  if (join_transfer == nullptr || binary == nullptr ||
      !prepared_join_transfer_edge_copies_are_published(control_flow,
                                                        *join_transfer) ||
      intent.publication->source_producer_block_label !=
          join_transfer->join_block_label) {
    return {.matched = true};
  }
  if (!c4c::backend::bir::is_compare_opcode(binary->opcode) ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
          names,
          lookups,
          control_flow.function_name,
          dependency_operand_authorities,
          *intent.publication,
          prepare::PreparedDependencyOperandRole::Lhs,
          binary->lhs) ||
      !rv64_select_edge_binary_operand_is_register_immediate_or_cast_authorized(
          names,
          lookups,
          control_flow.function_name,
          dependency_operand_authorities,
          *intent.publication,
          prepare::PreparedDependencyOperandRole::Rhs,
          binary->rhs) ||
      !prepared_select_edge_binary_source_has_authorized_consumers(
          names,
          control_flow,
          function,
          control_flow.function_name,
          carrier_alias_authorities,
          *binary,
          *intent.publication,
          *join_transfer)) {
    return {.matched = true};
  }
  const auto* destination_home = prepared_value_home_for_id(lookups, move.to_value_id);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  if (!destination.has_value() || *destination != destination_register ||
      !prepared_bir_values_have_same_name(names,
                                          intent.publication->destination_value,
                                          join_transfer->result)) {
    return {.matched = true};
  }

  auto edge_binary = *binary;
  edge_binary.result = intent.publication->destination_value;
  auto fragment = fragment_for_prepared_select_edge_source_dependencies(
      stack_layout,
      names,
      function,
      lookups,
      *intent.publication->source_producer_block_label,
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
          *intent.publication,
          edge_binary,
          destination_register,
          stack_frame_bytes);
  if (!edge_fragment.has_value()) {
    if (rv64_select_edge_binary_has_available_cast_dependency_authority(
            names,
            control_flow.function_name,
            dependency_operand_authorities,
            *intent.publication,
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
        publication.source_producer_block_label !=
            publication.join_transfer->join_block_label ||
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

bool prepared_select_is_authorized_carrier_alias(
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords*
        carrier_alias_authorities,
    const c4c::backend::bir::SelectInst& select) {
  if (carrier_alias_authorities == nullptr ||
      select.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto result_name = names.value_names.find(select.result.name);
  if (result_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& record : carrier_alias_authorities->records) {
    if (record.function_name != function_name ||
        !prepare::prepared_select_carrier_alias_authority_available(
            record.authority)) {
      continue;
    }
    for (const auto& alias : record.authority.carrier_aliases) {
      if (alias.carrier_value_name == result_name) {
        return true;
      }
    }
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
    std::size_t stack_frame_bytes) {
  if (select.result.type != c4c::backend::bir::TypeKind::I8 &&
      select.result.type != c4c::backend::bir::TypeKind::I16 &&
      select.result.type != c4c::backend::bir::TypeKind::I32 &&
      select.result.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  const auto normalized =
      normalize_rv64_branch_predicate(select.predicate, select.lhs, select.rhs);
  if (!normalized.has_value()) {
    return std::nullopt;
  }
  const auto funct3 = rv64_branch_funct3(normalized->opcode);
  if (!funct3.has_value()) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(select.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, select.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_offset(stack_layout,
                                            *destination_home,
                                            stack_frame_bytes,
                                            *size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  const std::uint32_t destination_register = destination.value_or(30);
  const std::string true_label =
      rv64_select_local_label(function_name, block_label, instruction_index, "true");
  const std::string end_label =
      rv64_select_local_label(function_name, block_label, instruction_index, "end");

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          28,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->lhs,
                                          stack_frame_bytes) ||
      !append_rv64_move_value_to_register(fragment,
                                          29,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, true_label);
  if (!append_rv64_move_value_to_register(fragment,
                                          destination_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          select.false_value,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_jump(fragment, end_label);
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = fragment.bytes.size(),
      .name = true_label,
  });
  if (!append_rv64_move_value_to_register(fragment,
                                          destination_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          select.true_value,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = fragment.bytes.size(),
      .name = end_label,
  });
  if (destination_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack(fragment,
                                          destination_register,
                                          *destination_stack_offset,
                                          *size_bytes)) {
    return std::nullopt;
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

bool prepared_select_publication_move_is_rv64_object_admitted(
    const EdgePublicationMoveIntent& intent) {
  if (intent.status != EdgePublicationMoveIntentStatus::Available ||
      intent.destination_register.empty() ||
      intent.destination_stack_offset_bytes.has_value() ||
      intent.source_stack_offset_bytes.has_value() ||
      intent.source_memory_byte_offset.has_value() ||
      intent.source_pointer_byte_delta.has_value()) {
    return false;
  }

  const auto destination = rv64_register_number(intent.destination_register);
  if (!destination.has_value()) {
    return false;
  }

  if (intent.source_immediate_i32.has_value()) {
    if (!intent.source_register.empty() ||
        !fits_signed_12_bit_immediate(*intent.source_immediate_i32)) {
      return false;
    }
    return true;
  }

  if (intent.source_register.empty()) {
    return false;
  }
  const auto source = rv64_register_number(intent.source_register);
  if (!source.has_value()) {
    return false;
  }
  return true;
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
  const auto* destination_home =
      prepared_value_home_for_id(lookups, *destination_value_id);
  return destination_home != nullptr &&
         destination_home->kind == prepare::PreparedValueHomeKind::StackSlot;
}

bool prepared_predecessor_select_publication_bundle_is_stack_join_materialized(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      bundle.execution_block_label != bundle.predecessor_label ||
      bundle.has_cycle || bundle.moves.empty() || bundle.steps.size() != bundle.moves.size()) {
    return false;
  }

  for (const auto& step : bundle.steps) {
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
        step.uses_cycle_temp_source) {
      return false;
    }
    const auto* move = prepare::find_prepared_parallel_copy_move_for_step(
        bundle,
        step);
    if (move == nullptr ||
        !prepared_select_publication_destination_is_stack_home(names,
                                                              lookups,
                                                              bundle,
                                                              *move)) {
      return false;
    }
  }
  return true;
}

bool prepared_predecessor_select_publication_bundle_is_rv64_object_admitted(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups* lookups,
    const prepare::PreparedParallelCopyBundle& bundle) {
  if (bundle.execution_site !=
      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return true;
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
    return true;
  }

  for (const auto& step : bundle.steps) {
    if (step.kind != prepare::PreparedParallelCopyStepKind::Move ||
        step.uses_cycle_temp_source) {
      return false;
    }
    const auto* move = prepare::find_prepared_parallel_copy_move_for_step(
        bundle,
        step);
    if (move == nullptr) {
      return false;
    }
    const auto destination_value_id =
        prepared_value_id_for_named_value(names, lookups, move->destination_value);
    if (!destination_value_id.has_value()) {
      if (move->carrier_kind ==
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
        return false;
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
    if (bundle.execution_block_label != bundle.predecessor_label ||
        bundle.has_cycle ||
        move->carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
        intent.publication == nullptr ||
        intent.publication->parallel_copy_bundle != &bundle ||
        intent.publication->carrier_kind !=
            prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
        (!prepared_select_publication_move_is_rv64_object_admitted(intent) &&
         !prepared_select_publication_destination_is_stack_home(names,
                                                               lookups,
                                                               bundle,
                                                               *move))) {
      return false;
    }
  }
  return true;
}

std::optional<std::string> diagnose_unsupported_prepared_param_homes(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
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
    const auto stack_offset = prepared_stack_slot_home_offset(stack_layout,
                                                             *home,
                                                             stack_frame_bytes,
                                                             *size_bytes);
    if (!source_register.has_value() || !stack_offset.has_value() ||
        !append_rv64_store_register_to_stack(fragment,
                                            *source_register,
                                            *stack_offset,
                                            *size_bytes)) {
      return std::nullopt;
    }
  }
  return fragment;
}

const c4c::backend::bir::Function* find_defined_bir_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto it = std::find_if(
      prepared.module.functions.begin(),
      prepared.module.functions.end(),
      [&](const c4c::backend::bir::Function& candidate) {
        return !candidate.is_declaration && candidate.name == function_name;
      });
  return it == prepared.module.functions.end() ? nullptr : &*it;
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

std::optional<RiscvEncodedFragment> fragment_for_prepared_compare_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string true_label,
    std::string false_label,
    std::size_t stack_frame_bytes) {
  const auto normalized = normalize_rv64_branch_predicate(opcode, lhs, rhs);
  if (!normalized.has_value()) {
    return std::nullopt;
  }
  const auto funct3 = rv64_branch_funct3(normalized->opcode);
  if (!funct3.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          28,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->lhs,
                                          stack_frame_bytes) ||
      !append_rv64_move_value_to_register(fragment,
                                          29,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
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

std::optional<RiscvEncodedFragment> fragment_for_prepared_fused_pointer_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::bir::Terminator& terminator,
    std::string true_label,
    std::string false_label,
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
          publication)) {
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
  if (!append_rv64_move_value_to_register(fragment,
                                          28,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->lhs,
                                          stack_frame_bytes) ||
      !append_rv64_move_value_to_register(fragment,
                                          29,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}

bool prepared_compare_feeds_supported_scalar_trunc_publication(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;

  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);
  if (binary.opcode != bir::BinaryOpcode::Sge ||
      binary.result.kind != bir::Value::Kind::Named ||
      binary.result.type != bir::TypeKind::I32 ||
      binary.operand_type != bir::TypeKind::I32 ||
      binary.lhs.type != bir::TypeKind::I32 ||
      binary.rhs.type != bir::TypeKind::I32 ||
      !rhs_immediate.has_value() ||
      !fits_signed_12_bit_immediate(*rhs_immediate)) {
    return false;
  }
  const auto* compare_home = prepared_value_home_for(names, lookups, binary.result);
  if (compare_home == nullptr ||
      !gpr_register_number_for_home(*compare_home).has_value()) {
    return false;
  }

  const bir::CastInst* selected_trunc = nullptr;
  for (std::size_t index = instruction_index + 1; index < block.insts.size(); ++index) {
    const auto* cast = std::get_if<bir::CastInst>(&block.insts[index]);
    if (cast == nullptr || cast->opcode != bir::CastOpcode::Trunc ||
        cast->operand.kind != bir::Value::Kind::Named ||
        cast->operand.name != binary.result.name) {
      continue;
    }
    if (selected_trunc != nullptr) {
      return false;
    }
    selected_trunc = cast;
  }
  if (selected_trunc == nullptr ||
      selected_trunc->operand.type != bir::TypeKind::I32 ||
      selected_trunc->result.type != bir::TypeKind::I16) {
    return false;
  }
  const auto* trunc_home = prepared_value_home_for(names, lookups, selected_trunc->result);
  if (trunc_home == nullptr ||
      gpr_register_number_for_home(*trunc_home).has_value()) {
    return trunc_home != nullptr;
  }
  return prepared_stack_slot_home_offset(stack_layout,
                                         *trunc_home,
                                         stack_frame_bytes,
                                         2)
      .has_value();
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_scalar_compare_trunc_source(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  if (!prepared_compare_feeds_supported_scalar_trunc_publication(stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 block,
                                                                 instruction_index,
                                                                 binary,
                                                                 stack_frame_bytes)) {
    return std::nullopt;
  }
  const auto destination =
      gpr_register_number_for_value(names, lookups, binary.result);
  const auto lhs = gpr_register_number_for_value(names, lookups, binary.lhs);
  const auto rhs = integer_immediate_for_value(names, lookups, binary.rhs);
  if (!destination.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !fits_signed_12_bit_immediate(*rhs)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            *destination,
                            2,
                            *lhs,
                            static_cast<std::int32_t>(*rhs)));
  append_le32(fragment.bytes,
              encode_i_type(0x13, *destination, 4, *destination, 1));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_terminator(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId block_label_id,
    std::size_t block_index,
    std::string_view function_name,
    const std::unordered_map<std::string, PreparedObjectCompare>& compares,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
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
                                                            *branch_condition,
                                                            block.terminator,
                                                            true_asm_label,
                                                            false_asm_label,
                                                            stack_frame_bytes);
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
        return std::nullopt;
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
      if (auto fragment = fragment_for_prepared_frame_address_materialization(
              prepared.stack_layout,
              prepared.names,
              &lookups,
              prepared_block_label,
              instruction_index,
              *binary);
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
                instruction_index,
                *binary,
                stack_frame_bytes);
            fragment.has_value()) {
          return fragment;
        }
      }
      auto fragment = fragment_for_prepared_binary(prepared.stack_layout,
                                                   prepared.names,
                                                   &lookups,
                                                   *binary,
                                                   stack_frame_bytes);
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
            gpr_register_number_for_home(*result_home).has_value()) {
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
                                                     stack_frame_bytes);
        if (fragment.has_value()) {
          return fragment;
        }
      }
    }
    if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
      auto fragment = fragment_for_prepared_cast(prepared.stack_layout,
                                                 prepared.names,
                                                 &lookups,
                                                 *cast,
                                                 stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (c4c::backend::bir::is_vrm_register_type(store->value.type)) {
        return RiscvEncodedFragment{};
      }
      auto fragment = fragment_for_prepared_store_local(
          prepared.stack_layout,
          prepared.names,
          &lookups,
          prepared_block_label,
          instruction_index,
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
          prepared_memory_access_for_instruction(
              &lookups,
              prepared_block_label,
              instruction_index),
          stack_frame_bytes);
      if (fragment.has_value()) {
        return fragment;
      }
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
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
    c4c::BlockLabelId prepared_block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Inst& inst,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  const auto local_memory_diagnostic =
      [&](const std::optional<std::size_t>& size_bytes,
          const prepare::PreparedMemoryAccess* access,
          bool f64_memory = false) -> std::optional<std::string> {
    if (!size_bytes.has_value()) {
      return std::string{
          "unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses"};
    }
    if (f64_memory) {
      if (!prepared_frame_slot_absolute_offset(stack_layout,
                                               access,
                                               stack_frame_bytes,
                                               *size_bytes)
               .has_value() &&
          !prepared_pointer_value_base_offset(&lookups, access, *size_bytes)
               .has_value()) {
        return std::string{
            "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing"};
      }
      return std::nullopt;
    }
    if (!prepared_frame_slot_absolute_offset(stack_layout,
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
             .has_value()) {
      return std::string{
          "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing"};
    }
    return std::nullopt;
  };

  if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst)) {
    const auto size_bytes = rv64_local_memory_size_for_type(store->value.type);
    const auto* access = prepared_memory_access_for_instruction(&lookups,
                                                               prepared_block_label,
                                                               instruction_index);
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
    if (load->address.has_value() &&
        load->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol) {
      return std::string{
          "unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes"};
    }
    return local_memory_diagnostic(
        rv64_local_memory_size_for_type(load->result.type),
        prepared_memory_access_for_instruction(&lookups,
                                               prepared_block_label,
                                               instruction_index),
        rv64_floating_type(load->result.type));
  }
  if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst)) {
    const auto access = prepared_memory_access_for_instruction(&lookups,
                                                               prepared_block_label,
                                                               instruction_index);
    const auto size_bytes = rv64_global_scalar_memory_size_for_type(load->result.type);
    if (!size_bytes.has_value()) {
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
          "unsupported_floating_cast: RV64 object route supports only prepared FPR width casts and I32/I64-to-F32/F64 integer-to-floating casts"};
    }
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr && bir::is_compare_opcode(binary->opcode) &&
      !terminator_uses_value_as_condition(block, binary->result) &&
      !prepared_compare_feeds_supported_scalar_trunc_publication(stack_layout,
                                                                 names,
                                                                 &lookups,
                                                                 block,
                                                                 instruction_index,
                                                                 *binary,
                                                                 stack_frame_bytes) &&
      !fragment_for_prepared_binary(stack_layout,
                                    names,
                                    &lookups,
                                    *binary,
                                    stack_frame_bytes)
           .has_value()) {
    return std::string{
        "unsupported_scalar_compare_publication: RV64 object route requires prepared scalar compare result homes and materializable operands"};
  }
  return std::nullopt;
}

RiscvPreparedObjectFunctionResult prepared_function_to_object_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow) {
  namespace prepare = c4c::backend::prepare;

  const std::string function_name(
      prepare::prepared_function_name(prepared.names, control_flow.function_name));
  if (function_name.empty()) {
    return make_rv64_prepared_function_rejection(
        "unsupported_function_admission: prepared function has no target name");
  }
  const auto* function = find_defined_bir_function(prepared, function_name);
  if (function == nullptr) {
    return make_rv64_prepared_function_rejection(
        "unsupported_function_admission: prepared function has no defined BIR body");
  }
  if (function->is_variadic) {
    if (auto diagnostic = rv64_variadic_function_admission_diagnostic(
            prepared,
            control_flow.function_name)) {
      return make_rv64_prepared_function_rejection(std::move(*diagnostic));
    }
  }
  if (!function->atomic_operations.empty()) {
    return make_rv64_prepared_function_rejection(
        "unsupported_instruction_fragment: atomic operations are not supported by the RV64 object route");
  }
  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto dependency_operand_authorities =
      prepare::collect_prepared_dependency_operand_authorities(prepared);
  const auto carrier_alias_authorities =
      prepare::collect_prepared_select_carrier_alias_authorities(prepared);
  const auto select_edge_source_producer_placements =
      prepare::collect_prepared_select_edge_source_producer_placements(prepared);
  RiscvObjectFunction object_function{
      .name = function_name,
      .global = true,
  };
  const auto* addressing = prepare::find_prepared_addressing(prepared, control_flow.function_name);
  const auto* frame_plan =
      prepare::find_prepared_frame_plan(prepared, control_flow.function_name);
  const auto* inline_asm_carriers =
      prepare::find_prepared_inline_asm_carriers(prepared, control_flow.function_name);
  const auto stack_frame_bytes =
      rv64_object_stack_frame_size(addressing, frame_plan, prepared.stack_layout);
  if (!stack_frame_bytes.has_value()) {
    return make_rv64_prepared_function_rejection(
        "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame");
  }
  if (auto diagnostic =
          diagnose_unsupported_prepared_param_homes(prepared.stack_layout,
                                                   prepared.names,
                                                   &lookups,
                                                   *function,
                                                   *stack_frame_bytes)) {
    return make_rv64_prepared_function_rejection(std::move(*diagnostic));
  }
  if (function->is_variadic) {
    if (auto diagnostic = diagnose_first_unsupported_prepared_variadic_helper(
            prepared,
            control_flow.function_name,
            *function,
            *stack_frame_bytes)) {
      return make_rv64_prepared_function_rejection(std::move(*diagnostic));
    }
  }

  bool has_call = false;
  for (const auto& block : function->blocks) {
    has_call = has_call ||
               std::any_of(block.insts.begin(),
                           block.insts.end(),
                           [](const c4c::backend::bir::Inst& inst) {
                             const auto* call =
                                 std::get_if<c4c::backend::bir::CallInst>(&inst);
                             return call != nullptr &&
                                    !call->inline_asm.has_value() &&
                                    !prepare::prepared_variadic_entry_helper_kind_for_call(
                                         *call)
                                         .has_value();
                           });
  }
  if (has_call) {
    const auto call_frame_size = rv64_call_frame_size(*stack_frame_bytes);
    if (!fits_signed_12_bit_immediate(static_cast<std::int64_t>(call_frame_size)) ||
        !fits_signed_12_bit_immediate(rv64_call_frame_ra_offset(*stack_frame_bytes))) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: call frame exceeds RV64 object-route immediate range");
    }
    auto prologue =
        make_rv64_call_frame_prologue_fragment(frame_plan, *stack_frame_bytes);
    if (!prologue.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved GPR save slots");
    }
    object_function.fragments.push_back(std::move(*prologue));
  } else if (*stack_frame_bytes > 0) {
    auto prologue =
        make_rv64_stack_frame_prologue_fragment(frame_plan, *stack_frame_bytes);
    if (!prologue.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved GPR save slots");
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
              prepare::classify_prepared_object_move_bundle_consumer(event);
          if (auto diagnostic =
                  prepare::diagnose_prepared_object_consumer(classification)) {
            return RiscvPreparedObjectFunctionResult{
                .prepared_consumer_category = diagnostic->category,
                .diagnostic = std::move(diagnostic->message),
            };
          }
          if (classification.parallel_copy_bundle != nullptr &&
              !prepared_predecessor_select_publication_bundle_is_rv64_object_admitted(
                  prepared.names,
                  &lookups,
                  *classification.parallel_copy_bundle)) {
            return make_rv64_prepared_function_rejection(
                "unsupported_move_bundle_target_shape: prepared select publication move bundle requires unsupported RV64 moves");
          }
          if (classification.parallel_copy_bundle != nullptr &&
              prepared_predecessor_select_publication_bundle_is_stack_join_materialized(
                  prepared.names,
                  &lookups,
                  *classification.parallel_copy_bundle)) {
            object_function.fragments.push_back(RiscvEncodedFragment{});
            break;
          }
          auto fragment =
              fragment_for_prepared_move_bundle(prepared.target_profile,
                                                prepared.stack_layout,
                                                prepared.names,
                                                control_flow,
                                                *function,
                                                &lookups,
                                                &dependency_operand_authorities,
                                                &carrier_alias_authorities,
                                                &select_edge_source_producer_placements,
                                                *stack_frame_bytes,
                                                *classification.move_bundle);
          if (!fragment.has_value()) {
            return make_rv64_prepared_function_rejection(
                rv64_prepared_move_bundle_fragment_failure_diagnostic(
                    prepared.names,
                    control_flow,
                    *function,
                    event,
                    classification,
                    &dependency_operand_authorities,
                    &select_edge_source_producer_placements));
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
                    prepared_block_label,
                    event.instruction_index,
                    *block,
                    *event.instruction,
                    *stack_frame_bytes)) {
              return make_rv64_prepared_function_rejection(std::move(*diagnostic));
            }
            return make_rv64_prepared_function_rejection(
                "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
          }
          object_function.fragments.push_back(std::move(*fragment));
          break;
        }
        case prepare::PreparedObjectTraversalEventKind::Terminator: {
          auto terminator_fragment =
              fragment_for_prepared_terminator(prepared,
                                               control_flow,
                                               prepared.names,
                                               &lookups,
                                               *block,
                                               prepared_block_label,
                                               event.block_index,
                                               function_name,
                                               compares,
                                               frame_plan,
                                               has_call,
                                               *stack_frame_bytes);
          if (!terminator_fragment.has_value()) {
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
                    prepared_block_label,
                    instruction_index,
                    block,
                    block.insts[instruction_index],
                    *stack_frame_bytes)) {
          return make_rv64_prepared_function_rejection(std::move(*diagnostic));
        }
        return make_rv64_prepared_function_rejection(
            "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
      }
      object_function.fragments.push_back(std::move(*fragment));
    }

    auto terminator_fragment =
        fragment_for_prepared_terminator(prepared,
                                         control_flow,
                                         prepared.names,
                                         &lookups,
                                         block,
                                         prepared_block_label,
                                         block_index,
                                         function_name,
                                         compares,
                                         frame_plan,
                                         has_call,
                                         *stack_frame_bytes);
    if (!terminator_fragment.has_value()) {
      return make_rv64_prepared_function_rejection(
          "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering");
    }
    object_function.fragments.push_back(std::move(*terminator_fragment));
  }
  return RiscvPreparedObjectFunctionResult{
      .function = std::move(object_function),
  };
}

std::optional<prepare::PreparedObjectConsumerDiagnostic>
diagnose_unplaced_parallel_copy_obligations(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow) {
  const auto obligations =
      prepare::collect_unplaced_prepared_object_parallel_copy_obligations(
          control_flow);
  for (const auto& obligation : obligations) {
    auto diagnostic = prepare::diagnose_prepared_object_consumer(obligation);
    if (diagnostic.has_value()) {
      return diagnostic;
    }
  }
  return std::nullopt;
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
  append_le32(fragment.bytes,
              encode_u_type(0x17, destination_register, 0));  // auipc rd, 0
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination_register,
                            0,
                            destination_register,
                            0));  // addi rd, rd, 0
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
        if (label.name.empty() || label.offset_bytes > fragment.bytes.size() ||
            symbols_by_name.find(label.name) != symbols_by_name.end()) {
          return std::nullopt;
        }
        const auto label_offset = fragment_offset + label.offset_bytes;
        object::bind_label(module, label.name, text.id, label_offset);
        auto& label_symbol = object::define_symbol(module,
                                                   label.name,
                                                   object::SymbolBinding::Local,
                                                   object::SymbolKind::NoType,
                                                   text.id,
                                                   label_offset,
                                                   0);
        symbols_by_name.emplace(label_symbol.name, label_symbol.id);
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
        const auto reloc_type = rv64_elf_relocation_type(fixup.kind);
        if (!reloc_type.has_value() || fixup.symbol_name.empty() ||
            fixup.offset_bytes > fragment.bytes.size()) {
          return std::nullopt;
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
                                  text.id,
                                  laid_out.section_offset + fixup.offset_bytes,
                                  *reloc_type,
                                  target_symbol,
                                  fixup.addend);
      }
    }
  }

  return module;
}

std::string rv64_prepared_object_global_label(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Global& global) {
  if (global.link_name_id != c4c::kInvalidLinkName) {
    const std::string_view spelling =
        module.names.link_names.spelling(global.link_name_id);
    if (!spelling.empty()) {
      return std::string{spelling};
    }
  }
  return global.name;
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
  return status ==
             prepare::PreparedSelectedObjectDataContractStatus::
                 UnsupportedButCoherent &&
         !global.is_extern &&
         !global.is_thread_local &&
         !global.is_constant &&
         (has_no_initializer || has_zero_pointer_initializer) &&
         object_data.object_label == global.link_name_id &&
         object_data.object_byte_offset == 0 &&
         object_data.object_size_bytes != 0 &&
         object_data.align_bytes != 0 &&
         object_data.emitted_bytes.empty() &&
         object_data.zero_fill_byte_count == 0;
}

std::optional<std::string> rv64_selected_symbol_pointer_initializer_label(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& global,
    const prepare::PreparedGlobalObjectData& object_data,
    prepare::PreparedSelectedObjectDataContractStatus status) {
  if (status !=
          prepare::PreparedSelectedObjectDataContractStatus::
              UnsupportedButCoherent ||
      global.is_extern ||
      global.is_thread_local ||
      global.is_constant ||
      global.type != c4c::backend::bir::TypeKind::Ptr ||
      global.size_bytes != 8 ||
      global.align_bytes < 8 ||
      !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Named ||
      global.initializer->type != c4c::backend::bir::TypeKind::Ptr ||
      global.initializer->pointer_symbol_link_name_id == c4c::kInvalidLinkName ||
      global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != c4c::kInvalidLinkName ||
      !global.initializer_elements.empty() ||
      object_data.object_label != global.link_name_id ||
      object_data.object_byte_offset != 0 ||
      object_data.object_size_bytes != 8 ||
      object_data.align_bytes < 8 ||
      !object_data.emitted_bytes.empty() ||
      object_data.zero_fill_byte_count != 0) {
    return std::nullopt;
  }
  auto label = rv64_prepared_link_name_label(
      prepared, global.initializer->pointer_symbol_link_name_id);
  if (label.empty()) {
    return std::nullopt;
  }
  return label;
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

    const auto* object_data = prepare::find_prepared_global_object_data(
        prepared.object_data, global.link_name_id);
    const auto facts = rv64_selected_object_data_contract_facts(object_data);
    const auto status =
        prepare::classify_prepared_selected_object_data_contract(facts);
    const auto report =
        prepare::verify_prepared_selected_object_data_contract(facts);
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
    }

    const auto label = rv64_prepared_object_data_label(*object_data);
    if (label.empty()) {
      return "unsupported_global_data: RV64 object route cannot emit unnamed prepared global";
    }

    const auto* existing = object::find_symbol(object_module, label);
    if (object_data->align_bytes == 0 ||
        (existing != nullptr && !object::is_undefined_symbol(*existing))) {
      return "unsupported_global_data: RV64 object route cannot emit prepared global symbol";
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

    object::align_section(*section, object_data->align_bytes, 0);
    const auto symbol_pointer_label =
        rv64_selected_symbol_pointer_initializer_label(
            prepared, global, *object_data, status);
    std::uint64_t offset = 0;
    if (selected_zero_fill) {
      offset = object::reserve_section_bytes(*section,
                                             object_data->object_size_bytes);
    } else if (symbol_pointer_label.has_value()) {
      std::vector<std::uint8_t> bytes;
      append_le64(bytes, 0);
      offset = object::append_section_bytes(*section, bytes);
      const auto target_symbol = rv64_find_or_declare_relocation_symbol(
          object_module,
          *symbol_pointer_label,
          rv64_prepared_link_symbol_kind(
              prepared, global.initializer->pointer_symbol_link_name_id));
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
                          object::SymbolBinding::Global,
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
    if (auto diagnostic =
            diagnose_unplaced_parallel_copy_obligations(control_flow)) {
      return RiscvPreparedObjectModuleResult{
          .prepared_consumer_category = diagnostic->category,
          .diagnostic = std::move(diagnostic->message),
      };
    }

    const std::string_view function_name =
        c4c::backend::prepare::prepared_function_name(prepared.names,
                                                      control_flow.function_name);
    if (function_name.empty()) {
      return make_rv64_prepared_module_rejection(
          "unsupported_function_admission: prepared function has no target name");
    }
    if (find_defined_bir_function(prepared, function_name) == nullptr) {
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
