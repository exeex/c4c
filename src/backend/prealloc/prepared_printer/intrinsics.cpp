#include "private.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

namespace intrinsic_printer {

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

std::string type_kind_name(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::Void:
      return "void";
    case c4c::backend::bir::TypeKind::I1:
      return "i1";
    case c4c::backend::bir::TypeKind::I8:
      return "i8";
    case c4c::backend::bir::TypeKind::I16:
      return "i16";
    case c4c::backend::bir::TypeKind::I32:
      return "i32";
    case c4c::backend::bir::TypeKind::I64:
      return "i64";
    case c4c::backend::bir::TypeKind::I128:
      return "i128";
    case c4c::backend::bir::TypeKind::Ptr:
      return "ptr";
    case c4c::backend::bir::TypeKind::F32:
      return "f32";
    case c4c::backend::bir::TypeKind::F64:
      return "f64";
    case c4c::backend::bir::TypeKind::F128:
      return "f128";
  }
  return "unknown";
}

std::string_view address_space_name(bir::AddressSpace address_space) {
  switch (address_space) {
    case bir::AddressSpace::Default:
      return "default";
    case bir::AddressSpace::Fs:
      return "fs";
    case bir::AddressSpace::Gs:
      return "gs";
    case bir::AddressSpace::Tls:
      return "tls";
  }
  return "unknown";
}

}  // namespace intrinsic_printer

void append_intrinsic_carriers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace intrinsic_printer;

  out << "--- prepared-intrinsic-carriers ---\n";
  for (const auto& function_carriers : module.intrinsic_carriers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_carriers.function_name)
        << "\n";
    for (const auto& carrier : function_carriers.carriers) {
      if (carrier.carrier_kind != PreparedIntrinsicCarrierKind::Complete) {
        continue;
      }
      out << "  intrinsic_carrier family="
          << bir::intrinsic_family_kind_name(carrier.family)
          << " operation=" << bir::intrinsic_operation_kind_name(carrier.operation)
          << " feature=" << bir::intrinsic_feature_kind_name(carrier.required_feature)
          << " block_index=" << carrier.block_index
          << " inst_index=" << carrier.inst_index
          << " operand_type=" << type_kind_name(carrier.operand_type)
          << " result_type=" << type_kind_name(carrier.result_type);
      if (!carrier.operand_roles.empty()) {
        out << " roles=";
        for (std::size_t index = 0; index < carrier.operand_roles.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          out << bir::intrinsic_operand_role_name(carrier.operand_roles[index]);
        }
      }
      if (carrier.vector_total_width_bytes != 0) {
        out << " vector_element_type=" << type_kind_name(carrier.vector_element_type)
            << " vector_element_width=" << carrier.vector_element_width_bytes
            << " vector_lanes=" << carrier.vector_lane_count
            << " vector_width=" << carrier.vector_total_width_bytes;
      }
      out << " signedness=" << bir::intrinsic_signedness_name(carrier.signedness)
          << " memory_access=" << bir::intrinsic_memory_access_kind_name(carrier.memory_access);
      if (carrier.memory_operand.has_value()) {
        out << " memory_size=" << carrier.memory_operand->size_bytes
            << " memory_align=" << carrier.memory_operand->align_bytes
            << " memory_address_space="
            << address_space_name(carrier.memory_operand->address_space)
            << " memory_volatile=" << (carrier.memory_operand->is_volatile ? "yes" : "no");
      }
      if (carrier.barrier_domain != bir::IntrinsicBarrierDomainKind::None) {
        out << " barrier_domain="
            << bir::intrinsic_barrier_domain_kind_name(carrier.barrier_domain);
      }
      if (carrier.immediate_value.has_value()) {
        out << " immediate=" << *carrier.immediate_value;
      }
      out
          << " side_effects=" << (carrier.has_side_effects ? "yes" : "no")
          << " requires_feature=" << (carrier.requires_feature ? "yes" : "no")
          << " prepared_call_plan=" << (carrier.has_prepared_call_plan ? "yes" : "no");
      if (carrier.source_callee_name.has_value()) {
        out << " source_callee=" << *carrier.source_callee_name;
      }
      if (carrier.operand_value_name.has_value()) {
        out << " operand=" << maybe_value_name(module.names, *carrier.operand_value_name);
      }
      if (carrier.operand_value_names.size() > 1) {
        out << " operands=";
        bool first = true;
        for (const auto operand_name : carrier.operand_value_names) {
          if (operand_name == kInvalidValueName) {
            continue;
          }
          if (!first) {
            out << ",";
          }
          first = false;
          out << maybe_value_name(module.names, operand_name);
        }
      }
      if (carrier.result_value_name.has_value()) {
        out << " result=" << maybe_value_name(module.names, *carrier.result_value_name);
      }
      out << " operand_homes=" << carrier.operand_homes.size()
          << " result_home=" << (carrier.result_home.has_value() ? "yes" : "no");
      out << "\n";
    }
    for (const auto& fact : function_carriers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
