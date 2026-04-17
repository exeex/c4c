#include "hir.hpp"

#include "hir_lowering.hpp"
#include "compile_time_engine.hpp"

#include <cstdio>
#include <string>

namespace c4c::hir {

Module build_hir(const Node* program_root,
                 const sema::ResolvedTypeTable* resolved_types,
                 SourceProfile source_profile,
                 const c4c::TargetProfile& target_profile) {
  // Stage 1: Initial HIR — lower AST to mixed compile-time/runtime HIR.
  InitialHirBuildResult initial = build_initial_hir(program_root, resolved_types);
  initial.module->source_profile = source_profile;
  initial.module->target_triple = target_profile.triple;
  // Module starts at HirPipelineStage::Initial (the default).

  // Stage 2: Normalized HIR — fixpoint reduction of compile-time work.
  auto ct_stats = run_compile_time_engine(
      *initial.module,
      initial.ct_state,
      initial.deferred_instantiate,
      initial.deferred_instantiate_type);

  initial.module->ct_info.deferred_instantiations = ct_stats.templates_deferred;
  initial.module->ct_info.deferred_consteval = ct_stats.consteval_deferred;
  initial.module->ct_info.total_iterations = ct_stats.iterations;
  initial.module->ct_info.templates_resolved = ct_stats.templates_instantiated;
  initial.module->ct_info.consteval_reduced = ct_stats.consteval_reduced;
  initial.module->ct_info.converged = ct_stats.converged;
  initial.module->ct_info.diagnostics.clear();
  for (const auto& diag : ct_stats.diagnostics) {
    std::string line;
    if (diag.file && diag.file[0]) line += std::string(diag.file) + ":";
    line += std::to_string(diag.line > 0 ? diag.line : 0);
    line += ":";
    line += std::to_string(diag.column > 0 ? diag.column : 1);
    line += ": error: ";
    line += diag.description;
    initial.module->ct_info.diagnostics.push_back(std::move(line));
  }
  initial.module->pipeline_stage = HirPipelineStage::Normalized;

  // Stage 3: Materialized HIR — finalize emission decisions.
  auto mat_stats = materialize_ready_functions(*initial.module);
  initial.module->ct_info.materialized_functions = mat_stats.materialized;
  initial.module->ct_info.non_materialized_functions = mat_stats.non_materialized;
  initial.module->pipeline_stage = HirPipelineStage::Materialized;

  return std::move(*initial.module);
}

std::string format_summary(const Module& module) {
  const ProgramSummary s = module.summary();
  char buf[256];
  std::snprintf(
      buf,
      sizeof(buf),
      "HIR summary: functions=%zu globals=%zu blocks=%zu statements=%zu expressions=%zu",
      s.functions,
      s.globals,
      s.blocks,
      s.statements,
      s.expressions);
  return std::string(buf);
}

}  // namespace c4c::hir
