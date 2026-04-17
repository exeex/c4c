#pragma once

// Shared HIR lowering surface.
//
// This header is intentionally not the public facade. It declares the shared
// types/helpers used while lowering AST + first-pass semantic information into
// HIR's second-pass semantic model.
//
// Lowering responsibility:
// - resolve template/dependent constructs far enough to build initial HIR
// - preserve deferred work when immediate resolution is not yet possible
// - hand deferred template/consteval/NTTP follow-up to compile_time_engine
//
// Implementation map:
// - hir_lowering_core.cpp: shared lowering utilities and initial build entry
// - hir_build.cpp: module/program-wide coordination
// - hir_expr.cpp: expression lowering core
// - hir_expr_call.cpp: call/member/consteval expression lowering
// - hir_expr_object.cpp: object materialization/new-delete expression lowering
// - hir_stmt.cpp: statement lowering
// - hir_stmt_switch.cpp: switch/case/default statement lowering
// - hir_types.cpp: type/layout/init lowering
// - hir_templates.cpp: template instantiation support
// - hir_functions.cpp: function/callable lowering helpers

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "compile_time_engine.hpp"
#include "hir_ir.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace c4c::hir {

class Lowerer;

// Shared lowering entry points.
struct InitialHirBuildResult {
  std::shared_ptr<Module> module;
  std::shared_ptr<CompileTimeState> ct_state;
  DeferredInstantiateFn deferred_instantiate;
  DeferredInstantiateTypeFn deferred_instantiate_type;
};

InitialHirBuildResult build_initial_hir(
    const Node* program_root,
    const sema::ResolvedTypeTable* resolved_types = nullptr);

// Cross-TU syntax and source-location helpers.
SourceSpan make_span(const Node* n);
NamespaceQualifier make_ns_qual(const Node* n, TextTable* texts = nullptr);
TextId make_text_id(std::string_view text, TextTable* texts = nullptr);
TextId make_unqualified_text_id(const Node* n, TextTable* texts = nullptr);
std::string decode_string_node(const Node* n);
std::string strip_quoted_string(const char* raw);

// Shared lowering result carriers.
struct QualifiedMethodRef {
  std::string struct_tag;
  std::string method_name;
  std::string key;
};

struct HirTemplateArg {
  bool is_value = false;
  TypeSpec type{};
  long long value = 0;
};

struct ResolvedTemplateArgs {
  std::vector<HirTemplateArg> concrete_args;
  std::vector<std::pair<std::string, TypeSpec>> type_bindings;
  std::vector<std::pair<std::string, long long>> nttp_bindings;
};

struct PreparedTemplateStructInstance {
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  TemplateStructInstanceKey instance_key;
};

// Shared lowering utilities.
std::string rewrite_gcc_asm_template(std::string text);
std::string sanitize_symbol(std::string s);
std::optional<QualifiedMethodRef> try_parse_qualified_struct_method_name(
    const Node* fn,
    bool include_const_suffix = true);
UnaryOp map_unary_op(const char* op);
BinaryOp map_binary_op(const char* op);
AssignOp map_assign_op(const char* op, NodeKind kind);
TypeSpec infer_int_literal_type(const Node* n);
TypeSpec normalize_generic_type(TypeSpec ts);
bool generic_type_compatible(TypeSpec a, TypeSpec b);
bool has_concrete_type(const TypeSpec& ts);
void compute_struct_layout(hir::Module* module, HirStructDef& def);

}  // namespace c4c::hir
