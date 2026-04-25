# Idea 97 Structured Identity Completion Audit

Status: Step 1 inventory in progress
Source Idea: `ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
Plan Step: Step 1 - Audit setup and inventory

## Scope Guard

This artifact is audit and planning evidence only. It does not request or record implementation edits, test rewrites, or expectation changes.

## Inventory Commands

- `git status --short`
- `sed -n '1,220p' todo.md`
- `sed -n '1,220p' plan.md`
- `sed -n '1,220p' ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
- `rg -n "(NamePath|QualifiedName|QualifiedNameLookup|LocalNameTable|SymbolId|MemberSymbolId|TextId|structured|legacy|mismatch|struct_tag_def_map|template_.*by_key|nttp_default_expr|UsingValueAlias|value_bind|using_value|find_value|lookup_value|resolve_value)" src/frontend/parser src/shared`
- `rg -n "(NamePath|QualifiedName|QualifiedNameLookup|LocalNameTable|SymbolId|MemberSymbolId|TextId|structured|legacy|mismatch|scoped|local_symbol|global_symbols|function_signatures|overload|enum|const_int|consteval|canonical|member|static_member|find\\(|lookup|resolve|bind)" src/frontend/sema src/frontend/parser/ast.hpp src/shared`
- `rg -n "(std::unordered_map<|std::map<|find\\(|\\[.*\\]|struct_defs|functions|globals|member_symbols|link_names|TypeSpec::tag|\\.tag|template_.*defs|specializations|instantiated_|enum_consts_|const_int_bindings_|struct_methods_|method_link|ret_types|function_decl_nodes_|consteval|deferred|NttpBindings|TypeBindings)" src/frontend/hir src/codegen src/backend | head -n 260`
- `rg -n "(struct (DefinitionState|TemplateState|Parser|Sema|Scope|Local|Module|Lowerer|FunctionContext)|class (Parser|Sema|Lowerer)|using .*Bindings|std::unordered_map<.*std::string|QualifiedNameLookup|LocalNameTable)" src/frontend/parser src/frontend/sema src/frontend/hir src/shared`
- Focused line-range reads with `nl -ba ... | sed -n ...` for `src/frontend/parser/impl/parser_state.hpp`, `src/frontend/parser/parser.hpp`, `src/frontend/parser/impl/core.cpp`, `src/frontend/parser/impl/types/template.cpp`, `src/frontend/parser/impl/types/base.cpp`, `src/frontend/sema/validate.hpp`, `src/frontend/sema/validate.cpp`, `src/frontend/sema/consteval.hpp`, `src/frontend/sema/consteval.cpp`, `src/frontend/sema/canonical_symbol.hpp`, `src/frontend/sema/canonical_symbol.cpp`, `src/frontend/hir/hir_ir.hpp`, `src/frontend/hir/impl/lowerer.hpp`, `src/frontend/hir/hir_build.cpp`, `src/frontend/hir/hir_types.cpp`, `src/frontend/hir/compile_time_engine.hpp`, `src/frontend/hir/impl/templates/global.cpp`, `src/frontend/hir/impl/templates/deferred_nttp.cpp`, `src/frontend/hir/impl/templates/type_resolution.cpp`, `src/codegen/lir/ir.hpp`, `src/codegen/shared/llvm_helpers.hpp`, and `src/codegen/lir/hir_to_lir/types.cpp`.

No build or test command was run; Step 1 proof is source inventory only.

## Parser Inventory

### Candidate Files

- `src/shared/text_id_table.hpp`
- `src/shared/qualified_name_table.hpp`
- `src/shared/local_name_table.hpp`
- `src/frontend/parser/ast.hpp`
- `src/frontend/parser/parser_types.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_support.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/support.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/types/types_helpers.hpp`
- `src/frontend/parser/impl/types/struct.cpp`

### Candidate Symbols And Surfaces

- Shared structured-name substrate:
  - `NamePathTable`, `QualifiedNameKey`, `QualifiedNameLookupTable` in `src/shared/qualified_name_table.hpp`.
  - `LocalNameTable<LocalNameKey, ...>` in `src/shared/local_name_table.hpp`.
  - `TextId`, `SymbolId`, `MemberSymbolId`, `TextTable`, `SymbolTable`, and `SemanticNameTable` in `src/shared/text_id_table.hpp`.
- Parser state:
  - `ParserSharedLookupState::parser_name_paths`.
  - `ParserBindingState::struct_typedefs` and `ParserBindingState::value_bindings` keyed by `QualifiedNameKey`.
  - Legacy/parser-local bridges in `ParserBindingState::non_atom_typedef_types`, `non_atom_var_types`, `enum_consts`, and `const_int_bindings`.
  - `ParserDefinitionState::struct_tag_def_map` keyed by rendered string tag.
  - `ParserTemplateState::template_struct_defs_by_key` and `template_struct_specializations_by_key` alongside legacy `template_struct_defs` and `template_struct_specializations`.
  - `ParserTemplateState::instantiated_template_struct_keys_by_key` alongside legacy `instantiated_template_struct_keys`.
  - `ParserTemplateState::nttp_default_expr_tokens_by_key` alongside legacy `nttp_default_expr_tokens`.
  - `ParserTemplateState::alias_template_info` keyed by `QualifiedNameKey`.
- Parser lookup APIs:
  - Structured typedef lookup and fallback: `find_structured_typedef_type`, `find_typedef_type(const QualifiedNameKey&, ...)`, and `find_visible_typedef_type`.
  - Structured value lookup and fallback: `find_structured_var_type`, `find_var_type(const QualifiedNameKey&, ...)`, `find_visible_var_type`, and `register_structured_var_type_binding`.
  - Local `TextId` lookup: `bind_local_typedef`, `bind_local_value`, `find_local_visible_typedef_type`, and `find_local_visible_var_type`.
  - Namespace/using lookup: `lookup_using_value_alias`, `lookup_value_in_context`, `lookup_type_in_context`, `resolve_visible_value`, and `resolve_visible_type`.
  - Template lookup: `find_template_struct_primary`, `find_template_struct_specializations`, `register_template_struct_primary`, and `register_template_struct_specialization`.
  - NTTP/template dedup: `eval_deferred_nttp_default`, `cache_nttp_default_expr_tokens`, `has_template_instantiation_dedup_key_for_direct_emit`, and `mark_template_instantiation_dedup_key_for_direct_emit`.
- Bridge/string surfaces for Step 2 classification:
  - `parser_support.hpp` / `impl/support.cpp` helper APIs still accept `std::unordered_map<std::string, Node*>`, `std::unordered_map<std::string, long long>`, and `std::unordered_map<std::string, TypeSpec>`.
  - `TypeSpec::tag`, `Node::name`, and `Node::unqualified_name` still feed rendered bridge names.
  - `struct_tag_def_map` receives both source tags and generated names in `src/frontend/parser/impl/types/struct.cpp`.
  - Template argument canonical strings remain inside `TemplateInstantiationKey::Argument::canonical_key`; this may be a structured-key component rather than a standalone semantic string lookup.

## Sema Inventory

### Candidate Files

- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/canonical_symbol.hpp`
- `src/frontend/sema/canonical_symbol.cpp`

### Candidate Symbols And Surfaces

- Structured-name substrate:
  - `SemaStructuredNameKey`, `SemaStructuredNameKeyHash`, `sema_local_name_key`, `sema_structured_name_key`, `sema_symbol_name_key`, `compare_sema_lookup_presence`, and `compare_sema_lookup_ptrs`.
- Validation maps with dual lookup:
  - String maps: `globals_`, `enum_consts_`, `funcs_`, `consteval_funcs_`, `ref_overload_sigs_`, `cpp_overload_sigs_`, `complete_structs_`, `complete_unions_`, `struct_field_names_`, `struct_static_member_types_`, `struct_base_tags_`, and `scopes_`.
  - Structured mirrors: `structured_globals_`, `structured_enum_consts_`, `structured_funcs_`, `consteval_funcs_by_text_`, `consteval_funcs_by_key_`, `structured_ref_overload_sigs_`, `structured_cpp_overload_sigs_`, `complete_structs_by_key_`, `complete_unions_by_key_`, `struct_field_text_ids_by_key_`, `struct_static_member_types_by_key_`, `struct_base_keys_by_key_`, and `structured_scopes_`.
  - Record tag bridge: `structured_record_keys_by_tag_` and `ambiguous_structured_record_tags_`.
- Validation lookup APIs:
  - `lookup_global_by_key`, `lookup_function_by_key`, `lookup_ref_overloads_by_key`, `lookup_cpp_overloads_by_key`.
  - `lookup_function_by_name`, `lookup_ref_overloads_by_name`, `lookup_cpp_overloads_by_name`, and `lookup_symbol`, each retaining legacy lookup with structured comparison where a reference key exists.
  - `bind_local`, `lookup_local_symbol_by_name`, and `lookup_local_symbol_by_key`.
  - `lookup_consteval_function_by_name`, `lookup_consteval_function_by_text`, and `lookup_consteval_function_by_key`.
  - `lookup_struct_static_member_type_by_key`, `lookup_struct_static_member_type_legacy`, `lookup_struct_static_member_type`, `has_struct_instance_field_by_key`, `has_struct_instance_field_legacy`, and `has_struct_instance_field`.
  - `record_global_const_binding`, `record_local_const_binding`, `bind_enum_constants_global`, and `bind_enum_constants_local`.
- Consteval and type binding maps:
  - `ConstEvalEnv` carries string maps, `TextId` maps, and `ConstEvalStructuredNameKey` maps for enum/local/named constants and NTTP bindings.
  - `TypeBindings`, `TypeBindingTextMap`, `TypeBindingStructuredMap`, `TypeBindingNameTextMap`, and `TypeBindingNameStructuredMap`.
  - `evaluate_consteval_call` still accepts `const std::unordered_map<std::string, const Node*>& consteval_fns` with text/key mirrors optional.
  - `InterpreterBindings` carries `by_name`, `by_text`, and `by_key` for consteval locals.
- Bridge/string surfaces for Step 3 classification:
  - `type_utils.cpp::static_eval_int` takes `std::unordered_map<std::string, long long>& enum_consts`.
  - `canonical_symbol.hpp::CanonicalIdentity::name` and canonical template-arg substitution maps use strings; likely canonical/debug/mangling identity rather than parser-style namespace lookup, but needs classification.
  - `struct_defs_by_unqualified_name_` and `structured_record_keys_by_tag_` are tag/name bridge helpers and need `blocked-by-hir` versus `sema-leftover` review.

## HIR Inventory

### Candidate Files

- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/impl/templates/global.cpp`
- `src/frontend/hir/impl/templates/deferred_nttp.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `src/frontend/hir/impl/stmt/range_for.cpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/core.cpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/llvm/calling_convention.cpp`

### Candidate Symbols And Surfaces

- HIR module maps:
  - `Module::fn_index`, `Module::global_index`, `Module::struct_defs`, and `Module::template_defs` are keyed by `SymbolName`/rendered string.
  - `Module::link_names` and `Module::member_symbols` provide stable IDs for link-visible names and member symbols.
  - `Module::lookup_function_id`, `lookup_global_id`, `find_function_by_name_legacy`, `find_global_by_name_legacy`, `resolve_function_decl`, and `resolve_global_decl` expose the current string fallback boundary.
- Type and tag consumers:
  - `TypeSpec::tag` is used for HIR struct layout lookups in `module_->struct_defs.find(ts.tag)` and codegen layout/type rendering.
  - `NamespaceQualifier` carries segment strings and `TextId` segment IDs, giving a possible upstream structured-key source.
  - `HirStructDef` carries `tag`, `tag_text_id`, `ns_qual`, `base_tags`, and `base_tag_text_ids`.
- Lowerer function-local maps:
  - `FunctionCtx::locals`, `local_fn_ptr_sigs`, `param_fn_ptr_sigs`, `static_globals`, `params`, `label_blocks`, `local_const_bindings`, `tpl_bindings`, `nttp_bindings`, and `pack_params` are string-keyed.
  - `DenseIdMap<LocalId, TypeSpec>` already provides a non-string local type mirror after local resolution.
- Lowerer global/template/member registries:
  - `enum_consts_`, `const_int_bindings_`, `struct_def_nodes_`, `template_struct_defs_`, `template_struct_specializations_`, `template_global_defs_`, and `template_global_specializations_`.
  - `instantiated_template_globals_` and `instantiated_tpl_struct_keys_` use structured `TemplateStructInstanceKey` surfaces.
  - `struct_static_member_decls_`, `struct_static_member_const_values_`, `struct_methods_`, `struct_method_link_name_ids_`, `struct_method_ret_types_`, `function_decl_nodes_`, `struct_constructors_`, `struct_destructors_`, `ref_overload_set_`, and `ref_overload_mangled_`.
- HIR lookup functions:
  - Method lookup: `find_struct_method_mangled`, `find_struct_method_link_name_id`, and `find_struct_method_return_type` use `"tag::method"` and `"_const"` suffix keys.
  - Static member lookup: `find_struct_static_member_decl` and `find_struct_static_member_const_value` use `{tag -> member}` string maps and recurse through `module_->struct_defs`.
  - Member ID lookup: `find_struct_member_symbol_id` uses `module_->member_symbols.find(tag + "::" + member)`.
  - Generic type inference looks through `ctx->nttp_bindings`, `ctx->locals`, `ctx->params`, `ctx->static_globals`, `module_->find_global_by_name_legacy`, `module_->find_function_by_name_legacy`, `ctx->tpl_bindings`, and `module_->struct_defs`.
- Compile-time engine:
  - `InstantiationRegistry::seed_work_` and `instances_` are string-keyed by template/function name, while `structured_seed_keys_` and `structured_instance_keys_` provide structured dedup sets.
  - `CompileTimeState::template_fn_defs_`, `template_struct_defs_`, `template_struct_specializations_`, `consteval_fn_defs_`, `enum_consts_`, and `const_int_bindings_` are string-keyed.
- Template and deferred surfaces:
  - `collect_template_global_definitions`, `find_template_global_primary`, and `find_template_global_specializations` use `item->name` and `primary_tpl->name`.
  - `ensure_template_global_instance` uses structured `TemplateStructInstanceKey` for instance dedup but still uses rendered `mangled` for `Module::lookup_global_id`.
  - `DeferredNttpExprEnv` uses string `type_lookup` and `nttp_lookup`.
  - `DeferredNttpTemplateLookup` uses string-keyed template and struct maps.
  - `resolve_struct_member_typedef_type` and related helpers use `struct_def_nodes_.find(tag)`, `module_->struct_defs.find(...)`, and string template binding keys.
- Downstream handoff surfaces:
  - `LinkNameId` and `LinkNameTable` are already present on HIR functions/globals and are propagated into LIR.
  - `LirModule::extern_decl_link_name_map` is keyed by `LinkNameId` with `extern_decl_name_map` as fallback.
  - LIR/codegen still consumes rendered `TypeSpec::tag` for LLVM type names, `mod.struct_defs.find(ts.tag)` for layout/ABI, and `std::vector<std::string> type_decls` for textual LLVM type declarations.
  - `str_pool_map` is a string literal byte-content dedup map, not declaration identity.

## Parser Findings By Classification

Step 1 inventory only. Step 2 should classify the parser surfaces above as `bridge-required`, `diagnostic-only`, `legacy-proof`, `parser-leftover`, or `blocked-by-downstream`.

## Sema Findings By Classification

Step 1 inventory only. Step 3 should classify the sema surfaces above as `bridge-required`, `diagnostic-only`, `legacy-proof`, `sema-leftover`, or `blocked-by-hir`.

## HIR Inventory By Subsystem

Step 1 inventory only. Step 4 should classify each HIR subsystem by upstream structured-key availability, mirror feasibility, rendered-name/codegen requirements, proof coverage, and parser/sema metadata dependencies.

## Proof Gaps And Validation Candidates

- No proof gap was discovered that required a build during Step 1.
- Parser classification proof candidates: parse-only namespace/using/template cases covering structured lookup and legacy mismatch counters.
- Sema classification proof candidates: focused frontend/sema cases for scoped locals, overloads, consteval, enum/const-int bindings, and member/static-member lookup.
- HIR first-slice proof candidates: HIR dump/summary CTest cases touching module function/global/struct lookup, template instantiation, consteval, member lookup, and LIR/codegen link-name preservation.

## Follow-On Recommendation Placeholders

- Idea 98 decision: pending Step 2 and Step 3 classification.
- Idea 99 scope: pending Step 4 first-slice analysis.
