#pragma once

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "compile_time_engine.hpp"
#include "ir.hpp"

#include <memory>
#include <string>

namespace c4c::hir {

class Lowerer;

SourceSpan make_span(const Node* n);
NamespaceQualifier make_ns_qual(const Node* n);
std::string decode_string_node(const Node* n);
std::string strip_quoted_string(const char* raw);

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
