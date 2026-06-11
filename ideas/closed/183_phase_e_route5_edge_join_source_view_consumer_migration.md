# 183 Phase E Route 5 edge join-source view consumer migration

## Goal

Switch one AArch64 edge-copy join-source reader to a Route 5 edge/join-source
view.

## Why This Exists

Phase D identified Route 5 as a bounded follow-up for edge-copy and join
parallel-copy source recovery. The migration should prove one selected
consumer boundary while retaining prepared helpers as oracles.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 edge-copy or join parallel-copy source recovery consumer.
- Route 5 records keyed by edge, predecessor, destination block, and copied
  value role.
- Missing-predecessor, no-source, memory-source, and absent-route cases.
- Prepared edge-publication, edge-publication source-producer, current-block
  join-source, and move-bundle helpers as fallback/oracle surfaces.

## Out Of Scope

- Moving parallel-copy scheduling, out-of-SSA placement, source/destination
  homes, move-bundle selection, branch spelling, or final edge-copy records
  into BIR.
- Hiding prepared edge-copy helpers after one consumer migration.
- Broad aggregate contraction.

## Acceptance Criteria

- The selected consumer uses Route 5 semantic join-source records where present
  and preserves prepared fallback behavior where absent.
- Route/prepared equivalence tests cover normal predecessor,
  missing-predecessor, no-source, memory-source, and absent-route cases.
- Existing route-index fail-closed coverage remains intact.

## Completion Note

Closed after migrating the selected AArch64 current-block join-source reader
behind `build_current_block_join_prepared_query_routing(...)` to prefer the
indexed Route 5 current-block join-source view when valid BIR data is
available. Prepared current-block join-source facts and related prepared
lookup surfaces remain the fallback/oracle for absent, incomplete, mismatched,
or unavailable Route 5 data.

Focused route/prepared coverage now covers normal predecessor routing,
missing predecessor-block behavior, no-source fallback, load-local
stack/memory-source routing, absent-route prepared fallback, and existing
route-index fail-closed behavior. Broad prepared API deletion, aggregate
contraction, and moving prepared policy into BIR remain out of scope.

Close guard:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'; } > test_after.log 2>&1`

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, with 3 passed and 0 failed in both canonical logs.

## Reviewer Reject Signals

- The implementation rescans predecessors or BIR nodes instead of using Route 5
  owned records.
- Helper renames or wrapper reshaping are claimed as migration progress.
- Prepared edge-copy or move-bundle policy is silently moved into BIR.
- Tests weaken oracle expectations or cover only the selected happy path.
- The slice claims broad prepared surface contraction.
