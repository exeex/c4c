#include "prepared_frame_emit.hpp"

#include "object_emission.hpp"
#include "rv64_line_assembler.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <unordered_set>

namespace c4c::backend::riscv::codegen {

namespace {

constexpr std::size_t kRv64StackFrameAlignment = 16;
constexpr std::uint32_t kRv64StackFrameScratchRegister = 5;  // t0

std::uint32_t rv64_temporary_gpr_avoiding(std::uint32_t reserved_register) {
  constexpr std::uint32_t t1 = 6;
  constexpr std::uint32_t t2 = 7;
  return reserved_register == t1 ? t2 : t1;
}

bool append_rv64_store_register_to_base(RiscvEncodedFragment& fragment,
                                        std::uint32_t source_register,
                                        std::uint32_t base_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_prepared_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_s_type(0x23,
                                       *funct3,
                                       base_register,
                                       source_register,
                                       offset));
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
  const auto funct3 = rv64_prepared_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x03,
                                       destination_register,
                                       *funct3,
                                       base_register,
                                       offset));
  return true;
}

}  // namespace

bool fits_signed_12_bit_load_offset(std::size_t offset_bytes) {
  return offset_bytes <= 2047;
}

bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

std::size_t align_riscv_stack_frame(std::size_t size_bytes) {
  return (size_bytes + 15) & ~std::size_t{15};
}

std::size_t align_riscv_stack_slot(std::size_t offset_bytes, std::size_t align_bytes) {
  if (align_bytes <= 1) {
    return offset_bytes;
  }
  return (offset_bytes + align_bytes - 1) / align_bytes * align_bytes;
}

std::optional<std::size_t> prepared_saved_register_stack_end(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan) {
  if (frame_plan == nullptr) {
    return std::size_t{0};
  }
  std::size_t end_offset = 0;
  for (const auto& saved : frame_plan->saved_callee_registers) {
    if (!saved.slot_placement.has_value() ||
        !saved.slot_placement->stack_offset_bytes.has_value() ||
        !saved.slot_placement->size_bytes.has_value()) {
      return std::nullopt;
    }
    const std::size_t offset = *saved.slot_placement->stack_offset_bytes;
    const std::size_t size = *saved.slot_placement->size_bytes;
    if (size == 0 || offset > std::numeric_limits<std::size_t>::max() - size) {
      return std::nullopt;
    }
    end_offset = std::max(end_offset, offset + size);
  }
  return end_offset;
}

bool rv64_prepared_supported_fixed_frame_alignment(std::size_t alignment) {
  return alignment > 0 && alignment <= kRv64StackFrameAlignment &&
         (alignment & (alignment - 1)) == 0;
}

std::optional<std::size_t> align_rv64_prepared_object_stack_frame_size(
    std::size_t frame_size) {
  if (frame_size == 0) {
    return std::size_t{0};
  }
  if (frame_size >
      std::numeric_limits<std::size_t>::max() -
          (kRv64StackFrameAlignment - 1)) {
    return std::nullopt;
  }
  const auto aligned =
      ((frame_size + kRv64StackFrameAlignment - 1) /
       kRv64StackFrameAlignment) *
      kRv64StackFrameAlignment;
  if (aligned >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return aligned;
}

const c4c::backend::prepare::PreparedFrameSlot*
rv64_prepared_find_function_frame_slot(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::backend::prepare::PreparedFrameSlotId slot_id,
    c4c::FunctionNameId function_name) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id && slot.function_name == function_name) {
      return &slot;
    }
  }
  return nullptr;
}

bool rv64_prepared_frame_slot_extent_is_supported(
    const c4c::backend::prepare::PreparedFrameSlot& slot,
    std::size_t frame_size) {
  return rv64_prepared_supported_fixed_frame_alignment(slot.align_bytes) &&
         slot.offset_bytes <=
             std::numeric_limits<std::size_t>::max() - slot.size_bytes &&
         slot.offset_bytes + slot.size_bytes <= frame_size;
}

std::optional<std::size_t> rv64_prepared_validated_fixed_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction& frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout) {
  namespace prepare = c4c::backend::prepare;

  if (frame_plan.function_name == c4c::kInvalidFunctionName ||
      frame_plan.has_dynamic_stack ||
      frame_plan.uses_frame_pointer_for_fixed_slots ||
      !rv64_prepared_supported_fixed_frame_alignment(
          frame_plan.frame_alignment_bytes)) {
    return std::nullopt;
  }
  if (addressing != nullptr) {
    if (addressing->function_name != frame_plan.function_name ||
        !rv64_prepared_supported_fixed_frame_alignment(
            addressing->frame_alignment_bytes) ||
        addressing->frame_alignment_bytes > frame_plan.frame_alignment_bytes ||
        addressing->frame_size_bytes > frame_plan.frame_size_bytes) {
      return std::nullopt;
    }
  }

  std::unordered_set<prepare::PreparedFrameSlotId> ordered_slots;
  ordered_slots.reserve(frame_plan.frame_slot_order.size());
  for (const auto slot_id : frame_plan.frame_slot_order) {
    if (!ordered_slots.insert(slot_id).second) {
      return std::nullopt;
    }
    const auto* slot = rv64_prepared_find_function_frame_slot(
        stack_layout, slot_id, frame_plan.function_name);
    if (slot == nullptr ||
        !rv64_prepared_frame_slot_extent_is_supported(
            *slot, frame_plan.frame_size_bytes)) {
      return std::nullopt;
    }
  }
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name != frame_plan.function_name) {
      continue;
    }
    if (ordered_slots.find(slot.slot_id) == ordered_slots.end() ||
        !rv64_prepared_frame_slot_extent_is_supported(
            slot, frame_plan.frame_size_bytes)) {
      return std::nullopt;
    }
  }

  std::size_t frame_size = frame_plan.frame_size_bytes;
  for (const auto& saved : frame_plan.saved_callee_registers) {
    if (!saved.placement.has_value() ||
        !prepare::has_prepared_register_placement(*saved.placement) ||
        !saved.slot_placement.has_value() ||
        !prepare::has_complete_prepared_saved_register_slot_placement(
            *saved.slot_placement)) {
      return std::nullopt;
    }
    const auto& slot = *saved.slot_placement;
    if (!rv64_prepared_supported_fixed_frame_alignment(*slot.align_bytes) ||
        !slot.fixed_location ||
        *slot.stack_offset_bytes >
            std::numeric_limits<std::size_t>::max() - *slot.size_bytes) {
      return std::nullopt;
    }
    frame_size = std::max(frame_size,
                          *slot.stack_offset_bytes + *slot.size_bytes);
  }

  return align_rv64_prepared_object_stack_frame_size(frame_size);
}

std::optional<std::size_t> rv64_prepared_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout) {
  if (frame_plan != nullptr) {
    return rv64_prepared_validated_fixed_frame_size(
        addressing, *frame_plan, stack_layout);
  }

  const auto addressing_frame_size =
      addressing == nullptr ? std::size_t{0} : addressing->frame_size_bytes;
  auto prepared_frame_size = stack_layout.frame_size_bytes;
  const auto frame_size = std::max(addressing_frame_size, prepared_frame_size);
  const auto aligned = align_rv64_prepared_object_stack_frame_size(frame_size);
  return aligned.has_value() &&
                 fits_signed_12_bit_immediate(static_cast<std::int64_t>(*aligned))
             ? aligned
             : std::nullopt;
}

std::optional<std::size_t> rv64_prepared_call_frame_size(
    std::size_t local_frame_bytes) {
  if (local_frame_bytes > std::numeric_limits<std::size_t>::max() - 16) {
    return std::nullopt;
  }
  return local_frame_bytes + 16;
}

std::optional<std::size_t> rv64_prepared_call_frame_ra_offset(
    std::size_t local_frame_bytes) {
  if (local_frame_bytes > std::numeric_limits<std::size_t>::max() - 8) {
    return std::nullopt;
  }
  return local_frame_bytes + 8;
}

bool rv64_prepared_is_callee_saved_gpr_register_name(std::string_view name) {
  return name == "s0" || name == "fp" || name == "s1" || name == "s2" ||
         name == "s3" || name == "s4" || name == "s5" || name == "s6" ||
         name == "s7" || name == "s8" || name == "s9" || name == "s10" ||
         name == "s11";
}

bool rv64_prepared_is_callee_saved_fpr_register_name(std::string_view name) {
  return name == "fs0" || name == "fs1" || name == "fs2" || name == "fs3" ||
         name == "fs4" || name == "fs5" || name == "fs6" || name == "fs7" ||
         name == "fs8" || name == "fs9" || name == "fs10" || name == "fs11";
}

std::optional<std::int32_t> rv64_prepared_saved_callee_gpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (saved.bank != prepare::PreparedRegisterBank::Gpr ||
      saved.register_name.empty() ||
      !rv64_prepared_is_callee_saved_gpr_register_name(saved.register_name) ||
      !rv64_prepared_register_number(saved.register_name).has_value() ||
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

std::optional<std::int32_t> rv64_prepared_saved_callee_fpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (saved.bank != prepare::PreparedRegisterBank::Fpr ||
      saved.register_name.empty() ||
      !rv64_prepared_is_callee_saved_fpr_register_name(saved.register_name) ||
      saved.contiguous_width != 1 ||
      saved.occupied_register_names.size() != 1 ||
      saved.occupied_register_names.front() != saved.register_name ||
      !saved.placement.has_value() ||
      saved.placement->bank != prepare::PreparedRegisterBank::Fpr ||
      saved.placement->pool != prepare::PreparedRegisterSlotPool::CalleeSaved ||
      saved.placement->contiguous_width != 1 ||
      !saved.slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *saved.slot_placement)) {
    return std::nullopt;
  }

  const auto& slot = *saved.slot_placement;
  if (slot.bank != prepare::PreparedRegisterBank::Fpr ||
      slot.register_name != saved.register_name ||
      slot.contiguous_width != 1 ||
      slot.occupied_register_names.size() != 1 ||
      slot.occupied_register_names.front() != saved.register_name ||
      slot.save_index != saved.save_index ||
      !slot.register_placement.has_value() ||
      slot.register_placement != saved.placement ||
      !slot.stack_offset_bytes.has_value() ||
      !slot.size_bytes.has_value() ||
      *slot.size_bytes != 8 ||
      !slot.align_bytes.has_value() ||
      *slot.align_bytes != 8 ||
      !slot.fixed_location ||
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

std::optional<std::size_t> rv64_prepared_stack_slot_home_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
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
      offset > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return offset;
}

std::optional<std::int32_t> rv64_prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  const auto offset = rv64_prepared_stack_slot_home_absolute_offset(
      stack_layout, home, stack_frame_bytes, size_bytes);
  if (!offset.has_value() ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(*offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(*offset);
}

std::optional<std::uint32_t> rv64_prepared_register_number(std::string_view name) {
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

std::optional<std::uint32_t> rv64_prepared_gpr_register_number_for_home(
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
  return rv64_prepared_register_number(*home.register_name);
}

std::optional<std::uint32_t> rv64_prepared_load_store_funct3_for_size(
    std::size_t size_bytes) {
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

bool append_rv64_prepared_store_register_to_stack(RiscvEncodedFragment& fragment,
                                         std::uint32_t source_register,
                                         std::int32_t offset,
                                         std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_prepared_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_s_type(0x23, *funct3, 2, source_register, offset));
  return true;
}

bool append_rv64_prepared_load_stack_to_register(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_prepared_load_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x03,
                                       destination_register,
                                       *funct3,
                                       2,
                                       offset));
  return true;
}

void append_rv64_prepared_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source) {
  if (destination == source) {
    return;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x13,
                                       destination,
                                       0,
                                       source,
                                       0));  // addi rd, rs, 0
}

void append_rv64_prepared_add_registers(RiscvEncodedFragment& fragment,
                               std::uint32_t destination,
                               std::uint32_t lhs,
                               std::uint32_t rhs) {
  rv64_append_le32(fragment.bytes,
                   rv64_encode_r_type(0x33,
                                       destination,
                                       0,
                                       lhs,
                                       rhs,
                                       0));  // add
}

void append_rv64_prepared_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate) {
  if (!fits_signed_12_bit_immediate(immediate)) {
    const std::uint64_t low_bits =
        static_cast<std::uint64_t>(immediate) & 0xfffU;
    const std::int64_t lo12 = low_bits >= 0x800U
                                  ? static_cast<std::int64_t>(low_bits) - 0x1000
                                  : static_cast<std::int64_t>(low_bits);
    const std::int64_t hi = (immediate - lo12) / 4096;
    append_rv64_prepared_load_immediate(fragment, destination, hi);
    rv64_append_le32(fragment.bytes,
                     rv64_encode_i_type(0x13,
                                         destination,
                                         1,
                                         destination,
                                         12));  // slli rd, rd, 12
    if (lo12 != 0) {
      rv64_append_le32(
          fragment.bytes,
          rv64_encode_i_type(0x13,
                             destination,
                             0,
                             destination,
                             static_cast<std::int32_t>(lo12)));  // addi rd, rd, lo
    }
    return;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x13,
                                       destination,
                                       0,
                                       0,
                                       static_cast<std::int32_t>(immediate)));
}

bool append_rv64_prepared_stack_pointer_adjustment(RiscvEncodedFragment& fragment,
                                          std::int64_t byte_delta) {
  if (fits_signed_12_bit_immediate(byte_delta)) {
    rv64_append_le32(fragment.bytes,
                     rv64_encode_i_type(0x13,
                                         2,
                                         0,
                                         2,
                                         static_cast<std::int32_t>(byte_delta)));
    return true;
  }
  append_rv64_prepared_load_immediate(
      fragment, kRv64StackFrameScratchRegister, byte_delta);
  append_rv64_prepared_add_registers(
      fragment, 2, 2, kRv64StackFrameScratchRegister);
  return true;
}

bool append_rv64_prepared_saved_callee_gpr_spills(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (frame_plan == nullptr) {
    return true;
  }
  for (const auto& saved : frame_plan->saved_callee_registers) {
    const auto source = rv64_prepared_register_number(saved.register_name);
    const auto offset =
        rv64_prepared_saved_callee_gpr_stack_offset(saved, stack_frame_bytes);
    if (!source.has_value() || !offset.has_value() ||
        !append_rv64_prepared_store_register_to_stack(
            fragment, *source, *offset, 8)) {
      return false;
    }
  }
  return true;
}

bool append_rv64_prepared_saved_callee_gpr_restores(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (frame_plan == nullptr) {
    return true;
  }
  for (auto it = frame_plan->saved_callee_registers.rbegin();
       it != frame_plan->saved_callee_registers.rend();
       ++it) {
    const auto destination = rv64_prepared_register_number(it->register_name);
    const auto offset =
        rv64_prepared_saved_callee_gpr_stack_offset(*it, stack_frame_bytes);
    if (!destination.has_value() || !offset.has_value() ||
        !append_rv64_prepared_load_stack_to_register(
            fragment, *destination, *offset, 8)) {
      return false;
    }
  }
  return true;
}

std::optional<RiscvEncodedFragment>
make_rv64_prepared_call_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes) {
  RiscvEncodedFragment fragment;
  const auto frame_size = rv64_prepared_call_frame_size(local_frame_bytes);
  const auto ra_offset = rv64_prepared_call_frame_ra_offset(local_frame_bytes);
  if (!frame_size.has_value() || !ra_offset.has_value() ||
      *frame_size >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  if (!append_rv64_prepared_stack_pointer_adjustment(
          fragment, -static_cast<std::int64_t>(*frame_size)) ||
      !append_rv64_prepared_store_register_to_stack_offset(
          fragment, 1, *ra_offset, 8)) {
    return std::nullopt;
  }
  if (!append_rv64_prepared_saved_callee_gpr_spills(
          fragment, frame_plan, local_frame_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment>
make_rv64_prepared_stack_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  RiscvEncodedFragment fragment;
  if (stack_frame_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
      !append_rv64_prepared_stack_pointer_adjustment(
          fragment, -static_cast<std::int64_t>(stack_frame_bytes))) {
    return std::nullopt;
  }
  if (!append_rv64_prepared_saved_callee_gpr_spills(
          fragment, frame_plan, stack_frame_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

bool append_rv64_prepared_call_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes) {
  const auto frame_size = rv64_prepared_call_frame_size(local_frame_bytes);
  const auto ra_offset = rv64_prepared_call_frame_ra_offset(local_frame_bytes);
  if (!frame_size.has_value() || !ra_offset.has_value() ||
      *frame_size >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  if (!append_rv64_prepared_saved_callee_gpr_restores(
          fragment, frame_plan, local_frame_bytes)) {
    return false;
  }
  return append_rv64_prepared_load_stack_offset_to_register(
             fragment, 1, *ra_offset, 8) &&
         append_rv64_prepared_stack_pointer_adjustment(
             fragment, static_cast<std::int64_t>(*frame_size));
}

bool append_rv64_prepared_stack_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (stack_frame_bytes == 0) {
    return frame_plan == nullptr || frame_plan->saved_callee_registers.empty();
  }
  if (!append_rv64_prepared_saved_callee_gpr_restores(
          fragment, frame_plan, stack_frame_bytes)) {
    return false;
  }
  if (stack_frame_bytes >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  return append_rv64_prepared_stack_pointer_adjustment(
      fragment, static_cast<std::int64_t>(stack_frame_bytes));
}

bool append_rv64_prepared_store_register_to_stack_offset(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::size_t offset,
    std::size_t size_bytes) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_prepared_store_register_to_stack(
        fragment,
        source_register,
        static_cast<std::int32_t>(signed_offset),
        size_bytes);
  }
  const auto scratch = rv64_temporary_gpr_avoiding(source_register);
  append_rv64_prepared_load_immediate(fragment, scratch, signed_offset);
  append_rv64_prepared_add_registers(fragment, scratch, 2, scratch);
  return append_rv64_store_register_to_base(
      fragment, source_register, scratch, 0, size_bytes);
}

bool append_rv64_prepared_load_stack_offset_to_register(RiscvEncodedFragment& fragment,
                                               std::uint32_t destination_register,
                                               std::size_t offset,
                                               std::size_t size_bytes) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_prepared_load_stack_to_register(
        fragment,
        destination_register,
        static_cast<std::int32_t>(signed_offset),
        size_bytes);
  }
  const auto scratch = rv64_temporary_gpr_avoiding(destination_register);
  append_rv64_prepared_load_immediate(fragment, scratch, signed_offset);
  append_rv64_prepared_add_registers(fragment, scratch, 2, scratch);
  return append_rv64_load_base_to_register(
      fragment, destination_register, scratch, 0, size_bytes);
}

namespace {

std::string sanitize_riscv_label_component(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (const char ch : text) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '_') {
      result.push_back(ch);
    } else {
      result.push_back('_');
    }
  }
  if (result.empty()) {
    return "anon";
  }
  if (std::isdigit(static_cast<unsigned char>(result.front()))) {
    result.insert(result.begin(), '_');
  }
  return result;
}

bool fits_i32_bytewise_stack_offsets(std::int64_t stack_offset) {
  return fits_signed_12_bit_immediate(stack_offset) &&
         fits_signed_12_bit_immediate(stack_offset + 1) &&
         fits_signed_12_bit_immediate(stack_offset + 2) &&
         fits_signed_12_bit_immediate(stack_offset + 3);
}

bool fixed_byte_split_frame_slot_span_covers(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::FunctionNameId function_name,
    std::size_t start_offset,
    std::size_t size_bytes) {
  if (size_bytes == 0 ||
      start_offset > std::numeric_limits<std::size_t>::max() - size_bytes) {
    return false;
  }

  const std::size_t end_offset = start_offset + size_bytes;
  for (std::size_t cursor = start_offset; cursor < end_offset; ++cursor) {
    bool found_slot = false;
    for (const auto& candidate : stack_layout.frame_slots) {
      if (candidate.function_name != function_name ||
          !candidate.fixed_location ||
          candidate.size_bytes != 1 ||
          candidate.offset_bytes != cursor ||
          candidate.offset_bytes >
              std::numeric_limits<std::size_t>::max() - candidate.size_bytes) {
        continue;
      }
      const std::size_t candidate_end =
          candidate.offset_bytes + candidate.size_bytes;
      if (candidate_end != cursor + 1) {
        return false;
      }
      if (found_slot) {
        return false;
      }
      found_slot = true;
    }
    if (!found_slot) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string riscv_local_block_label(std::string_view function_name,
                                    std::string_view block_label) {
  return ".L" + sanitize_riscv_label_component(function_name) + "_" +
         sanitize_riscv_label_component(block_label);
}

std::string bir_block_label_spelling(const c4c::backend::bir::Module& module,
                                     const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view spelling = module.names.block_labels.spelling(block.label_id);
    if (!spelling.empty()) {
      return std::string(spelling);
    }
  }
  return block.label;
}

std::string bir_target_label_spelling(const c4c::backend::bir::Module& module,
                                      c4c::BlockLabelId label_id,
                                      std::string_view fallback) {
  if (label_id != c4c::kInvalidBlockLabel) {
    const std::string_view spelling = module.names.block_labels.spelling(label_id);
    if (!spelling.empty()) {
      return std::string(spelling);
    }
  }
  return std::string(fallback);
}

std::optional<std::int64_t> simple_frame_slot_sp_offset_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedMemoryAccess& access) {
  if (!access.address.frame_slot_id.has_value()) {
    return std::nullopt;
  }
  const auto* frame_slot = c4c::backend::prepare::find_frame_slot_by_id(
      prepared.stack_layout,
      *access.address.frame_slot_id);
  if (frame_slot == nullptr || frame_slot->function_name != function_name ||
      access.address.byte_offset < 0) {
    return std::nullopt;
  }
  const auto access_offset = static_cast<std::size_t>(access.address.byte_offset);
  if (access_offset > frame_slot->size_bytes) {
    return std::nullopt;
  }
  if (frame_slot->offset_bytes >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) -
          access_offset) {
    return std::nullopt;
  }
  const std::size_t concrete_offset = frame_slot->offset_bytes + access_offset;
  const bool access_fits_slot =
      access.address.size_bytes <= frame_slot->size_bytes - access_offset;
  const bool byte_split_aggregate_i32_access =
      !access_fits_slot &&
      access.address.byte_offset == 0 &&
      access.address.size_bytes == 4 &&
      frame_slot->fixed_location &&
      frame_slot->size_bytes == 1;
  if (!access_fits_slot &&
      (!byte_split_aggregate_i32_access ||
       !fixed_byte_split_frame_slot_span_covers(
          prepared.stack_layout,
          function_name,
          concrete_offset,
          access.address.size_bytes))) {
    return std::nullopt;
  }
  const auto stack_offset =
      static_cast<std::int64_t>(concrete_offset);
  if (!fits_signed_12_bit_immediate(stack_offset)) {
    return std::nullopt;
  }
  return stack_offset;
}

std::optional<std::int64_t> simple_frame_slot_sp_offset_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization) {
  if (!materialization.frame_slot_id.has_value() || materialization.byte_offset < 0) {
    return std::nullopt;
  }
  const auto* frame_slot = c4c::backend::prepare::find_frame_slot_by_id(
      prepared.stack_layout,
      *materialization.frame_slot_id);
  if (frame_slot == nullptr || frame_slot->function_name != function_name) {
    return std::nullopt;
  }
  const auto materialization_offset =
      static_cast<std::size_t>(materialization.byte_offset);
  const auto stack_offset = static_cast<std::int64_t>(materialization_offset);
  if (!fits_signed_12_bit_immediate(stack_offset)) {
    return std::nullopt;
  }
  return stack_offset;
}

std::optional<std::string> emit_i32_store_to_stack_offset(std::string_view source_register,
                                                          std::int64_t stack_offset) {
  if (stack_offset % 4 == 0) {
    return "    sw " + std::string(source_register) + ", " +
           std::to_string(stack_offset) + "(sp)\n";
  }
  if (!fits_i32_bytewise_stack_offsets(stack_offset)) {
    return std::nullopt;
  }
  std::string out;
  out += "    sb " + std::string(source_register) + ", " +
         std::to_string(stack_offset) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 8\n";
  out += "    sb t2, " + std::to_string(stack_offset + 1) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 16\n";
  out += "    sb t2, " + std::to_string(stack_offset + 2) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 24\n";
  out += "    sb t2, " + std::to_string(stack_offset + 3) + "(sp)\n";
  return out;
}

std::optional<std::string> emit_i32_load_from_stack_offset(std::string_view destination_register,
                                                           std::int64_t stack_offset) {
  if (stack_offset % 4 == 0) {
    return "    lw " + std::string(destination_register) + ", " +
           std::to_string(stack_offset) + "(sp)\n";
  }
  if (!fits_i32_bytewise_stack_offsets(stack_offset)) {
    return std::nullopt;
  }
  std::string out;
  out += "    lbu " + std::string(destination_register) + ", " +
         std::to_string(stack_offset) + "(sp)\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 1) + "(sp)\n";
  out += "    slli t2, t2, 8\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 2) + "(sp)\n";
  out += "    slli t2, t2, 16\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 3) + "(sp)\n";
  out += "    slli t2, t2, 24\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    slli " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", 32\n";
  out += "    srai " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", 32\n";
  return out;
}

}  // namespace c4c::backend::riscv::codegen
