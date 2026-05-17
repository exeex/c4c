#include "dynamic_stack.hpp"

#include <string>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] std::optional<ValueNameId> maybe_named_value_id(const PreparedNameTables& names,
                                                              const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const ValueNameId id = names.value_names.find(value.name);
  if (id == kInvalidValueName) {
    return std::nullopt;
  }
  return id;
}

[[nodiscard]] BlockLabelId find_preferred_block_label_id(const PreparedNameTables& names,
                                                         const bir::NameTables& bir_names,
                                                         BlockLabelId label_id,
                                                         std::string_view raw_label) {
  if (label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return names.block_labels.find(raw_label);
}

[[nodiscard]] std::string dynamic_alloca_type_text(std::string_view callee) {
  constexpr std::string_view kPrefix = "llvm.dynamic_alloca.";
  if (!is_dynamic_alloca_call(callee)) {
    return {};
  }
  return std::string(callee.substr(kPrefix.size()));
}

}  // namespace

bool is_dynamic_alloca_call(std::string_view callee) {
  return callee.substr(0, std::string_view("llvm.dynamic_alloca.").size()) ==
         "llvm.dynamic_alloca.";
}

void populate_dynamic_stack_plan(PreparedBirModule& prepared) {
  prepared.dynamic_stack_plan.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedDynamicStackPlanFunction function_plan{
        .function_name = function_name_id,
        .requires_stack_save_restore = false,
        .uses_frame_pointer_for_fixed_slots = false,
        .operations = {},
    };

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      const BlockLabelId block_label_id = find_preferred_block_label_id(
          prepared.names, prepared.module.names, block.label_id, block.label);
      for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[inst_index]);
        if (call == nullptr) {
          continue;
        }

        std::optional<PreparedDynamicStackOpKind> op_kind;
        if (call->callee == "llvm.stacksave") {
          op_kind = PreparedDynamicStackOpKind::StackSave;
        } else if (is_dynamic_alloca_call(call->callee)) {
          op_kind = PreparedDynamicStackOpKind::DynamicAlloca;
        } else if (call->callee == "llvm.stackrestore") {
          op_kind = PreparedDynamicStackOpKind::StackRestore;
        }
        if (!op_kind.has_value()) {
          continue;
        }

        PreparedDynamicStackOp op{
            .function_name = function_name_id,
            .block_label = block_label_id,
            .instruction_index = inst_index,
            .kind = *op_kind,
            .result_value_name = std::nullopt,
            .operand_value_name = std::nullopt,
            .allocation_type_text = {},
            .element_size_bytes = 0,
            .element_align_bytes = 0,
        };
        if (call->result.has_value()) {
          op.result_value_name = maybe_named_value_id(prepared.names, *call->result);
        }
        if (!call->args.empty()) {
          op.operand_value_name = maybe_named_value_id(prepared.names, call->args.front());
        }
        if (*op_kind == PreparedDynamicStackOpKind::DynamicAlloca) {
          op.allocation_type_text = dynamic_alloca_type_text(call->callee);
        } else {
          function_plan.requires_stack_save_restore = true;
        }
        function_plan.operations.push_back(std::move(op));
      }
    }

    if (const auto* frame_plan = find_prepared_frame_plan(prepared, function_name_id);
        frame_plan != nullptr) {
      function_plan.uses_frame_pointer_for_fixed_slots =
          frame_plan->uses_frame_pointer_for_fixed_slots;
    }

    if (!function_plan.operations.empty()) {
      prepared.dynamic_stack_plan.functions.push_back(std::move(function_plan));
    }
  }
}

}  // namespace c4c::backend::prepare
