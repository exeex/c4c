#include "lir_to_bir.hpp"

#include <utility>

namespace c4c::backend {

namespace {

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
    summary.function_name = function.name;
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
