#include "module.hpp"

#include <string_view>
#include <utility>

namespace c4c::backend::aarch64::module {
namespace {

[[nodiscard]] std::optional<c4c::BlockLabelId> prepared_label_for_bir_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const auto prepared_label =
          c4c::backend::prepare::resolve_prepared_block_label_id(names, structured_label);
      if (prepared_label.has_value()) {
        return prepared_label;
      }
    }
  }
  return c4c::backend::prepare::resolve_prepared_block_label_id(names, block.label);
}

[[nodiscard]] const c4c::backend::bir::Function* find_source_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  if (function_name == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (const auto& function : prepared.module.functions) {
    std::optional<c4c::FunctionNameId> candidate;
    if (function.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view structured_name =
          prepared.module.names.link_names.spelling(function.link_name_id);
      candidate = c4c::backend::prepare::resolve_prepared_function_name_id(prepared.names,
                                                                           structured_name);
    } else {
      candidate =
          c4c::backend::prepare::resolve_prepared_function_name_id(prepared.names, function.name);
    }
    if (candidate.has_value() && *candidate == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const c4c::backend::bir::Block* find_source_block(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Function* function,
    c4c::BlockLabelId block_label) {
  if (function == nullptr || block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (const auto& block : function->blocks) {
    const auto candidate =
        prepared_label_for_bir_block(prepared.names, prepared.module.names, block);
    if (candidate.has_value() && *candidate == block_label) {
      return &block;
    }
  }
  return nullptr;
}

[[nodiscard]] BlockRecord build_block_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Function* source_function,
    const c4c::backend::prepare::PreparedControlFlowBlock& block) {
  return BlockRecord{
      .block_label = block.block_label,
      .label = c4c::backend::prepare::prepared_block_label(prepared.names, block.block_label),
      .terminator_kind = block.terminator_kind,
      .branch_target_label = block.branch_target_label,
      .true_label = block.true_label,
      .false_label = block.false_label,
      .source_block = find_source_block(prepared, source_function, block.block_label),
      .control_flow = &block,
  };
}

[[nodiscard]] FunctionRecord build_function_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& function) {
  const auto* source_function = find_source_function(prepared, function.function_name);
  FunctionRecord record{
      .function_name = function.function_name,
      .label = c4c::backend::prepare::prepared_function_name(prepared.names,
                                                             function.function_name),
      .source_function = source_function,
      .control_flow = &function,
  };
  record.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    record.blocks.push_back(build_block_record(prepared, source_function, block));
  }
  return record;
}

[[nodiscard]] std::vector<FunctionRecord> build_function_records(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  std::vector<FunctionRecord> functions;
  functions.reserve(prepared.control_flow.functions.size());
  for (const auto& function : prepared.control_flow.functions) {
    functions.push_back(build_function_record(prepared, function));
  }
  return functions;
}

}  // namespace

BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile =
      c4c::backend::aarch64::abi::resolve_target_profile(prepared);
  if (auto error = c4c::backend::aarch64::abi::validate_prepared_module_handoff(prepared)) {
    return BuildResult{.module = std::nullopt, .error = std::move(error)};
  }
  return BuildResult{.module = Module{.prepared = &prepared,
                                      .target_profile = target_profile,
                                      .functions = build_function_records(prepared)},
                     .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module
