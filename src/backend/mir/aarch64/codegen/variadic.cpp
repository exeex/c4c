#include "variadic.hpp"

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

void append_missing_variadic_entry_fact(std::vector<std::string>& missing,
                                        std::string_view fact) {
  missing.push_back(std::string{fact});
}

[[nodiscard]] std::vector<std::string> missing_variadic_entry_facts(
    const prepare::PreparedVariadicEntryPlanFunction& plan) {
  std::vector<std::string> missing = plan.missing_required_facts;
  if (!plan.named_register_counts.gp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.gp");
  }
  if (!plan.named_register_counts.fp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.fp");
  }
  if (plan.register_save_area.required) {
    if (!plan.register_save_area.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.size_bytes");
    }
    if (!plan.register_save_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.align_bytes");
    }
    if (!plan.register_save_area.slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.slot_id");
    }
    if (!plan.register_save_area.stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.stack_offset_bytes");
    }
    if (!plan.register_save_area.gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_offset_bytes");
    }
    if (!plan.register_save_area.fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_offset_bytes");
    }
    if (!plan.register_save_area.gp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_slot_size_bytes");
    }
    if (!plan.register_save_area.fp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_slot_size_bytes");
    }
    if (!plan.register_save_area.saved_gp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_gp_register_count");
    }
    if (!plan.register_save_area.saved_fp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_fp_register_count");
    }
    if (!plan.register_save_area.initial_gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_gp_offset_bytes");
    }
    if (!plan.register_save_area.initial_fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_fp_offset_bytes");
    }
  }
  if (plan.overflow_area.required) {
    if (!plan.overflow_area.base_slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_slot_id");
    }
    if (!plan.overflow_area.base_stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_stack_offset_bytes");
    }
    if (!plan.overflow_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.align_bytes");
    }
  }
  if (plan.va_list_layout.required) {
    if (!plan.va_list_layout.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.size_bytes");
    }
    if (!plan.va_list_layout.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.align_bytes");
    }
    if (plan.va_list_layout.fields.empty()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.fields");
    }
  }
  if (!plan.helper_resources.required_helpers.empty()) {
    if (!plan.helper_resources.scratch_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "helper_resources.scratch_register_count");
    }
    if (!plan.helper_resources.scratch_stack_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "helper_resources.scratch_stack_bytes");
    }
    if (plan.helper_operand_homes.empty()) {
      append_missing_variadic_entry_fact(missing, "helper_operand_homes");
    }
  }
  return missing;
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

[[nodiscard]] std::optional<VariadicVaStartRecord> make_variadic_va_start_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
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

}  // namespace

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
  const auto missing = missing_variadic_entry_facts(*entry_plan);
  if (!missing.empty()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        variadic_entry_missing_fact_message(missing));
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
      return homes.destination_va_list.has_value();
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
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_start.destination_va_list)
          << " named_gp=" << va_start.named_gp_register_count
          << " named_fp=" << va_start.named_fp_register_count
          << " va_list_size=" << va_start.va_list_size_bytes
          << " va_list_align=" << va_start.va_list_align_bytes
          << " scratch_registers=" << va_start.scratch_register_count
          << " scratch_stack=" << va_start.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.rsa slot#" << va_start.register_save_area_slot_id
          << " stack+" << va_start.register_save_area_stack_offset_bytes
          << " size=" << va_start.register_save_area_size_bytes
          << " align=" << va_start.register_save_area_align_bytes
          << " gp_offset=" << va_start.register_save_area_gp_offset_bytes
          << " fp_offset=" << va_start.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_start.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_start.register_save_area_fp_slot_size_bytes
          << " saved_gp=" << va_start.saved_gp_register_count
          << " saved_fp=" << va_start.saved_fp_register_count;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.initial_offsets gp=" << va_start.initial_gp_offset_bytes
          << " fp=" << va_start.initial_fp_offset_bytes
          << " overflow_slot#" << va_start.overflow_area_base_slot_id
          << " overflow_stack+" << va_start.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_start.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_start.va_list_fields) {
      std::ostringstream out;
      out << "va.start.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " offset=" << field.offset_bytes
          << " size=" << field.size_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
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
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_aggregate_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " destination_payload="
          << prepared_value_home_name(va_arg.destination_payload_home)
          << " payload_size=" << va_arg.payload_size_bytes
          << " payload_align=" << va_arg.payload_align_bytes
          << " copy_size=" << va_arg.copy_size_bytes
          << " copy_align=" << va_arg.copy_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.aggregate.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " payload_offset=" << va_arg.source_payload_offset_bytes
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
      out << "va.arg.aggregate.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
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
