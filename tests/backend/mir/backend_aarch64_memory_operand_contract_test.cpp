#include "src/backend/mir/aarch64/codegen/instruction.hpp"
#include "src/backend/prealloc/pointer_value_memory_freshness.hpp"

#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedMemoryAccess make_pointer_value_memory_access(bool store = false) {
  auto range = bir::make_memory_byte_range(8, 4);
  return prepare::PreparedMemoryAccess{
      .function_name = c4c::FunctionNameId{100},
      .block_label = c4c::BlockLabelId{200},
      .inst_index = 3,
      .result_value_name = store ? std::optional<c4c::ValueNameId>{}
                                 : std::optional<c4c::ValueNameId>{c4c::ValueNameId{301}},
      .stored_value_name = store ? std::optional<c4c::ValueNameId>{c4c::ValueNameId{302}}
                                 : std::optional<c4c::ValueNameId>{},
      .address_space = bir::AddressSpace::Tls,
      .is_volatile = true,
      .address =
          prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
              .pointer_value_name = c4c::ValueNameId{300},
              .byte_offset = 8,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
              .provenance =
                  bir::MemoryAccessProvenance{
                      .base_identity =
                          bir::MemoryProvenanceBaseIdentity{
                              .kind = bir::MemoryProvenanceBaseIdentityKind::LocalSlot,
                              .slot_name_id = c4c::SlotNameId{700},
                          },
                      .object_extent =
                          bir::MemoryObjectExtent{
                              .completeness =
                                  bir::MemoryObjectExtentCompleteness::Complete,
                              .size_bytes = 64,
                              .size_known = true,
                          },
                      .requested_range = range,
                      .layout_authority =
                          bir::MemoryLayoutAuthorityKind::StructuredLayout,
                      .range_verdict = bir::MemoryRangeVerdict::ProvenInBounds,
                  },
          },
  };
}

prepare::PreparedPointerValueMemoryFreshnessQuery make_pointer_value_query(
    const prepare::PreparedMemoryAccess& access,
    std::vector<prepare::PreparedPointerValueMemoryFreshnessAuthority> candidates) {
  return prepare::PreparedPointerValueMemoryFreshnessQuery{
      .access = &access,
      .pointer_value_id = prepare::PreparedValueId{900},
      .pointer_value_name = c4c::ValueNameId{300},
      .use_mode = prepare::prepared_pointer_value_memory_use_mode(access),
      .candidates = std::move(candidates),
  };
}

int expect_pointer_value_query_status(
    const char* label,
    const prepare::PreparedPointerValueMemoryFreshnessQuery& query,
    prepare::PreparedValueFreshnessQueryStatus expected) {
  const auto result =
      prepare::find_prepared_pointer_value_memory_freshness_authority(query);
  if (result.status != expected) {
    std::cerr << "expected " << label << " to return "
              << prepare::prepared_value_freshness_query_status_name(expected)
              << ", got "
              << prepare::prepared_value_freshness_query_status_name(result.status)
              << "\n";
    return 1;
  }
  if (expected == prepare::PreparedValueFreshnessQueryStatus::Selected &&
      result.authority == nullptr) {
    std::cerr << "expected " << label << " to return selected authority\n";
    return 1;
  }
  if (expected != prepare::PreparedValueFreshnessQueryStatus::Selected &&
      prepare::prepared_pointer_value_memory_freshness_available(query)) {
    std::cerr << "expected " << label << " to remain unavailable\n";
    return 1;
  }
  return 0;
}

int supported_memory_base_contract_is_explicit() {
  const aarch64_codegen::MemoryOperand frame{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{10},
      .block_label = c4c::BlockLabelId{20},
      .instruction_index = 1,
      .result_value_id = prepare::PreparedValueId{30},
      .result_value_name = c4c::ValueNameId{40},
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{50},
      .byte_offset = 8,
      .size_bytes = 4,
      .align_bytes = 4,
      .address_space = bir::AddressSpace::Fs,
      .is_volatile = true,
      .can_use_base_plus_offset = true,
  };
  const aarch64_codegen::MemoryOperand symbol{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{10},
      .block_label = c4c::BlockLabelId{20},
      .instruction_index = 2,
      .stored_value_id = prepare::PreparedValueId{31},
      .stored_value_name = c4c::ValueNameId{41},
      .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
      .symbol_name = c4c::LinkNameId{51},
      .byte_offset = -16,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Gs,
  };
  const aarch64_codegen::MemoryOperand pointer{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{10},
      .block_label = c4c::BlockLabelId{20},
      .instruction_index = 3,
      .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
      .pointer_value_name = c4c::ValueNameId{42},
      .pointer_value_id = prepare::PreparedValueId{32},
      .byte_offset = 24,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Tls,
  };
  const aarch64_codegen::MemoryOperand string{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{10},
      .block_label = c4c::BlockLabelId{20},
      .instruction_index = 4,
      .base_kind = aarch64_codegen::MemoryBaseKind::StringConstant,
      .string_name = c4c::TextId{52},
      .string_symbol_name = c4c::LinkNameId{53},
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
  };

  if (frame.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      symbol.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      pointer.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      string.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly) {
    return fail("expected all memory operands to stay record-only");
  }
  if (aarch64_codegen::memory_base_kind_name(frame.base_kind) != "frame_slot" ||
      aarch64_codegen::memory_base_kind_name(symbol.base_kind) != "symbol" ||
      aarch64_codegen::memory_base_kind_name(pointer.base_kind) != "pointer_value" ||
      aarch64_codegen::memory_base_kind_name(string.base_kind) != "string_constant") {
    return fail("expected supported memory base vocabulary to be explicit");
  }
  if (frame.frame_slot_id != prepare::PreparedFrameSlotId{50} ||
      symbol.symbol_name != c4c::LinkNameId{51} ||
      pointer.pointer_value_name != c4c::ValueNameId{42} ||
      pointer.pointer_value_id != prepare::PreparedValueId{32} ||
      string.string_name != c4c::TextId{52} ||
      string.string_symbol_name != c4c::LinkNameId{53}) {
    return fail("expected memory bases to preserve structured identities");
  }
  if (frame.address_space != bir::AddressSpace::Fs || !frame.is_volatile ||
      symbol.address_space != bir::AddressSpace::Gs || symbol.is_volatile ||
      pointer.address_space != bir::AddressSpace::Tls || pointer.is_volatile ||
      string.address_space != bir::AddressSpace::Default || string.is_volatile) {
    return fail("expected memory operands to preserve volatility and address space exactly");
  }
  return 0;
}

int unsupported_and_fail_closed_contract_is_explicit() {
  const aarch64_codegen::MemoryOperand deferred{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported,
      .base_kind = aarch64_codegen::MemoryBaseKind::None,
  };

  if (aarch64_codegen::memory_operand_support_kind_name(deferred.support) !=
          "deferred_unsupported" ||
      aarch64_codegen::memory_base_kind_name(deferred.base_kind) != "none") {
    return fail("expected unsupported memory forms to defer explicitly");
  }
  if (aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess) !=
          "missing_prepared_memory_access" ||
      aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::UnsupportedBase) !=
          "unsupported_base" ||
      aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::AddressFactMismatch) !=
          "address_fact_mismatch" ||
      aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingPointerValueHome) !=
          "missing_pointer_value_home" ||
      aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::AmbiguousPointerValueHome) !=
          "ambiguous_pointer_value_home" ||
      aarch64_codegen::prepared_memory_operand_record_error_name(
          aarch64_codegen::PreparedMemoryOperandRecordError::StringIdentityMismatch) !=
          "string_identity_mismatch") {
    return fail("expected prepared memory conversion failures to be named explicitly");
  }
  return 0;
}

int memory_instruction_records_do_not_select_or_emit() {
  const auto memory_instruction = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{11},
                  .block_label = c4c::BlockLabelId{21},
                  .instruction_index = 5,
                  .result_value_id = prepare::PreparedValueId{34},
                  .result_value_name = c4c::ValueNameId{44},
                  .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
                  .pointer_value_name = c4c::ValueNameId{43},
                  .pointer_value_id = prepare::PreparedValueId{33},
                  .byte_offset = 32,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .address_space = bir::AddressSpace::Gs,
                  .is_volatile = true,
              },
          .result_value_id = prepare::PreparedValueId{34},
          .result_value_name = c4c::ValueNameId{44},
          .value_type = bir::TypeKind::I32,
      });

  const auto* payload =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&memory_instruction.payload);
  if (memory_instruction.family != aarch64_codegen::InstructionFamily::Memory ||
      memory_instruction.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      payload == nullptr ||
      aarch64_codegen::memory_instruction_kind_name(payload->memory_kind) != "load" ||
      payload->address.pointer_value_id != prepare::PreparedValueId{33} ||
      payload->address.address_space != bir::AddressSpace::Gs ||
      !payload->address.is_volatile ||
      payload->result_value_id != prepare::PreparedValueId{34} ||
      payload->result_value_name != c4c::ValueNameId{44} ||
      payload->value_type != bir::TypeKind::I32) {
    return fail("expected memory instruction records to preserve load metadata only");
  }
  if (std::holds_alternative<aarch64_codegen::BranchInstructionRecord>(
          memory_instruction.payload) ||
      std::holds_alternative<aarch64_codegen::ScalarInstructionRecord>(
          memory_instruction.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(memory_instruction.payload) ||
      std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          memory_instruction.payload) ||
      std::holds_alternative<aarch64_codegen::AssemblerInstructionRecord>(
          memory_instruction.payload) ||
      std::holds_alternative<aarch64_codegen::ObjectInstructionRecord>(
          memory_instruction.payload)) {
    return fail("expected memory records not to own branch, scalar, call, return, asm, or object payloads");
  }
  return 0;
}

int memory_and_spill_nodes_fail_closed_when_required_facts_are_missing() {
  const auto unsupported_memory = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported,
                  .function_name = c4c::FunctionNameId{12},
                  .block_label = c4c::BlockLabelId{22},
                  .instruction_index = 6,
                  .base_kind = aarch64_codegen::MemoryBaseKind::None,
              },
          .result_value_id = prepare::PreparedValueId{35},
          .result_value_name = c4c::ValueNameId{45},
          .value_type = bir::TypeKind::I32,
      });
  if (unsupported_memory.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported ||
      unsupported_memory.opcode != aarch64_codegen::MachineOpcode::Load ||
      unsupported_memory.selection.diagnostic !=
          "memory operand is outside the selected subset") {
    return fail("expected unsupported memory operands to fail closed without guessing");
  }
  const auto missing_symbol_identity = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{12},
                  .block_label = c4c::BlockLabelId{22},
                  .instruction_index = 8,
                  .result_value_id = prepare::PreparedValueId{35},
                  .result_value_name = c4c::ValueNameId{45},
                  .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
                  .symbol_name = c4c::LinkNameId{55},
              },
          .result_value_id = prepare::PreparedValueId{35},
          .result_value_name = c4c::ValueNameId{45},
          .value_type = bir::TypeKind::I32,
      });
  if (missing_symbol_identity.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      missing_symbol_identity.selection.diagnostic !=
          "symbol memory node is missing symbol identity") {
    return fail("expected symbol memory without printable identity to fail closed");
  }

  const auto scalar_symbol_memory = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{12},
                  .block_label = c4c::BlockLabelId{22},
                  .instruction_index = 9,
                  .result_value_id = prepare::PreparedValueId{35},
                  .result_value_name = c4c::ValueNameId{45},
                  .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
                  .symbol_name = c4c::LinkNameId{55},
                  .symbol_label = "g.contract",
                  .can_use_base_plus_offset = true,
              },
          .result_value_id = prepare::PreparedValueId{35},
          .result_value_name = c4c::ValueNameId{45},
          .value_type = bir::TypeKind::I32,
      });
  if (scalar_symbol_memory.selection.status !=
      aarch64_codegen::MachineNodeSelectionStatus::Selected) {
    return fail("expected scalar symbol-backed memory with structured identity to select");
  }

  const auto incomplete_spill = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{36},
          .value_name = c4c::ValueNameId{46},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Spill,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::SpillToSlot,
          .slot =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{12},
                  .block_label = c4c::BlockLabelId{22},
                  .instruction_index = 7,
                  .stored_value_id = prepare::PreparedValueId{36},
                  .stored_value_name = c4c::ValueNameId{46},
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{56},
                  .byte_offset = 40,
                  .byte_offset_is_prepared_snapshot = true,
              },
          .slot_id = prepare::PreparedFrameSlotId{56},
          .stack_offset_bytes = std::size_t{40},
          .stack_offset_is_prepared_snapshot = true,
      });
  if (incomplete_spill.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      incomplete_spill.opcode != aarch64_codegen::MachineOpcode::SpillToSlot ||
      incomplete_spill.pseudo != aarch64_codegen::MachinePseudoKind::SpillToSlot ||
      incomplete_spill.selection.diagnostic !=
          "spill/reload node is missing slot or scratch facts") {
    return fail("expected incomplete spill pseudo nodes to fail closed structurally");
  }
  return 0;
}

int pointer_value_memory_freshness_requires_selected_authority() {
  const auto access = make_pointer_value_memory_access();
  if (!prepare::prepared_pointer_value_memory_has_proven_authority(access.address)) {
    return fail("expected pointer-value memory access support facts to be complete");
  }

  const auto authority =
      prepare::make_prepared_pointer_value_memory_freshness_authority(
          access,
          prepare::PreparedValueId{900},
          prepare::PreparedPointerValueMemoryUseMode::Load);
  const auto selected = make_pointer_value_query(access, {authority});
  if (const int status = expect_pointer_value_query_status(
          "exact pointer-value memory freshness",
          selected,
          prepare::PreparedValueFreshnessQueryStatus::Selected);
      status != 0) {
    return status;
  }

  const auto no_candidate = make_pointer_value_query(access, {});
  if (const int status = expect_pointer_value_query_status(
          "support-only pointer-value memory facts",
          no_candidate,
          prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
      status != 0) {
    return status;
  }

  const auto ambiguous = make_pointer_value_query(access, {authority, authority});
  if (const int status = expect_pointer_value_query_status(
          "ambiguous pointer-value memory freshness",
          ambiguous,
          prepare::PreparedValueFreshnessQueryStatus::AmbiguousCandidate);
      status != 0) {
    return status;
  }

  auto missing_pointer_name_access = access;
  missing_pointer_name_access.address.pointer_value_name.reset();
  if (const int status = expect_pointer_value_query_status(
          "missing pointer-value identity",
          make_pointer_value_query(missing_pointer_name_access, {authority}),
          prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
      status != 0) {
    return status;
  }
  return 0;
}

int pointer_value_memory_freshness_rejects_wrong_candidate_dimensions() {
  const auto access = make_pointer_value_memory_access();
  const auto authority =
      prepare::make_prepared_pointer_value_memory_freshness_authority(
          access,
          prepare::PreparedValueId{900},
          prepare::PreparedPointerValueMemoryUseMode::Load);

  auto expect_rejected = [&](const char* label,
                             prepare::PreparedPointerValueMemoryFreshnessAuthority
                                 candidate) -> int {
    return expect_pointer_value_query_status(
        label,
        make_pointer_value_query(access, {candidate}),
        prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
  };

  {
    auto candidate = authority;
    candidate.function_name = c4c::FunctionNameId{101};
    if (const int status = expect_rejected("stale function point", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.block_label = c4c::BlockLabelId{201};
    candidate.freshness.reference.block_label = c4c::BlockLabelId{201};
    if (const int status = expect_rejected("stale block point", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.inst_index = 4;
    candidate.freshness.reference.instruction_index = 4;
    if (const int status = expect_rejected("stale instruction point", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.freshness.value_id = prepare::PreparedValueId{901};
    if (const int status = expect_rejected("wrong pointer value id", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.freshness.value_name = c4c::ValueNameId{301};
    if (const int status = expect_rejected("wrong pointer value name", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.use_mode = prepare::PreparedPointerValueMemoryUseMode::Store;
    if (const int status = expect_rejected("wrong load/store use mode", candidate);
        status != 0) {
      return status;
    }
  }
  return 0;
}

int pointer_value_memory_freshness_rejects_wrong_vocabulary() {
  const auto access = make_pointer_value_memory_access();
  const auto authority =
      prepare::make_prepared_pointer_value_memory_freshness_authority(
          access,
          prepare::PreparedValueId{900},
          prepare::PreparedPointerValueMemoryUseMode::Load);

  auto expect_rejected = [&](const char* label,
                             prepare::PreparedPointerValueMemoryFreshnessAuthority
                                 candidate) -> int {
    return expect_pointer_value_query_status(
        label,
        make_pointer_value_query(access, {candidate}),
        prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
  };

  {
    auto candidate = authority;
    candidate.freshness.use_kind =
        prepare::PreparedValueFreshnessUseKind::PointerBasePlusOffsetSource;
    if (const int status = expect_rejected("wrong freshness use vocabulary", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.freshness.source_kind =
        prepare::PreparedValueFreshnessSourceKind::PointerBasePlusOffset;
    if (const int status = expect_rejected("wrong freshness source vocabulary", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.freshness.proof_kind =
        prepare::PreparedValueFreshnessProofKind::PointerBasePlusOffsetAuthority;
    if (const int status = expect_rejected("wrong freshness proof vocabulary", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.freshness.rank = prepare::PreparedValueFreshnessSourceRank::PointerBasePlusOffset;
    if (const int status = expect_rejected("wrong freshness rank vocabulary", candidate);
        status != 0) {
      return status;
    }
  }
  return 0;
}

int pointer_value_memory_freshness_rejects_support_only_mutations() {
  const auto access = make_pointer_value_memory_access();
  const auto authority =
      prepare::make_prepared_pointer_value_memory_freshness_authority(
          access,
          prepare::PreparedValueId{900},
          prepare::PreparedPointerValueMemoryUseMode::Load);

  auto expect_rejected = [&](const char* label,
                             prepare::PreparedPointerValueMemoryFreshnessAuthority
                                 candidate) -> int {
    return expect_pointer_value_query_status(
        label,
        make_pointer_value_query(access, {candidate}),
        prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
  };

  {
    auto candidate = authority;
    candidate.byte_offset = 12;
    candidate.requested_range = bir::make_memory_byte_range(12, 4);
    if (const int status = expect_rejected("wrong offset and requested range", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.size_bytes = 8;
    candidate.requested_range = bir::make_memory_byte_range(8, 8);
    if (const int status = expect_rejected("wrong memory range size", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.provenance_base_kind =
        bir::MemoryProvenanceBaseIdentityKind::FormalParameter;
    if (const int status = expect_rejected("wrong provenance base kind", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.provenance_base_slot_name = c4c::SlotNameId{701};
    if (const int status = expect_rejected("wrong provenance base identity", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.layout_authority = bir::MemoryLayoutAuthorityKind::ScalarLayout;
    if (const int status = expect_rejected("wrong layout authority", candidate);
        status != 0) {
      return status;
    }
  }
  {
    auto candidate = authority;
    candidate.can_use_base_plus_offset = false;
    if (const int status = expect_rejected("wrong target memory operand shape", candidate);
        status != 0) {
      return status;
    }
  }

  auto target_shape_only_access = access;
  target_shape_only_access.address.provenance.layout_authority =
      bir::MemoryLayoutAuthorityKind::ScalarLayout;
  if (!prepare::prepared_pointer_value_memory_has_proven_authority(
          target_shape_only_access.address)) {
    return fail("expected alternate support facts to remain address-authoritative");
  }
  if (const int status = expect_pointer_value_query_status(
          "range-layout-target support without freshness",
          make_pointer_value_query(target_shape_only_access, {}),
          prepare::PreparedValueFreshnessQueryStatus::NoCandidate);
      status != 0) {
    return status;
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = supported_memory_base_contract_is_explicit(); status != 0) {
    return status;
  }
  if (const int status = unsupported_and_fail_closed_contract_is_explicit(); status != 0) {
    return status;
  }
  if (const int status = memory_instruction_records_do_not_select_or_emit(); status != 0) {
    return status;
  }
  if (const int status = memory_and_spill_nodes_fail_closed_when_required_facts_are_missing();
      status != 0) {
    return status;
  }
  if (const int status = pointer_value_memory_freshness_requires_selected_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          pointer_value_memory_freshness_rejects_wrong_candidate_dimensions();
      status != 0) {
    return status;
  }
  if (const int status = pointer_value_memory_freshness_rejects_wrong_vocabulary();
      status != 0) {
    return status;
  }
  if (const int status = pointer_value_memory_freshness_rejects_support_only_mutations();
      status != 0) {
    return status;
  }
  return 0;
}
