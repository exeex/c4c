#include "private.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

namespace variadic_printer {

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

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string optional_size_text(const std::optional<std::size_t>& value) {
  return value.has_value() ? std::to_string(*value) : std::string("<unknown>");
}

std::string optional_signed_size_text(const std::optional<std::ptrdiff_t>& value) {
  return value.has_value() ? std::to_string(*value) : std::string("<unknown>");
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

}  // namespace variadic_printer

void append_variadic_entry_plans(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace variadic_printer;

  out << "--- prepared-variadic-entry-plans ---\n";
  for (const auto& function_plan : module.variadic_entry_plans.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << " named_params=" << function_plan.named_parameter_count
        << " named_gp=" << optional_size_text(function_plan.named_register_counts.gp)
        << " named_fp=" << optional_size_text(function_plan.named_register_counts.fp)
        << " helpers=" << function_plan.helper_resources.required_helpers.size()
        << "\n";

    out << "  register_save_area required="
        << (function_plan.register_save_area.required ? "yes" : "no")
        << " size=" << optional_size_text(function_plan.register_save_area.size_bytes)
        << " align=" << optional_size_text(function_plan.register_save_area.align_bytes)
        << " slot=";
    if (function_plan.register_save_area.slot_id.has_value()) {
      out << "#" << *function_plan.register_save_area.slot_id;
    } else {
      out << "<none>";
    }
    out << " stack_offset="
        << optional_size_text(function_plan.register_save_area.stack_offset_bytes)
        << " gp_offset="
        << optional_size_text(function_plan.register_save_area.gp_offset_bytes)
        << " fp_offset="
        << optional_size_text(function_plan.register_save_area.fp_offset_bytes)
        << " gp_slot="
        << optional_size_text(function_plan.register_save_area.gp_slot_size_bytes)
        << " fp_slot="
        << optional_size_text(function_plan.register_save_area.fp_slot_size_bytes)
        << " saved_gp="
        << optional_size_text(function_plan.register_save_area.saved_gp_register_count)
        << " saved_fp="
        << optional_size_text(function_plan.register_save_area.saved_fp_register_count)
        << " initial_gp_offset="
        << optional_signed_size_text(function_plan.register_save_area.initial_gp_offset_bytes)
        << " initial_fp_offset="
        << optional_signed_size_text(function_plan.register_save_area.initial_fp_offset_bytes)
        << "\n";

    out << "  overflow_area required="
        << (function_plan.overflow_area.required ? "yes" : "no")
        << " base_slot=";
    if (function_plan.overflow_area.base_slot_id.has_value()) {
      out << "#" << *function_plan.overflow_area.base_slot_id;
    } else {
      out << "<none>";
    }
    out << " base_stack_offset="
        << optional_size_text(function_plan.overflow_area.base_stack_offset_bytes)
        << " align=" << optional_size_text(function_plan.overflow_area.align_bytes)
        << "\n";

    out << "  va_list_layout required="
        << (function_plan.va_list_layout.required ? "yes" : "no")
        << " size=" << optional_size_text(function_plan.va_list_layout.size_bytes)
        << " align=" << optional_size_text(function_plan.va_list_layout.align_bytes)
        << " fields=" << function_plan.va_list_layout.fields.size()
        << "\n";
    for (const auto& field : function_plan.va_list_layout.fields) {
      out << "    field kind="
          << prepared_variadic_va_list_field_kind_name(field.kind)
          << " offset=" << field.offset_bytes
          << " size=" << field.size_bytes
          << "\n";
    }

    out << "  helper_resources scratch_registers="
        << optional_size_text(function_plan.helper_resources.scratch_register_count)
        << " scratch_stack="
        << optional_size_text(function_plan.helper_resources.scratch_stack_bytes)
        << " helpers=[";
    for (std::size_t index = 0;
         index < function_plan.helper_resources.required_helpers.size();
         ++index) {
      if (index != 0) {
        out << ",";
      }
      out << prepared_variadic_entry_helper_kind_name(
          function_plan.helper_resources.required_helpers[index]);
    }
    out << "]"
        << "\n";
    for (const auto helper : function_plan.helper_resources.required_helpers) {
      out << "    helper kind="
          << prepared_variadic_entry_helper_kind_name(helper)
          << "\n";
    }
    for (const auto& homes : function_plan.helper_operand_homes) {
      out << "    helper_operand kind="
          << prepared_variadic_entry_helper_kind_name(homes.helper)
          << " block=" << homes.block_index
          << " inst=" << homes.instruction_index;
      const auto append_home = [&](std::string_view label,
                                   const std::optional<PreparedValueHome>& home) {
        if (!home.has_value()) {
          out << " " << label << "=<none>";
          return;
        }
        out << " " << label << "=" << maybe_value_name(module.names, home->value_name)
            << ":" << prepared_value_home_kind_name(home->kind);
        if (home->register_name.has_value()) {
          out << ":reg=" << *home->register_name;
        }
        if (home->slot_id.has_value()) {
          out << ":slot=#" << *home->slot_id;
        }
        if (home->offset_bytes.has_value()) {
          out << ":offset=" << *home->offset_bytes;
        }
      };
      append_home("dst_va_list", homes.destination_va_list);
      append_home("dst_va_list_addr", homes.destination_va_list_address);
      append_home("src_va_list", homes.source_va_list);
      append_home("scalar_result", homes.scalar_result);
      append_home("aggregate_dst", homes.aggregate_destination_payload);
      if (homes.helper == PreparedVariadicEntryHelperKind::VaArg) {
        out << " scalar_access_plan=";
        if (!homes.scalar_access_plan.has_value()) {
          out << "<none>";
        } else {
          const auto& plan = *homes.scalar_access_plan;
          out << "source_class="
              << prepared_variadic_scalar_va_arg_source_class_name(plan.source_class)
              << ":type=" << type_kind_name(plan.value_type)
              << ":size=" << plan.value_size_bytes
              << ":align=" << plan.value_align_bytes;
          if (plan.result_home.has_value()) {
            out << ":result_home="
                << maybe_value_name(module.names, plan.result_home->value_name)
                << "/" << prepared_value_home_kind_name(plan.result_home->kind);
          }
          if (plan.source_field.has_value()) {
            out << ":source_field="
                << prepared_variadic_va_list_field_kind_name(*plan.source_field);
            if (plan.source_field_offset_bytes.has_value()) {
              out << "@" << *plan.source_field_offset_bytes;
            }
          }
          if (plan.source_slot_size_bytes.has_value()) {
            out << ":source_slot=" << *plan.source_slot_size_bytes;
          }
          if (plan.progression_field.has_value()) {
            out << ":progression_field="
                << prepared_variadic_va_list_field_kind_name(*plan.progression_field);
            if (plan.progression_field_offset_bytes.has_value()) {
              out << "@" << *plan.progression_field_offset_bytes;
            }
          }
          if (plan.progression_stride_bytes.has_value()) {
            out << ":progression_stride=" << *plan.progression_stride_bytes;
          }
          if (plan.overflow_source_field.has_value()) {
            out << ":overflow_source_field="
                << prepared_variadic_va_list_field_kind_name(*plan.overflow_source_field);
            if (plan.overflow_source_field_offset_bytes.has_value()) {
              out << "@" << *plan.overflow_source_field_offset_bytes;
            }
          }
          if (plan.overflow_stride_bytes.has_value()) {
            out << ":overflow_stride=" << *plan.overflow_stride_bytes;
          }
        }
      } else if (homes.helper == PreparedVariadicEntryHelperKind::VaArgAggregate) {
        out << " aggregate_access_plan=";
        if (!homes.aggregate_access_plan.has_value()) {
          out << "<none>";
        } else {
          const auto& plan = *homes.aggregate_access_plan;
          out << "source_class="
              << prepared_variadic_aggregate_va_arg_source_class_name(plan.source_class)
              << ":payload_size=" << plan.payload_size_bytes
              << ":payload_align=" << plan.payload_align_bytes;
          if (plan.destination_payload_home.has_value()) {
            out << ":destination_payload="
                << maybe_value_name(module.names,
                                    plan.destination_payload_home->value_name)
                << "/"
                << prepared_value_home_kind_name(
                       plan.destination_payload_home->kind);
          }
          if (plan.payload_write_address.has_value()) {
            out << ":payload_write_address="
                << maybe_value_name(module.names,
                                    plan.payload_write_address->result_value_name)
                << "/frame_slot:slot=#"
                << plan.payload_write_address->frame_slot_id
                << ":offset="
                << plan.payload_write_address->stack_offset_bytes
                << ":materialization_block="
                << maybe_block_label(
                       module.names,
                       plan.payload_write_address->materialization_block_label)
                << ":materialization_inst="
                << plan.payload_write_address->materialization_instruction_index;
          }
          if (plan.source_field.has_value()) {
            out << ":source_field="
                << prepared_variadic_va_list_field_kind_name(*plan.source_field);
            if (plan.source_field_offset_bytes.has_value()) {
              out << "@" << *plan.source_field_offset_bytes;
            }
          }
          if (plan.source_payload_offset_bytes.has_value()) {
            out << ":source_payload_offset="
                << *plan.source_payload_offset_bytes;
          }
          if (plan.source_slot_size_bytes.has_value()) {
            out << ":source_slot=" << *plan.source_slot_size_bytes;
          }
          if (plan.copy_size_bytes.has_value()) {
            out << ":copy_size=" << *plan.copy_size_bytes;
          }
          if (plan.copy_align_bytes.has_value()) {
            out << ":copy_align=" << *plan.copy_align_bytes;
          }
          if (plan.progression_field.has_value()) {
            out << ":progression_field="
                << prepared_variadic_va_list_field_kind_name(*plan.progression_field);
            if (plan.progression_field_offset_bytes.has_value()) {
              out << "@" << *plan.progression_field_offset_bytes;
            }
          }
          if (plan.progression_stride_bytes.has_value()) {
            out << ":progression_stride=" << *plan.progression_stride_bytes;
          }
          if (plan.overflow_source_field_offset_bytes.has_value()) {
            out << ":overflow_source_field=overflow_arg_area@"
                << *plan.overflow_source_field_offset_bytes;
          }
          if (plan.overflow_stride_bytes.has_value()) {
            out << ":overflow_stride=" << *plan.overflow_stride_bytes;
          }
          if (plan.register_save_lane_count.has_value()) {
            out << ":register_save_lanes=" << *plan.register_save_lane_count;
          }
          if (plan.register_save_lane_size_bytes.has_value()) {
            out << ":register_save_lane_size="
                << *plan.register_save_lane_size_bytes;
          }
        }
      }
      out << "\n";
    }
    for (const auto& fact : function_plan.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
