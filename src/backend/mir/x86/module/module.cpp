#include "module.hpp"

#include "../x86.hpp"
#include "../abi/abi.hpp"
#include "../core/core.hpp"

#include "../../../prealloc/prepared_printer.hpp"
#include "../../../prealloc/target_register_profile.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace c4c::backend::x86::module {

namespace {

[[noreturn]] void throw_prepared_control_flow_handoff_error(std::string detail) {
  throw std::invalid_argument("canonical prepared-module handoff rejected x86 control-flow "
                              "label authority: " +
                              std::move(detail));
}

[[noreturn]] void throw_prepared_value_location_handoff_error(std::string detail) {
  throw std::invalid_argument("canonical prepared-module handoff rejected x86 value-location "
                              "authority: " +
                              std::move(detail));
}

std::string render_prepared_label_id(c4c::BlockLabelId label) {
  if (label == c4c::kInvalidBlockLabel) {
    return "<invalid>";
  }
  return "#" + std::to_string(label);
}

std::string render_stack_memory_operand(std::size_t byte_offset, std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string render_global_i32_memory_operand(const Data& data,
                                             std::string_view global_name,
                                             std::size_t byte_offset) {
  std::string operand = "DWORD PTR [rip + " + data.render_asm_symbol_name(global_name);
  if (byte_offset != 0) {
    operand += " + " + std::to_string(byte_offset);
  }
  operand += "]";
  return operand;
}

std::string_view require_prepared_block_label(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId label,
    std::string_view context) {
  if (label == c4c::kInvalidBlockLabel) {
    throw_prepared_control_flow_handoff_error(std::string(context) + " has an invalid label id");
  }
  const auto spelling = c4c::backend::prepare::prepared_block_label(names, label);
  if (spelling.empty()) {
    throw_prepared_control_flow_handoff_error(std::string(context) + " references unknown label " +
                                              render_prepared_label_id(label));
  }
  return spelling;
}

bool bir_block_matches_prepared_label(
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  const auto prepared_spelling = c4c::backend::prepare::prepared_block_label(names, prepared_label);
  if (block.label_id != c4c::kInvalidBlockLabel) {
    return bir_names.block_labels.spelling(block.label_id) == prepared_spelling;
  }
  return block.label == prepared_spelling;
}

const c4c::backend::bir::Block* find_bir_block_by_prepared_label(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  for (const auto& block : function.blocks) {
    if (bir_block_matches_prepared_label(block, bir_names, names, prepared_label)) {
      return &block;
    }
  }
  return nullptr;
}

void require_prepared_target_block(const c4c::backend::bir::Function& function,
                                   const c4c::backend::bir::NameTables& bir_names,
                                   const c4c::backend::prepare::PreparedNameTables& names,
                                   c4c::BlockLabelId target_label,
                                   std::string_view context) {
  require_prepared_block_label(names, target_label, context);
  if (find_bir_block_by_prepared_label(function, bir_names, names, target_label) == nullptr) {
    throw_prepared_control_flow_handoff_error(std::string(context) + " label " +
                                              render_prepared_label_id(target_label) +
                                              " does not name a BIR block by id");
  }
}

std::optional<c4c::BlockLabelId> bir_block_label_id(
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const auto structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      return c4c::backend::prepare::resolve_prepared_block_label_id(names, structured_label);
    }
  }
  return c4c::backend::prepare::resolve_prepared_block_label_id(names, block.label);
}

void validate_prepared_block_targets(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowBlock& prepared_block) {
  const auto* bir_block =
      find_bir_block_by_prepared_label(function, bir_names, names, prepared_block.block_label);
  if (bir_block == nullptr) {
    throw_prepared_control_flow_handoff_error(
        "prepared block " + render_prepared_label_id(prepared_block.block_label) +
        " does not match any BIR block by label id");
  }
  if (bir_block->terminator.kind != prepared_block.terminator_kind) {
    throw_prepared_control_flow_handoff_error(
        "prepared block " + render_prepared_label_id(prepared_block.block_label) +
        " terminator kind drifted from BIR");
  }

  switch (prepared_block.terminator_kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      break;
    case c4c::backend::bir::TerminatorKind::Branch:
      require_prepared_target_block(function,
                                    bir_names,
                                    names,
                                    prepared_block.branch_target_label,
                                    "prepared branch target");
      break;
    case c4c::backend::bir::TerminatorKind::CondBranch:
      require_prepared_target_block(function,
                                    bir_names,
                                    names,
                                    prepared_block.true_label,
                                    "prepared true branch target");
      require_prepared_target_block(function,
                                    bir_names,
                                    names,
                                    prepared_block.false_label,
                                    "prepared false branch target");
      break;
  }
}

void validate_prepared_branch_condition(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedBranchCondition& condition) {
  require_prepared_target_block(
      function, bir_names, names, condition.block_label, "prepared condition block");
  require_prepared_target_block(
      function, bir_names, names, condition.true_label, "prepared condition true target");
  require_prepared_target_block(
      function, bir_names, names, condition.false_label, "prepared condition false target");

  const auto* block = c4c::backend::prepare::find_prepared_control_flow_block(
      control_flow, condition.block_label);
  if (block == nullptr || block->terminator_kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      block->true_label != condition.true_label || block->false_label != condition.false_label) {
    throw_prepared_control_flow_handoff_error(
        "prepared branch condition targets drifted from prepared block targets");
  }
}

void validate_prepared_control_flow_handoff(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function) {
  if (module.control_flow.functions.empty()) {
    return;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared function id");
  }

  const auto* control_flow = c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, *function_name);
  if (control_flow == nullptr) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared control-flow record");
  }

  for (const auto& block : function.blocks) {
    const auto block_label = bir_block_label_id(module.module.names, module.names, block);
    if (!block_label.has_value()) {
      throw_prepared_control_flow_handoff_error("BIR block '" + block.label +
                                                "' has no prepared block id");
    }
    if (c4c::backend::prepare::find_prepared_control_flow_block(*control_flow, *block_label) ==
        nullptr) {
      throw_prepared_control_flow_handoff_error("BIR block '" + block.label +
                                                "' has no prepared control-flow block");
    }
  }

  for (const auto& block : control_flow->blocks) {
    validate_prepared_block_targets(function, module.module.names, module.names, block);
  }
  for (const auto& condition : control_flow->branch_conditions) {
    validate_prepared_branch_condition(
        function, module.module.names, module.names, *control_flow, condition);
  }
}

bool is_grouped_span(std::size_t contiguous_width,
                     const std::vector<std::string>& occupied_register_names) {
  return contiguous_width > 1 || occupied_register_names.size() > 1;
}

std::string render_grouped_span(c4c::backend::prepare::PreparedRegisterBank bank,
                                std::optional<std::string_view> register_name,
                                std::size_t contiguous_width,
                                const std::vector<std::string>& occupied_register_names) {
  std::string rendered =
      std::string(c4c::backend::prepare::prepared_register_bank_name(bank)) + ":";
  rendered += register_name.has_value() ? std::string(*register_name) : std::string("<none>");
  rendered += "/w" + std::to_string(contiguous_width);
  if (!occupied_register_names.empty()) {
    rendered += "[";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        rendered += ",";
      }
      rendered += occupied_register_names[index];
    }
    rendered += "]";
  }
  return rendered;
}

std::string render_optional_grouped_span(
    std::optional<c4c::backend::prepare::PreparedRegisterBank> bank,
    std::optional<std::string_view> register_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  return render_grouped_span(bank.value_or(c4c::backend::prepare::PreparedRegisterBank::None),
                             register_name,
                             contiguous_width,
                             occupied_register_names);
}

void append_grouped_authority_comments(c4c::backend::x86::core::Text& out,
                                       const c4c::backend::prepare::PreparedBirModule& module,
                                       c4c::FunctionNameId function_name) {
  const auto consumed = c4c::backend::x86::consume_plans(module, function_name);
  const auto* frame_plan = consumed.frame;
  const auto* call_plans = consumed.calls;
  const auto* regalloc = consumed.regalloc;
  const auto* storage_plan = consumed.storage;

  std::size_t grouped_saved = 0;
  std::size_t grouped_preserved = 0;
  std::size_t grouped_clobbered = 0;
  std::size_t grouped_call_argument_spans = 0;
  std::size_t grouped_call_result_spans = 0;
  std::size_t grouped_spills = 0;
  std::size_t grouped_reloads = 0;
  std::size_t grouped_storage = 0;

  if (frame_plan != nullptr) {
    grouped_saved = static_cast<std::size_t>(std::count_if(
        frame_plan->saved_callee_registers.begin(),
        frame_plan->saved_callee_registers.end(),
        [](const c4c::backend::prepare::PreparedSavedRegister& saved) {
          return is_grouped_span(saved.contiguous_width, saved.occupied_register_names);
        }));
  }

  if (call_plans != nullptr) {
    for (const auto& call : call_plans->calls) {
      grouped_call_argument_spans += static_cast<std::size_t>(std::count_if(
          call.arguments.begin(),
          call.arguments.end(),
          [](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
            return is_grouped_span(argument.destination_contiguous_width,
                                   argument.destination_occupied_register_names);
          }));
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        ++grouped_call_result_spans;
      }
      grouped_preserved += static_cast<std::size_t>(std::count_if(
          call.preserved_values.begin(),
          call.preserved_values.end(),
          [](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
            return is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names);
          }));
      grouped_clobbered += static_cast<std::size_t>(std::count_if(
          call.clobbered_registers.begin(),
          call.clobbered_registers.end(),
          [](const c4c::backend::prepare::PreparedClobberedRegister& clobber) {
            return is_grouped_span(clobber.contiguous_width, clobber.occupied_register_names);
          }));
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      switch (op.op_kind) {
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Spill:
          ++grouped_spills;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Reload:
          ++grouped_reloads;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Rematerialize:
          break;
      }
    }
  }

  if (storage_plan != nullptr) {
    grouped_storage = static_cast<std::size_t>(std::count_if(
        storage_plan->values.begin(),
        storage_plan->values.end(),
        [](const c4c::backend::prepare::PreparedStoragePlanValue& value) {
          return is_grouped_span(value.contiguous_width, value.occupied_register_names);
        }));
  }

  if (grouped_saved == 0 && grouped_preserved == 0 && grouped_clobbered == 0 &&
      grouped_call_argument_spans == 0 && grouped_call_result_spans == 0 &&
      grouped_spills == 0 && grouped_reloads == 0 && grouped_storage == 0) {
    return;
  }

  out.append_line("    # grouped authority: saved=" + std::to_string(grouped_saved) +
                  " preserved=" + std::to_string(grouped_preserved) +
                  " clobbered=" + std::to_string(grouped_clobbered) +
                  " call_args=" + std::to_string(grouped_call_argument_spans) +
                  " call_results=" + std::to_string(grouped_call_result_spans) +
                  " spills=" + std::to_string(grouped_spills) +
                  " reloads=" + std::to_string(grouped_reloads) +
                  " storage=" + std::to_string(grouped_storage));

  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!is_grouped_span(saved.contiguous_width, saved.occupied_register_names)) {
        continue;
      }
      out.append_line("    # grouped saved save_index=" + std::to_string(saved.save_index) +
                      " span=" +
                      render_grouped_span(saved.bank,
                                          std::string_view(saved.register_name),
                                          saved.contiguous_width,
                                          saved.occupied_register_names));
    }
  }

  if (call_plans != nullptr) {
    for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
      const auto& call = call_plans->calls[call_index];
      for (const auto& argument : call.arguments) {
        if (!is_grouped_span(argument.destination_contiguous_width,
                             argument.destination_occupied_register_names)) {
          continue;
        }
        std::string line = "    # grouped arg call#" + std::to_string(call_index) +
                           " arg#" + std::to_string(argument.arg_index);
        if (argument.source_value_id.has_value()) {
          line += " source_value_id=" + std::to_string(*argument.source_value_id);
        }
        line += " dest_span=" +
                render_optional_grouped_span(argument.destination_register_bank,
                                             argument.destination_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *argument.destination_register_name}
                                                 : std::nullopt,
                                             argument.destination_contiguous_width,
                                             argument.destination_occupied_register_names);
        out.append_line(line);
      }
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        const auto& result = *call.result;
        std::string line = "    # grouped result call#" + std::to_string(call_index);
        if (result.destination_value_id.has_value()) {
          line += " destination_value_id=" + std::to_string(*result.destination_value_id);
        }
        line += " source_span=" +
                render_optional_grouped_span(result.source_register_bank,
                                             result.source_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *result.source_register_name}
                                                 : std::nullopt,
                                             result.source_contiguous_width,
                                             result.source_occupied_register_names);
        out.append_line(line);
      }
      for (const auto& preserved : call.preserved_values) {
        if (!is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names)) {
          continue;
        }
        std::string line =
            "    # grouped preserve call#" + std::to_string(call_index) + " value=" +
            std::string(
                c4c::backend::prepare::prepared_value_name(module.names, preserved.value_name)) +
            " route=" +
            std::string(
                c4c::backend::prepare::prepared_call_preservation_route_name(preserved.route)) +
            " span=" +
            render_grouped_span(
                preserved.register_bank.value_or(
                    c4c::backend::prepare::PreparedRegisterBank::None),
                preserved.register_name.has_value()
                    ? std::optional<std::string_view>{*preserved.register_name}
                    : std::nullopt,
                preserved.contiguous_width,
                preserved.occupied_register_names);
        if (preserved.callee_saved_save_index.has_value()) {
          line += " save_index=" + std::to_string(*preserved.callee_saved_save_index);
        }
        out.append_line(line);
      }
      for (const auto& clobbered : call.clobbered_registers) {
        if (!is_grouped_span(clobbered.contiguous_width, clobbered.occupied_register_names)) {
          continue;
        }
        out.append_line("    # grouped clobber call#" + std::to_string(call_index) +
                        " span=" +
                        render_grouped_span(clobbered.bank,
                                            std::string_view(clobbered.register_name),
                                            clobbered.contiguous_width,
                                            clobbered.occupied_register_names));
      }
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      if (op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Spill &&
          op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Reload) {
        continue;
      }
      std::string line =
          "    # grouped " +
          std::string(c4c::backend::prepare::prepared_spill_reload_op_kind_name(op.op_kind)) +
          " value_id=" + std::to_string(op.value_id);
      if (const auto value_it = std::find_if(
              regalloc->values.begin(),
              regalloc->values.end(),
              [&op](const c4c::backend::prepare::PreparedRegallocValue& value) {
                return value.value_id == op.value_id;
              });
          value_it != regalloc->values.end()) {
        line += " value=" +
                std::string(
                    c4c::backend::prepare::prepared_value_name(module.names, value_it->value_name));
      }
      line += " span=" +
              render_grouped_span(op.register_bank,
                                  op.register_name.has_value()
                                      ? std::optional<std::string_view>{*op.register_name}
                                      : std::nullopt,
                                  op.contiguous_width,
                                  op.occupied_register_names);
      if (op.slot_id.has_value()) {
        line += " slot_id=#" + std::to_string(*op.slot_id);
      }
      if (op.stack_offset_bytes.has_value()) {
        line += " stack_offset=" + std::to_string(*op.stack_offset_bytes);
      }
      out.append_line(line);
    }
  }

  if (storage_plan != nullptr) {
    for (const auto& value : storage_plan->values) {
      if (!is_grouped_span(value.contiguous_width, value.occupied_register_names)) {
        continue;
      }
      out.append_line(
          "    # grouped storage value=" +
          std::string(c4c::backend::prepare::prepared_value_name(module.names, value.value_name)) +
          " span=" +
          render_grouped_span(value.bank,
                              value.register_name.has_value()
                                  ? std::optional<std::string_view>{*value.register_name}
                                  : std::nullopt,
                              value.contiguous_width,
                              value.occupied_register_names));
    }
  }
}

bool block_defines_named_value(const c4c::backend::bir::Block& block, std::string_view name) {
  if (name.empty()) {
    return false;
  }
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary != nullptr && binary->result.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->result.name == name) {
      return true;
    }
  }
  return false;
}

const c4c::backend::bir::Global* find_supported_same_module_i32_global(
    const c4c::backend::bir::Module& module,
    std::string_view global_name,
    std::size_t byte_offset) {
  for (const auto& global : module.globals) {
    if (global.name != global_name || global.is_extern || global.is_thread_local) {
      continue;
    }
    if (byte_offset > global.size_bytes || global.size_bytes - byte_offset < 4) {
      return nullptr;
    }
    return &global;
  }
  return nullptr;
}

std::optional<std::int32_t> fold_supported_pointer_compare_return(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function) {
  if (function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::int32_t> folded_i32_values;
  std::unordered_map<std::string_view, std::size_t> symbolic_pointer_roots;
  std::size_t next_symbolic_pointer_root = 1;

  const auto intern_symbolic_pointer_root = [&](std::string_view value_name) -> std::size_t {
    const auto [it, inserted] =
        symbolic_pointer_roots.emplace(value_name, next_symbolic_pointer_root);
    if (inserted) {
      ++next_symbolic_pointer_root;
    }
    return it->second;
  };

  const auto resolve_pointer = [&](const c4c::backend::bir::Value& value)
      -> std::optional<std::size_t> {
    if (value.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      if (value.immediate < 0) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(value.immediate);
    }
    const auto is_pointer_param = std::any_of(
        function.params.begin(), function.params.end(), [&](const c4c::backend::bir::Param& param) {
          return param.type == c4c::backend::bir::TypeKind::Ptr && param.name == value.name;
        });
    if (is_pointer_param || block_defines_named_value(block, value.name)) {
      return std::nullopt;
    }
    if (!value.name.empty() && value.name.front() == '@') {
      const std::string_view symbol_name(value.name.data() + 1, value.name.size() - 1);
      const auto names_string_constant =
          std::any_of(module.string_constants.begin(),
                      module.string_constants.end(),
                      [&](const c4c::backend::bir::StringConstant& constant) {
                        return constant.name == symbol_name;
                      });
      const auto names_global =
          std::any_of(module.globals.begin(),
                      module.globals.end(),
                      [&](const c4c::backend::bir::Global& global) {
                        return global.name == symbol_name;
                      });
      if (!names_string_constant && !names_global) {
        return std::nullopt;
      }
    }
    return intern_symbolic_pointer_root(value.name);
  };

  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        binary->operand_type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto lhs = resolve_pointer(binary->lhs);
    const auto rhs = resolve_pointer(binary->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    switch (binary->opcode) {
      case c4c::backend::bir::BinaryOpcode::Eq:
        folded_i32_values.emplace(binary->result.name, *lhs == *rhs ? 1 : 0);
        break;
      case c4c::backend::bir::BinaryOpcode::Ne:
        folded_i32_values.emplace(binary->result.name, *lhs != *rhs ? 1 : 0);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto& return_value = *block.terminator.value;
  if (return_value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto found = folded_i32_values.find(return_value.name);
  if (found == folded_i32_values.end()) {
    return std::nullopt;
  }
  return found->second;
}

const c4c::backend::prepare::PreparedValueLocationFunction*
require_prepared_value_location_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function) {
  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared function id");
  }
  for (const auto& function_locations : module.value_locations.functions) {
    if (function_locations.function_name == *function_name) {
      return &function_locations;
    }
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has no prepared value-location record");
}

const c4c::backend::prepare::PreparedMoveResolution& require_prepared_i32_return_move(
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    std::size_t block_index) {
  const auto* before_return = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations,
      c4c::backend::prepare::PreparedMovePhase::BeforeReturn,
      block_index,
      block.insts.size());
  if (before_return == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return move bundle");
  }
  if (before_return->moves.size() != 1) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an ambiguous prepared return move bundle");
  }
  const auto& move = before_return->moves.front();
  if (move.destination_kind !=
          c4c::backend::prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::Register ||
      !move.destination_register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared return ABI move");
  }
  return move;
}

const c4c::backend::prepare::PreparedMoveResolution& require_prepared_i32_instruction_move(
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::size_t block_index,
    std::size_t instruction_index) {
  const auto* before_instruction = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations,
      c4c::backend::prepare::PreparedMovePhase::BeforeInstruction,
      block_index,
      instruction_index);
  if (before_instruction == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared instruction move bundle");
  }
  if (before_instruction->moves.size() != 1) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an ambiguous prepared instruction move bundle");
  }
  const auto& move = before_instruction->moves.front();
  if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::Value) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared instruction move");
  }
  return move;
}

const c4c::backend::prepare::PreparedValueHome& require_prepared_i32_value_home(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::string_view value_name,
    std::string_view context) {
  const auto prepared_value_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names, value_name);
  if (!prepared_value_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name + "' " +
                                                std::string(context) +
                                                " has no prepared name id");
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(function_locations, *prepared_value_name);
  if (home == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name + "' " +
                                                std::string(context) + " has no prepared home");
  }
  return *home;
}

std::string render_prepared_i32_value_home_operand(
    const c4c::backend::prepare::PreparedValueHome& home,
    const c4c::backend::bir::Function& function) {
  switch (home.kind) {
    case c4c::backend::prepare::PreparedValueHomeKind::Register:
      if (!home.register_name.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a register home without a register");
      }
      return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
    case c4c::backend::prepare::PreparedValueHomeKind::StackSlot:
      if (!home.offset_bytes.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a stack home without an offset");
      }
      return render_stack_memory_operand(*home.offset_bytes, "DWORD");
    case c4c::backend::prepare::PreparedValueHomeKind::None:
    case c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate:
    case c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset:
      break;
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has an unsupported prepared i32 return home");
}

bool append_prepared_i32_immediate_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (!block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  if (!function.return_abi.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return ABI");
  }
  const auto return_register = c4c::backend::prepare::call_result_destination_register_name(
      c4c::backend::x86::abi::resolve_target_profile(module), *function.return_abi);
  if (!return_register.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared register return ABI destination");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(*return_register) + ", " +
                  std::to_string(block.terminator.value->immediate));
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_passthrough_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (!block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto value_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names,
                                                            block.terminator.value->name);
  if (!value_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' returns a value with no prepared name id");
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(*function_locations, *value_name);
  if (home == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' returns a value with no prepared home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != home->value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from value home");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    sub rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + render_prepared_i32_value_home_operand(*home, function));
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    add rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    ret");
  return true;
}

std::optional<std::string_view> supported_i32_binary_immediate_mnemonic(
    c4c::backend::bir::BinaryOpcode opcode);

bool append_prepared_i32_rematerialized_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() != 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.front());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.name != block.terminator.value->name ||
      binary->lhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      binary->lhs.type != c4c::backend::bir::TypeKind::I32 ||
      binary->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      binary->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      !supported_i32_binary_immediate_mnemonic(binary->opcode).has_value()) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& home = require_prepared_i32_value_home(module,
                                                     *function_locations,
                                                     function,
                                                     block.terminator.value->name,
                                                     "return value");
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate) {
    return false;
  }
  if (!home.immediate_i32.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has a rematerializable return without an immediate");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from rematerialized home");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + std::to_string(*home.immediate_i32));
  out.append_line("    ret");
  return true;
}

std::optional<std::string_view> supported_i32_binary_immediate_mnemonic(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return "add";
    case c4c::backend::bir::BinaryOpcode::Sub:
      return "sub";
    case c4c::backend::bir::BinaryOpcode::Mul:
      return "imul";
    case c4c::backend::bir::BinaryOpcode::And:
      return "and";
    case c4c::backend::bir::BinaryOpcode::Or:
      return "or";
    case c4c::backend::bir::BinaryOpcode::Xor:
      return "xor";
    case c4c::backend::bir::BinaryOpcode::Shl:
      return "shl";
    case c4c::backend::bir::BinaryOpcode::LShr:
      return "shr";
    case c4c::backend::bir::BinaryOpcode::AShr:
      return "sar";
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      break;
  }
  return std::nullopt;
}

bool is_commutative_i32_binary_immediate(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
      return true;
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

bool append_prepared_i32_binary_immediate_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() != 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.front());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.name != block.terminator.value->name) {
    return false;
  }

  const auto mnemonic = supported_i32_binary_immediate_mnemonic(binary->opcode);
  if (!mnemonic.has_value()) {
    return false;
  }

  const c4c::backend::bir::Value* named_operand = nullptr;
  const c4c::backend::bir::Value* immediate_operand = nullptr;
  if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
    named_operand = &binary->lhs;
    immediate_operand = &binary->rhs;
  } else if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
             binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
             is_commutative_i32_binary_immediate(binary->opcode)) {
    named_operand = &binary->rhs;
    immediate_operand = &binary->lhs;
  } else {
    return false;
  }

  if (named_operand->type != c4c::backend::bir::TypeKind::I32 ||
      immediate_operand->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& source_home = require_prepared_i32_value_home(
      module, *function_locations, function, named_operand->name, "binary source value");
  const auto& result_home = require_prepared_i32_value_home(
      module, *function_locations, function, binary->result.name, "binary result value");

  const auto& instruction_move =
      require_prepared_i32_instruction_move(*function_locations, function, 0, 0);
  if (instruction_move.from_value_id != source_home.value_id ||
      instruction_move.to_value_id != result_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' instruction move drifted from value homes");
  }
  if (instruction_move.destination_register_name.has_value() &&
      result_home.register_name.has_value() &&
      *instruction_move.destination_register_name != *result_home.register_name) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' instruction move destination drifted from result home");
  }
  if (result_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !result_home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared i32 binary result home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != result_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from result home");
  }

  const auto result_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*result_home.register_name);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    sub rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    mov " + result_register + ", " +
                  render_prepared_i32_value_home_operand(source_home, function));
  out.append_line("    " + std::string(*mnemonic) + " " + result_register + ", " +
                  std::to_string(immediate_operand->immediate));
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + result_register);
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    add rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_same_module_global_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() < 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* load_global =
      std::get_if<c4c::backend::bir::LoadGlobalInst>(&block.insts.back());
  if (load_global == nullptr ||
      load_global->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load_global->result.type != c4c::backend::bir::TypeKind::I32 ||
      load_global->result.name != block.terminator.value->name ||
      load_global->address.has_value() ||
      find_supported_same_module_i32_global(
          module.module, load_global->global_name, load_global->byte_offset) == nullptr) {
    return false;
  }

  for (std::size_t inst_index = 0; inst_index + 1 < block.insts.size(); ++inst_index) {
    const auto* store_global =
        std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index]);
    if (store_global == nullptr ||
        store_global->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        store_global->value.type != c4c::backend::bir::TypeKind::I32 ||
        store_global->address.has_value() ||
        find_supported_same_module_i32_global(
            module.module, store_global->global_name, store_global->byte_offset) == nullptr) {
      return false;
    }
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& load_home = require_prepared_i32_value_home(module,
                                                         *function_locations,
                                                         function,
                                                         load_global->result.name,
                                                         "global load result value");
  if (load_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !load_home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared i32 global load home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != load_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from global load home");
  }

  const auto return_register = c4c::backend::x86::abi::narrow_i32_register_name(
      *return_move.destination_register_name);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  for (std::size_t inst_index = 0; inst_index + 1 < block.insts.size(); ++inst_index) {
    const auto& store_global =
        *std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index]);
    out.append_line("    mov " +
                    render_global_i32_memory_operand(
                        data, store_global.global_name, store_global.byte_offset) +
                    ", " + std::to_string(store_global.value.immediate));
  }
  out.append_line("    mov " + return_register + ", " +
                  render_global_i32_memory_operand(
                      data, load_global->global_name, load_global->byte_offset));
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_same_module_global_sub_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  std::unordered_map<std::string_view, std::string> value_locations;
  std::unordered_map<std::string_view, const c4c::backend::prepare::PreparedValueHome*> value_homes;
  std::string current_eax_value;

  const auto require_register_home =
      [&](std::string_view value_name,
          std::string_view context) -> const c4c::backend::prepare::PreparedValueHome& {
    const auto& home =
        require_prepared_i32_value_home(module, *function_locations, function, value_name, context);
    if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
        !home.register_name.has_value()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has an unsupported prepared i32 " +
                                                  std::string(context) + " home");
    }
    value_homes[value_name] = &home;
    return home;
  };

  const auto narrow_home_register =
      [](const c4c::backend::prepare::PreparedValueHome& home) -> std::string {
    return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
  };

  const auto value_location = [&](std::string_view value_name) -> std::optional<std::string> {
    const auto found = value_locations.find(value_name);
    if (found == value_locations.end()) {
      return std::nullopt;
    }
    return found->second;
  };

  const auto value_is_used_later = [&](std::string_view value_name,
                                       std::size_t after_inst_index) -> bool {
    for (std::size_t later_index = after_inst_index + 1; later_index < block.insts.size();
         ++later_index) {
      const auto* later_binary =
          std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[later_index]);
      if (later_binary == nullptr) {
        continue;
      }
      if ((later_binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
           later_binary->lhs.name == value_name) ||
          (later_binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
           later_binary->rhs.name == value_name)) {
        return true;
      }
    }
    return false;
  };

  const auto validate_binary_operand_move =
      [&](const c4c::backend::bir::Value& operand,
          const c4c::backend::prepare::PreparedValueHome& result_home,
          const c4c::backend::prepare::PreparedMoveBundle& bundle) {
    if (operand.kind != c4c::backend::bir::Value::Kind::Named) {
      return;
    }
    const auto home_found = value_homes.find(operand.name);
    if (home_found == value_homes.end()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' binary operand has no tracked prepared home");
    }
    const auto matching_move =
        std::find_if(bundle.moves.begin(),
                     bundle.moves.end(),
                     [&](const c4c::backend::prepare::PreparedMoveResolution& move) {
                       return move.from_value_id == home_found->second->value_id &&
                              move.to_value_id == result_home.value_id &&
                              move.destination_kind ==
                                  c4c::backend::prepare::PreparedMoveDestinationKind::Value &&
                              move.destination_storage_kind ==
                                  c4c::backend::prepare::PreparedMoveStorageKind::Register;
                     });
    if (matching_move == bundle.moves.end()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' instruction move drifted from binary operand homes");
    }
  };

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");

  bool saw_binary_sub = false;
  for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
    if (const auto* store_global =
            std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index])) {
      if (store_global->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store_global->value.type != c4c::backend::bir::TypeKind::I32 ||
          store_global->address.has_value() ||
          find_supported_same_module_i32_global(
              module.module, store_global->global_name, store_global->byte_offset) == nullptr) {
        return false;
      }
      function_out.append_line("    mov " +
                               render_global_i32_memory_operand(
                                   data, store_global->global_name, store_global->byte_offset) +
                               ", " + std::to_string(store_global->value.immediate));
      continue;
    }

    if (const auto* load_global =
            std::get_if<c4c::backend::bir::LoadGlobalInst>(&block.insts[inst_index])) {
      if (load_global->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load_global->result.type != c4c::backend::bir::TypeKind::I32 ||
          load_global->address.has_value() ||
          find_supported_same_module_i32_global(
              module.module, load_global->global_name, load_global->byte_offset) == nullptr) {
        return false;
      }
      require_register_home(load_global->result.name, "global load result value");
      function_out.append_line("    mov eax, " +
                               render_global_i32_memory_operand(
                                   data, load_global->global_name, load_global->byte_offset));
      value_locations[load_global->result.name] = "eax";
      current_eax_value = load_global->result.name;
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[inst_index]);
    if (binary == nullptr || binary->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
        binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        binary->lhs.type != c4c::backend::bir::TypeKind::I32 ||
        binary->rhs.type != c4c::backend::bir::TypeKind::I32) {
      return false;
    }

    const auto& result_home = require_register_home(binary->result.name, "binary result value");
    const auto* instruction_move = c4c::backend::prepare::find_prepared_move_bundle(
        *function_locations,
        c4c::backend::prepare::PreparedMovePhase::BeforeInstruction,
        0,
        inst_index);
    if (instruction_move == nullptr) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has no prepared binary instruction move bundle");
    }
    const std::size_t named_operand_count =
        (binary->lhs.kind == c4c::backend::bir::Value::Kind::Named ? 1U : 0U) +
        (binary->rhs.kind == c4c::backend::bir::Value::Kind::Named ? 1U : 0U);
    if (instruction_move->moves.size() != named_operand_count) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has an ambiguous prepared binary instruction move bundle");
    }
    validate_binary_operand_move(binary->lhs, result_home, *instruction_move);
    validate_binary_operand_move(binary->rhs, result_home, *instruction_move);

    std::string rhs_operand;
    if (binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
      rhs_operand = std::to_string(binary->rhs.immediate);
    } else {
      const auto rhs_location = value_location(binary->rhs.name);
      if (!rhs_location.has_value()) {
        return false;
      }
      function_out.append_line("    mov ecx, " + *rhs_location);
      rhs_operand = "ecx";
    }

    if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
      function_out.append_line("    mov eax, " + std::to_string(binary->lhs.immediate));
    } else {
      const auto lhs_location = value_location(binary->lhs.name);
      if (!lhs_location.has_value()) {
        return false;
      }
      if (*lhs_location != "eax") {
        function_out.append_line("    mov eax, " + *lhs_location);
      }
    }
    function_out.append_line("    sub eax, " + rhs_operand);

    const auto result_register = narrow_home_register(result_home);
    if (result_register != "eax") {
      function_out.append_line("    mov " + result_register + ", eax");
    }
    if (value_is_used_later(binary->result.name, inst_index)) {
      function_out.append_line("    mov ecx, eax");
    }
    value_locations[binary->result.name] = result_register;
    current_eax_value = binary->result.name;
    saw_binary_sub = true;
  }

  if (!saw_binary_sub) {
    return false;
  }

  const auto return_location = value_location(block.terminator.value->name);
  if (!return_location.has_value()) {
    return false;
  }
  const auto return_home_found = value_homes.find(block.terminator.value->name);
  if (return_home_found == value_homes.end()) {
    return false;
  }
  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != return_home_found->second->value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from expression home");
  }
  const auto return_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*return_move.destination_register_name);
  if (current_eax_value != block.terminator.value->name && *return_location != return_register) {
    function_out.append_line("    mov " + return_register + ", " + *return_location);
  }
  function_out.append_line("    ret");
  out.append_raw(function_out.take_text());
  return true;
}

bool append_supported_scalar_function(c4c::backend::x86::core::Text& out,
                                      const c4c::backend::prepare::PreparedBirModule& module,
                                      const c4c::backend::bir::Function& function,
                                      const Data& data) {
  validate_prepared_control_flow_handoff(module, function);
  if (append_prepared_i32_immediate_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_rematerialized_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_passthrough_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_binary_immediate_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_same_module_global_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_same_module_global_sub_return_function(out, module, function, data)) {
    return true;
  }
  const auto folded_return = fold_supported_pointer_compare_return(module.module, function);
  if (!folded_return.has_value()) {
    return false;
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov eax, " + std::to_string(*folded_return));
  out.append_line("    ret");
  return true;
}

void append_function_stub(c4c::backend::x86::core::Text& out,
                          const c4c::backend::prepare::PreparedBirModule& module,
                          const c4c::backend::bir::Function& function,
                          const Data& data) {
  validate_prepared_control_flow_handoff(module, function);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (const auto function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
      function_name.has_value()) {
    append_grouped_authority_comments(out, module, *function_name);
  }
  out.append_line("    # x86 backend contract-first stub");
  out.append_line("    # behavior recovery will replace this body");
  if (function.return_type != c4c::backend::bir::TypeKind::Void) {
    out.append_line("    xor eax, eax");
  }
  out.append_line("    ret");
}

}  // namespace

std::string emit(const c4c::backend::prepare::PreparedBirModule& module) {
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  if (!c4c::backend::x86::abi::is_x86_target(target_profile)) {
    throw std::invalid_argument("x86::module::emit requires an x86 target profile");
  }

  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto data = make_data(module, target_triple);

  c4c::backend::x86::core::Text out;
  out.append_line(".intel_syntax noprefix");
  out.append_line(".text");

  bool emitted_any_function = false;
  bool emitted_only_supported_scalar_functions = true;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    emitted_any_function = true;
    if (!append_supported_scalar_function(out, module, function, data)) {
      emitted_only_supported_scalar_functions = false;
      out.append_line("# x86 backend contract-first module emitter");
      append_function_stub(out, module, function, data);
    }
  }

  if (!emitted_any_function) {
    out.append_line("# no defined functions");
  }

  if (!(emitted_any_function && emitted_only_supported_scalar_functions) ||
      !module.module.globals.empty()) {
    out.append_raw(data.emit_data());
  }
  return out.take_text();
}

}  // namespace c4c::backend::x86::module
