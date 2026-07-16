# Current Packet

Status: Active
Source Idea Path: ideas/open/852_hir_canonical_semantic_aggregate_ref_binding.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize the binding and attach occurrence refs

## Just Finished

- Completed plan Step 1 by read-only trace. Selected seam/API: HIR `Lowerer` signature construction, specifically `Lowerer::lower_function` for ordinary free-function returns and `Lowerer::append_explicit_callable_param` for explicit parameters, with the binding feeding the existing `Lowerer::qtype_from(TypeSpec, ValueCategory, std::optional<HirAggregateRef>)` parameter before `Function::return_type` / `Param::type` are finalized.
- Source facts inspected: sema exposes `ResolvedTypeTable::lookup(const Node*)`, `CanonicalFunctionSig::{return_type, params}`, and recursive `CanonicalType::{kind, identity, element_type, function_sig}` in `src/frontend/sema/canonical_symbol.hpp`; `build_initial_hir` passes the table into `Lowerer::resolved_types_` in `src/frontend/hir/hir_build.cpp`.
- Return path inspected: `src/frontend/hir/hir_functions.cpp` lines 532-543 currently prepares `fn_node->type` then calls `qtype_from(ret_ts)` without feeding an aggregate ref. Parameter path inspected: lines 627-629 call `append_callable_params`, lines 1242-1244 pass each explicit source parameter into `append_explicit_callable_param`, and lines 1153-1158 currently assign `param.type = qtype_from(reference_storage_ts(param_ts), ValueCategory::LValue)` without feeding an aggregate ref.
- Definition/ref registration inspected: ordinary structs are lowered during `collect_initial_type_definitions` before non-method functions (`src/frontend/hir/hir_build.cpp` lines 445-448 and 1041-1044); `lower_struct_def` stores `HirStructDef` in `Module::struct_defs` then immediately sets `stored_def.aggregate_ref = module_->register_aggregate_definition(stored_def)` (`src/frontend/hir/hir_types.cpp` lines 3579-3581). Template struct instantiation has the same store-then-register pattern in `src/frontend/hir/impl/templates/struct_instantiation.cpp` lines 563-566.
- Authority rejected: parser pointers, `record_def`, owner-key/tag/text, rendered lookup, `Node*` maps, and unfed sidecars are not the occurrence authority. They may only help locate an already registered HIR definition during materialization; the attached occurrence authority must be the module-owned `HirAggregateRef`, validated by `Module::owns_aggregate_ref` in `qtype_from` (`src/frontend/hir/hir_types.cpp` lines 469-481).

## Suggested Next

- Execute plan Step 2 by adding a narrow HIR-owned materialization helper used by `lower_function` and `append_explicit_callable_param`: obtain the canonical function type from `resolved_types_->lookup(fn_node)` or canonicalize the declaration as a fallback only when equivalent production canonical data is present, traverse return/parameter `CanonicalType` wrappers recursively to the aggregate leaf, bind canonical aggregate identity to an already registered `HirStructDef::aggregate_ref`, and feed the resulting owned ref into `qtype_from` for `Function::return_type` and `Param::type`.

## Watchouts

- Ordering must remain after aggregate registration and before signature finalization: `lower_initial_program` registers ordinary and materialized template struct definitions before `lower_non_method_functions_and_globals`, so Step 2 should run inside function signature construction and fail closed if a definition/ref is missing at that point.
- The canonical identity relation should be sema `CanonicalTypeKind::{Struct,Union}` identity to a registered HIR definition/ref. Recursive traversal must preserve wrappers such as pointer/reference/array/function-signature children; only aggregate leaves get refs, and unsupported/missing canonical, incomplete, ambiguous, invalid, foreign, wrong-module, or use-before-registration cases must leave `aggregate_ref` unset or report an explicit fail-closed diagnostic according to the implementation surface.
- Do not start LIR migration or recover identity through parser, key, tag, text, `record_def`, or `Node*` authority. Do not hide the current unset-ref failure behind a helper that is not fed from production canonical signature/type facts.

## Proof

- No build required by packet because only `todo.md` evidence changed. Proof was a read-only production trace of `src/frontend/sema/canonical_symbol.hpp`, `src/frontend/hir/hir_ir.hpp`, `src/frontend/hir/impl/lowerer.hpp`, `src/frontend/hir/hir_build.cpp`, `src/frontend/hir/hir_types.cpp`, `src/frontend/hir/hir_functions.cpp`, and `src/frontend/hir/impl/templates/struct_instantiation.cpp`; no `test_after.log` produced for this documentation-only packet.
