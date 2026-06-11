# 171 Route 5 current-block join-source migration

## Goal

Replace `mir::find_bir_current_block_join_source_identity(...)` and a selected
join-source consumer with Route 5 BIR edge/join-source records before hiding
prepared current-block join-source helpers.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 5 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Current-block join-source semantic identity.
- Edge/join value records.
- Source/no-source and optional memory-source cases.
- One selected edge-copy/join/publication consumer.

## Out Of Scope

- Move bundle order, cycle temporary routing, execution site, phase/carrier
  policy, coalescing, redundancy, destination registers, storage-sharing
  checks, prepared move records, and AArch64 edge-copy emission policy.

## Acceptance Criteria

- The selected helper/consumer reads `Route5EdgeJoinSourceIndex`.
- Edge publication and join-source oracle equivalence is proven.
- Missing predecessor, no-source, and memory-source cases are covered.
- Prepared current-block join-source helpers remain visible until consumers no
  longer need them.

## Proof Route

Run edge publication and join-source oracle tests, then the edge-copy/join or
publication target subset that observes the migrated helper.

## Retired Runbook Outcome

The active Route 5 current-block join-source migration runbook was exhausted
after Step 6 validation handoff. The selected Route 5 helper and isolated
consumer proof are green for `backend_prepared_lookup_helper` and
`backend_aarch64_current_block_join_routing`.

No prepared current-block join-source helper surface was proven private enough
to contract in this runbook. The remaining prepared public seams should move
through the broader prepared aggregate/privacy work instead of reopening this
Route 5 route.

Close-level confidence is still blocked by an ambient
`backend_aarch64_instruction_dispatch` failure at
`expected selected f64 global readback to feed call ABI move`, before the
selected Route 5 consumer. That failure is outside Route 5 current-block
join-source scope and is tracked separately.

## Closure Outcome

Closed after the Route 5 closure validation plan proved that the prior external
instruction-dispatch blocker no longer recurs.

Closure proof:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1
```

Result: `backend_prepared_lookup_helper`,
`backend_aarch64_current_block_join_routing`, and
`backend_aarch64_instruction_dispatch` passed. The canonical regression guard
compared matching close-scope logs with 3/3 passing before and 3/3 passing
after, no new failures, and non-decreasing pass count.

The selected helper/consumer route covers `Route5EdgeJoinSourceIndex`, edge
publication and join-source oracle equivalence, missing predecessor, no-source,
and memory-source cases. No additional prepared current-block join-source
helper contraction is claimed by this closure.

## Reviewer Reject Signals

- Encoding parallel-copy scheduling or register-sharing decisions in BIR.
- Contracting prepared join-source helpers before consumer migration.
- Proving only one predecessor/successor happy path.
