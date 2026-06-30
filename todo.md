Status: Active
Source Idea Path: ideas/open/464_select_carrier_alias_metadata.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Carrier-Alias Metadata Packet

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 464. Prepared carrier-alias authority
metadata is now published/queryable in the prepared control-flow/publication
layer, with focused positive and fail-closed coverage. No RV64 target lowering
was touched.

Implementation table:

| Field | Value |
| --- | --- |
| Metadata surface | Added `PreparedSelectCarrierAliasAuthority*` records plus planner, availability predicate, and collector in `publication_plans` |
| Authority keys | Function, predecessor/successor edge, join/final result, selected source, binary source-producer site, carrier values, and optional carrier value ids |
| Use closure | Planner proves the selected source-producer result is consumed only by authorized carrier alias selects |
| Positive coverage | Duplicate carrier aliases accepted directly and through `collect_prepared_select_carrier_alias_authorities` from a prepared module fixture |
| Negative coverage | Missing source producer, raw-name-only/mismatched carrier result, and extra non-carrier source use stay fail-closed |
| Files changed | `src/backend/prealloc/publication_plans.hpp`, `src/backend/prealloc/publication_plans.cpp`, `tests/backend/bir/backend_prepare_stack_layout_test.cpp` |
| Consumer boundary | RV64 ULE rematerialization remains out of scope for idea 464; a later idea may consume this metadata |

Artifact:
`build/agent_state/464_step3_carrier_alias_metadata/summary.md`.

## Suggested Next

Execute Step 4 from `plan.md`: residual disposition and close readiness.
Re-check that the metadata prerequisite is complete, preserve the no-RV64
boundary for this idea, and decide whether a later RV64 ULE rematerialization
idea can be reactivated.

Suggested artifact directory:
`build/agent_state/464_step4_residual_disposition/`.

## Watchouts

- Do not make RV64 ULE rematerialization or target consumer changes before the
  metadata exists.
- Do not infer duplicate-carrier authority from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Preserve fail-closed behavior for extra non-carrier uses and mismatched
  source/destination/edge facts.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
