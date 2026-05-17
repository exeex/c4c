#include "private.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace runtime_helper_printer {

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

std::string move_resolution_op_kind_name(PreparedMoveResolutionOpKind kind) {
  switch (kind) {
    case PreparedMoveResolutionOpKind::Move:
      return "move";
    case PreparedMoveResolutionOpKind::SaveDestinationToTemp:
      return "save_destination_to_temp";
  }
  return "unknown";
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

void append_register_placement(
    std::ostringstream& out,
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

void append_preserved_value_summary(std::ostringstream& out,
                                    const PreparedNameTables& names,
                                    const PreparedCallPreservedValue& preserved) {
  out << maybe_value_name(names, preserved.value_name) << "#" << preserved.value_id << ":"
      << prepared_call_preservation_route_name(preserved.route);
  if (preserved.route == PreparedCallPreservationRoute::StackSlot && preserved.slot_id.has_value()) {
    out << ":slot#" << *preserved.slot_id;
  }
  if (preserved.register_name.has_value()) {
    out << ":" << *preserved.register_name;
    if (preserved.contiguous_width > 1) {
      out << "/w" << preserved.contiguous_width;
    }
    if (!preserved.occupied_register_names.empty()) {
      out << "[";
      for (std::size_t index = 0; index < preserved.occupied_register_names.size(); ++index) {
        if (index != 0) {
          out << ",";
        }
        out << preserved.occupied_register_names[index];
      }
      out << "]";
    }
  } else if (preserved.stack_offset_bytes.has_value()) {
    out << ":stack+" << *preserved.stack_offset_bytes;
  }
  if (preserved.route == PreparedCallPreservationRoute::StackSlot) {
    if (preserved.stack_size_bytes.has_value()) {
      out << ":size=" << *preserved.stack_size_bytes;
    }
    if (preserved.stack_align_bytes.has_value()) {
      out << ":align=" << *preserved.stack_align_bytes;
    }
  }
  if (preserved.callee_saved_save_index.has_value()) {
    out << ":save" << *preserved.callee_saved_save_index;
  }
}

void append_clobbered_register_summary(std::ostringstream& out,
                                       const PreparedClobberedRegister& clobbered) {
  out << prepared_register_bank_name(clobbered.bank) << ":" << clobbered.register_name
      << "/w" << clobbered.contiguous_width;
  if (!clobbered.occupied_register_names.empty()) {
    out << "[";
    for (std::size_t index = 0; index < clobbered.occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << clobbered.occupied_register_names[index];
    }
    out << "]";
  }
}

}  // namespace runtime_helper_printer

void append_f128_runtime_helpers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace runtime_helper_printer;

  out << "--- prepared-f128-runtime-helpers ---\n";
  auto append_carrier =
      [&](std::string_view label,
          const std::optional<PreparedF128RuntimeHelper::CarrierBinding>& carrier) {
        out << " " << label << "=";
        if (!carrier.has_value()) {
          out << "<missing>";
          return;
        }
        out << maybe_value_name(module.names, carrier->value_name)
            << "#" << carrier->value_id
            << "[kind=" << prepared_f128_carrier_kind_name(carrier->carrier_kind)
            << ",width=" << carrier->width_bytes
            << ",align=" << carrier->align_bytes
            << ",bank=" << prepared_register_bank_name(carrier->register_bank)
            << ",class=" << prepared_register_class_name(carrier->register_class);
        if (carrier->register_name.has_value()) {
          out << ",reg=" << *carrier->register_name;
        }
        if (carrier->slot_id.has_value()) {
          out << ",slot=#" << *carrier->slot_id;
        }
        if (carrier->stack_offset_bytes.has_value()) {
          out << ",stack_offset=" << *carrier->stack_offset_bytes;
        }
        out << "]";
      };

  for (const auto& function_helpers : module.f128_runtime_helpers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_helpers.function_name)
        << "\n";
    auto append_abi_binding =
        [&](std::string_view label,
            const std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>& binding) {
          out << " " << label << "=";
          if (!binding.has_value()) {
            out << "<missing>";
            return;
          }
          out << maybe_value_name(module.names, binding->value_name)
              << "#" << binding->value_id
              << "[width=" << binding->width_bytes;
          if (binding->helper_argument_index.has_value()) {
            out << ",arg=" << *binding->helper_argument_index;
          } else {
            out << ",result";
          }
          out << ",abi_index=" << binding->abi_register_index
              << ",bank=" << prepared_register_bank_name(binding->register_bank)
              << ",class=" << prepared_register_class_name(binding->register_class)
              << ",abi_reg=" << binding->register_name
              << ",width_units=" << binding->contiguous_width << "]";
          append_register_placement(out, "abi_placement", binding->register_placement);
        };
    auto append_marshaling_move =
        [&](std::string_view label,
            const std::optional<PreparedF128RuntimeHelper::MarshalingMove>& move) {
          out << " " << label << "=";
          if (!move.has_value()) {
            out << "<missing>";
            return;
          }
          out << prepared_f128_runtime_helper_marshal_direction_name(move->direction)
              << "[value=" << maybe_value_name(module.names, move->carrier.value_name)
              << "#" << move->carrier.value_id
              << ",carrier_kind="
              << prepared_f128_carrier_kind_name(move->carrier.carrier_kind)
              << ",carrier_width=" << move->carrier.width_bytes
              << ",abi_index=" << move->abi_register.abi_register_index
              << ",abi_reg=" << move->abi_register.register_name << "]";
        };
    auto append_scalar_result =
        [&](std::string_view label,
            const std::optional<PreparedF128RuntimeHelper::ScalarResultOwnership>& scalar) {
          out << " " << label << "=";
          if (!scalar.has_value()) {
            out << "<missing>";
            return;
          }
          out << maybe_value_name(module.names, scalar->value_name)
              << "#" << scalar->value_id
              << "[type=" << type_kind_name(scalar->type)
              << ",width=" << scalar->width_bytes
              << ",bank=" << prepared_register_bank_name(scalar->register_bank)
              << ",home=" << prepared_value_home_kind_name(scalar->home_kind);
          if (scalar->register_name.has_value()) {
            out << ",reg=" << *scalar->register_name;
          }
          out << "]";
        };
    auto append_scalar_marshaling_move =
        [&](std::string_view label,
            const std::optional<PreparedF128RuntimeHelper::ScalarMarshalingMove>& move) {
          out << " " << label << "=";
          if (!move.has_value()) {
            out << "<missing>";
            return;
          }
          out << prepared_f128_runtime_helper_marshal_direction_name(move->direction)
              << "[value=" << maybe_value_name(module.names, move->scalar_result.value_name)
              << "#" << move->scalar_result.value_id
              << ",scalar_width=" << move->scalar_result.width_bytes
              << ",abi_index=" << move->abi_register.abi_register_index
              << ",abi_reg=" << move->abi_register.register_name << "]";
        };
    auto append_scalar_cmp_result_consumption =
        [&](const std::optional<PreparedF128RuntimeHelper::ScalarCmpResultConsumption>&
                consumption) {
          out << " cmp_result_consumption=";
          if (!consumption.has_value()) {
            out << "<missing>";
            return;
          }
          out << "[cmp_type=" << type_kind_name(consumption->cmp_type)
              << ",bir_result_type=" << type_kind_name(consumption->bir_result_type)
              << ",zero_test="
              << prepared_f128_cmp_result_zero_test_name(consumption->zero_test)
              << ",consumes_cmp="
              << (consumption->consumes_helper_cmp_result ? "yes" : "no")
              << ",owns_i1=" << (consumption->owns_bir_i1_result ? "yes" : "no")
              << "]";
        };
    for (const auto& helper : function_helpers.helpers) {
      out << "  f128_helper block=" << helper.block_index
          << " inst=" << helper.instruction_index
          << " family=" << prepared_f128_runtime_helper_family_name(helper.helper_family)
          << " kind=" << prepared_f128_runtime_helper_kind_name(helper.helper_kind)
          << " opcode=";
      if (helper.source_cast_opcode.has_value()) {
        out << bir::render_cast_opcode(*helper.source_cast_opcode);
      } else {
        out << bir::render_binary_opcode(helper.source_binary_opcode);
      }
      out << " callee=" << helper.callee_name
          << " source_type=" << type_kind_name(helper.source_type)
          << " result_type=" << type_kind_name(helper.result_type);
      if (helper.source_cast_opcode.has_value()) {
        out << " operand=" << maybe_value_name(module.names, helper.operand_value_name)
            << "#" << helper.operand_value_id;
      }
      out << " result=" << maybe_value_name(module.names, helper.result_value_name)
          << "#" << helper.result_value_id
          << " lhs=" << maybe_value_name(module.names, helper.lhs_value_name)
          << "#" << helper.lhs_value_id
          << " rhs=" << maybe_value_name(module.names, helper.rhs_value_name)
          << "#" << helper.rhs_value_id
          << " result_ownership="
          << prepared_f128_runtime_helper_result_ownership_name(helper.result_ownership)
          << " resources=[";
      bool printed_resource = false;
      auto append_resource = [&](std::string_view resource) {
        if (printed_resource) {
          out << ",";
        }
        out << resource;
        printed_resource = true;
      };
      if (helper.resource_policy.call_boundary) {
        append_resource("call_boundary");
      }
      if (helper.resource_policy.runtime_helper_callee) {
        append_resource("runtime_helper_callee");
      }
      if (helper.resource_policy.caller_saved_clobbers) {
        append_resource("caller_saved_clobbers");
      }
      if (helper.resource_policy.preserves_source_operation_identity) {
        append_resource("source_operation_identity");
      }
      if (!printed_resource) {
        out << "<none>";
      }
      out << "] abi_transition="
          << prepared_f128_runtime_helper_abi_transition_name(helper.abi_policy.transition)
          << " arg_bank=" << prepared_register_bank_name(helper.abi_policy.argument_bank)
          << " result_bank=" << prepared_register_bank_name(helper.abi_policy.result_bank)
          << " arg_count=" << helper.abi_policy.argument_count
          << " result_count=" << helper.abi_policy.result_count
          << " width=" << helper.abi_policy.width_bytes;
      if (!helper.clobbered_registers.empty()) {
        out << " clobbers=";
        for (std::size_t index = 0; index < helper.clobbered_registers.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          append_clobbered_register_summary(out, helper.clobbered_registers[index]);
        }
      }
      out << " carriers";
      append_carrier("lhs", helper.lhs_carrier);
      append_carrier("rhs", helper.rhs_carrier);
      append_carrier("result", helper.result_carrier);
      out << " abi_bindings";
      append_abi_binding("lhs", helper.lhs_abi_argument);
      append_abi_binding("rhs", helper.rhs_abi_argument);
      if (helper.scalar_operand_abi_argument.has_value()) {
        append_abi_binding("scalar_operand", helper.scalar_operand_abi_argument);
      }
      append_abi_binding("result", helper.result_abi_result);
      out << " marshaling";
      append_marshaling_move("lhs", helper.lhs_argument_move);
      append_marshaling_move("rhs", helper.rhs_argument_move);
      if (helper.scalar_operand_argument_move.has_value()) {
        append_scalar_marshaling_move("scalar_operand", helper.scalar_operand_argument_move);
      }
      append_marshaling_move("result", helper.result_unmarshal_move);
      if (helper.scalar_result_unmarshal_move.has_value()) {
        append_scalar_marshaling_move("scalar_result", helper.scalar_result_unmarshal_move);
      }
      if (helper.scalar_operand.has_value()) {
        append_scalar_result("scalar_operand", helper.scalar_operand);
      }
      if (helper.scalar_result.has_value()) {
        append_scalar_result("scalar_result", helper.scalar_result);
      }
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison) {
        append_scalar_cmp_result_consumption(helper.scalar_cmp_result_consumption);
      }
      out << " live_preservation=[evaluated="
          << (helper.live_preservation_policy.evaluated ? "yes" : "no")
          << ",caller_saved_clobbers="
          << (helper.live_preservation_policy.caller_saved_clobbers_modeled ? "yes" : "no")
          << ",additional="
          << (helper.live_preservation_policy.no_additional_live_preservation_required
                  ? "none"
                  : "required")
          << ",preserved=" << helper.live_preservation_policy.preserved_values.size()
          << "]";
      if (!helper.live_preservation_policy.preserved_values.empty()) {
        out << " preserved_values=[";
        for (std::size_t index = 0;
             index < helper.live_preservation_policy.preserved_values.size();
             ++index) {
          if (index != 0) {
            out << ",";
          }
          append_preserved_value_summary(
              out, module.names, helper.live_preservation_policy.preserved_values[index]);
        }
        out << "]";
      }
      out << " selected_call_ownership=[owns_terminal_call="
          << (helper.selected_call_ownership.owns_terminal_call ? "yes" : "no")
          << ",callee=" << (helper.selected_call_ownership.has_callee_identity ? "yes" : "no")
          << ",resources=" << (helper.selected_call_ownership.has_resource_policy ? "yes" : "no")
          << ",clobbers=" << (helper.selected_call_ownership.has_clobber_policy ? "yes" : "no")
          << ",abi_bindings=" << (helper.selected_call_ownership.has_abi_bindings ? "yes" : "no")
          << ",marshaling=" << (helper.selected_call_ownership.has_marshaling ? "yes" : "no")
          << ",live_preservation="
          << (helper.selected_call_ownership.has_live_preservation ? "yes" : "no")
          << "]";
      if (!helper.missing_required_facts.empty()) {
        out << " missing_facts=";
        for (std::size_t index = 0; index < helper.missing_required_facts.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          out << helper.missing_required_facts[index];
        }
      }
      out << "\n";
    }
    for (const auto& fact : function_helpers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

void append_i128_runtime_helpers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace runtime_helper_printer;

  out << "--- prepared-i128-runtime-helpers ---\n";
  auto append_lane = [&](std::string_view label,
                         const std::optional<PreparedI128RuntimeHelper::LaneBinding>& lane) {
    out << " " << label << "=";
    if (!lane.has_value()) {
      out << "<missing>";
      return;
    }
    out << maybe_value_name(module.names, lane->value_name)
        << "#" << lane->value_id
        << ":" << prepared_i128_lane_role_name(lane->role)
        << "[index=" << lane->lane_index
        << ",width=" << lane->width_bytes
        << ",carrier=" << prepared_i128_carrier_kind_name(lane->carrier_kind);
    if (lane->register_name.has_value()) {
      out << ",reg=" << *lane->register_name;
    }
    if (lane->slot_id.has_value()) {
      out << ",slot=#" << *lane->slot_id;
    }
    if (lane->stack_offset_bytes.has_value()) {
      out << ",stack_offset=" << *lane->stack_offset_bytes;
    }
    out << "]";
  };
  auto append_scalar =
      [&](std::string_view label,
          const std::optional<PreparedI128RuntimeHelper::ScalarValueOwnership>& scalar) {
        out << " " << label << "=";
        if (!scalar.has_value()) {
          out << "<none>";
          return;
        }
        out << maybe_value_name(module.names, scalar->value_name)
            << "#" << scalar->value_id
            << "[type=" << type_kind_name(scalar->type)
            << ",width=" << scalar->width_bytes
            << ",bank=" << prepared_register_bank_name(scalar->register_bank)
            << ",home=" << prepared_value_home_kind_name(scalar->home_kind);
        if (scalar->register_name.has_value()) {
          out << ",reg=" << *scalar->register_name;
        }
        if (scalar->slot_id.has_value()) {
          out << ",slot=#" << *scalar->slot_id;
        }
        if (scalar->stack_offset_bytes.has_value()) {
          out << ",stack_offset=" << *scalar->stack_offset_bytes;
        }
        out << "]";
      };
  auto append_abi_binding =
      [&](std::string_view label,
          const std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>& binding) {
        out << " " << label << "=";
        if (!binding.has_value()) {
          out << "<missing>";
          return;
        }
        out << maybe_value_name(module.names, binding->value_name)
            << "#" << binding->value_id
            << ":" << prepared_i128_lane_role_name(binding->role)
            << "[index=" << binding->lane_index
            << ",width=" << binding->width_bytes;
        if (binding->helper_argument_index.has_value()) {
          out << ",arg=" << *binding->helper_argument_index;
        } else {
          out << ",result";
        }
        out << ",abi_index=" << binding->abi_register_index
            << ",bank=" << prepared_register_bank_name(binding->register_bank)
            << ",class=" << prepared_register_class_name(binding->register_class)
            << ",reg=" << binding->register_name
            << ",contiguous_width=" << binding->contiguous_width;
        if (!binding->occupied_register_names.empty()) {
          out << ",occupied=";
          for (std::size_t index = 0; index < binding->occupied_register_names.size(); ++index) {
            if (index != 0) {
              out << ",";
            }
            out << binding->occupied_register_names[index];
          }
        }
        out << "]";
        append_register_placement(out, "placement", binding->register_placement);
      };
  auto append_marshaling_move =
      [&](std::string_view label,
          const std::optional<PreparedI128RuntimeHelper::MarshalingMove>& move) {
        out << " " << label << "=";
        if (!move.has_value()) {
          out << "<missing>";
          return;
        }
        out << prepared_i128_runtime_helper_marshal_direction_name(move->direction)
            << "[phase=" << prepared_move_phase_name(move->phase)
            << ",op=" << move_resolution_op_kind_name(move->op_kind)
            << ",value=" << maybe_value_name(module.names, move->carrier_lane.value_name)
            << "#" << move->carrier_lane.value_id
            << ",lane=" << prepared_i128_lane_role_name(move->carrier_lane.role)
            << "#" << move->carrier_lane.lane_index
            << ",width=" << move->carrier_lane.width_bytes;
        if (move->carrier_lane.register_name.has_value()) {
          out << ",carrier_reg=" << *move->carrier_lane.register_name;
        }
        if (move->carrier_lane.slot_id.has_value()) {
          out << ",carrier_slot=#" << *move->carrier_lane.slot_id;
        }
        if (move->carrier_lane.stack_offset_bytes.has_value()) {
          out << ",carrier_stack_offset=" << *move->carrier_lane.stack_offset_bytes;
        }
        if (move->abi_register.helper_argument_index.has_value()) {
          out << ",arg=" << *move->abi_register.helper_argument_index;
        } else {
          out << ",result";
        }
        out << ",abi_index=" << move->abi_register.abi_register_index
            << ",abi_reg=" << move->abi_register.register_name << "]";
        append_register_placement(out, "abi_placement", move->abi_register.register_placement);
      };
  auto append_scalar_marshaling_move =
      [&](std::string_view label,
          const std::optional<PreparedI128RuntimeHelper::ScalarMarshalingMove>& move) {
        out << " " << label << "=";
        if (!move.has_value()) {
          out << "<missing>";
          return;
        }
        out << prepared_i128_runtime_helper_marshal_direction_name(move->direction)
            << "[phase=" << prepared_move_phase_name(move->phase)
            << ",op=" << move_resolution_op_kind_name(move->op_kind)
            << ",value=" << maybe_value_name(module.names, move->scalar_value.value_name)
            << "#" << move->scalar_value.value_id
            << ",type=" << type_kind_name(move->scalar_value.type)
            << ",width=" << move->scalar_value.width_bytes
            << ",bank=" << prepared_register_bank_name(move->scalar_value.register_bank);
        if (move->scalar_value.register_name.has_value()) {
          out << ",scalar_reg=" << *move->scalar_value.register_name;
        }
        if (move->scalar_value.slot_id.has_value()) {
          out << ",scalar_slot=#" << *move->scalar_value.slot_id;
        }
        if (move->scalar_value.stack_offset_bytes.has_value()) {
          out << ",scalar_stack_offset=" << *move->scalar_value.stack_offset_bytes;
        }
        if (move->abi_register.helper_argument_index.has_value()) {
          out << ",arg=" << *move->abi_register.helper_argument_index;
        } else {
          out << ",result";
        }
        out << ",abi_index=" << move->abi_register.abi_register_index
            << ",abi_reg=" << move->abi_register.register_name << "]";
        append_register_placement(out, "abi_placement", move->abi_register.register_placement);
      };
  for (const auto& function_helpers : module.i128_runtime_helpers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_helpers.function_name)
        << "\n";
    for (const auto& helper : function_helpers.helpers) {
      out << "  i128_helper block=" << helper.block_index
          << " inst=" << helper.instruction_index
          << " family=" << prepared_i128_runtime_helper_family_name(helper.helper_family)
          << " kind=" << prepared_i128_runtime_helper_kind_name(helper.helper_kind)
          << " opcode=";
      if (helper.source_cast_opcode.has_value()) {
        out << bir::render_cast_opcode(*helper.source_cast_opcode);
      } else {
        out << bir::render_binary_opcode(helper.source_binary_opcode);
      }
      out
          << " callee=" << helper.callee_name
          << " source_type=" << type_kind_name(helper.source_type)
          << " result_type=" << type_kind_name(helper.result_type)
          << " result=" << maybe_value_name(module.names, helper.result_value_name)
          << "#" << helper.result_value_id
          << " operand=" << maybe_value_name(module.names, helper.operand_value_name)
          << "#" << helper.operand_value_id
          << " source_width=" << helper.source_width_bytes
          << " result_width=" << helper.result_width_bytes
          << " source_signed=" << (helper.source_signed ? "yes" : "no")
          << " result_signed=" << (helper.result_signed ? "yes" : "no")
          << " lhs=" << maybe_value_name(module.names, helper.lhs_value_name)
          << "#" << helper.lhs_value_id
          << " rhs=" << maybe_value_name(module.names, helper.rhs_value_name)
          << "#" << helper.rhs_value_id
          << " result_ownership="
          << prepared_i128_runtime_helper_result_ownership_name(helper.result_ownership)
          << " memory_return=";
      if (helper.memory_return.has_value()) {
        out << maybe_value_name(module.names, helper.memory_return->destination_value_name)
            << "#" << helper.memory_return->destination_value_id
            << "[size=" << helper.memory_return->size_bytes
            << ",align=" << helper.memory_return->align_bytes;
        if (helper.memory_return->slot_id.has_value()) {
          out << ",slot=#" << *helper.memory_return->slot_id;
        }
        if (helper.memory_return->stack_offset_bytes.has_value()) {
          out << ",stack_offset=" << *helper.memory_return->stack_offset_bytes;
        }
        out << "]";
      } else {
        out << "<none>";
      }
      out << " resources=[";
      bool printed_resource = false;
      auto append_resource = [&](std::string_view resource) {
        if (printed_resource) {
          out << ",";
        }
        out << resource;
        printed_resource = true;
      };
      if (helper.resource_policy.call_boundary) {
        append_resource("call_boundary");
      }
      if (helper.resource_policy.runtime_helper_callee) {
        append_resource("runtime_helper_callee");
      }
      if (helper.resource_policy.caller_saved_clobbers) {
        append_resource("caller_saved_clobbers");
      }
      if (helper.resource_policy.preserves_source_operation_identity) {
        append_resource("source_operation_identity");
      }
      if (!printed_resource) {
        out << "<none>";
      }
      out << "]"
          << " abi_transition="
          << prepared_i128_runtime_helper_abi_transition_name(helper.abi_policy.transition)
          << " arg_bank=" << prepared_register_bank_name(helper.abi_policy.argument_bank)
          << " result_bank=" << prepared_register_bank_name(helper.abi_policy.result_bank)
          << " arg_count=" << helper.abi_policy.argument_count
          << " lanes_per_arg=" << helper.abi_policy.lanes_per_argument
          << " result_lanes=" << helper.abi_policy.result_lane_count
          << " lane_width=" << helper.abi_policy.lane_width_bytes;
      if (!helper.clobbered_registers.empty()) {
        out << " clobbers=";
        for (std::size_t index = 0; index < helper.clobbered_registers.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          append_clobbered_register_summary(out, helper.clobbered_registers[index]);
        }
      }
      out
          << " lanes";
      append_lane("lhs.low", helper.lhs_low_lane);
      append_lane("lhs.high", helper.lhs_high_lane);
      append_lane("rhs.low", helper.rhs_low_lane);
      append_lane("rhs.high", helper.rhs_high_lane);
      append_lane("result.low", helper.result_low_lane);
      append_lane("result.high", helper.result_high_lane);
      out << " scalar_ownership";
      append_scalar("operand", helper.scalar_operand);
      append_scalar("result", helper.scalar_result);
      out << " abi_bindings";
      append_abi_binding("lhs.low", helper.lhs_low_abi_argument);
      append_abi_binding("lhs.high", helper.lhs_high_abi_argument);
      append_abi_binding("rhs.low", helper.rhs_low_abi_argument);
      append_abi_binding("rhs.high", helper.rhs_high_abi_argument);
      append_abi_binding("result.low", helper.result_low_abi_result);
      append_abi_binding("result.high", helper.result_high_abi_result);
      append_abi_binding("scalar.operand", helper.scalar_operand_abi_argument);
      append_abi_binding("scalar.result", helper.scalar_result_abi_result);
      out << " marshaling";
      append_marshaling_move("lhs.low", helper.lhs_low_argument_move);
      append_marshaling_move("lhs.high", helper.lhs_high_argument_move);
      append_marshaling_move("rhs.low", helper.rhs_low_argument_move);
      append_marshaling_move("rhs.high", helper.rhs_high_argument_move);
      append_marshaling_move("result.low", helper.result_low_unmarshal_move);
      append_marshaling_move("result.high", helper.result_high_unmarshal_move);
      append_scalar_marshaling_move("scalar.operand", helper.scalar_operand_argument_move);
      append_scalar_marshaling_move("scalar.result", helper.scalar_result_unmarshal_move);
      out << " live_preservation=[evaluated="
          << (helper.live_preservation_policy.evaluated ? "yes" : "no")
          << ",caller_saved_clobbers="
          << (helper.live_preservation_policy.caller_saved_clobbers_modeled ? "yes" : "no")
          << ",additional="
          << (helper.live_preservation_policy.no_additional_live_preservation_required
                  ? "none"
                  : "required")
          << ",preserved=" << helper.live_preservation_policy.preserved_values.size()
          << "]";
      if (!helper.live_preservation_policy.preserved_values.empty()) {
        out << " preserved_values=[";
        for (std::size_t index = 0;
             index < helper.live_preservation_policy.preserved_values.size();
             ++index) {
          if (index != 0) {
            out << ",";
          }
          append_preserved_value_summary(
              out, module.names, helper.live_preservation_policy.preserved_values[index]);
        }
        out << "]";
      }
      out << " selected_call_ownership=[owns_terminal_call="
          << (helper.selected_call_ownership.owns_terminal_call ? "yes" : "no")
          << ",callee=" << (helper.selected_call_ownership.has_callee_identity ? "yes" : "no")
          << ",resources=" << (helper.selected_call_ownership.has_resource_policy ? "yes" : "no")
          << ",clobbers=" << (helper.selected_call_ownership.has_clobber_policy ? "yes" : "no")
          << ",abi_bindings=" << (helper.selected_call_ownership.has_abi_bindings ? "yes" : "no")
          << ",marshaling=" << (helper.selected_call_ownership.has_marshaling ? "yes" : "no")
          << ",live_preservation="
          << (helper.selected_call_ownership.has_live_preservation ? "yes" : "no")
          << "]";
      if (!helper.missing_required_facts.empty()) {
        out << " missing_facts=";
        for (std::size_t index = 0; index < helper.missing_required_facts.size(); ++index) {
          if (index != 0) {
            out << ",";
          }
          out << helper.missing_required_facts[index];
        }
      }
      out << "\n";
    }
    for (const auto& fact : function_helpers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}


}  // namespace c4c::backend::prepare
