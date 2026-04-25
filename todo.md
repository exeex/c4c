Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory compile-time registry surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for compile-time, template, and consteval registry surfaces.

Relevant `CompileTimeState` registry APIs in `src/frontend/hir/compile_time_engine.hpp`:
- Registrations: `register_template_def`, `register_template_struct_def`, `register_template_struct_specialization(std::string, Node*)`, `register_template_struct_specialization(Node*, Node*)`, `register_consteval_def`.
- Lookups: `has_template_def`, `has_template_struct_def`, `has_consteval_def`, `is_consteval_template`, `find_template_def`, `find_template_struct_def`, `find_template_struct_specializations(std::string)`, `find_template_struct_specializations(Node*)`, `find_consteval_def`, plus iteration/metadata paths `for_each_template_def`, `record_deferred_instance`, and `instances_to_hir_metadata`.

Registration call sites found:
- `src/frontend/hir/hir_build.cpp`: `collect_consteval_function_definitions` registers consteval functions by `item->name`; `collect_template_function_definitions` registers template functions by `item->name`; `collect_function_template_specializations` looks up a template function by `item->name` before registering specialization ownership.
- `src/frontend/hir/hir_build.cpp`: `collect_initial_type_definitions` registers primary template structs through `register_template_struct_primary(item->name, item)` and specialization patterns through `register_template_struct_specialization(primary_tpl, item)`, with a rendered-name namespace fallback for `template_origin_name`.
- `src/frontend/hir/impl/templates/templates.cpp`: `Lowerer::register_template_struct_primary` dual-writes the lowerer local map and `ct_state_->register_template_struct_def`; `Lowerer::register_template_struct_specialization` dual-writes the lowerer local map and `ct_state_->register_template_struct_specialization`.

Lookup call sites and string-use classification:
- Semantic template function lookup: `hir_build.cpp` `record_template_seed`, `resolve_template_call_name`, `collect_template_instantiations`, `collect_consteval_template_instantiations`, `instantiate_deferred_template`; `impl/expr/call.cpp` explicit/deduced template call lowering and pack-forward expansion; `hir_types.cpp` template call result inference.
- Semantic consteval lookup: `impl/expr/call.cpp` `try_lower_consteval_call_expr`; `impl/compile_time/engine.cpp` `evaluate_pending_consteval` and `try_evaluate_consteval_call_expr`.
- Semantic/template availability guard: `impl/compile_time/engine.cpp` deferred template pre-check uses `has_template_def` and `is_consteval_template`.
- Semantic template struct lookup: `compile_time_engine.hpp` pending template type canonicalization uses `find_template_struct_def`; lowerer-local template struct lookup remains in `impl/templates/templates.cpp` and feeds `CompileTimeState` registration.
- Diagnostic/output spelling: `impl/compile_time/engine.cpp` unresolved-template diagnostics concatenate `TemplateCallInfo::source_template`; `hir_build.cpp` materializes `HirTemplateDef::name` and `name_text_id` from the rendered map key; `TemplateCallInfo::source_template` and consteval call metadata carry stable text ids for HIR output/inspection.
- Compatibility fallback: every current registry map remains keyed by rendered `std::string`; direct call `DeclRef`s are built from mangled/rendered names; `materialize_hir_template_defs`, `record_deferred_instance`, and `instances_to_hir_metadata` consume rendered source template names.
- Parity observation: existing `InstantiationRegistry::verify_parity` and `dump_parity` cover seed/instance parity, not structured-vs-rendered definition registry parity. New parity should be observational only.

Structured source identity available now:
- Declaration nodes already carry `namespace_context_id`, `is_global_qualified`, `unqualified_name`, `unqualified_text_id`, `qualifier_segments`, and `qualifier_text_ids`.
- HIR helpers already normalize this shape as `NamespaceQualifier` plus `TextId` via `make_ns_qual` and `make_unqualified_text_id`; `ModuleDeclLookupKey` is an existing pattern for kind + namespace context + qualifier segment `TextId`s + declaration `TextId`.
- Registration sites that receive the declaration `Node*` can provide a structured key immediately for template functions, primary template structs, struct specialization owners when `primary_tpl` is present, and consteval functions.

Temporary declaration-pointer bridge needed:
- Most lookup sites currently have only rendered names such as `n->left->name`, `call->left->name`, `TemplateCallInfo::source_template`, `PendingConstevalExpr::fn_name`, or `tpl_name`. They do not consistently carry the original declaration key through the HIR call metadata.
- First implementation should therefore use a structured map keyed from declaration nodes for registration plus a temporary `const Node*` bridge for lookups that already recovered the declaration through the legacy string map. Fully structured lookup overloads for call refs should wait until Step 4 extends metadata handoff.

## Suggested Next

First implementation packet: edit only `src/frontend/hir/compile_time_engine.hpp` to define HIR-owned structured registry key types and dual-write storage for template function definitions, primary template struct definitions, template struct specialization owners, and consteval function definitions.

Boundary:
- Add key construction from `const Node*` using declaration kind, `namespace_context_id`, `is_global_qualified`, qualifier `TextId`s, and unqualified declaration `TextId`; fall back to a temporary declaration-pointer component only when the AST text identity is incomplete.
- Add structured mirror maps beside the existing rendered-name maps.
- Keep all existing string registration and lookup APIs behavior-preserving. Existing call sites should continue to compile unchanged in this first packet.
- Dual-write only when `register_*` receives a declaration node with enough identity or an explicit primary declaration pointer.
- Add only observational helpers/counters if needed for later parity; do not change emitted names, diagnostics, `TemplateCallInfo`, `HirTemplateDef`, or call lowering yet.

Fallback policy: structured mirror is best-effort; missing or incomplete identity silently leaves the rendered-name map as the authoritative compatibility path. No lookup should prefer structured identity until a later packet provides complete lookup metadata and a rendered fallback.

## Watchouts

Do not remove legacy string maps, change emitted names or diagnostics, downgrade expectations, or migrate enum/const-int, method/member, static-member, `TypeSpec::tag`, `Module::struct_defs`, struct layout, or codegen type-name ownership in this plan.

The `forward` template path in `impl/expr/call.cpp` is a string-only compatibility call today. Treat it as legacy fallback until structured call metadata exists.

The deferred engine paths use `TemplateCallInfo::source_template` and `module.template_defs` by rendered name; these are output/compatibility surfaces and should not be changed in the first implementation packet.

AST-backed function-callee queries resolved the translation unit symbols but did not resolve method callees by unqualified function name; the inventory used targeted source neighborhoods for those classifications.

## Proof

Inventory-only packet; no build or tests run, and `test_after.log` was not rewritten.

Next implementation proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'
```
