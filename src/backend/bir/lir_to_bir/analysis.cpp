#include "../lir_to_bir.hpp"

#include <utility>

namespace c4c::backend {

namespace {

std::string_view resolve_link_name(const c4c::LinkNameTable& link_names,
                                   c4c::LinkNameId id) {
  if (id == c4c::kInvalidLinkName) return {};
  const std::string_view spelling = link_names.spelling(id);
  return spelling.empty() ? std::string_view{} : spelling;
}

std::string resolved_function_name(const c4c::codegen::lir::LirModule& module,
                                   const c4c::codegen::lir::LirFunction& function) {
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           function.link_name_id);
  if (!resolved_name.empty()) {
    return std::string(resolved_name);
  }
  return function.name;
}

std::size_t count_instructions(const c4c::codegen::lir::LirFunction& function) {
  std::size_t count = 0;
  for (const auto& block : function.blocks) {
    count += block.insts.size();
  }
  return count;
}

}  // namespace

BirModuleAnalysis analyze_module(BirLoweringContext& context) {
  BirModuleAnalysis analysis;
  analysis.function_count = context.lir_module.functions.size();
  analysis.global_count = context.lir_module.globals.size();
  analysis.string_constant_count = context.lir_module.string_pool.size();
  analysis.extern_decl_count = context.lir_module.extern_decls.size();

  for (const auto& function : context.lir_module.functions) {
    BirFunctionPreScan summary;
    summary.function_name = resolved_function_name(context.lir_module, function);
    summary.block_count = function.blocks.size();
    summary.instruction_count = count_instructions(function);
    summary.has_control_flow = function.blocks.size() > 1;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block.insts) {
        if (std::holds_alternative<c4c::codegen::lir::LirCall>(instruction)) {
          summary.has_calls = true;
        }
        if (std::holds_alternative<c4c::codegen::lir::LirLoad>(instruction) ||
            std::holds_alternative<c4c::codegen::lir::LirStore>(instruction) ||
            std::holds_alternative<c4c::codegen::lir::LirGep>(instruction)) {
          summary.has_memory_ops = true;
        }
      }
    }
    analysis.has_calls = analysis.has_calls || summary.has_calls;
    analysis.has_memory_ops = analysis.has_memory_ops || summary.has_memory_ops;
    analysis.has_control_flow = analysis.has_control_flow || summary.has_control_flow;
    analysis.functions.push_back(std::move(summary));
  }

  context.note("pre_scan", "collected module/function summaries for lir_to_bir");
  return analysis;
}

}  // namespace c4c::backend
