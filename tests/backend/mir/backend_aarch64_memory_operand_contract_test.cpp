#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <variant>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
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
  const auto global_memory = aarch64_codegen::make_memory_instruction(
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
  if (global_memory.selection.status !=
      aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported) {
    return fail("expected global/linker memory forms to remain fail-closed in this slice");
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
  return 0;
}
