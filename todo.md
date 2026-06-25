Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Traversal and Query Helpers

# Current Packet

## Just Finished

Step 3 added the target-independent prepared object consumer diagnostic query
`diagnose_prepared_object_consumer` in
`src/backend/prealloc/prepared_object_traversal.{hpp,cpp}`. The helper maps
the existing select, value-home, move-bundle, and frame-slot consumer
classifications/statuses into precise shared diagnostic categories and
messages, returns no diagnostic for consumable states, and keeps fail-closed
missing-contract explanations out of target-local emission code.

`backend_prepared_object_consumer_contract` now covers diagnostic category-name
stability plus representative shared diagnostics for missing prepared select
join-transfer authority, missing prepared value-home authority, missing
prepared move-bundle authority, and missing prepared frame-slot owner authority.

## Suggested Next

Continue Step 3 by deciding whether the shared prepared object traversal/query
surface is complete enough for a target-connection packet, or whether one more
shared-only packet should add convenience wrappers around combined
classification-plus-diagnostic query flows.

## Watchouts

- This idea is about shared BIR/prepared contract completion, not reducing the
  full RV64 gcc torture scan failure count.
- Do not implement globals/data sections, vararg/FPR ABI, or full RV64
  instruction coverage here.
- Do not accept the retracted `20000113-1.c` slice as target-local progress.
- Keep target backends hook-shaped: concrete machine emission only, no
  rediscovery of CFG/data-flow semantics.
- `PreparedMovePhase::BlockEntry` can represent edge copies whose
  `PreparedParallelCopyBundle::execution_site` is actually
  `PredecessorTerminator`; consumers should use the shared traversal placement
  instead of interpreting the raw phase directly.
- Critical-edge parallel copies currently return no block event from the shared
  placement helper; a later packet should add an explicit split-edge or
  diagnostic contract before any target tries to emit them.
- The select classifier intentionally treats duplicate join-transfer rows,
  non-select carrier kinds, and incomplete select-materialization edge indexes
  as fail-closed statuses instead of falling back to target-local
  reconstruction.
- The value-home consumer helper treats duplicate prepared homes, conflicting
  lookup indexes, `None` homes, and incomplete register/stack/immediate/pointer
  homes as fail-closed statuses; target consumers should not fall back to local
  value-home discovery when one of those statuses appears.
- The move-bundle consumer helper treats traversal events as authoritative; a
  target should consume the returned bundle only when the status is
  `available`, and should not infer block-entry/pre-terminator placement from
  `PreparedMovePhase` alone.
- The frame-slot consumer helper treats prepared stack layout as authoritative;
  a target should consume stack homes only when the status is `available`, and
  should not reconstruct slot ownership from offsets or target-local frame
  scans after a fail-closed status.
- The new consumer diagnostic helper intentionally mirrors existing shared
  classifier statuses; target code should surface these categories/messages
  instead of converting fail-closed statuses into a coarse prepared-module
  shape failure.
- RV64 `object_emission.cpp` remains untouched; target consumers should only be
  connected after shared tests and hooks cover the needed contract.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepared_object_consumer_contract|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
