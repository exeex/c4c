Status: Active
Source Idea Path: ideas/open/101_hir_enum_const_int_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read semantic lookup paths

# Current Packet

## Just Finished

Step 4 routed the remaining owned Lowerer-side `ConstEvalEnv` construction paths through structured global mirrors from `ct_state_`.

Completed work:
- Added a Lowerer-local structured-map adapter that converts `ct_state_->enum_consts_by_key()` and `ct_state_->const_int_bindings_by_key()` from `CompileTimeValueBindingKey` to consteval `ConstEvalStructuredNameKey` maps.
- Routed all owned Lowerer `ConstEvalEnv` construction sites in global init folding, local const folding, constexpr-if, switch case/range folding, consteval-call argument folding, and ternary condition folding through that adapter.
- Kept legacy `enum_consts_`, `const_int_bindings_`, and `ctx.local_const_bindings` installed as the primary maps; existing `ConstEvalEnv::lookup` still selects the legacy result and only compares structured lookup advisory data.
- Preserved local const maps and block-local enum behavior as legacy-only by adding only global structured enum/named-const maps from `ct_state_`.

## Suggested Next

Next packet can hand Step 4 to reviewer/supervisor for route-quality review and acceptance, unless the supervisor wants a separate bridge plan for remaining string-only probes.

## Watchouts

The adapter rebuilds short-lived structured maps next to each `ConstEvalEnv` so pointer lifetimes remain local to the evaluation call. Remaining string-only or bridge-required paths include `static_eval_int(...)` callers, direct `enum_consts_.find(...)` probes, and fallback const-int registration paths that lack declaration metadata. Do not change `ConstEvalEnv::lookup` result selection, consteval evaluator behavior, tests, or unrelated HIR lowering files for this packet.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_deferred_consteval_chain|cpp_hir_deferred_consteval_multi|cpp_hir_deferred_consteval_incomplete_type|cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_value_arg_static_member_trait|cpp_hir_stmt_switch_helper)$') > test_after.log 2>&1
```

Result: build succeeded; 7/7 selected tests passed.
