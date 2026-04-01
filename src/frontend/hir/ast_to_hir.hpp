#pragma once

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "compile_time_engine.hpp"
#include "ir.hpp"

#include <memory>
#include <optional>
#include <string>

namespace c4c::hir {

class Lowerer;

SourceSpan make_span(const Node* n);
NamespaceQualifier make_ns_qual(const Node* n);
std::string decode_string_node(const Node* n);
std::string strip_quoted_string(const char* raw);

struct QualifiedMethodRef {
  std::string struct_tag;
  std::string method_name;
  std::string key;
};

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
