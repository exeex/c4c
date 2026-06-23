#include "prepared_frame_emit.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

namespace c4c::backend::riscv::codegen {

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
