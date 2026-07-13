#include "private.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
}

const PreparedRegallocValue* find_regalloc_value(const PreparedRegallocFunction& function,
                                                 PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
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

void append_spill_slot_placement(std::ostringstream& out,
                                 std::string_view label,
                                 const std::optional<PreparedSpillSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " " << label << "=slot#" << placement->slot_id
      << "+stack" << placement->offset_bytes;
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

}  // namespace

void append_regalloc(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-regalloc ---\n";
  for (const auto& function : module.regalloc.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function.function_name) << "\n";
    for (const auto& op : function.spill_reload_ops) {
      out << "  spill_reload kind=" << prepared_spill_reload_op_kind_name(op.op_kind)
          << " value_id=" << op.value_id;
      if (const auto* value = find_regalloc_value(function, op.value_id); value != nullptr) {
        out << " value=" << maybe_value_name(module.names, value->value_name);
      }
      out << " source_value=" << maybe_value_name(module.names, op.source_value_name);
      out << " block_index=" << op.block_index
          << " inst_index=" << op.instruction_index
          << " bank=" << prepared_register_bank_name(op.register_bank)
          << " class=" << prepared_register_class_name(op.register_class);
      append_register_placement(out, "placement", op.register_placement);
      append_spill_slot_placement(out, "spill_slot", op.spill_slot_placement);
      if (op.register_name.has_value()) {
        out << " reg=" << *op.register_name;
      }
      append_register_occupancy(out, op.contiguous_width, op.occupied_register_names);
      if (op.slot_id.has_value()) {
        out << " slot_id=#" << *op.slot_id;
      }
      if (op.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *op.stack_offset_bytes;
      }
      out << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
