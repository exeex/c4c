#include "private.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace storage_printer {

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

std::string storage_encoding_kind_name(PreparedStorageEncodingKind kind) {
  switch (kind) {
    case PreparedStorageEncodingKind::None:
      return "none";
    case PreparedStorageEncodingKind::Register:
      return "register";
    case PreparedStorageEncodingKind::FrameSlot:
      return "frame_slot";
    case PreparedStorageEncodingKind::Immediate:
      return "immediate";
    case PreparedStorageEncodingKind::ComputedAddress:
      return "computed_address";
    case PreparedStorageEncodingKind::SymbolAddress:
      return "symbol_address";
  }
  return "unknown";
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

}  // namespace storage_printer

void append_storage_plans(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace storage_printer;

  out << "--- prepared-storage-plans ---\n";
  for (const auto& function_plan : module.storage_plans.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << "\n";
    for (const auto& value : function_plan.values) {
      out << "  storage " << maybe_value_name(module.names, value.value_name)
          << " value_id=" << value.value_id
          << " encoding=" << storage_encoding_kind_name(value.encoding)
          << " bank=" << prepared_register_bank_name(value.bank);
      append_register_placement(out, "placement", value.register_placement);
      append_spill_slot_placement(out, "spill_slot", value.spill_slot_placement);
      if (value.register_name.has_value()) {
        out << " reg=" << *value.register_name;
      }
      append_register_occupancy(out,
                                value.contiguous_width,
                                value.occupied_register_names);
      if (value.slot_id.has_value()) {
        out << " slot_id=#" << *value.slot_id;
      }
      if (value.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *value.stack_offset_bytes;
      }
      if (value.immediate_i32.has_value()) {
        out << " imm_i32=" << *value.immediate_i32;
      }
      if (value.immediate_f128.has_value()) {
        out << " imm_f128=0x" << std::hex << std::uppercase << std::setfill('0')
            << std::setw(16) << value.immediate_f128->high_bits
            << std::setw(16) << value.immediate_f128->low_bits
            << std::dec;
      }
      if (value.symbol_name.has_value()) {
        out << " symbol=" << prepared_link_name(module.names, *value.symbol_name);
      }
      out << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
