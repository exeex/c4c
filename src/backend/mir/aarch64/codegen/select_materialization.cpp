#include "select_materialization.hpp"

#include "../abi/abi.hpp"
#include "comparison.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "fp_value_materialization.hpp"
#include "instruction.hpp"
#include "operands.hpp"
#include "../../../prealloc/calls.hpp"
#include "../../../prealloc/control_flow.hpp"
#include "../../../prealloc/value_locations.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

[[nodiscard]] std::optional<prepare::PreparedDirectGlobalSelectChainDependency>
bir_call_argument_direct_global_select_chain_dependency(
    const module::BlockLoweringContext& context,
    std::size_t before_instruction_index,
    std::size_t argument_index,
    std::string_view source_value_name) {
  if (context.bir_block == nullptr ||
      before_instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto* call_inst =
      std::get_if<bir::CallInst>(&context.bir_block->insts[before_instruction_index]);
  if (call_inst == nullptr) {
    return std::nullopt;
  }
  const auto routing =
      bir::find_call_argument_publication_source_routing(*call_inst, argument_index);
  const auto* dependency = routing.direct_global_select_chain_dependency;
  if (dependency == nullptr ||
      dependency->source_value_name != source_value_name) {
    return std::nullopt;
  }
  return prepare::PreparedDirectGlobalSelectChainDependency{
      .contains_direct_global_load = dependency->contains_direct_global_load,
      .root_is_select = dependency->root_is_select,
      .root_instruction_index = dependency->root_instruction_index,
  };
}

}  // namespace

[[nodiscard]] std::string select_chain_label(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    c4c::ValueNameId root_value_name,
    std::uint8_t target_index,
    std::size_t package_index,
    std::size_t label_index,
    std::string_view suffix) {
  const auto function_name = context.function.control_flow != nullptr
                                 ? context.function.control_flow->function_name
                                 : c4c::kInvalidFunctionName;
  const auto block_label = context.control_flow_block != nullptr
                               ? context.control_flow_block->block_label
                               : c4c::kInvalidBlockLabel;
  return ".Lselect_mat_" + std::to_string(function_name) + "_" +
         std::to_string(block_label) + "_" + std::to_string(instruction_index) +
         "_" + std::to_string(root_value_name) + "_" + std::to_string(target_index) +
         "_" + std::to_string(package_index) + "_" + std::to_string(label_index) +
         "_" + std::string{suffix};
}

[[nodiscard]] std::string select_chain_local_label_reference(
    std::size_t label_index,
    std::string_view suffix) {
  const auto numeric_label =
      label_index * 2U + (suffix == "true" ? std::size_t{1} : std::size_t{2});
  return std::to_string(numeric_label) + "f";
}

[[nodiscard]] std::string select_chain_local_label_definition(
    std::size_t label_index,
    std::string_view suffix) {
  const auto numeric_label =
      label_index * 2U + (suffix == "true" ? std::size_t{1} : std::size_t{2});
  return std::to_string(numeric_label) + ":";
}

[[nodiscard]] bool emit_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::size_t root_instruction_index,
    c4c::ValueNameId root_value_name,
    std::size_t package_index,
    std::vector<std::string>& lines,
    std::size_t& label_index,
    std::vector<std::string_view>& active_values,
    bool reload_current_memory_loads,
    const prepare::PreparedDirectGlobalSelectChainDependency*
        direct_global_dependency) {
  if (value.kind == bir::Value::Kind::Immediate) {
    const auto target_view = scalar_view_for_type(value.type);
    const auto target = target_view.has_value()
                            ? gp_register_name(target_index, *target_view)
                            : std::nullopt;
    if (!target.has_value()) {
      return false;
    }
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  const bool root_value = active_values.empty() &&
                          direct_global_dependency != nullptr;
  if (root_value) {
    const auto prepared_value_name = prepared_named_value_id(context, value);
    if (!direct_global_dependency->contains_direct_global_load ||
        !direct_global_dependency->root_instruction_index.has_value() ||
        !prepared_value_name.has_value() ||
        *prepared_value_name != root_value_name) {
      return false;
    }
  }

  const auto producer =
      find_prepared_same_block_select_producer(context, value, before_instruction_index);
  if (root_value &&
      direct_global_dependency->root_is_select != (producer.select != nullptr)) {
    return false;
  }
  if (producer.select == nullptr) {
    return emit_value_publication_to_register(
        context,
        value,
        before_instruction_index,
        target_index,
        scratch_index,
        lines,
        reload_current_memory_loads);
  }

  const auto condition = branch_condition_suffix(producer.select->predicate);
  const auto compare_view = scalar_view_for_type(producer.select->compare_type);
  if (!condition.has_value() || !compare_view.has_value()) {
    return false;
  }

  active_values.push_back(value.name);
  const auto current_label = label_index++;
  const auto true_label =
      select_chain_local_label_reference(current_label, "true");
  const auto true_definition =
      select_chain_local_label_definition(current_label, "true");
  const auto end_label =
      select_chain_local_label_reference(current_label, "end");
  const auto end_definition =
      select_chain_local_label_definition(current_label, "end");

  const auto lhs_name = gp_register_name(target_index, *compare_view);
  if (!lhs_name.has_value() ||
      !emit_value_publication_to_register(context,
                                          producer.select->lhs,
                                          producer.instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines,
                                          reload_current_memory_loads)) {
    active_values.pop_back();
    return false;
  }
  std::optional<std::string> rhs_name;
  if (producer.select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(producer.select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(producer.select->rhs.immediate);
  } else {
    rhs_name = gp_register_name(scratch_index, *compare_view);
    if (!rhs_name.has_value() ||
        !emit_value_publication_to_register(context,
                                            producer.select->rhs,
                                            producer.instruction_index,
                                            scratch_index,
                                            target_index,
                                            lines,
                                            reload_current_memory_loads)) {
      active_values.pop_back();
      return false;
    }
  }
  lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
  lines.push_back("b." + std::string{*condition} + " " + true_label);

  if (!emit_select_chain_value_to_register(context,
                                           producer.select->false_value,
                                           producer.instruction_index,
                                           target_index,
                                           scratch_index,
                                           root_instruction_index,
                                           root_value_name,
                                           package_index,
                                           lines,
                                           label_index,
                                           active_values,
                                           reload_current_memory_loads,
                                           direct_global_dependency)) {
    active_values.pop_back();
    return false;
  }
  lines.push_back("b " + end_label);
  lines.push_back(true_definition);
  if (!emit_select_chain_value_to_register(context,
                                           producer.select->true_value,
                                           producer.instruction_index,
                                           target_index,
                                           scratch_index,
                                           root_instruction_index,
                                           root_value_name,
                                           package_index,
                                           lines,
                                           label_index,
                                           active_values,
                                           reload_current_memory_loads,
                                           direct_global_dependency)) {
    active_values.pop_back();
    return false;
  }
  lines.push_back(end_definition);
  active_values.pop_back();
  return true;
}

[[nodiscard]] bool emit_fp_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterReference destination,
    std::uint8_t gp_scratch_index,
    std::uint8_t gp_auxiliary_index,
    std::vector<std::string>& lines,
    std::vector<std::string_view>& active_values,
    const prepare::PreparedDirectGlobalSelectChainDependency*
        direct_global_dependency) {
  const auto destination_view = scalar_fp_register_view(destination, value.type);
  if (!destination_view.has_value()) {
    return false;
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return emit_fp_value_to_register(context,
                                     value,
                                     before_instruction_index,
                                     *destination_view,
                                     gp_scratch_index,
                                     lines);
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  const auto prepared_producer =
      find_prepared_same_block_select_producer(context, value, before_instruction_index);
  const bir::SelectInst* select = prepared_producer.select;
  auto producer_instruction_index = prepared_producer.instruction_index;
  if (select == nullptr && context.bir_block != nullptr) {
    for (std::size_t index = 0;
         index < before_instruction_index && index < context.bir_block->insts.size();
         ++index) {
      const auto* candidate =
          std::get_if<bir::SelectInst>(&context.bir_block->insts[index]);
      if (candidate != nullptr &&
          candidate->result.kind == bir::Value::Kind::Named &&
          candidate->result.name == value.name &&
          candidate->result.type == value.type) {
        select = candidate;
        producer_instruction_index = index;
        break;
      }
    }
  }
  if (select == nullptr) {
    const auto previous_size = lines.size();
    if (emit_fp_value_to_register(context,
                                  value,
                                  before_instruction_index,
                                  *destination_view,
                                  gp_scratch_index,
                                  lines)) {
      return true;
    }
    lines.resize(previous_size);
    std::optional<prepare::PreparedValueHomeLookups> local_value_home_lookups;
    const auto* value_home_lookups = context.function.value_home_lookups;
    if (value_home_lookups == nullptr &&
        context.function.value_locations != nullptr) {
      local_value_home_lookups =
          prepare::make_prepared_value_home_lookups(context.function.value_locations);
      value_home_lookups = &*local_value_home_lookups;
    }
    const auto* home = context.function.prepared != nullptr
                           ? prepare::find_prepared_value_home_for_bir_value(
                                 context.function.prepared->names,
                                 value_home_lookups,
                                 context.function.regalloc,
                                 context.function.value_locations,
                                 value)
                           : nullptr;
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::Register ||
        !home->register_name.has_value()) {
      return false;
    }
    const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return false;
    }
    const auto source = scalar_fp_register_view(*parsed, value.type);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = abi::register_name(*source);
    const auto destination_name = abi::register_name(*destination_view);
    if (source_name != destination_name) {
      lines.push_back("fmov " + std::string{destination_name} + ", " +
                      std::string{source_name});
    }
    return true;
  }
  const auto condition = branch_condition_suffix(select->predicate);
  const auto compare_view = scalar_view_for_type(select->compare_type);
  if (!condition.has_value() || !compare_view.has_value()) {
    return false;
  }
  if (direct_global_dependency != nullptr &&
      direct_global_dependency->root_is_select != true) {
    return false;
  }

  const auto gp_lhs = abi::gp_register(gp_scratch_index, *compare_view);
  const auto gp_rhs = abi::gp_register(gp_auxiliary_index, *compare_view);
  if (!gp_lhs.has_value() || !gp_rhs.has_value()) {
    return false;
  }
  std::optional<abi::RegisterReference> true_scratch;
  for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
    if (scratch.index == destination_view->index) {
      continue;
    }
    true_scratch = scalar_fp_register_view(scratch, value.type);
    if (true_scratch.has_value()) {
      break;
    }
  }
  if (!true_scratch.has_value()) {
    return false;
  }

  auto true_value = select->true_value;
  true_value.type = value.type;
  auto false_value = select->false_value;
  false_value.type = value.type;
  active_values.push_back(value.name);
  if (!emit_fp_select_chain_value_to_register(context,
                                              false_value,
                                              producer_instruction_index,
                                              *destination_view,
                                              gp_scratch_index,
                                              gp_auxiliary_index,
                                              lines,
                                              active_values,
                                              nullptr) ||
      !emit_fp_select_chain_value_to_register(context,
                                              true_value,
                                              producer_instruction_index,
                                              *true_scratch,
                                              gp_scratch_index,
                                              gp_auxiliary_index,
                                              lines,
                                              active_values,
                                              nullptr)) {
    active_values.pop_back();
    return false;
  }

  auto lhs = select->lhs;
  lhs.type = select->compare_type;
  if (!emit_value_publication_to_register(context,
                                          lhs,
                                          producer_instruction_index,
                                          gp_scratch_index,
                                          gp_auxiliary_index,
                                          lines)) {
    active_values.pop_back();
    return false;
  }
  std::string rhs_name;
  if (select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(select->rhs.immediate);
  } else {
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            producer_instruction_index,
                                            gp_auxiliary_index,
                                            gp_scratch_index,
                                            lines)) {
      active_values.pop_back();
      return false;
    }
    rhs_name = abi::register_name(*gp_rhs);
  }
  lines.push_back("cmp " + std::string{abi::register_name(*gp_lhs)} + ", " +
                  rhs_name);
  lines.push_back("fcsel " + std::string{abi::register_name(*destination_view)} +
                  ", " + std::string{abi::register_name(*true_scratch)} +
                  ", " + std::string{abi::register_name(*destination_view)} +
                  ", " + std::string{*condition});
  active_values.pop_back();
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_select_chain_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_direct_global_select_chain_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    const prepare::PreparedCallArgumentPlan* argument_plan,
    BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value() || argument_plan == nullptr ||
      argument_plan->instruction_index != before_instruction_index ||
      !argument_plan->source_value_id.has_value()) {
    return std::nullopt;
  }
  std::optional<prepare::PreparedValueHomeLookups> local_value_home_lookups;
  const auto* value_home_lookups = context.function.value_home_lookups;
  if (value_home_lookups == nullptr && context.function.value_locations != nullptr) {
    local_value_home_lookups =
        prepare::make_prepared_value_home_lookups(context.function.value_locations);
    value_home_lookups = &*local_value_home_lookups;
  }
  const auto* value_home =
      prepare::find_prepared_value_home_for_bir_value(
          context.function.prepared->names,
          value_home_lookups,
          context.function.regalloc,
          context.function.value_locations,
          value);
  if (value_home == nullptr ||
      value_home->value_id != *argument_plan->source_value_id ||
      value_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  const auto routing =
      prepare::find_prepared_call_argument_publication_source_routing(*argument_plan);
  const auto bir_dependency =
      bir_call_argument_direct_global_select_chain_dependency(
          context, before_instruction_index, argument_plan->arg_index, value.name);
  std::optional<prepare::PreparedDirectGlobalSelectChainDependency>
      prepared_dependency;
  if (routing.direct_global_select_chain_dependency != nullptr &&
      routing.direct_global_select_chain_dependency->source_value_name == *value_name) {
    prepared_dependency =
        routing.direct_global_select_chain_dependency->direct_global_dependency;
  }
  const auto direct_global_dependency =
      bir_dependency.has_value() ? bir_dependency : prepared_dependency;
  if (!direct_global_dependency.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  if (context.function.value_locations != nullptr &&
      (value.type == bir::TypeKind::F32 || value.type == bir::TypeKind::F64)) {
    if (value_home->kind != prepare::PreparedValueHomeKind::Register ||
        !value_home->register_name.has_value()) {
      return std::nullopt;
    }
    const auto parsed =
        abi::parse_aarch64_register_name(*value_home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return std::nullopt;
    }
    const auto result_register = scalar_fp_register_view(*parsed, value.type);
    if (!result_register.has_value()) {
      return std::nullopt;
    }
    std::vector<std::string> lines;
    std::vector<std::string_view> active_values;
    if (!emit_fp_select_chain_value_to_register(context,
                                                value,
                                                before_instruction_index,
                                                *result_register,
                                                scratches.front().index,
                                                scratches[1].index,
                                                lines,
                                                active_values,
                                                &*direct_global_dependency) ||
        lines.empty()) {
      return std::nullopt;
    }
    RegisterOperand emitted{
        .reg = *result_register,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = value_home->value_id,
        .value_name = *value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Fpr,
        .expected_view = result_register->view,
    };
    record_emitted_scalar_register(scalar_state, *value_name, emitted);
    return make_select_chain_materialization_instruction(
        context, before_instruction_index, std::move(lines));
  }
  const auto result_view = scalar_view_for_type(value.type);
  if (!result_view.has_value()) {
    return std::nullopt;
  }
  std::optional<abi::RegisterReference> result_register;
  std::optional<prepare::PreparedValueHome> result_home;
  result_home = *value_home;
  if (value_home->kind == prepare::PreparedValueHomeKind::Register &&
      value_home->register_name.has_value()) {
    if (const auto parsed = abi::parse_aarch64_register_name(*value_home->register_name);
        parsed.has_value() &&
        parsed->bank == abi::RegisterBank::GeneralPurpose) {
      result_register = abi::gp_register(parsed->index, *result_view);
    }
  }
  if (!result_register.has_value()) {
    result_register = abi::gp_register(scratches[0].index, *result_view);
  }
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  std::optional<std::uint8_t> scratch_index;
  for (const auto scratch : scratches) {
    if (scratch.index != result_register->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  const std::size_t package_index = argument_plan->arg_index;
  std::size_t label_index = 0;
  std::vector<std::string_view> active_values;
  if (!emit_select_chain_value_to_register(context,
                                           value,
                                           before_instruction_index,
                                           result_register->index,
                                           *scratch_index,
                                           before_instruction_index,
                                           *value_name,
                                           package_index,
                                           lines,
                                           label_index,
                                           active_values,
                                           true,
                                           &*direct_global_dependency) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = result_home.has_value()
                      ? std::optional<prepare::PreparedValueId>{result_home->value_id}
                      : std::nullopt,
      .value_name = *value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = result_view,
  };
  record_emitted_scalar_register(scalar_state, *value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, before_instruction_index, std::move(lines));
}

}  // namespace c4c::backend::aarch64::codegen
