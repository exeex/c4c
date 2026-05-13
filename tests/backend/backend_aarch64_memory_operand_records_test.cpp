#include "src/backend/mir/aarch64/codegen/records.hpp"

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

int frame_slot_memory_records_preserve_prepared_access_identity() {
  const auto operand = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{10},
          .block_label = c4c::BlockLabelId{20},
          .instruction_index = 3,
          .result_value_id = prepare::PreparedValueId{30},
          .result_value_name = c4c::ValueNameId{40},
          .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
          .frame_slot_id = prepare::PreparedFrameSlotId{50},
          .byte_offset = 12,
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Fs,
          .is_volatile = true,
          .can_use_base_plus_offset = true,
      });

  const auto* memory = std::get_if<aarch64_codegen::MemoryOperand>(&operand.payload);
  if (operand.kind != aarch64_codegen::OperandKind::Memory || memory == nullptr) {
    return fail("expected memory operand payload");
  }
  if (memory->surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      aarch64_codegen::memory_operand_support_kind_name(memory->support) != "prepared" ||
      memory->function_name != c4c::FunctionNameId{10} ||
      memory->block_label != c4c::BlockLabelId{20} || memory->instruction_index != 3 ||
      memory->result_value_id != prepare::PreparedValueId{30} ||
      memory->result_value_name != c4c::ValueNameId{40} ||
      memory->stored_value_id.has_value() || memory->stored_value_name.has_value()) {
    return fail("expected memory record to preserve prepared access identity");
  }
  if (memory->base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      aarch64_codegen::memory_base_kind_name(memory->base_kind) != "frame_slot" ||
      memory->frame_slot_id != prepare::PreparedFrameSlotId{50} ||
      memory->byte_offset != 12 || memory->size_bytes != 4 || memory->align_bytes != 4 ||
      memory->address_space != bir::AddressSpace::Fs || !memory->is_volatile ||
      !memory->can_use_base_plus_offset) {
    return fail("expected frame-slot memory record to preserve address facts");
  }
  return 0;
}

int symbol_and_string_memory_records_keep_structured_identity() {
  const auto symbol = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{11},
          .block_label = c4c::BlockLabelId{21},
          .instruction_index = 4,
          .stored_value_id = prepare::PreparedValueId{31},
          .stored_value_name = c4c::ValueNameId{41},
          .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
          .symbol_name = c4c::LinkNameId{51},
          .byte_offset = -8,
          .size_bytes = 8,
          .align_bytes = 8,
          .address_space = bir::AddressSpace::Gs,
          .can_use_base_plus_offset = true,
      });
  const auto string = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{12},
          .block_label = c4c::BlockLabelId{22},
          .instruction_index = 5,
          .result_value_id = prepare::PreparedValueId{32},
          .result_value_name = c4c::ValueNameId{42},
          .base_kind = aarch64_codegen::MemoryBaseKind::StringConstant,
          .string_name = c4c::TextId{52},
          .string_symbol_name = c4c::LinkNameId{53},
          .size_bytes = 8,
          .align_bytes = 8,
          .address_space = bir::AddressSpace::Tls,
          .is_volatile = true,
      });

  const auto* symbol_memory = std::get_if<aarch64_codegen::MemoryOperand>(&symbol.payload);
  const auto* string_memory = std::get_if<aarch64_codegen::MemoryOperand>(&string.payload);
  if (symbol_memory == nullptr || string_memory == nullptr) {
    return fail("expected symbol and string memory operands");
  }
  if (symbol_memory->base_kind != aarch64_codegen::MemoryBaseKind::Symbol ||
      symbol_memory->symbol_name != c4c::LinkNameId{51} ||
      symbol_memory->stored_value_id != prepare::PreparedValueId{31} ||
      symbol_memory->stored_value_name != c4c::ValueNameId{41} ||
      symbol_memory->byte_offset != -8 ||
      symbol_memory->address_space != bir::AddressSpace::Gs ||
      symbol_memory->is_volatile) {
    return fail("expected symbol memory record to preserve link and store facts");
  }
  if (string_memory->base_kind != aarch64_codegen::MemoryBaseKind::StringConstant ||
      string_memory->string_name != c4c::TextId{52} ||
      string_memory->string_symbol_name != c4c::LinkNameId{53} ||
      string_memory->result_value_id != prepare::PreparedValueId{32} ||
      string_memory->result_value_name != c4c::ValueNameId{42} ||
      string_memory->address_space != bir::AddressSpace::Tls ||
      !string_memory->is_volatile) {
    return fail("expected string memory record to preserve structured string identity");
  }
  return 0;
}

int pointer_and_deferred_memory_records_are_explicit_target_mir_record_data() {
  const auto pointer = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{13},
          .block_label = c4c::BlockLabelId{23},
          .instruction_index = 6,
          .stored_value_name = c4c::ValueNameId{43},
          .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
          .pointer_value_name = c4c::ValueNameId{44},
          .pointer_value_id = prepare::PreparedValueId{33},
          .byte_offset = 24,
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Default,
          .can_use_base_plus_offset = true,
      });
  const auto deferred = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported,
          .function_name = c4c::FunctionNameId{14},
          .block_label = c4c::BlockLabelId{24},
          .instruction_index = 7,
          .base_kind = aarch64_codegen::MemoryBaseKind::None,
          .address_space = bir::AddressSpace::Default,
      });

  const auto* pointer_memory = std::get_if<aarch64_codegen::MemoryOperand>(&pointer.payload);
  const auto* deferred_memory = std::get_if<aarch64_codegen::MemoryOperand>(&deferred.payload);
  if (pointer_memory == nullptr || deferred_memory == nullptr) {
    return fail("expected pointer and deferred memory operands");
  }
  if (pointer_memory->base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
      pointer_memory->pointer_value_name != c4c::ValueNameId{44} ||
      pointer_memory->pointer_value_id != prepare::PreparedValueId{33} ||
      pointer_memory->stored_value_name != c4c::ValueNameId{43} ||
      pointer_memory->byte_offset != 24 ||
      !pointer_memory->can_use_base_plus_offset) {
    return fail("expected pointer memory record to preserve value identity slots");
  }
  if (deferred_memory->surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      deferred_memory->support != aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported ||
      aarch64_codegen::memory_operand_support_kind_name(deferred_memory->support) !=
          "deferred_unsupported" ||
      deferred_memory->base_kind != aarch64_codegen::MemoryBaseKind::None ||
      deferred_memory->function_name != c4c::FunctionNameId{14} ||
      deferred_memory->block_label != c4c::BlockLabelId{24} ||
      deferred_memory->instruction_index != 7) {
    return fail("expected unsupported memory records to defer explicitly");
  }
  return 0;
}

int volatility_and_address_space_records_stay_distinct() {
  const auto non_volatile_gs = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{16},
          .block_label = c4c::BlockLabelId{26},
          .instruction_index = 9,
          .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
          .symbol_name = c4c::LinkNameId{56},
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Gs,
          .is_volatile = false,
      });
  const auto volatile_tls = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .function_name = c4c::FunctionNameId{16},
          .block_label = c4c::BlockLabelId{26},
          .instruction_index = 10,
          .base_kind = aarch64_codegen::MemoryBaseKind::Symbol,
          .symbol_name = c4c::LinkNameId{56},
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Tls,
          .is_volatile = true,
      });

  const auto* non_volatile_memory =
      std::get_if<aarch64_codegen::MemoryOperand>(&non_volatile_gs.payload);
  const auto* volatile_memory =
      std::get_if<aarch64_codegen::MemoryOperand>(&volatile_tls.payload);
  if (non_volatile_memory == nullptr || volatile_memory == nullptr) {
    return fail("expected volatility proof memory operands");
  }
  if (non_volatile_memory->address_space != bir::AddressSpace::Gs ||
      non_volatile_memory->is_volatile) {
    return fail("expected non-volatile GS memory record to preserve facts exactly");
  }
  if (volatile_memory->address_space != bir::AddressSpace::Tls ||
      !volatile_memory->is_volatile) {
    return fail("expected volatile TLS memory record to preserve facts exactly");
  }
  if (non_volatile_memory->address_space == volatile_memory->address_space ||
      non_volatile_memory->is_volatile == volatile_memory->is_volatile) {
    return fail("expected direct records to keep volatility and address space distinct");
  }
  return 0;
}

int memory_instruction_records_do_not_select_or_emit_load_store_behavior() {
  const auto instruction = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{15},
                  .block_label = c4c::BlockLabelId{25},
                  .instruction_index = 8,
                  .stored_value_id = prepare::PreparedValueId{34},
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{54},
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
          .value =
              aarch64_codegen::make_prepared_value_operand(
                  aarch64_codegen::PreparedValueOperand{
                      .value_id = prepare::PreparedValueId{34},
                      .function_name = c4c::FunctionNameId{15},
                      .value_name = c4c::ValueNameId{45},
                      .type = bir::TypeKind::I32,
                  }),
          .value_type = bir::TypeKind::I32,
      });

  const auto* memory =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&instruction.payload);
  if (instruction.family != aarch64_codegen::InstructionFamily::Memory ||
      instruction.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.opcode != aarch64_codegen::MachineOpcode::Store ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) != "store" ||
      instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.operands.size() != 2 || instruction.uses.size() != 2 ||
      !instruction.defs.empty() || instruction.side_effects.size() != 1 ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryWrite ||
      memory == nullptr ||
      aarch64_codegen::memory_instruction_kind_name(memory->memory_kind) != "store" ||
      memory->address.function_name != c4c::FunctionNameId{15} ||
      memory->address.stored_value_id != prepare::PreparedValueId{34} ||
      !memory->value.has_value()) {
    return fail("expected memory instruction node to preserve target MIR store metadata");
  }
  if (std::holds_alternative<aarch64_codegen::CallInstructionRecord>(instruction.payload) ||
      std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(instruction.payload) ||
      std::holds_alternative<aarch64_codegen::AssemblerInstructionRecord>(instruction.payload) ||
      std::holds_alternative<aarch64_codegen::ObjectInstructionRecord>(instruction.payload)) {
    return fail("expected memory records not to own call, return, assembler, or object behavior");
  }
  return 0;
}

int unsupported_machine_node_status_fails_closed_without_text() {
  const auto unsupported = aarch64_codegen::make_unsupported_machine_instruction(
      aarch64_codegen::InstructionFamily::Memory,
      aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported,
      "global/string memory selection is deferred");

  if (unsupported.family != aarch64_codegen::InstructionFamily::Memory ||
      unsupported.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      unsupported.opcode != aarch64_codegen::MachineOpcode::Unspecified ||
      unsupported.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported ||
      unsupported.selection.diagnostic != "global/string memory selection is deferred" ||
      !unsupported.operands.empty() || !unsupported.defs.empty() ||
      !unsupported.uses.empty() || !unsupported.clobbers.empty() ||
      !unsupported.side_effects.empty()) {
    return fail("expected unsupported machine node status to fail closed structurally");
  }
  if (aarch64_codegen::machine_node_selection_status_name(unsupported.selection.status) !=
          "deferred_unsupported" ||
      aarch64_codegen::machine_opcode_name(unsupported.opcode) != "unspecified" ||
      aarch64_codegen::machine_opcode_name(aarch64_codegen::MachineOpcode::SpillToSlot) !=
          "spill_to_slot" ||
      aarch64_codegen::machine_pseudo_kind_name(aarch64_codegen::MachinePseudoKind::ReloadFromSlot) !=
          "reload_from_slot" ||
      aarch64_codegen::machine_side_effect_kind_name(
          aarch64_codegen::MachineSideEffectKind::MemoryWrite) != "memory_write") {
    return fail("expected unsupported diagnostics to avoid assembly text identity");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = frame_slot_memory_records_preserve_prepared_access_identity();
      status != 0) {
    return status;
  }
  if (const int status = symbol_and_string_memory_records_keep_structured_identity();
      status != 0) {
    return status;
  }
  if (const int status = pointer_and_deferred_memory_records_are_explicit_target_mir_record_data();
      status != 0) {
    return status;
  }
  if (const int status = volatility_and_address_space_records_stay_distinct(); status != 0) {
    return status;
  }
  if (const int status = memory_instruction_records_do_not_select_or_emit_load_store_behavior();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_machine_node_status_fails_closed_without_text();
      status != 0) {
    return status;
  }
  return 0;
}
