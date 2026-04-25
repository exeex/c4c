Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit setup and inventory

# Current Packet

## Just Finished

Step 1 inventory created `review/97_structured_identity_completion_audit.md` and recorded parser, sema, and HIR source evidence for idea 97. Confirmed `plan.md` and `todo.md` point to `ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`.

Inventory commands used:
- `git status --short`
- `sed -n '1,220p' todo.md`
- `sed -n '1,220p' plan.md`
- `sed -n '1,220p' ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
- `rg -n "(NamePath|QualifiedName|QualifiedNameLookup|LocalNameTable|SymbolId|MemberSymbolId|TextId|structured|legacy|mismatch|struct_tag_def_map|template_.*by_key|nttp_default_expr|UsingValueAlias|value_bind|using_value|find_value|lookup_value|resolve_value)" src/frontend/parser src/shared`
- `rg -n "(NamePath|QualifiedName|QualifiedNameLookup|LocalNameTable|SymbolId|MemberSymbolId|TextId|structured|legacy|mismatch|scoped|local_symbol|global_symbols|function_signatures|overload|enum|const_int|consteval|canonical|member|static_member|find\\(|lookup|resolve|bind)" src/frontend/sema src/frontend/parser/ast.hpp src/shared`
- `rg -n "(std::unordered_map<|std::map<|find\\(|\\[.*\\]|struct_defs|functions|globals|member_symbols|link_names|TypeSpec::tag|\\.tag|template_.*defs|specializations|instantiated_|enum_consts_|const_int_bindings_|struct_methods_|method_link|ret_types|function_decl_nodes_|consteval|deferred|NttpBindings|TypeBindings)" src/frontend/hir src/codegen src/backend | head -n 260`
- `rg -n "(struct (DefinitionState|TemplateState|Parser|Sema|Scope|Local|Module|Lowerer|FunctionContext)|class (Parser|Sema|Lowerer)|using .*Bindings|std::unordered_map<.*std::string|QualifiedNameLookup|LocalNameTable)" src/frontend/parser src/frontend/sema src/frontend/hir src/shared`
- Focused `nl -ba ... | sed -n ...` reads for the parser, sema, HIR, compile-time engine, and codegen/link-name handoff files recorded in the review artifact.

Main surfaces found:
- Parser: `QualifiedNameKey`, `NamePathTable`, `LocalNameTable`, `ParserBindingState::{struct_typedefs,value_bindings}`, `ParserTemplateState::*_by_key` mirrors, legacy `template_struct_defs`, `template_struct_specializations`, `instantiated_template_struct_keys`, `nttp_default_expr_tokens`, `struct_tag_def_map`, namespace/using value lookup, and parser support helpers that still accept string-keyed maps.
- Sema: `SemaStructuredNameKey`, structured mirrors for globals/functions/overloads/consteval/enum/local scopes/member lookup, string legacy maps in `validate.cpp`, `ConstEvalEnv` string/TextId/structured maps, `InterpreterBindings`, and canonical symbol/type string-name surfaces.
- HIR: `Module::{fn_index,global_index,struct_defs,template_defs}`, `LinkNameTable` and `MemberSymbolTable` handoffs, `Lowerer::FunctionCtx` string lookup maps, HIR template/consteval/enum/const-int registries, method/static-member/member-symbol lookup keys, compile-time engine seed/instance maps, and LIR/codegen `TypeSpec::tag` / `LinkNameId` boundaries.

## Suggested Next

Run Step 2 parser completion audit using `review/97_structured_identity_completion_audit.md` as the source map. Classify parser value binding lookup, using value aliases, NTTP default cache keys, template instantiation dedup keys, and parser string helper overloads into the idea 97 parser categories.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Create idea 98 only if parser/sema leftovers are meaningful after classification.
- Always keep HIR migration scope in idea 99 separate from parser/sema completion work.
- HIR already has `LinkNameId`/`MemberSymbolId` handoff surfaces, but the dominant identity maps remain rendered-string keyed; Step 4 should avoid collapsing rendered codegen requirements into parser/sema leftover work.

## Proof

No build or test run. The delegated proof for Step 1 was source inventory only unless an audit claim needed proof; no such claim was made. No `test_after.log` was produced for this read-only inventory packet.
