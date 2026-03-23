#pragma once

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "compile_time_engine.hpp"
#include "ir.hpp"

#include <memory>

namespace c4c::hir {

struct InitialHirBuildResult {
  std::shared_ptr<Module> module;
  std::shared_ptr<CompileTimeState> ct_state;
  DeferredInstantiateFn deferred_instantiate;
  DeferredInstantiateTypeFn deferred_instantiate_type;
};

InitialHirBuildResult build_initial_hir(
    const Node* program_root,
    const sema::ResolvedTypeTable* resolved_types = nullptr);

}  // namespace c4c::hir
