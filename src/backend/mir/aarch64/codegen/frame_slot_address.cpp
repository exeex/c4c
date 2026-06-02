#include "frame_slot_address.hpp"

#include "../../../prealloc/prepared_lookups.hpp"

#include <cstdint>
#include <limits>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;

[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context) {
  return context.frame_plan != nullptr &&
         context.frame_plan->uses_frame_pointer_for_fixed_slots;
}

[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes,
                                             std::string_view base_register) {
  std::string address{"["};
  address += base_register;
  if (offset_bytes != 0) {
    address += ", #";
    address += std::to_string(offset_bytes);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::string frame_slot_address(
    const module::FunctionLoweringContext& context,
    std::size_t offset_bytes) {
  return frame_slot_address(offset_bytes,
                            fixed_slots_use_frame_pointer(context) ? "x29" : "sp");
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  return prepare::find_frame_slot_by_id(stack_layout, slot_id);
}

[[nodiscard]] const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id) {
  return prepare::find_stack_object_by_id(stack_layout, object_id);
}

[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
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
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout, *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  const auto offset =
      static_cast<std::int64_t>(slot->offset_bytes) + access->address.byte_offset;
  std::string address =
      fixed_slots_use_frame_pointer(context.function) ? "[x29" : "[sp";
  if (offset != 0) {
    address += ", #";
    address += std::to_string(offset);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
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
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout,
                      *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset);
}

[[nodiscard]] std::optional<MemoryOperand> make_prepared_frame_slot_memory_operand(
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t stack_offset_bytes,
    std::size_t size_bytes,
    std::size_t align_bytes,
    bool uses_frame_pointer_base) {
  if (stack_offset_bytes >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::RecordOnly,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = function_name,
      .block_label = block_label,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = slot_id,
      .byte_offset = static_cast<std::int64_t>(stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
      .uses_frame_pointer_base = uses_frame_pointer_base,
  };
}

}  // namespace c4c::backend::aarch64::codegen
