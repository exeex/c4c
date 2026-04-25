Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Materialization and parity proof

# Current Packet

## Just Finished

Step 4 lifecycle review completed after `d4d5d2c1` and `476e787c`: the declaration-aware template struct specialization lookup route is committed, and the remaining `CompileTimeState` lookup call sites are either string-only compatibility paths or call-expression-node paths without a declaration identity to hand to the structured overloads. Step 4 is complete under the current runbook contract.

## Suggested Next

Next coherent packet: begin Step 5 by proving materialization and consteval behavior remain stable with the structured mirrors enabled. Inspect `materialize_hir_template_defs`, deferred compile-time engine lookup paths, and focused HIR template/consteval tests for coverage of structured mirror and legacy fallback behavior without weakening expectations.

## Watchouts

Do not route string-only or call-expression-only lookups through fake structured keys. If Step 5 exposes a real metadata handoff for those paths, treat that as new implementation scope; otherwise preserve legacy fallback behavior and prove parity.

## Proof

No new proof was run for this lifecycle-only Step 4 review. The latest Step 4 implementation proof remains in `test_after.log`.

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_if_constexpr_template_chain_cpp)$') > test_after.log 2>&1
```

Result: build completed and all 3 selected tests passed.
