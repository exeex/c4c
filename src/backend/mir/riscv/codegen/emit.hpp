#pragma once

#include "../assembler/mod.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/prealloc.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend::riscv {

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::riscv

namespace c4c::backend::riscv::codegen {

enum class EdgePublicationMoveIntentStatus {
  Available,
  MissingSharedLookups,
  MissingPublication,
  UnsupportedPublication,
  UnsupportedSourceHome,
  UnsupportedDestinationHome,
};

struct EdgePublicationMoveIntent {
  EdgePublicationMoveIntentStatus status =
      EdgePublicationMoveIntentStatus::MissingPublication;
  const c4c::backend::prepare::PreparedEdgePublication* publication = nullptr;
  c4c::backend::prepare::PreparedValueId destination_value_id = 0;
  std::optional<c4c::backend::prepare::PreparedValueId> source_value_id;
  c4c::backend::bir::TypeKind source_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind destination_type = c4c::backend::bir::TypeKind::Void;
  std::string source_register;
  std::optional<std::int64_t> source_immediate_i32;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> source_stack_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_stack_size_bytes;
  std::optional<std::size_t> source_stack_align_bytes;
  c4c::backend::prepare::PreparedTypedStackSourceExtensionPolicy
      source_stack_extension_policy =
          c4c::backend::prepare::PreparedTypedStackSourceExtensionPolicy::None;
  std::optional<c4c::backend::prepare::PreparedValueId>
      source_memory_base_value_id;
  std::string source_memory_base_register;
  std::optional<std::int64_t> source_memory_byte_offset;
  std::optional<std::size_t> source_memory_size_bytes;
  std::optional<std::size_t> source_memory_align_bytes;
  std::optional<c4c::backend::prepare::PreparedValueId> source_pointer_base_value_id;
  std::string source_pointer_base_register;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::string destination_register;
  c4c::backend::prepare::PreparedRegisterBank destination_register_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::optional<c4c::backend::prepare::PreparedRegisterPlacement>
      destination_register_placement;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> destination_stack_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_size_bytes;
  std::string instruction_text;
};

[[nodiscard]] EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id);

[[nodiscard]] EdgePublicationMoveIntent append_edge_publication_move_instruction(
    std::string& output,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id);

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);
std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module);
std::string peephole_optimize(std::string asm_text);

}  // namespace c4c::backend::riscv::codegen
