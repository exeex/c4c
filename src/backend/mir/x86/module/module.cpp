#include "module.hpp"

#include "../x86.hpp"
#include "../abi/abi.hpp"
#include "../core/core.hpp"

#include "../../../prealloc/prepared_printer.hpp"

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

std::string render_prepared_label_id(c4c::BlockLabelId label) {
  if (label == c4c::kInvalidBlockLabel) {
    return "<invalid>";
  }
  return "#" + std::to_string(label);
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

bool append_supported_scalar_function(c4c::backend::x86::core::Text& out,
                                      const c4c::backend::prepare::PreparedBirModule& module,
                                      const c4c::backend::bir::Function& function,
                                      const Data& data) {
  validate_prepared_control_flow_handoff(module, function);
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

  if (!(emitted_any_function && emitted_only_supported_scalar_functions)) {
    out.append_raw(data.emit_data());
  }
  return out.take_text();
}

}  // namespace c4c::backend::x86::module
