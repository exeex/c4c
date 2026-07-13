#include "private.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace frame_printer {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

const PreparedFrameSlot* find_frame_slot(const PreparedBirModule& module,
                                         PreparedFrameSlotId slot_id) {
  for (const auto& slot : module.stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

std::string optional_size_text(const std::optional<std::size_t>& value) {
  return value.has_value() ? std::to_string(*value) : std::string("<unknown>");
}

void append_register_placement(std::ostringstream& out,
                               std::string_view label,
                               const std::optional<PreparedRegisterPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " " << label << "="
      << prepared_register_bank_name(placement->bank) << ":"
      << prepared_register_slot_pool_name(placement->pool)
      << "#" << placement->slot_index
      << "/w" << placement->contiguous_width;
}

void append_saved_register_slot_placement(
    std::ostringstream& out,
    const std::optional<PreparedSavedRegisterSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " slot_placement=";
  if (placement->slot_id.has_value()) {
    out << "slot#" << *placement->slot_id;
  } else {
    out << "<none>";
  }
  out << "+stack" << optional_size_text(placement->stack_offset_bytes)
      << " slot_size=" << optional_size_text(placement->size_bytes)
      << " slot_align=" << optional_size_text(placement->align_bytes)
      << " fixed_location=" << (placement->fixed_location ? "yes" : "no")
      << " slot_reg=" << prepared_register_bank_name(placement->bank)
      << ":" << placement->register_name
      << " slot_save_index=" << placement->save_index
      << " slot_width=" << placement->contiguous_width;
  if (!placement->occupied_register_names.empty()) {
    out << " slot_units=";
    for (std::size_t index = 0; index < placement->occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << placement->occupied_register_names[index];
    }
  }
  append_register_placement(out, "slot_register_placement", placement->register_placement);
}

void append_register_occupancy(std::ostringstream& out,
                               std::size_t contiguous_width,
                               const std::vector<std::string>& occupied_register_names) {
  out << " width=" << contiguous_width;
  if (!occupied_register_names.empty()) {
    out << " units=";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << occupied_register_names[index];
    }
  }
}

}  // namespace frame_printer

void append_stack_layout(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace frame_printer;

  out << "--- prepared-stack-layout ---\n";
  out << "frame_size=" << module.stack_layout.frame_size_bytes
      << " frame_alignment=" << module.stack_layout.frame_alignment_bytes << "\n";
  for (const auto& object : module.stack_layout.objects) {
    out << "object #" << object.object_id
        << " func=" << maybe_function_name(module.names, object.function_name)
        << " name=" << prepared_stack_object_name(module.names, object)
        << " source_kind=" << object.source_kind
        << " type=" << c4c::backend::bir::render_type(object.type)
        << " size=" << object.size_bytes
        << " align=" << object.align_bytes
        << " address_exposed=" << (object.address_exposed ? "yes" : "no")
        << " requires_home_slot=" << (object.requires_home_slot ? "yes" : "no")
        << " permanent_home_slot=" << (object.permanent_home_slot ? "yes" : "no")
        << "\n";
  }
  for (const auto& slot : module.stack_layout.frame_slots) {
    out << "slot #" << slot.slot_id
        << " object_id=" << slot.object_id
        << " func=" << maybe_function_name(module.names, slot.function_name)
        << " offset=" << slot.offset_bytes
        << " size=" << slot.size_bytes
        << " align=" << slot.align_bytes
        << " fixed_location=" << (slot.fixed_location ? "yes" : "no")
        << "\n";
  }
}

void append_frame_plan(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace frame_printer;

  out << "--- prepared-frame-plan ---\n";
  for (const auto& function_plan : module.frame_plan.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << " frame_size=" << function_plan.frame_size_bytes
        << " frame_alignment=" << function_plan.frame_alignment_bytes
        << " has_dynamic_stack=" << (function_plan.has_dynamic_stack ? "yes" : "no")
        << " fixed_slots_use_fp="
        << (function_plan.uses_frame_pointer_for_fixed_slots ? "yes" : "no")
        << "\n";
    for (const auto& saved : function_plan.saved_callee_registers) {
      out << "  saved_register bank=" << prepared_register_bank_name(saved.bank);
      append_register_placement(out, "placement", saved.placement);
      out << " reg=" << saved.register_name
          << " save_index=" << saved.save_index;
      append_register_occupancy(out, saved.contiguous_width, saved.occupied_register_names);
      append_saved_register_slot_placement(out, saved.slot_placement);
      out << "\n";
    }
    for (const auto slot_id : function_plan.frame_slot_order) {
      out << "  frame_slot_order slot_id=#" << slot_id;
      if (const auto* slot = find_frame_slot(module, slot_id); slot != nullptr) {
        out << " offset=" << slot->offset_bytes
            << " size=" << slot->size_bytes
            << " align=" << slot->align_bytes;
      }
      out << "\n";
    }
  }
}

void append_dynamic_stack_plan(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace frame_printer;

  out << "--- prepared-dynamic-stack-plan ---\n";
  for (const auto& function_plan : module.dynamic_stack_plan.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << " requires_stack_save_restore="
        << (function_plan.requires_stack_save_restore ? "yes" : "no")
        << " fixed_slots_use_fp="
        << (function_plan.uses_frame_pointer_for_fixed_slots ? "yes" : "no")
        << "\n";
    for (const auto& op : function_plan.operations) {
      out << "  dynamic_stack_op block=" << maybe_block_label(module.names, op.block_label)
          << " inst_index=" << op.instruction_index
          << " kind=" << prepared_dynamic_stack_op_kind_name(op.kind);
      if (op.result_value_name.has_value()) {
        out << " result=" << prepared_value_name(module.names, *op.result_value_name);
      }
      if (op.operand_value_name.has_value()) {
        out << " operand=" << prepared_value_name(module.names, *op.operand_value_name);
      }
      if (!op.allocation_type_text.empty()) {
        out << " alloc_type=" << op.allocation_type_text;
      }
      if (op.element_size_bytes != 0) {
        out << " elem_size=" << op.element_size_bytes;
      }
      if (op.element_align_bytes != 0) {
        out << " elem_align=" << op.element_align_bytes;
      }
      out << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
