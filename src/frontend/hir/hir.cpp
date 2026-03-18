#include "hir.hpp"

#include "ast_to_hir.hpp"
#include "compile_time_engine.hpp"

#include <cstdio>
#include <string>

namespace c4c::hir {

Module build_hir(const Node* program_root,
                 const sema::ResolvedTypeTable* resolved_types) {
  InitialHirBuildResult initial = build_initial_hir(program_root, resolved_types);

  auto ct_stats = run_compile_time_engine(
      *initial.module,
      initial.ct_state,
      initial.deferred_instantiate,
      initial.deferred_consteval);

  initial.module->ct_info.deferred_instantiations = ct_stats.templates_deferred;
  initial.module->ct_info.deferred_consteval = ct_stats.consteval_deferred;
  initial.module->ct_info.total_iterations = ct_stats.iterations;
  initial.module->ct_info.templates_resolved = ct_stats.templates_instantiated;
  initial.module->ct_info.consteval_reduced = ct_stats.consteval_reduced;
  initial.module->ct_info.converged = ct_stats.converged;

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
