#include "variadic.hpp"
#include "constant_materialization.hpp"
#include "dispatch_publication.hpp"
#include "memory.hpp"
#include "mir/printer.hpp"

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string variadic_entry_missing_fact_message(
    const std::vector<std::string>& missing) {
  std::string message =
      "AArch64 variadic entry helper lowering requires complete prepared variadic entry facts";
  if (!missing.empty()) {
    message += "; missing fact=";
    message += missing.front();
  }
  return message;
}

[[nodiscard]] std::string variadic_entry_missing_fact_message(std::string_view missing) {
  std::string message =
      "AArch64 variadic entry helper lowering requires complete prepared variadic entry facts";
  if (!missing.empty()) {
    message += "; missing fact=";
    message += missing;
  }
  return message;
}

[[nodiscard]] std::string_view missing_variadic_entry_helper_consumption_fact(
    const prepare::PreparedVariadicEntryPlanFunction& entry) {
  if (entry.helper_resources.required_helpers.empty()) {
    return {};
  }
  if (!entry.helper_resources.scratch_register_count.has_value()) {
    return "helper_resources.scratch_register_count";
  }
  if (!entry.helper_resources.scratch_stack_bytes.has_value()) {
    return "helper_resources.scratch_stack_bytes";
  }
  if (entry.helper_operand_homes.empty()) {
    return "helper_operand_homes";
  }
  return {};
}

[[nodiscard]] std::optional<VariadicVaStartRecord> make_variadic_va_start_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !homes.destination_va_list_address.has_value() ||
      !entry.named_register_counts.gp.has_value() ||
      !entry.named_register_counts.fp.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.register_save_area.saved_gp_register_count.has_value() ||
      !entry.register_save_area.saved_fp_register_count.has_value() ||
      !entry.register_save_area.initial_gp_offset_bytes.has_value() ||
      !entry.register_save_area.initial_fp_offset_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicVaStartRecord{
      .destination_va_list = *homes.destination_va_list,
      .destination_va_list_address = *homes.destination_va_list_address,
      .named_gp_register_count = *entry.named_register_counts.gp,
      .named_fp_register_count = *entry.named_register_counts.fp,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .va_list_fields = entry.va_list_layout.fields,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .saved_gp_register_count = *entry.register_save_area.saved_gp_register_count,
      .saved_fp_register_count = *entry.register_save_area.saved_fp_register_count,
      .initial_gp_offset_bytes = *entry.register_save_area.initial_gp_offset_bytes,
      .initial_fp_offset_bytes = *entry.register_save_area.initial_fp_offset_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

[[nodiscard]] std::optional<VariadicScalarVaArgRecord>
make_variadic_scalar_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.source_va_list.has_value() || !homes.scalar_result.has_value() ||
      !homes.scalar_access_plan.has_value()) {
    return std::nullopt;
  }
  const auto& plan = *homes.scalar_access_plan;
  if (plan.source_class == prepare::PreparedVariadicScalarVaArgSourceClass::Unknown ||
      plan.value_type == bir::TypeKind::Void ||
      plan.value_size_bytes == 0 ||
      plan.value_align_bytes == 0 ||
      !plan.result_home.has_value() ||
      !plan.source_field.has_value() ||
      !plan.source_field_offset_bytes.has_value() ||
      !plan.source_slot_size_bytes.has_value() ||
      !plan.progression_field.has_value() ||
      !plan.progression_field_offset_bytes.has_value() ||
      !plan.progression_stride_bytes.has_value() ||
      !plan.overflow_source_field.has_value() ||
      !plan.overflow_source_field_offset_bytes.has_value() ||
      !plan.overflow_stride_bytes.has_value() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicScalarVaArgRecord{
      .source_class = plan.source_class,
      .value_type = plan.value_type,
      .value_size_bytes = plan.value_size_bytes,
      .value_align_bytes = plan.value_align_bytes,
      .source_va_list = *homes.source_va_list,
      .result_home = *plan.result_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .overflow_source_field = *plan.overflow_source_field,
      .overflow_source_field_offset_bytes = *plan.overflow_source_field_offset_bytes,
      .overflow_stride_bytes = *plan.overflow_stride_bytes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

[[nodiscard]] std::optional<VariadicAggregateVaArgRecord>
make_variadic_aggregate_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes)) {
    return std::nullopt;
  }
  const auto& plan = *homes.aggregate_access_plan;
  if (!entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicAggregateVaArgRecord{
      .source_class = plan.source_class,
      .function_name_id = entry.function_name,
      .block_index = plan.block_index,
      .instruction_index = plan.instruction_index,
      .payload_size_bytes = plan.payload_size_bytes,
      .payload_align_bytes = plan.payload_align_bytes,
      .source_va_list = *homes.source_va_list,
      .destination_payload_home = *plan.destination_payload_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_payload_offset_bytes = *plan.source_payload_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .copy_size_bytes = *plan.copy_size_bytes,
      .copy_align_bytes = *plan.copy_align_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .overflow_source_field_offset_bytes =
          plan.overflow_source_field_offset_bytes.value_or(std::size_t{0}),
      .overflow_stride_bytes =
          plan.overflow_stride_bytes.value_or(plan.payload_size_bytes),
      .register_save_lane_count =
          plan.register_save_lane_count.value_or(std::size_t{0}),
      .register_save_lane_size_bytes =
          plan.register_save_lane_size_bytes.value_or(std::size_t{0}),
      .register_save_lane_destination_homes =
          plan.register_save_lane_destination_homes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

[[nodiscard]] std::optional<VariadicVaCopyRecord> make_variadic_va_copy_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !homes.source_va_list.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  std::vector<VariadicVaCopyFieldRecord> field_copies;
  field_copies.reserve(entry.va_list_layout.fields.size());
  for (const auto& field : entry.va_list_layout.fields) {
    if (field.size_bytes == 0) {
      return std::nullopt;
    }
    field_copies.push_back(VariadicVaCopyFieldRecord{
        .kind = field.kind,
        .source_offset_bytes = field.offset_bytes,
        .destination_offset_bytes = field.offset_bytes,
        .size_bytes = field.size_bytes,
    });
  }

  return VariadicVaCopyRecord{
      .destination_va_list = *homes.destination_va_list,
      .source_va_list = *homes.source_va_list,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .field_copies = std::move(field_copies),
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

[[nodiscard]] mir::TargetInstructionPrintResult target_unsupported(
    std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

[[nodiscard]] mir::TargetInstructionPrintResult target_printed(
    std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

[[nodiscard]] std::string prepared_value_home_name(
    const prepare::PreparedValueHome& home) {
  std::ostringstream out;
  out << "value#" << home.value_id << ":"
      << prepare::prepared_value_home_kind_name(home.kind);
  if (home.register_name.has_value()) {
    out << ":" << *home.register_name;
  }
  if (home.slot_id.has_value()) {
    out << ":slot#" << *home.slot_id;
  }
  if (home.offset_bytes.has_value()) {
    out << ":offset+" << *home.offset_bytes;
  }
  return out.str();
}

[[nodiscard]] std::optional<unsigned> parse_x_register_index(
    const std::optional<std::string>& name) {
  if (!name.has_value() || name->size() < 2 || (*name)[0] != 'x') {
    return std::nullopt;
  }
  unsigned index = 0;
  for (std::size_t pos = 1; pos < name->size(); ++pos) {
    const char ch = (*name)[pos];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    index = index * 10U + static_cast<unsigned>(ch - '0');
  }
  return index <= 30U ? std::optional<unsigned>{index} : std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> va_start_scratch_register(
    const prepare::PreparedValueHome& destination) {
  const auto destination_index = parse_x_register_index(destination.register_name);
  if (!destination_index.has_value()) {
    return std::nullopt;
  }
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (scratch.index != *destination_index) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedVariadicVaListField* find_va_list_field(
    const VariadicVaStartRecord& va_start,
    prepare::PreparedVariadicVaListFieldKind kind) {
  for (const auto& field : va_start.va_list_fields) {
    if (field.kind == kind) {
      return &field;
    }
  }
  return nullptr;
}

[[nodiscard]] std::string va_list_field_address(
    std::string_view destination_register,
    const prepare::PreparedVariadicVaListField& field) {
  std::ostringstream out;
  out << "[" << destination_register;
  if (field.offset_bytes != 0) {
    out << ", #" << field.offset_bytes;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::vector<std::string> materialize_stack_address_lines(
    abi::RegisterReference scratch,
    std::size_t stack_offset_bytes) {
  const std::string scratch_name = abi::register_name(scratch);
  if (stack_offset_bytes <= 4095U) {
    return {"add " + scratch_name + ", sp, #" + std::to_string(stack_offset_bytes)};
  }
  auto lines = materialize_integer_constant_lines(
      scratch, static_cast<std::uint64_t>(stack_offset_bytes), 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + scratch_name + ", sp, " + scratch_name);
  return lines;
}

bool append_va_start_pointer_field_store(
    std::vector<std::string>& lines,
    const VariadicVaStartRecord& va_start,
    prepare::PreparedVariadicVaListFieldKind kind,
    std::size_t stack_offset_bytes,
    abi::RegisterReference scratch,
    std::string_view destination_register) {
  const auto* field = find_va_list_field(va_start, kind);
  if (field == nullptr) {
    return true;
  }
  if (field->size_bytes != 8) {
    return false;
  }
  auto address_lines = materialize_stack_address_lines(scratch, stack_offset_bytes);
  if (address_lines.empty()) {
    return false;
  }
  lines.insert(lines.end(), address_lines.begin(), address_lines.end());
  lines.push_back("str " + std::string{abi::register_name(scratch)} + ", " +
                  va_list_field_address(destination_register, *field));
  return true;
}

bool append_va_start_i32_field_store(std::vector<std::string>& lines,
                                     const VariadicVaStartRecord& va_start,
                                     prepare::PreparedVariadicVaListFieldKind kind,
                                     std::ptrdiff_t value,
                                     abi::RegisterReference scratch,
                                     std::string_view destination_register) {
  const auto* field = find_va_list_field(va_start, kind);
  if (field == nullptr) {
    return true;
  }
  if (field->size_bytes != 4) {
    return false;
  }
  const auto scratch_w = abi::w_register(scratch.index);
  auto value_lines = materialize_integer_constant_lines(
      scratch_w, static_cast<std::uint32_t>(value), 32);
  if (value_lines.empty()) {
    return false;
  }
  lines.insert(lines.end(), value_lines.begin(), value_lines.end());
  lines.push_back("str " + std::string{abi::register_name(scratch_w)} + ", " +
                  va_list_field_address(destination_register, *field));
  return true;
}

void append_va_start_register_save_area_lines(std::vector<std::string>& lines,
                                              const VariadicVaStartRecord& va_start) {
  const auto stack_address_text = [](std::size_t stack_offset) {
    std::ostringstream out;
    out << "[sp";
    if (stack_offset != 0) {
      out << ", #" << stack_offset;
    }
    out << "]";
    return out.str();
  };
  for (std::size_t reg_index = va_start.named_gp_register_count; reg_index < 8;
       ++reg_index) {
    const std::size_t saved_index = reg_index - va_start.named_gp_register_count;
    const std::size_t stack_offset =
        va_start.register_save_area_stack_offset_bytes +
        va_start.register_save_area_gp_offset_bytes +
        saved_index * va_start.register_save_area_gp_slot_size_bytes;
    lines.push_back("str x" + std::to_string(reg_index) + ", " +
                    stack_address_text(stack_offset));
  }
  for (std::size_t reg_index = va_start.named_fp_register_count; reg_index < 8;
       ++reg_index) {
    const std::size_t saved_index = reg_index - va_start.named_fp_register_count;
    const std::size_t stack_offset =
        va_start.register_save_area_stack_offset_bytes +
        va_start.register_save_area_fp_offset_bytes +
        saved_index * va_start.register_save_area_fp_slot_size_bytes;
    lines.push_back("str q" + std::to_string(reg_index) + ", " +
                    stack_address_text(stack_offset));
  }
}

[[nodiscard]] std::optional<std::vector<std::string>> print_va_start_lowering_lines(
    const VariadicVaStartRecord& va_start) {
  if (va_start.destination_va_list.kind != prepare::PreparedValueHomeKind::Register ||
      !va_start.destination_va_list.register_name.has_value() ||
      va_start.destination_va_list_address.kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      !va_start.destination_va_list_address.offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto scratch = va_start_scratch_register(va_start.destination_va_list);
  if (!scratch.has_value()) {
    return std::nullopt;
  }

  const std::string destination_register = *va_start.destination_va_list.register_name;
  const auto destination_index = parse_x_register_index(destination_register);
  if (!destination_index.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  auto destination_lines = materialize_stack_address_lines(
      abi::x_register(*destination_index),
      *va_start.destination_va_list_address.offset_bytes);
  if (destination_lines.empty()) {
    return std::nullopt;
  }
  lines.insert(lines.end(), destination_lines.begin(), destination_lines.end());
  append_va_start_register_save_area_lines(lines, va_start);
  const auto gp_top_stack_offset =
      va_start.register_save_area_stack_offset_bytes +
      va_start.register_save_area_gp_offset_bytes +
      va_start.saved_gp_register_count * va_start.register_save_area_gp_slot_size_bytes;
  const auto fp_top_stack_offset =
      va_start.register_save_area_stack_offset_bytes +
      va_start.register_save_area_fp_offset_bytes +
      va_start.saved_fp_register_count * va_start.register_save_area_fp_slot_size_bytes;

  if (!append_va_start_pointer_field_store(
          lines,
          va_start,
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          va_start.overflow_area_base_stack_offset_bytes,
          *scratch,
          destination_register) ||
      !append_va_start_pointer_field_store(
          lines,
          va_start,
          prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea,
          gp_top_stack_offset,
          *scratch,
          destination_register) ||
      !append_va_start_pointer_field_store(
          lines,
          va_start,
          prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea,
          fp_top_stack_offset,
          *scratch,
          destination_register) ||
      !append_va_start_i32_field_store(
          lines,
          va_start,
          prepare::PreparedVariadicVaListFieldKind::GpOffset,
          va_start.initial_gp_offset_bytes,
          *scratch,
          destination_register) ||
      !append_va_start_i32_field_store(
          lines,
          va_start,
          prepare::PreparedVariadicVaListFieldKind::FpOffset,
          va_start.initial_fp_offset_bytes,
          *scratch,
          destination_register)) {
    return std::nullopt;
  }
  return lines;
}

[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t offset_bytes) {
  std::ostringstream out;
  out << "[" << base;
  if (offset_bytes != 0) {
    out << ", #" << offset_bytes;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool unsigned_scaled_offset_is_encodable(std::size_t offset_bytes,
                                                       std::size_t width_bytes) {
  if (width_bytes == 0) {
    return false;
  }
  return offset_bytes % width_bytes == 0 && offset_bytes / width_bytes <= 4095U;
}

[[nodiscard]] bool unsigned_byte_offset_is_encodable(std::size_t offset_bytes) {
  return offset_bytes <= 4095U;
}

[[nodiscard]] bool unsigned_memory_offset_is_encodable(std::size_t offset_bytes,
                                                       std::size_t width_bytes) {
  if (width_bytes == 1) {
    return unsigned_byte_offset_is_encodable(offset_bytes);
  }
  return unsigned_scaled_offset_is_encodable(offset_bytes, width_bytes);
}

[[nodiscard]] std::string stack_address(std::size_t offset_bytes) {
  return register_indirect_address("sp", offset_bytes);
}

[[nodiscard]] std::vector<std::size_t> aggregate_copy_chunks(
    std::size_t size_bytes) {
  std::vector<std::size_t> chunks;
  std::size_t remaining = size_bytes;
  while (remaining >= 8) {
    chunks.push_back(8);
    remaining -= 8;
  }
  if (remaining >= 4) {
    chunks.push_back(4);
    remaining -= 4;
  }
  while (remaining > 0) {
    chunks.push_back(1);
    --remaining;
  }
  return chunks;
}

[[nodiscard]] std::size_t align_aggregate_offset(std::size_t value,
                                                 std::size_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::string_view aggregate_copy_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

[[nodiscard]] std::string_view aggregate_copy_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

[[nodiscard]] abi::RegisterReference aggregate_copy_data_register(
    abi::RegisterReference scratch,
    std::size_t width_bytes) {
  if (width_bytes == 1 || width_bytes == 4) {
    return abi::w_register(scratch.index);
  }
  return abi::x_register(scratch.index);
}

bool append_add_unsigned_immediate(std::vector<std::string>& lines,
                                   abi::RegisterReference destination,
                                   abi::RegisterReference scratch,
                                   std::size_t value) {
  if (value == 0) {
    return true;
  }
  const std::string destination_name = abi::register_name(destination);
  if (value <= 4095U) {
    lines.push_back("add " + destination_name + ", " + destination_name +
                    ", #" + std::to_string(value));
    return true;
  }
  auto materialized = materialize_integer_constant_lines(
      scratch, static_cast<std::uint64_t>(value), 64);
  if (materialized.empty()) {
    return false;
  }
  lines.insert(lines.end(), materialized.begin(), materialized.end());
  lines.push_back("add " + destination_name + ", " + destination_name +
                  ", " + std::string{abi::register_name(scratch)});
  return true;
}

[[nodiscard]] std::optional<std::vector<std::string>>
materialize_aggregate_destination_address_lines(abi::RegisterReference scratch,
                                                std::size_t stack_offset_bytes) {
  auto lines = materialize_stack_address_lines(scratch, stack_offset_bytes);
  if (lines.empty()) {
    return std::nullopt;
  }
  return lines;
}

bool append_aggregate_source_address_lines(std::vector<std::string>& lines,
                                           const VariadicAggregateVaArgRecord& va_arg,
                                           abi::RegisterReference source_scratch,
                                           abi::RegisterReference add_scratch,
                                           std::size_t copy_offset_bytes) {
  if (va_arg.source_va_list.kind != prepare::PreparedValueHomeKind::Register ||
      !va_arg.source_va_list.register_name.has_value()) {
    return false;
  }
  if (va_arg.source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea &&
      va_arg.source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::RegisterSaveArea) {
    return false;
  }
  const std::string source_name = abi::register_name(source_scratch);
  lines.push_back("ldr " + source_name + ", " +
                  register_indirect_address(*va_arg.source_va_list.register_name,
                                            va_arg.source_field_offset_bytes));
  return append_add_unsigned_immediate(lines,
                                       source_scratch,
                                       add_scratch,
                                       va_arg.source_payload_offset_bytes +
                                           copy_offset_bytes);
}

bool append_aggregate_destination_store_to_home(
    std::vector<std::string>& lines,
    const prepare::PreparedValueHome& destination_home,
    abi::RegisterReference address_scratch,
    abi::RegisterReference data_register,
    std::size_t width_bytes,
    std::size_t copy_offset_bytes) {
  if (destination_home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !destination_home.offset_bytes.has_value()) {
    return false;
  }
  const auto stack_offset = *destination_home.offset_bytes + copy_offset_bytes;
  const auto store_mnemonic = aggregate_copy_store_mnemonic(width_bytes);
  if (store_mnemonic.empty()) {
    return false;
  }
  if (unsigned_memory_offset_is_encodable(stack_offset, width_bytes)) {
    lines.push_back(std::string{store_mnemonic} + " " +
                    std::string{abi::register_name(data_register)} + ", " +
                    stack_address(stack_offset));
    return true;
  }
  auto address_lines =
      materialize_aggregate_destination_address_lines(address_scratch, stack_offset);
  if (!address_lines.has_value()) {
    return false;
  }
  lines.insert(lines.end(), address_lines->begin(), address_lines->end());
  lines.push_back(std::string{store_mnemonic} + " " +
                  std::string{abi::register_name(data_register)} + ", [" +
                  std::string{abi::register_name(address_scratch)} + "]");
  return true;
}

bool append_aggregate_destination_store(std::vector<std::string>& lines,
                                        const VariadicAggregateVaArgRecord& va_arg,
                                        abi::RegisterReference address_scratch,
                                        abi::RegisterReference data_register,
                                        std::size_t width_bytes,
                                        std::size_t copy_offset_bytes) {
  return append_aggregate_destination_store_to_home(lines,
                                                    va_arg.destination_payload_home,
                                                    address_scratch,
                                                    data_register,
                                                    width_bytes,
                                                    copy_offset_bytes);
}

[[nodiscard]] std::string aggregate_va_arg_label(
    const VariadicAggregateVaArgRecord& va_arg,
    std::string_view suffix) {
  std::ostringstream out;
  out << ".Lva_arg_aggregate_" << va_arg.function_name_id << "_"
      << va_arg.block_index << "_" << va_arg.instruction_index << "_"
      << suffix;
  return out.str();
}

bool append_aggregate_copy_from_va_list_field(
    std::vector<std::string>& lines,
    const VariadicAggregateVaArgRecord& va_arg,
    prepare::PreparedVariadicVaListFieldKind source_field,
    std::size_t source_field_offset_bytes,
    std::size_t source_slot_size_bytes,
    std::size_t lane_size_bytes,
    std::size_t lane_count,
    std::optional<abi::RegisterReference> register_save_progression_offset,
    abi::RegisterReference source_scratch,
    abi::RegisterReference address_scratch) {
  if (va_arg.source_va_list.kind != prepare::PreparedValueHomeKind::Register ||
      !va_arg.source_va_list.register_name.has_value() ||
      lane_size_bytes == 0 ||
      lane_count == 0) {
    return false;
  }

  const bool register_save_area =
      source_field == prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea ||
      source_field == prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea;
  const bool use_lane_destination_homes =
      va_arg.register_save_lane_destination_homes.size() == lane_count;
  for (std::size_t lane_index = 0; lane_index < lane_count; ++lane_index) {
    std::size_t lane_chunk_offset = 0;
    for (const auto width_bytes : aggregate_copy_chunks(lane_size_bytes)) {
      const auto load_mnemonic = aggregate_copy_load_mnemonic(width_bytes);
      if (load_mnemonic.empty()) {
        return false;
      }
      const std::string source_name = abi::register_name(source_scratch);
      lines.push_back("ldr " + source_name + ", " +
                      register_indirect_address(
                          *va_arg.source_va_list.register_name,
                          source_field_offset_bytes));
      if (register_save_area) {
        const std::string address_name = abi::register_name(address_scratch);
        if (register_save_progression_offset.has_value()) {
          lines.push_back("add " + source_name + ", " + source_name + ", " +
                          std::string{abi::register_name(
                              *register_save_progression_offset)});
        } else {
          lines.push_back("ldrsw " + address_name + ", " +
                          register_indirect_address(
                              *va_arg.source_va_list.register_name,
                              va_arg.progression_field_offset_bytes));
          lines.push_back("add " + source_name + ", " + source_name + ", " +
                          address_name);
        }
      }
      if (!append_add_unsigned_immediate(
              lines,
              source_scratch,
              address_scratch,
              lane_index * source_slot_size_bytes + lane_chunk_offset)) {
        return false;
      }
      const auto data_register =
          aggregate_copy_data_register(source_scratch, width_bytes);
      lines.push_back(std::string{load_mnemonic} + " " +
                      std::string{abi::register_name(data_register)} + ", [" +
                      std::string{abi::register_name(source_scratch)} + "]");
      const auto& destination_home =
          use_lane_destination_homes
              ? va_arg.register_save_lane_destination_homes[lane_index]
              : va_arg.destination_payload_home;
      const std::size_t destination_offset =
          use_lane_destination_homes
              ? lane_chunk_offset
              : lane_index * lane_size_bytes + lane_chunk_offset;
      if (!append_aggregate_destination_store_to_home(lines,
                                                      destination_home,
                                                      address_scratch,
                                                      data_register,
                                                      width_bytes,
                                                      destination_offset)) {
        return false;
      }
      lane_chunk_offset += width_bytes;
    }
  }
  return true;
}

[[nodiscard]] std::optional<std::vector<std::string>>
print_aggregate_va_arg_lowering_lines(const VariadicAggregateVaArgRecord& va_arg) {
  if (va_arg.copy_size_bytes == 0 || va_arg.progression_stride_bytes == 0 ||
      va_arg.scratch_register_count < 2) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto source_scratch = abi::x_register(scratches[0].index);
  const auto address_scratch = abi::x_register(scratches[1].index);
  std::vector<std::string> lines;
  if (va_arg.source_va_list.kind != prepare::PreparedValueHomeKind::Register ||
      !va_arg.source_va_list.register_name.has_value()) {
    return std::nullopt;
  }

  if (va_arg.source_class ==
      prepare::PreparedVariadicAggregateVaArgSourceClass::RegisterSaveArea) {
    if (va_arg.register_save_lane_count == 0 ||
        va_arg.register_save_lane_size_bytes == 0 ||
        va_arg.source_slot_size_bytes == 0 ||
        va_arg.register_save_lane_count *
                va_arg.register_save_lane_size_bytes !=
            va_arg.copy_size_bytes) {
      return std::nullopt;
    }
    const auto w_source = abi::w_register(source_scratch.index);
    const auto w_address = abi::w_register(address_scratch.index);
    const std::string source_w_name = abi::register_name(w_source);
    const std::string address_w_name = abi::register_name(w_address);
    const std::string address_x_name = abi::register_name(address_scratch);
    const auto overflow_label = aggregate_va_arg_label(va_arg, "overflow");
    const auto done_label = aggregate_va_arg_label(va_arg, "done");
    lines.push_back("ldrsw " + address_x_name + ", " +
                    register_indirect_address(
                        *va_arg.source_va_list.register_name,
                        va_arg.progression_field_offset_bytes));
    lines.push_back("mov " + source_w_name + ", " + address_w_name);
    if (!append_add_unsigned_immediate(
            lines, w_source, w_address, va_arg.progression_stride_bytes)) {
      return std::nullopt;
    }
    lines.push_back("cmp " + source_w_name + ", #0");
    lines.push_back("b.gt " + overflow_label);
    if (!append_aggregate_copy_from_va_list_field(
            lines,
            va_arg,
            va_arg.source_field,
            va_arg.source_field_offset_bytes,
            va_arg.source_slot_size_bytes,
            va_arg.register_save_lane_size_bytes,
            va_arg.register_save_lane_count,
            address_scratch,
            source_scratch,
            address_scratch)) {
      return std::nullopt;
    }
    lines.push_back("ldr " + source_w_name + ", " +
                    register_indirect_address(
                        *va_arg.source_va_list.register_name,
                        va_arg.progression_field_offset_bytes));
    if (!append_add_unsigned_immediate(
            lines, w_source, w_address, va_arg.progression_stride_bytes)) {
      return std::nullopt;
    }
    lines.push_back("str " + source_w_name + ", " +
                    register_indirect_address(
                        *va_arg.source_va_list.register_name,
                        va_arg.progression_field_offset_bytes));
    lines.push_back("b " + done_label);
    lines.push_back(overflow_label + ":");
    const bool use_hfa_lane_homes =
        va_arg.register_save_lane_destination_homes.size() ==
            va_arg.register_save_lane_count &&
        va_arg.register_save_lane_size_bytes != 0;
    if (!append_aggregate_copy_from_va_list_field(
            lines,
            va_arg,
            prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
            va_arg.overflow_source_field_offset_bytes,
            use_hfa_lane_homes ? va_arg.register_save_lane_size_bytes
                               : va_arg.copy_size_bytes,
            use_hfa_lane_homes ? va_arg.register_save_lane_size_bytes
                               : va_arg.copy_size_bytes,
            use_hfa_lane_homes ? va_arg.register_save_lane_count : 1,
            std::nullopt,
            source_scratch,
            address_scratch)) {
      return std::nullopt;
    }
    const std::string source_name = abi::register_name(source_scratch);
    lines.push_back("ldr " + source_name + ", " +
                    register_indirect_address(
                        *va_arg.source_va_list.register_name,
                        va_arg.overflow_source_field_offset_bytes));
    if (!append_add_unsigned_immediate(
            lines,
            source_scratch,
            address_scratch,
            va_arg.overflow_stride_bytes != 0
                ? va_arg.overflow_stride_bytes
                : align_aggregate_offset(va_arg.copy_size_bytes,
                                         va_arg.payload_align_bytes))) {
      return std::nullopt;
    }
    lines.push_back("str " + source_name + ", " +
                    register_indirect_address(
                        *va_arg.source_va_list.register_name,
                        va_arg.overflow_source_field_offset_bytes));
    lines.push_back(done_label + ":");
    return lines;
  }

  std::size_t copy_offset = 0;
  for (const auto width_bytes : aggregate_copy_chunks(va_arg.copy_size_bytes)) {
    const auto load_mnemonic = aggregate_copy_load_mnemonic(width_bytes);
    if (load_mnemonic.empty() ||
        !append_aggregate_source_address_lines(lines,
                                               va_arg,
                                               source_scratch,
                                               address_scratch,
                                               copy_offset)) {
      return std::nullopt;
    }
    const auto data_register =
        aggregate_copy_data_register(source_scratch, width_bytes);
    lines.push_back(std::string{load_mnemonic} + " " +
                    std::string{abi::register_name(data_register)} + ", [" +
                    std::string{abi::register_name(source_scratch)} + "]");
    if (!append_aggregate_destination_store(lines,
                                            va_arg,
                                            address_scratch,
                                            data_register,
                                            width_bytes,
                                            copy_offset)) {
      return std::nullopt;
    }
    copy_offset += width_bytes;
  }
  const std::string source_name = abi::register_name(source_scratch);
  lines.push_back("ldr " + source_name + ", " +
                  register_indirect_address(*va_arg.source_va_list.register_name,
                                            va_arg.progression_field_offset_bytes));
  if (!append_add_unsigned_immediate(lines,
                                     source_scratch,
                                     address_scratch,
                                     va_arg.progression_stride_bytes)) {
    return std::nullopt;
  }
  lines.push_back("str " + source_name + ", " +
                  register_indirect_address(*va_arg.source_va_list.register_name,
                                            va_arg.progression_field_offset_bytes));
  return lines;
}

}  // namespace

[[nodiscard]] bool emit_prepared_va_list_field_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto address = prepared_va_list_field_address(context, load_local.slot_name);
  if (!address.has_value()) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!mnemonic.has_value() || !target.has_value()) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " + *address);
  return true;
}

std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_entry_helper_kind(
    std::string_view callee) {
  if (callee == "llvm.va_start.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaStart;
  }
  if (callee == "llvm.va_copy.p0.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaCopy;
  }
  constexpr std::string_view va_arg_prefix = "llvm.va_arg.";
  if (callee.substr(0, va_arg_prefix.size()) == va_arg_prefix) {
    if (callee == "llvm.va_arg.aggregate") {
      return prepare::PreparedVariadicEntryHelperKind::VaArgAggregate;
    }
    return prepare::PreparedVariadicEntryHelperKind::VaArg;
  }
  return std::nullopt;
}

const prepare::PreparedVariadicEntryPlanFunction* require_prepared_variadic_entry_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto function_name = context.function.control_flow != nullptr
                                 ? context.function.control_flow->function_name
                                 : c4c::kInvalidFunctionName;
  const auto* entry_plan =
      context.function.prepared != nullptr
          ? prepare::find_prepared_variadic_entry_plan(*context.function.prepared,
                                                       function_name)
          : nullptr;
  if (entry_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 variadic entry helper lowering requires a PreparedVariadicEntryPlanFunction");
    return nullptr;
  }
  const auto& missing = entry_plan->missing_required_facts;
  if (!missing.empty()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        variadic_entry_missing_fact_message(missing));
    return nullptr;
  }
  if (const auto missing_consumption_fact =
          missing_variadic_entry_helper_consumption_fact(*entry_plan);
      !missing_consumption_fact.empty()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        variadic_entry_missing_fact_message(missing_consumption_fact));
    return nullptr;
  }
  return entry_plan;
}

bool variadic_helper_operand_homes_complete(
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  const auto scalar_access_plan_complete = [&homes]() {
    if (!homes.scalar_access_plan.has_value()) {
      return false;
    }
    const auto& plan = *homes.scalar_access_plan;
    return plan.source_class !=
               prepare::PreparedVariadicScalarVaArgSourceClass::Unknown &&
           plan.value_type != bir::TypeKind::Void &&
           plan.value_size_bytes != 0 &&
           plan.value_align_bytes != 0 &&
           plan.result_home.has_value() &&
           plan.source_field.has_value() &&
           plan.source_field_offset_bytes.has_value() &&
           plan.source_slot_size_bytes.has_value() &&
           plan.progression_field.has_value() &&
           plan.progression_field_offset_bytes.has_value() &&
           plan.progression_stride_bytes.has_value() &&
           plan.overflow_source_field.has_value() &&
           plan.overflow_source_field_offset_bytes.has_value() &&
           plan.overflow_stride_bytes.has_value();
  };
  switch (homes.helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return homes.destination_va_list.has_value() &&
             homes.destination_va_list_address.has_value();
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return homes.scalar_result.has_value() && homes.source_va_list.has_value() &&
             scalar_access_plan_complete();
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(
          homes);
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return homes.destination_va_list.has_value() && homes.source_va_list.has_value();
  }
  return false;
}

std::string variadic_helper_missing_consumption_fact_message(
    prepare::PreparedVariadicEntryHelperKind helper) {
  switch (helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return {};
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return "AArch64 scalar va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg.scalar_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return "AArch64 aggregate va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg_aggregate.aggregate_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return "AArch64 va_copy lowering requires prepared source and destination "
             "va_list homes";
  }
  return {};
}

void complete_variadic_call_record(CallInstructionRecord& instruction) {
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} &&
      !instruction.variadic_va_start.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_start =
        make_variadic_va_start_record(*instruction.source_variadic_entry,
                                      *instruction.source_variadic_helper_operand_homes);
    if (instruction.variadic_va_start.has_value() &&
        instruction.variadic_va_start_overflow_area_stack_offset_bytes.has_value()) {
      instruction.variadic_va_start->overflow_area_base_stack_offset_bytes =
          *instruction.variadic_va_start_overflow_area_stack_offset_bytes;
    }
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArg} &&
      !instruction.variadic_scalar_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_scalar_va_arg =
        make_variadic_scalar_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate} &&
      !instruction.variadic_aggregate_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_aggregate_va_arg =
        make_variadic_aggregate_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaCopy} &&
      !instruction.variadic_va_copy.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_copy =
        make_variadic_va_copy_record(*instruction.source_variadic_entry,
                                     *instruction.source_variadic_helper_operand_homes);
  }
}

std::optional<MachineNodeStatusRecord> variadic_call_selection_status(
    const CallInstructionRecord& instruction) {
  if (!instruction.variadic_entry_helper.has_value()) {
    return std::nullopt;
  }
  if (instruction.source_variadic_entry == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "variadic entry helper node is missing prepared entry provenance"};
  }
  if (instruction.source_variadic_helper_operand_homes == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "variadic entry helper node is missing prepared operand-home provenance"};
  }
  if (*instruction.variadic_entry_helper ==
      prepare::PreparedVariadicEntryHelperKind::VaStart) {
    if (!instruction.variadic_va_start.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "va_start helper node is missing structured prepared va_start record"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (*instruction.variadic_entry_helper ==
      prepare::PreparedVariadicEntryHelperKind::VaArg) {
    if (!instruction.variadic_scalar_va_arg.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "scalar va_arg machine-node lowering requires complete prepared fact "
              "helper_operand_homes.va_arg.scalar_access_plan"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (*instruction.variadic_entry_helper ==
      prepare::PreparedVariadicEntryHelperKind::VaArgAggregate) {
    if (!instruction.variadic_aggregate_va_arg.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "aggregate va_arg machine-node lowering requires complete prepared fact "
              "helper_operand_homes.va_arg_aggregate.aggregate_access_plan"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (*instruction.variadic_entry_helper ==
      prepare::PreparedVariadicEntryHelperKind::VaCopy) {
    if (!instruction.variadic_va_copy.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "va_copy machine-node lowering requires complete prepared source and "
              "destination va_list homes plus va_list_layout field facts"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  return MachineNodeStatusRecord{
      .status = MachineNodeSelectionStatus::DeferredUnsupported,
      .diagnostic =
          "variadic entry helper machine-node lowering is outside the selected va_start subset"};
}

std::optional<mir::TargetInstructionPrintResult> print_variadic_call(
    const CallInstructionRecord& call,
    std::string bad_header,
    std::string_view mnemonic) {
  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaStart}) {
    if (!call.variadic_va_start.has_value() || call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header +
          "va_start node is missing structured prepared va_start provenance");
    }
    if (mnemonic.empty()) {
      return target_unsupported(bad_header + "va_start mnemonic is not printable");
    }

    const auto& va_start = *call.variadic_va_start;
    auto lines = print_va_start_lowering_lines(va_start);
    if (!lines.has_value()) {
      return target_unsupported(
          bad_header +
          "va_start helper lowering requires register destination and valid "
          "AAPCS64 va_list field sizes");
    }
    return target_printed(std::move(*lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArg}) {
    if (!call.variadic_scalar_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header +
          "scalar va_arg node is missing structured prepared access-plan provenance");
    }
    if (mnemonic.empty()) {
      return target_unsupported(bad_header + "scalar va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_scalar_va_arg;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_scalar_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " result=" << prepared_value_home_name(va_arg.result_home)
          << " value_size=" << va_arg.value_size_bytes
          << " value_align=" << va_arg.value_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " slot_size=" << va_arg.source_slot_size_bytes
          << " rsa_slot#" << va_arg.register_save_area_slot_id
          << " rsa_stack+" << va_arg.register_save_area_stack_offset_bytes
          << " rsa_size=" << va_arg.register_save_area_size_bytes
          << " rsa_align=" << va_arg.register_save_area_align_bytes
          << " gp_base=" << va_arg.register_save_area_gp_offset_bytes
          << " fp_base=" << va_arg.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_arg.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_arg.register_save_area_fp_slot_size_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.overflow_source_field)
          << " overflow_field_offset=" << va_arg.overflow_source_field_offset_bytes
          << " overflow_stride=" << va_arg.overflow_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArgAggregate}) {
    if (!call.variadic_aggregate_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header +
          "aggregate va_arg node is missing structured prepared access-plan provenance");
    }
    if (mnemonic.empty()) {
      return target_unsupported(bad_header + "aggregate va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_aggregate_va_arg;
    auto lines = print_aggregate_va_arg_lowering_lines(va_arg);
    if (!lines.has_value()) {
      return target_unsupported(
          bad_header +
          "aggregate va_arg helper lowering requires register va_list source, "
          "stack-slot destination payload, two scratch registers, and supported "
          "prepared source class");
    }
    return target_printed(std::move(*lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaCopy}) {
    if (!call.variadic_va_copy.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header +
          "va_copy node is missing structured prepared va_copy provenance");
    }
    if (mnemonic.empty()) {
      return target_unsupported(bad_header + "va_copy mnemonic is not printable");
    }

    const auto& va_copy = *call.variadic_va_copy;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_copy.destination_va_list)
          << " source=" << prepared_value_home_name(va_copy.source_va_list)
          << " va_list_size=" << va_copy.va_list_size_bytes
          << " va_list_align=" << va_copy.va_list_align_bytes
          << " scratch_registers=" << va_copy.scratch_register_count
          << " scratch_stack=" << va_copy.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_copy.field_copies) {
      std::ostringstream out;
      out << "va.copy.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " source_offset=" << field.source_offset_bytes
          << " destination_offset=" << field.destination_offset_bytes
          << " size=" << field.size_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
