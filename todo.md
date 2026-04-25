Status: Active
Source Idea Path: ideas/open/101_hir_enum_const_int_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read semantic lookup paths

# Current Packet

## Just Finished

Step 4 implemented the first compile-time-engine dual-read handoff for consteval environments built from `CompileTimeState`.

Completed work:
- Added a local `engine.cpp` adapter that converts `CompileTimeState::enum_consts_by_key()` and `CompileTimeState::const_int_bindings_by_key()` from `CompileTimeValueBindingKey` to consteval `ConstEvalStructuredNameKey` maps.
- Routed the three `engine.cpp` `ConstEvalEnv` construction paths that read `CompileTimeState` through that adapter: pending consteval evaluation, direct consteval call expression evaluation, and static-assert expression evaluation.
- Kept legacy `enum_consts()` and `const_int_bindings()` installed as the primary maps; existing `ConstEvalEnv::lookup` still selects the legacy result and only compares structured lookup advisory data.
- Left string-only and missing-metadata paths fallback-only by not inventing text maps or metadata outside the existing structured mirrors.

## Suggested Next

Next packet should decide whether any remaining HIR-side consteval env construction paths can receive complete source identity without crossing into `hir_build.cpp`, `hir_types.cpp`, or consteval evaluator behavior. If not, hand Step 4 back to the supervisor for review/acceptance rather than widening the slice.

## Watchouts

The adapter rebuilds short-lived structured maps next to each `ConstEvalEnv` so pointer lifetimes remain local to the evaluation call. `sync_global_const_bindings` remains string-only because it does not carry declaration metadata. Do not change `ConstEvalEnv::lookup` result selection, consteval evaluator behavior, tests, or unrelated HIR lowering files for this packet.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_deferred_consteval_chain|cpp_hir_deferred_consteval_multi|cpp_hir_deferred_consteval_incomplete_type|cpp_hir_template_value_arg_static_member_trait|cpp_hir_stmt_switch_helper)$') > test_after.log 2>&1
```

Result: build succeeded; 5/5 selected tests passed.
