# Idea 97 Structured Identity Completion Audit

Status: Step 6 final consistency check complete
Source Idea: `ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
Plan Step: Step 6 - Final audit consistency check

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

### bridge-required

- Rendered-name bridge output remains required where parser APIs must return compatibility spellings for existing string consumers. Evidence: `src/frontend/parser/impl/core.cpp:230` `render_structured_name`, `src/frontend/parser/impl/core.cpp:239` `render_value_binding_name`, and `src/frontend/parser/impl/core.cpp:249` `render_lookup_name_in_context` render `QualifiedNameKey` back to text before falling through to `compatibility_namespace_name_in_context`; `src/shared/qualified_name_table.hpp:120` `render_qualified_name` is the shared renderer. These are bridges, not semantic lookup owners.
- Using-declaration import still materializes bridge strings while preserving structured identity. Evidence: `src/frontend/parser/impl/declarations.cpp:1638` builds `imported_value_key`, `src/frontend/parser/impl/declarations.cpp:1642` renders it with `render_qualified_name`, and `src/frontend/parser/impl/declarations.cpp:1688` registers the compatibility binding when the imported value has a type.
- Template primary/specialization registration still mirrors rendered names for legacy consumers. Evidence: `src/frontend/parser/impl/types/template.cpp:288` stores `template_struct_defs_by_key`, while `src/frontend/parser/impl/types/template.cpp:294` and `src/frontend/parser/impl/types/template.cpp:299` also store `template_struct_defs` by fallback and bridge names; specialization mirrors appear at `src/frontend/parser/impl/types/template.cpp:317` and `src/frontend/parser/impl/types/template.cpp:324`.

### diagnostic-only

- Parser mismatch counters are audit/proof telemetry rather than lookup dependencies. Evidence: `src/frontend/parser/impl/parser_state.hpp:165` `template_struct_instantiation_key_mismatch_count` and `src/frontend/parser/impl/parser_state.hpp:171` `nttp_default_expr_cache_mismatch_count`; increments are limited to comparison paths at `src/frontend/parser/impl/types/template.cpp:56`, `src/frontend/parser/impl/types/template.cpp:1158`, `src/frontend/parser/impl/types/template.cpp:1180`, and `src/frontend/parser/impl/types/base.cpp:103`.
- `VisibleNameResult::compatibility_spelling` output is diagnostic/bridge text once `VisibleNameResult::key` is present. Evidence: value resolution fills key plus compatibility spelling in `src/frontend/parser/impl/core.cpp:2870` through `src/frontend/parser/impl/core.cpp:2897`, and using aliases do the same in `src/frontend/parser/impl/core.cpp:2834` through `src/frontend/parser/impl/core.cpp:2844`.

### legacy-proof

- Value binding lookup is structured-first for namespace/global paths and TextId-native for locals, with legacy string/symbol mirrors retained for comparison and compatibility. Evidence: `src/frontend/parser/impl/parser_state.hpp:74` `ParserBindingState::value_bindings`, `src/frontend/parser/impl/core.cpp:1213` `find_structured_var_type`, `src/frontend/parser/impl/core.cpp:1220` `find_visible_var_type`, and `src/frontend/parser/impl/core.cpp:1278` `register_structured_var_type_binding`. Local values use `LocalNameTable` through `src/frontend/parser/impl/core.cpp:791` `bind_local_value` and `src/frontend/parser/impl/core.cpp:807` `find_local_visible_var_type`.
- The remaining value string lookup APIs are compatibility fallback once structured spelling recovery fails or the caller only has text. Evidence: `src/frontend/parser/impl/core.cpp:1152` `has_var_type`, `src/frontend/parser/impl/core.cpp:1166` `find_var_type(TextId, fallback)`, `src/frontend/parser/impl/core.cpp:1189` `find_var_type(std::string)`, and `src/frontend/parser/impl/core.cpp:262` `cache_legacy_var_type_binding`.
- Using value aliases now carry structured target identity plus a compatibility name. Evidence: `src/frontend/parser/impl/parser_state.hpp:228` `UsingValueAlias` stores `target_key` and `compatibility_name`; `src/frontend/parser/impl/declarations.cpp:1693` stores `{imported_alias_key, imported_value_name}`; `src/frontend/parser/impl/core.cpp:2815` `lookup_using_value_alias` resolves the `target_key` first and only falls back to the compatibility name when needed.
- NTTP default cache keys are structured-first when the template name has a `TextId`, with legacy cache entries used for fallback and mismatch proof. Evidence: `src/frontend/parser/impl/parser_state.hpp:92` `NttpDefaultExprKey`, `src/frontend/parser/impl/parser_state.hpp:166` `nttp_default_expr_tokens_by_key`, `src/frontend/parser/impl/types/template.cpp:1131` through `src/frontend/parser/impl/types/template.cpp:1166` requiring structured tokens when `template_text_id` is valid, and `src/frontend/parser/impl/types/template.cpp:1200` `cache_nttp_default_expr_tokens` populating both structured and legacy caches.
- Template instantiation dedup keys are structured-first for parser-owned dedup, with rendered keys mirrored to keep old emit paths synchronized. Evidence: `src/frontend/parser/impl/parser_state.hpp:112` `TemplateInstantiationKey`, `src/frontend/parser/impl/parser_state.hpp:162` `instantiated_template_struct_keys_by_key`, `src/frontend/parser/impl/types/template.cpp:41` `sync_template_instantiation_dedup_keys`, `src/frontend/parser/impl/types/template.cpp:66` `mark_template_instantiation_dedup_keys`, and the direct-emit variants in `src/frontend/parser/impl/types/base.cpp:89` and `src/frontend/parser/impl/types/base.cpp:114`.

### parser-leftover

- Public parser-support helper overloads still expose string-keyed maps and should be reviewed as parser cleanup candidates once downstream tag identity work is scoped. Evidence: `src/frontend/parser/parser_support.hpp:19` `eval_const_int(... std::unordered_map<std::string, long long>*)`, `src/frontend/parser/parser_support.hpp:23` `resolve_typedef_chain(... std::unordered_map<std::string, TypeSpec>&)`, and `src/frontend/parser/parser_support.hpp:25` `types_compatible_p(... std::unordered_map<std::string, TypeSpec>&)`.
- The string-keyed const-int helper overload remains a real string lookup path for `NK_VAR` by `n->name`. Evidence: `src/frontend/parser/impl/support.cpp:712` `eval_const_int` string overload and `src/frontend/parser/impl/support.cpp:720` through `src/frontend/parser/impl/support.cpp:727` map lookup by `n->name`. The TextId overload at `src/frontend/parser/impl/support.cpp:631` and `src/frontend/parser/impl/support.cpp:639` is the structured replacement surface, so this is parser cleanup rather than downstream blocking.

### blocked-by-downstream

- Parser struct/tag maps and `TypeSpec::tag` outputs are rendered-name bridges required by HIR/codegen-era type identity. Evidence: `src/frontend/parser/impl/parser_state.hpp:84` `struct_tag_def_map`, `src/frontend/parser/impl/types/struct.cpp:2287` storing both `source_tag` and canonical `sd->name`, and `src/frontend/parser/impl/types/template.cpp:376`, `src/frontend/parser/impl/types/template.cpp:393`, and `src/frontend/parser/impl/types/base.cpp:3213` registering instantiated template structs by mangled tag.
- ABI/layout helper paths still need rendered struct tags until HIR/type identity owns a non-string struct key. Evidence: `src/frontend/parser/impl/support.cpp:517` `struct_align`, `src/frontend/parser/impl/support.cpp:536` `struct_sizeof`, and `src/frontend/parser/impl/support.cpp:565` `compute_offsetof` all take `const char* tag` plus `std::unordered_map<std::string, Node*>`.
- Template instantiation output must still produce rendered/mangled tags for `TypeSpec::tag`. Evidence: `src/frontend/parser/impl/types/template.cpp:361` `build_template_struct_mangled_name`, `src/frontend/parser/impl/types/template.cpp:385` assigning `out_resolved->tag`, `src/frontend/parser/impl/types/base.cpp:2581` through `src/frontend/parser/impl/types/base.cpp:2584` setting pending template origin/debug refs and `ts.tag`, and `src/frontend/parser/impl/types/base.cpp:3216` assigning instantiated `ts.tag`.

### Step 2 Handoff

- No parser value-binding or using-value-alias surface found after idea 95 requires a new parser semantic lookup route; they are structured-first with legacy proof/bridge fallback.
- Meaningful parser leftovers are limited to public helper overloads that still accept string maps for const-int and typedef-chain/type-compatibility support.
- Rendered struct/tag/template names are blocked by downstream HIR/type/codegen identity and should be kept separate from parser/sema completion work.

## Sema Findings By Classification

### bridge-required

- Canonical-symbol names are bridge/canonicalization strings rather than post-96 validation lookup owners. Evidence: `src/frontend/sema/canonical_symbol.hpp:221` through `src/frontend/sema/canonical_symbol.hpp:237` define `CanonicalIdentity::name` and identity lookup, while `src/frontend/sema/canonical_symbol.cpp:936` through `src/frontend/sema/canonical_symbol.cpp:943` and `src/frontend/sema/canonical_symbol.cpp:958` through `src/frontend/sema/canonical_symbol.cpp:985` encode source spellings for ABI-style type/name mangling.
- Canonical helper conversion still writes nominal type spelling into `TypeSpec::tag`, which is a compatibility handoff to the current type representation rather than an independent sema lookup table. Evidence: `src/frontend/sema/canonical_symbol.cpp:711` through `src/frontend/sema/canonical_symbol.cpp:717`.
- Consteval entry points keep string registries in their public API for existing callers while accepting text/key mirrors. Evidence: `src/frontend/sema/consteval.hpp:313` through `src/frontend/sema/consteval.hpp:320` requires `consteval_fns` and optional text/key maps, and `src/frontend/sema/validate.cpp:1422` through `src/frontend/sema/validate.cpp:1424` passes the legacy registry plus mirrors.

### diagnostic-only

- Sema dual-lookup match helpers are comparison telemetry; their results are intentionally discarded at call sites. Evidence: `src/frontend/sema/validate.hpp:39` through `src/frontend/sema/validate.hpp:57` define the match enum and pointer comparison helper, and calls such as `src/frontend/sema/validate.cpp:680` through `src/frontend/sema/validate.cpp:682`, `src/frontend/sema/validate.cpp:693` through `src/frontend/sema/validate.cpp:695`, and `src/frontend/sema/validate.cpp:973` through `src/frontend/sema/validate.cpp:978` cast the comparison result to `void`.
- Consteval and type-binding mirror comparisons are also advisory proof checks. Evidence: `src/frontend/sema/consteval.hpp:205` through `src/frontend/sema/consteval.hpp:210` compares legacy/text/key constant lookup then returns the legacy result; `src/frontend/sema/consteval.cpp:119` through `src/frontend/sema/consteval.cpp:122` compares type-binding mirrors and returns the legacy substitution; `src/frontend/sema/consteval.cpp:577` through `src/frontend/sema/consteval.cpp:581` compares consteval function mirrors and returns the legacy callee.
- User-facing diagnostic strings are not semantic lookup ownership. Evidence: undeclared-name and unknown-type diagnostics render `n->name` or `n->type.tag` at `src/frontend/sema/validate.cpp:1957` and `src/frontend/sema/validate.cpp:2285` through `src/frontend/sema/validate.cpp:2288`.

### legacy-proof

- Scoped local symbols are structured-preferred when a local `TextId` key exists, with the string map retained as the legacy mirror and for declarations that do not yet have a structured key. Evidence: `src/frontend/sema/validate.cpp:551` through `src/frontend/sema/validate.cpp:553` define both local maps, `src/frontend/sema/validate.cpp:769` through `src/frontend/sema/validate.cpp:781` dual-writes `bind_local`, and `src/frontend/sema/validate.cpp:822` through `src/frontend/sema/validate.cpp:831` compares both maps and returns the structured local before the legacy local.
- Local initialization updates use the same dual path, so uninitialized-read tracking is covered by structured proof instead of a standalone string-only path. Evidence: `src/frontend/sema/validate.cpp:939` through `src/frontend/sema/validate.cpp:960`.
- Global variables, function signatures, and overload sets retain legacy maps with structured mirrors and comparison checks. Evidence: `src/frontend/sema/validate.cpp:494` through `src/frontend/sema/validate.cpp:527` define legacy and structured maps; `src/frontend/sema/validate.cpp:614` through `src/frontend/sema/validate.cpp:620` mirrors globals; `src/frontend/sema/validate.cpp:623` through `src/frontend/sema/validate.cpp:640` mirrors functions/ref-overloads/C++ overloads; lookup comparisons appear at `src/frontend/sema/validate.cpp:675` through `src/frontend/sema/validate.cpp:710`.
- Function and overload call paths go through the dual lookup helpers. Evidence: function-name expression classification uses `lookup_function_by_name` and `lookup_cpp_overloads_by_name` at `src/frontend/sema/validate.cpp:1970` through `src/frontend/sema/validate.cpp:1971`, and call validation checks ref overloads, C++ overloads, then plain functions at `src/frontend/sema/validate.cpp:2118` through `src/frontend/sema/validate.cpp:2196`.
- Global and local const-int bindings are dual-written for non-enum declarations and are exposed to `ConstEvalEnv` with text/key mirrors. Evidence: const maps are declared at `src/frontend/sema/validate.cpp:501` through `src/frontend/sema/validate.cpp:512`; `record_global_const_binding` writes name/text/key maps at `src/frontend/sema/validate.cpp:882` through `src/frontend/sema/validate.cpp:890`; `record_local_const_binding` writes scoped name/text/key maps at `src/frontend/sema/validate.cpp:893` through `src/frontend/sema/validate.cpp:904`; `populate_const_eval_env` exports them at `src/frontend/sema/validate.cpp:859` through `src/frontend/sema/validate.cpp:871`.
- Consteval function registry and interpreter locals are dual-written with legacy names retained as proof/compatibility. Evidence: `record_consteval_function` writes name/text/key maps at `src/frontend/sema/validate.cpp:642` through `src/frontend/sema/validate.cpp:650`; `lookup_consteval_function_by_name` compares the mirrors at `src/frontend/sema/validate.cpp:727` through `src/frontend/sema/validate.cpp:737`; `InterpreterBindings` carries `by_name`, `by_text`, and `by_key` at `src/frontend/sema/consteval.cpp:596` through `src/frontend/sema/consteval.cpp:610`, with snapshot/restore preserving all three at `src/frontend/sema/consteval.cpp:613` through `src/frontend/sema/consteval.cpp:655`.

### sema-leftover

- Enum type/value bindings still have structured containers that are not populated for enum variants. Evidence: `structured_enum_consts_`, `enum_const_vals_global_by_text_`, `enum_const_vals_global_by_key_`, `enum_const_vals_scopes_by_text_`, and `enum_const_vals_scopes_by_key_` are declared at `src/frontend/sema/validate.cpp:498` through `src/frontend/sema/validate.cpp:506`, but global enum binding writes only `enum_consts_` and `enum_const_vals_global_` at `src/frontend/sema/validate.cpp:907` through `src/frontend/sema/validate.cpp:914`, and local enum binding calls `bind_local` without a structured key and writes only `enum_const_vals_scopes_` at `src/frontend/sema/validate.cpp:917` through `src/frontend/sema/validate.cpp:927`. The lookup path compares `structured_enum_consts_` at `src/frontend/sema/validate.cpp:847` through `src/frontend/sema/validate.cpp:855`, so the mirror is currently inert for enum variants.
- Template NTTP placeholders in validation local scopes are string-only. Evidence: `bind_template_nttps` creates an integer local for each NTTP and calls `bind_local(n->template_param_names[i], ...)` without a `SemaStructuredNameKey` at `src/frontend/sema/validate.cpp:1204` through `src/frontend/sema/validate.cpp:1216`; the AST stores template parameter names as `const char**` without parallel `TextId` fields at `src/frontend/parser/ast.hpp:330` through `src/frontend/parser/ast.hpp:337`.
- Template type-parameter tracking in validation is still name-set based. Evidence: `template_type_params_` is a `std::unordered_set<std::string>` at `src/frontend/sema/validate.cpp:548`; `record_template_type_params_recursive` inserts `template_param_names` at `src/frontend/sema/validate.cpp:1219` through `src/frontend/sema/validate.cpp:1228`; cast validation scans `current_fn_node_->template_param_names` and `template_type_params_` by string at `src/frontend/sema/validate.cpp:2251` through `src/frontend/sema/validate.cpp:2283`.
- Consteval call NTTP bindings expose structured/text slots but bind only the legacy name map. Evidence: `ConstEvalEnv` has `nttp_bindings`, `nttp_bindings_by_text`, and `nttp_bindings_by_key` at `src/frontend/sema/consteval.hpp:154` through `src/frontend/sema/consteval.hpp:157`, and lookup checks all three at `src/frontend/sema/consteval.hpp:276` through `src/frontend/sema/consteval.hpp:297`; `bind_consteval_call_env` writes only `out_nttp_bindings` for explicit/default NTTP values at `src/frontend/sema/consteval.cpp:438` through `src/frontend/sema/consteval.cpp:452` and `src/frontend/sema/consteval.cpp:475` through `src/frontend/sema/consteval.cpp:477`, then installs only `env.nttp_bindings` at `src/frontend/sema/consteval.cpp:506`.
- Type-binding text mirrors are declared and threaded but deliberately unused. Evidence: `TypeBindingTextMap` and `TypeBindingNameTextMap` exist at `src/frontend/sema/consteval.hpp:91` through `src/frontend/sema/consteval.hpp:123`; `record_type_binding_mirrors` discards the text outputs at `src/frontend/sema/consteval.cpp:136` through `src/frontend/sema/consteval.cpp:145`, while only structured keys are written at `src/frontend/sema/consteval.cpp:146` through `src/frontend/sema/consteval.cpp:150`.

### blocked-by-hir

- Struct completeness still returns legacy tag-set results because sema receives record identity through `TypeSpec::tag`. Evidence: complete record maps and structured tag bridge maps are declared at `src/frontend/sema/validate.cpp:528` through `src/frontend/sema/validate.cpp:533`; `is_complete_object_type` extracts `ts.tag`, compares the structured record key when available, and returns `legacy_complete` at `src/frontend/sema/validate.cpp:965` through `src/frontend/sema/validate.cpp:979`.
- Record-key recovery is a tag-string bridge, not a complete structured owner model. Evidence: `structured_record_key_for_tag` and `note_structured_record_key_for_tag` use rendered tags at `src/frontend/sema/validate.cpp:987` through `src/frontend/sema/validate.cpp:1003`, and `note_struct_def` records the tag plus structured key at `src/frontend/sema/validate.cpp:1013` through `src/frontend/sema/validate.cpp:1028`.
- Member and static-member lookup have structured mirrors but still enter and return through rendered record tags and member strings. Evidence: `struct_field_names_`, `struct_static_member_types_`, and `struct_base_tags_` sit beside text/key mirrors at `src/frontend/sema/validate.cpp:536` through `src/frontend/sema/validate.cpp:547`; static-member lookup compares by-key lookup but returns the legacy result at `src/frontend/sema/validate.cpp:1059` through `src/frontend/sema/validate.cpp:1104`; instance-field lookup compares by-key lookup but returns the legacy result at `src/frontend/sema/validate.cpp:1107` through `src/frontend/sema/validate.cpp:1146`; `infer_expr` still splits static member spelling from `n->name` using `"::"` at `src/frontend/sema/validate.cpp:1911` through `src/frontend/sema/validate.cpp:1924`.
- Out-of-class method owner resolution and implicit `this` setup are constrained by rendered record names. Evidence: `qualified_method_owner_struct` parses `fn->name` text at `src/frontend/sema/validate.cpp:377` through `src/frontend/sema/validate.cpp:383`; `struct_defs_by_unqualified_name_` indexes record nodes by string at `src/frontend/sema/validate.cpp:535` and is queried at `src/frontend/sema/validate.cpp:1149` through `src/frontend/sema/validate.cpp:1173`; `validate_function` stores `current_method_struct_tag_` and assigns `this_ts.tag` from it at `src/frontend/sema/validate.cpp:1545` through `src/frontend/sema/validate.cpp:1556`.
- Consteval record layout lookup is HIR-structure keyed by rendered `TypeSpec::tag`. Evidence: `ConstEvalEnv::struct_defs` is a `std::unordered_map<std::string, HirStructDef>` at `src/frontend/sema/consteval.hpp:159` through `src/frontend/sema/consteval.hpp:162`, and `lookup_record_layout` looks up `ts.tag` at `src/frontend/sema/consteval.cpp:154` through `src/frontend/sema/consteval.cpp:160`.
- `static_eval_int` remains a string-map helper for HIR and legacy constant evaluation callers, not a sema validation owner after idea 96. Evidence: the API accepts `std::unordered_map<std::string, long long>` at `src/frontend/sema/type_utils.hpp:69` through `src/frontend/sema/type_utils.hpp:72`, lookup is by `n->name` at `src/frontend/sema/type_utils.cpp:861` through `src/frontend/sema/type_utils.cpp:870`, and current references are HIR/lowering-side string maps such as `src/frontend/hir/hir_types.cpp:1279` and `src/frontend/hir/impl/expr/scalar_control.cpp:131`.

### Step 3 Handoff

- Sema value/function/overload/local/consteval lookup is broadly dual-written and either structured-preferred for locals or legacy-returning with structured proof for global/function-like maps.
- Meaningful sema leftovers remain in enum variant mirrors, template NTTP/type-parameter validation placeholders, consteval NTTP binding mirrors, and dead text-binding mirror parameters.
- Struct completeness, member/static-member lookup, `TypeSpec::tag`, record layout lookup, and `static_eval_int` should remain out of idea 98 unless a narrow sema metadata handoff is needed; the main cleanup there is blocked by HIR/type identity migration.

## HIR Inventory By Subsystem

### Module Function And Global Identity

- Current surface: `Module::fn_index` and `Module::global_index` are rendered `SymbolName` maps, while `Function` and `GlobalVar` carry rendered `name`, `LinkNameId`, and `NamespaceQualifier`. Evidence: `src/frontend/hir/hir_ir.hpp:681` through `src/frontend/hir/hir_ir.hpp:687`, `src/frontend/hir/hir_ir.hpp:744` through `src/frontend/hir/hir_ir.hpp:750`, and `src/frontend/hir/hir_ir.hpp:949` through `src/frontend/hir/hir_ir.hpp:968`.
- Lookup behavior already has an ID/legacy boundary: `resolve_function_decl` uses `DeclRef::link_name_id` before `find_function_by_name_legacy`, and `resolve_global_decl` uses `DeclRef::global`, then `link_name_id`, then legacy name lookup. Evidence: `src/frontend/hir/hir_ir.hpp:1042` through `src/frontend/hir/hir_ir.hpp:1091` and `src/frontend/hir/hir_ir.hpp:1120` through `src/frontend/hir/hir_ir.hpp:1136`.
- Upstream structured-key availability: strong enough for a first mirror. AST nodes expose unqualified text IDs, qualifier segments, qualifier text IDs, and namespace context IDs at `src/frontend/parser/ast.hpp:292` through `src/frontend/parser/ast.hpp:295`; HIR imports qualifier structure through `make_ns_qual` at `src/frontend/hir/hir_lowering_core.cpp:30` through `src/frontend/hir/hir_lowering_core.cpp:43`, and imports unqualified name text through `make_unqualified_text_id` at `src/frontend/hir/hir_lowering_core.cpp:51` through `src/frontend/hir/hir_lowering_core.cpp:59`. `DeclRef` already stores `name_text_id`, `link_name_id`, IDs, and `ns_qual` at `src/frontend/hir/hir_ir.hpp:297` through `src/frontend/hir/hir_ir.hpp:305`.
- Mirror feasibility: good. A HIR-only `SymbolLookupKey` can be built from declaration kind, `NamespaceQualifier::context_id`, qualifier segment text IDs, global-qualified bit, and unqualified `TextId`. The only missing declaration-side field is `Function::name_text_id` / `GlobalVar::name_text_id`; those can be populated from the existing AST metadata during `lower_function`, consteval lowering, bodyless callable registration, and `lower_global`. Evidence for registration sites: `src/frontend/hir/hir_functions.cpp:129` through `src/frontend/hir/hir_functions.cpp:138`, `src/frontend/hir/hir_functions.cpp:90` through `src/frontend/hir/hir_functions.cpp:104`, `src/frontend/hir/hir_build.cpp:488` through `src/frontend/hir/hir_build.cpp:509`, and `src/frontend/hir/hir_types.cpp:1535` through `src/frontend/hir/hir_types.cpp:1635`.
- Rendered-name/codegen requirements: rendered `name` and `LinkNameId` must remain authoritative for emission and ABI/link spelling. Evidence: HIR interns link names during lowering at `src/frontend/hir/hir_functions.cpp:135` through `src/frontend/hir/hir_functions.cpp:138` and `src/frontend/hir/hir_types.cpp:1587`; LIR resolves emitted names through `LinkNameId` with fallback spelling at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:27` through `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:31`; LIR extern declarations already dedup by `LinkNameId` before raw name at `src/codegen/lir/ir.hpp:569` through `src/codegen/lir/ir.hpp:610`.
- Proof coverage: focused proof should include `frontend_hir_tests` plus HIR dump cases that exercise namespace globals, function/template calls, template global instantiation, consteval-only functions, and link-name fallback. Candidate CTest selectors: `frontend_hir_tests`, `cpp_hir_template_global_specialization`, `cpp_hir_if_constexpr_branch_unlocks_later`, and a namespace runtime/HIR case if the slice adds one.
- Parser/sema metadata dependency: no parser/sema rewrite is required for a first mirror. The AST metadata is already present; the HIR slice only needs to preserve it on declarations and compare structured lookup against existing string lookup.

### Struct, Type Tag, And Layout Identity

- Current surface: `Module::struct_defs` and `struct_def_order` are keyed by rendered tags, and `TypeSpec::tag` drives layout, type compatibility, and codegen lookup. Evidence: `src/frontend/hir/hir_ir.hpp:963` through `src/frontend/hir/hir_ir.hpp:965`; `src/frontend/hir/hir_lowering_core.cpp:329` through `src/frontend/hir/hir_lowering_core.cpp:370`; `src/frontend/hir/hir_lowering_core.cpp:467` through `src/frontend/hir/hir_lowering_core.cpp:477`; and `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:54` through `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:56`.
- Upstream structured-key availability: partial. `HirStructDef` already records `tag_text_id`, `NamespaceQualifier`, and `base_tag_text_ids`, and fields have `field_text_id` plus `MemberSymbolId`. Evidence: `src/frontend/hir/hir_ir.hpp:777` through `src/frontend/hir/hir_ir.hpp:809`; population appears at `src/frontend/hir/hir_types.cpp:1206`, `src/frontend/hir/hir_types.cpp:1243` through `src/frontend/hir/hir_types.cpp:1244`, and `src/frontend/hir/hir_types.cpp:1312` through `src/frontend/hir/hir_types.cpp:1317`.
- Mirror feasibility: moderate, but not the first slice. HIR could add a struct-key mirror for `struct_defs`, but most type consumers still receive only `TypeSpec::tag`; a mirror would need a bridging recovery path from tag to key before it could be used broadly.
- Rendered-name/codegen requirements: high. LLVM type declarations are still built from rendered tags and base tags at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:227` through `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:276`; helper formatting uses `TypeSpec::tag` for LLVM struct names at `src/codegen/shared/llvm_helpers.hpp:444` through `src/codegen/shared/llvm_helpers.hpp:445`.
- Proof coverage: HIR layout and codegen cases already cover this area, including packed/aligned layout, field-array layout, builtin layout queries, inheritance, and member access. This needs a later slice with before/after HIR dump and emitted LLVM checks because a bad tag mirror could perturb layout or type declarations.
- Parser/sema metadata dependency: struct metadata exists in HIR but sema still classifies struct completeness/member lookup as blocked by HIR because record identity is handed around as `TypeSpec::tag`. This should remain outside the first slice.

### Member, Static Member, And Method Identity

- Current surface: static members are stored as `{tag -> member}` string maps; methods use `"struct_tag::method_name"` keys and const suffixes; member-symbol lookup constructs `tag + "::" + member`. Evidence: declarations in `src/frontend/hir/impl/lowerer.hpp:959` through `src/frontend/hir/impl/lowerer.hpp:968`, static member lookup in `src/frontend/hir/hir_types.cpp:926` through `src/frontend/hir/hir_types.cpp:960`, member symbol lookup in `src/frontend/hir/hir_types.cpp:963` through `src/frontend/hir/hir_types.cpp:977`, and method registration in `src/frontend/hir/hir_types.cpp:1460` through `src/frontend/hir/hir_types.cpp:1531`.
- Upstream structured-key availability: member-level identity is strong (`field_text_id`, `MemberSymbolId`), but owner identity is still rendered tag based. `MemberExpr` carries `field_text_id`, `resolved_owner_tag`, and `member_symbol_id` at `src/frontend/hir/hir_ir.hpp:409` through `src/frontend/hir/hir_ir.hpp:415`; HIR populates member refs at `src/frontend/hir/impl/expr/scalar_control.cpp:210` through `src/frontend/hir/impl/expr/scalar_control.cpp:218`.
- Mirror feasibility: good for field/member lookup after owner-tag migration begins; weak as a first slice because the owner key would still be derived from `TypeSpec::tag`. Codegen already prefers `MemberSymbolId` for field resolution and falls back to field text, so this surface is partially mirrored downstream. Evidence: `src/codegen/lir/hir_to_lir/lvalue.cpp:601` through `src/codegen/lir/hir_to_lir/lvalue.cpp:618` and `src/codegen/lir/hir_to_lir/lowering.hpp:381` through `src/codegen/lir/hir_to_lir/lowering.hpp:393`.
- Rendered-name/codegen requirements: method mangled names and struct tags remain emission/link bridges. Range-for and operator lowering still route through method maps and legacy function lookup, for example `src/frontend/hir/impl/stmt/range_for.cpp:19` through `src/frontend/hir/impl/stmt/range_for.cpp:53` and `src/frontend/hir/impl/expr/call.cpp:72` through `src/frontend/hir/impl/expr/call.cpp:116`.
- Proof coverage: HIR dump tests cover inherited methods, member calls, operator/member helpers, range-for helpers, and template struct body instantiation. This is a good second or third HIR slice after module symbol mirrors are proven.
- Parser/sema metadata dependency: no parser/sema rewrite is needed for member names, but owner identity is blocked on HIR struct/type keying.

### Function-Local Symbol And Template Binding Identity

- Current surface: `FunctionCtx` local maps remain string-keyed for locals, function pointer signatures, params, static globals, labels, const bindings, template bindings, NTTP bindings, packs, and method owner tag. Evidence: `src/frontend/hir/impl/lowerer.hpp:263` through `src/frontend/hir/impl/lowerer.hpp:293`.
- Upstream structured-key availability: mixed. AST names and HIR `DeclRef` have `TextId`, and `LocalId` is already the durable local identity after resolution, but `LocalDecl` currently lacks a `name_text_id`. Params do have `name_text_id` at `src/frontend/hir/hir_ir.hpp:214` through `src/frontend/hir/hir_ir.hpp:219`.
- Mirror feasibility: moderate. Local and param mirrors can be keyed by `TextId` and compared against string maps, but the slice would touch many expression/statement lowering paths and is less isolated than module function/global mirrors.
- Rendered-name/codegen requirements: local names, label names, and generated temporary names remain dump/debug and codegen spellings. They should be preserved even if lookup gains a text-id mirror.
- Proof coverage: `frontend_hir_tests` and helper HIR dump tests for statement declarations, scalar control, call/member helpers, range-for, and template function pack signatures are relevant.
- Parser/sema metadata dependency: likely none for locals/params; template parameter name text IDs are weaker because AST template parameter names are still mostly `const char**` arrays, so type/NTTP binding mirrors should follow parser/sema template metadata cleanup rather than lead HIR migration.

### Template And Compile-Time Registries

- Current surface: HIR has both semantic instance keys and rendered-name registries. `InstantiationRegistry` uses `FunctionTemplateInstanceKey` / `TemplateStructInstanceKey` for dedup when a primary definition is available, while `seed_work_`, `instances_`, `template_fn_defs_`, `template_struct_defs_`, `template_struct_specializations_`, and `consteval_fn_defs_` remain string-keyed by template/function name. Evidence: `src/frontend/hir/compile_time_engine.hpp:383` through `src/frontend/hir/compile_time_engine.hpp:526`, `src/frontend/hir/compile_time_engine.hpp:733` through `src/frontend/hir/compile_time_engine.hpp:843`, and private maps at `src/frontend/hir/compile_time_engine.hpp:1012` through `src/frontend/hir/compile_time_engine.hpp:1024`.
- Upstream structured-key availability: good for realized template instances because primary `Node*` plus `SpecializationKey` is already in use. Weak for definition registries where `register_template_def`, `register_template_struct_def`, and `register_consteval_def` still accept rendered names. Evidence: `src/frontend/hir/compile_time_engine.hpp:743` through `src/frontend/hir/compile_time_engine.hpp:776`.
- Mirror feasibility: good but not first. The registry already has parity checks (`verify_parity`) and structured dedup sets, so the next useful cleanup would mirror definition lookup by declaration key or primary-node identity while preserving string lookup. It is broader than module symbol mirrors because deferred instantiation, consteval, static assertions, and template metadata all depend on it.
- Rendered-name/codegen requirements: template instantiations still need rendered/mangled names for module lookup and emission. Evidence: `record_seed` returns a mangled name at `src/frontend/hir/compile_time_engine.hpp:486` through `src/frontend/hir/compile_time_engine.hpp:525`; template metadata interns mangled link names at `src/frontend/hir/hir_build.cpp:392` through `src/frontend/hir/hir_build.cpp:397`; deferred instantiation probes `module.template_defs` and `target_name` at `src/frontend/hir/impl/compile_time/engine.cpp:171` through `src/frontend/hir/impl/compile_time/engine.cpp:235`.
- Proof coverage: HIR template/consteval cases are strong: deferred NTTP expression cases, template struct body instantiation, template global specialization, inherited member typedef, multistage shape chain, and if-constexpr branch unlocks. Registry parity output is source-level evidence but should be paired with focused CTest.
- Parser/sema metadata dependency: no parser/sema rewrite is required for semantic instance keys; definition-name mirrors would benefit from the same declaration structured key used by module function/global lookup.

### Enum, Const-Int, And Consteval Runtime Environments

- Current surface: HIR lowerer and compile-time state keep enum constants and const-int bindings in string maps, and consteval environments consume those maps. Evidence: `src/frontend/hir/impl/lowerer.hpp:937` through `src/frontend/hir/impl/lowerer.hpp:943`, `src/frontend/hir/compile_time_engine.hpp:936` through `src/frontend/hir/compile_time_engine.hpp:944`, and `src/frontend/hir/impl/compile_time/engine.cpp:103` through `src/frontend/hir/impl/compile_time/engine.cpp:122`.
- Upstream structured-key availability: mixed. Sema Step 3 found enum variants as a sema leftover and `static_eval_int` as blocked-by-HIR. HIR callers still look up `enum_consts_` and NTTP names by `n->name`, for example `src/frontend/hir/impl/expr/scalar_control.cpp:129` through `src/frontend/hir/impl/expr/scalar_control.cpp:146`.
- Mirror feasibility: moderate after sema enum/NTTP mirror cleanup, but risky as a first HIR slice because constant lookup participates in template/consteval reduction and can silently alter compile-time folding.
- Rendered-name/codegen requirements: rendered constant names are not ABI output, but diagnostics and compatibility APIs still use them.
- Proof coverage: consteval and HIR deferred-template tests would catch many regressions; however source evidence shows this should be scoped after idea 98 sema leftovers or alongside a narrowly defined HIR constant environment mirror.
- Parser/sema metadata dependency: yes for enum variant structured keys and NTTP parameter text/key population. Do not include this in idea 99's first HIR slice.

### Downstream Link And Codegen Handoff

- Current surface: link-visible function/global identity is already carried by `LinkNameId` from HIR into LIR. Evidence: `Function::link_name_id`, `GlobalVar::link_name_id`, and `DeclRef::link_name_id` in `src/frontend/hir/hir_ir.hpp:681` through `src/frontend/hir/hir_ir.hpp:686`, `src/frontend/hir/hir_ir.hpp:744` through `src/frontend/hir/hir_ir.hpp:749`, and `src/frontend/hir/hir_ir.hpp:297` through `src/frontend/hir/hir_ir.hpp:305`; LIR carries link IDs at `src/codegen/lir/ir.hpp:486` through `src/codegen/lir/ir.hpp:497` and `src/codegen/lir/ir.hpp:524` through `src/codegen/lir/ir.hpp:528`.
- Upstream structured-key availability: existing `LinkNameId` is a stable rendered-link identity, not a full source semantic key. It is a bridge that should coexist with HIR source-name keys.
- Mirror feasibility: already partly realized downstream. The first HIR source-key slice should not replace `LinkNameId`; it should use source-key lookup for semantic declaration resolution and keep link IDs for ABI/codegen.
- Rendered-name/codegen requirements: mandatory. LIR and LLVM helpers still render names and struct type strings; `str_pool_map`/string literal byte dedup remains unrelated to declaration identity.
- Proof coverage: emitted LLVM and runtime tests are relevant only after behavior changes. The first mirror slice can use HIR dumps and existing CTest without changing emitted names.
- Parser/sema metadata dependency: none.

### Recommended First HIR Slice

Start idea 99 with a module-level function/global structured lookup mirror, not struct/type tags or template/consteval constants.

Proposed first slice:

- Add a small HIR declaration lookup key for functions/globals based on source-name metadata already available in HIR lowering: declaration kind, namespace context/global-qualified bit, qualifier segment text IDs, and unqualified `TextId`.
- Add `name_text_id` to HIR `Function` and `GlobalVar`, populate it where functions/globals are lowered or registered, and dual-write `Module` mirrors next to `fn_index` and `global_index`.
- Add structured lookup helpers for `DeclRef` that try existing concrete IDs first (`GlobalId`, `LinkNameId`), then the new source-key mirror when `DeclRef::name_text_id` / `ns_qual` are present, then legacy `fn_index` / `global_index`.
- Compare structured and legacy results in proof mode or internal helpers, but return the existing behavior during the slice. Preserve `Function::name`, `GlobalVar::name`, `LinkNameId`, mangled template names, and all codegen rendered output.
- Validate with `frontend_hir_tests` plus focused HIR CTest selectors for function/global lookup, namespace lookup, template global specialization, and consteval template call materialization. If codegen output is touched unexpectedly, escalate to LIR/codegen subsets.

Reasoning:

- It is HIR-owned and behavior-preserving.
- It uses existing parser metadata without reopening parser/sema work.
- It exercises the same dual-write / dual-read discipline used by ideas 95 and 96.
- It avoids the high-risk `TypeSpec::tag`, `struct_defs`, member-method, enum, const-int, and compile-time registry surfaces until one small HIR mirror is proven.

## Proof Gaps And Validation Candidates

- No proof gap was discovered that required a build during Step 1.
- No Step 3 sema leftover claim required runtime proof; source evidence was sufficient for classification.
- No Step 4 HIR claim required runtime proof; source evidence was sufficient for subsystem classification.
- First HIR implementation slice validation should start with `ctest --test-dir build -R "frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain" --output-on-failure`, then broaden only if the slice touches emitted codegen or struct/type layout.
- Parser classification proof candidates: parse-only namespace/using/template cases covering structured lookup and legacy mismatch counters.
- Sema classification proof candidates: focused frontend/sema cases for scoped locals, overloads, consteval, enum/const-int bindings, and member/static-member lookup.
- HIR first-slice proof candidates: HIR dump/summary CTest cases touching module function/global/struct lookup, template instantiation, consteval, member lookup, and LIR/codegen link-name preservation.

## Follow-On Recommendations

- Idea 98 is required and has been drafted as `ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md`.
- Idea 98 scope is parser/sema-only post-cleanup work: parser helper overload leftovers, sema enum variant mirror population, sema template NTTP/type-parameter validation mirrors, consteval NTTP binding mirrors, and type-binding text mirror cleanup. It explicitly excludes HIR module maps, `TypeSpec::tag`, struct layout, member/method owner identity, compile-time engine registries, codegen names, diagnostics, and link-name output.
- Idea 99 has been drafted as `ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`.
- Idea 99 scope is the HIR module function/global structured lookup mirror as the first behavior-preserving HIR slice. It should add declaration-side name text IDs, dual-write structured module mirrors beside rendered `fn_index` / `global_index`, preserve concrete IDs and `LinkNameId` precedence, keep rendered names for diagnostics/codegen/link output, and avoid struct/type, member/method, template registry, enum/const-int, and consteval environment cleanup in the first slice.

## Final Consistency Check

Acceptance status for idea 97:

- The review artifact classifies parser, sema, and HIR lookup identity state after ideas 95 and 96.
- Parser findings are grouped as `bridge-required`, `diagnostic-only`, `legacy-proof`, `parser-leftover`, and `blocked-by-downstream`.
- Sema findings are grouped as `bridge-required`, `diagnostic-only`, `legacy-proof`, `sema-leftover`, and `blocked-by-hir`.
- Parser/sema leftovers are explicitly separated from bridge-required, diagnostic-only, legacy-proof, and downstream/HIR-blocked strings.
- Idea 98 is required and exists at `ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md`.
- Idea 98 targets only parser/sema leftovers and keeps HIR/type/codegen rendered-name bridges out of scope.
- Idea 99 exists at `ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`.
- Idea 99 defines the HIR dual-lookup starting strategy as a module function/global structured lookup mirror and preserves rendered names, mangled/template names, diagnostics, `LinkNameId`, and codegen/link output.
- HIR cleanup remains separate from parser/sema completion work: HIR module mirrors start in idea 99, while parser helper overloads and sema enum/template/consteval mirror leftovers remain in idea 98.

Step 6 proof commands:

- `git status --short`
- `sed -n '1,240p' plan.md`
- `sed -n '1,260p' todo.md`
- `sed -n '1,260p' ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
- `sed -n '1,260p' ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md`
- `sed -n '1,280p' ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`
- `sed -n '1,320p' review/97_structured_identity_completion_audit.md`
- `sed -n '260,620p' review/97_structured_identity_completion_audit.md`
- `find ideas/open -maxdepth 1 -type f | sort`
- `find review -maxdepth 1 -type f | sort`

No build, test, or `test_after.log` was needed for Step 6 because this packet was a source/document consistency audit only.

Suggested supervisor lifecycle decision: route to the plan owner for idea 97 closure/deactivation review, leaving idea 98 and idea 99 open as separate follow-on source ideas.
