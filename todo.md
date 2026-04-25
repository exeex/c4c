Status: Active
Source Idea Path: ideas/open/101_hir_enum_const_int_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consteval, switch/case, and template value proof

# Current Packet

## Just Finished

Step 4 is accepted route-wise and validation-wise.

Completed work:
- Added a Lowerer-local structured-map adapter that converts `ct_state_->enum_consts_by_key()` and `ct_state_->const_int_bindings_by_key()` from `CompileTimeValueBindingKey` to consteval `ConstEvalStructuredNameKey` maps.
- Routed all owned Lowerer `ConstEvalEnv` construction sites in global init folding, local const folding, constexpr-if, switch case/range folding, consteval-call argument folding, and ternary condition folding through that adapter.
- Kept legacy `enum_consts_`, `const_int_bindings_`, and `ctx.local_const_bindings` installed as the primary maps; existing `ConstEvalEnv::lookup` still selects the legacy result and only compares structured lookup advisory data.
- Preserved local const maps and block-local enum behavior as legacy-only by adding only global structured enum/named-const maps from `ct_state_`.
- Step 4 implementation commits `99b7ba28` and `fda04ed5` are committed.
- Reviewer report `review/value_binding_step4_route_review.md` found no blocking issues, judged the route on track, matched to the source idea, found no testcase overfit, and recommended advancing to Step 5.
- Supervisor reran the exact Step 4 proof into `test_after.log`; regression guard passed against matching `test_before.log` with 7 passed before and 7 passed after, with no new failures.

## Suggested Next

Next packet should execute Step 5: prove the structured value-binding mirrors remain behavior-preserving across consteval, switch/case, constant-expression, and template value consumers.

Suggested Step 5 packet:
- Inventory the focused HIR enum, const-int, consteval, switch/case, and template value tests under `tests/cpp/internal` that cover the value-binding consumers named by the source idea.
- Verify consteval environment handoff still sees the same enum constants and const-int bindings with structured mirrors enabled.
- Verify switch/case and constant-expression lowering still resolve the same values.
- Verify template value argument paths keep existing behavior.
- Add or adjust focused tests only if needed to prove structured mirror and fallback behavior; do not weaken expectations, add named-case shortcuts, or expand into static-member/owner-identity scope.

## Watchouts

The Step 4 route intentionally keeps legacy lookup as the selected result and structured lookup advisory. Remaining string-only or bridge-required paths include `static_eval_int(...)` callers, direct `enum_consts_.find(...)` probes, and fallback const-int registration paths that lack declaration metadata. Step 5 should prove consumer behavior and only add tests where they validate real structured mirror or fallback behavior; it should not change `ConstEvalEnv::lookup` result selection or drift into idea 102 owner/static-member scope.

## Proof

Supervisor reran the exact Step 4 proof and preserved output in `test_after.log`:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_deferred_consteval_chain|cpp_hir_deferred_consteval_multi|cpp_hir_deferred_consteval_incomplete_type|cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_value_arg_static_member_trait|cpp_hir_stmt_switch_helper)$') > test_after.log 2>&1
```

Result: build succeeded; 7/7 selected tests passed. Regression guard passed against matching `test_before.log` with 7 passed before and 7 passed after, with no new failures.
