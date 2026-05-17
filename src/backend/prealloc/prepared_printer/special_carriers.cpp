#include "private.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace special_carrier_printer {

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

}  // namespace special_carrier_printer

void append_i128_carriers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace special_carrier_printer;

  out << "--- prepared-i128-carriers ---\n";
  for (const auto& function_carriers : module.i128_carriers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_carriers.function_name)
        << "\n";
    for (const auto& carrier : function_carriers.carriers) {
      out << "  i128_carrier " << maybe_value_name(module.names, carrier.value_name)
          << " value_id=" << carrier.value_id
          << " kind=" << prepared_i128_carrier_kind_name(carrier.kind)
          << " lane_width=" << carrier.lane_width_bytes
          << " size=" << carrier.total_size_bytes
          << " align=" << carrier.total_align_bytes
          << " bank=" << prepared_register_bank_name(carrier.register_bank)
          << " class=" << prepared_register_class_name(carrier.register_class);
      append_register_placement(out, "placement", carrier.register_placement);
      append_register_occupancy(out,
                                carrier.contiguous_width,
                                carrier.occupied_register_names);
      if (carrier.slot_id.has_value()) {
        out << " slot_id=#" << *carrier.slot_id;
      }
      if (carrier.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *carrier.stack_offset_bytes;
      }

      auto append_lane = [&](const PreparedI128LaneCarrier& lane) {
        out << " " << prepared_i128_lane_role_name(lane.role)
            << "[index=" << lane.lane_index
            << ",width=" << lane.width_bytes;
        if (lane.register_name.has_value()) {
          out << ",reg=" << *lane.register_name;
        }
        if (lane.slot_id.has_value()) {
          out << ",slot=#" << *lane.slot_id;
        }
        if (lane.stack_offset_bytes.has_value()) {
          out << ",stack_offset=" << *lane.stack_offset_bytes;
        }
        out << "]";
      };
      append_lane(carrier.low_lane);
      append_lane(carrier.high_lane);

      if (!carrier.missing_required_facts.empty()) {
        out << " missing_facts=";
        for (std::size_t index = 0; index < carrier.missing_required_facts.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          out << carrier.missing_required_facts[index];
        }
      }
      out << "\n";
    }
    for (const auto& fact : function_carriers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

void append_f128_carriers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace special_carrier_printer;

  out << "--- prepared-f128-carriers ---\n";
  for (const auto& function_carriers : module.f128_carriers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_carriers.function_name)
        << "\n";
    for (const auto& carrier : function_carriers.carriers) {
      out << "  f128_carrier " << maybe_value_name(module.names, carrier.value_name)
          << " value_id=" << carrier.value_id
          << " kind=" << prepared_f128_carrier_kind_name(carrier.kind)
          << " size=" << carrier.total_size_bytes
          << " align=" << carrier.total_align_bytes
          << " bank=" << prepared_register_bank_name(carrier.register_bank)
          << " class=" << prepared_register_class_name(carrier.register_class);
      append_register_placement(out, "placement", carrier.register_placement);
      append_register_occupancy(out,
                                carrier.contiguous_width,
                                carrier.occupied_register_names);
      if (carrier.register_name.has_value()) {
        out << " reg=" << *carrier.register_name;
      }
      if (carrier.slot_id.has_value()) {
        out << " slot_id=#" << *carrier.slot_id;
      }
      if (carrier.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *carrier.stack_offset_bytes;
      }
      if (carrier.constant_payload.has_value()) {
        out << " constant_payload=0x" << std::hex << std::uppercase << std::setfill('0')
            << std::setw(16) << carrier.constant_payload->high_bits
            << std::setw(16) << carrier.constant_payload->low_bits
            << std::dec;
      }
      if (!carrier.missing_required_facts.empty()) {
        out << " missing_facts=";
        for (std::size_t index = 0; index < carrier.missing_required_facts.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          out << carrier.missing_required_facts[index];
        }
      }
      out << "\n";
    }
    for (const auto& fact : function_carriers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
