Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Traversal and Query Helpers

# Current Packet

## Just Finished

Step 3 extended the shared prepared object-consumer helper surface in
`src/backend/prealloc/prepared_object_traversal.{hpp,cpp}` with
`classify_prepared_object_select_consumer`. The target-independent query
classifies BIR instructions/selects as non-selects, ordinary selects to emit,
prepared select-materialization join-transfer carriers to consume through the
published edge/block-entry transfer contract, or fail-closed missing,
ambiguous, unsupported, and malformed prepared-contract statuses.

`backend_prepared_object_consumer_contract` now covers ordinary-select vs
required-carrier-missing behavior, matching select-materialized join-transfer
classification, duplicate/ambiguous join transfers, unsupported non-select
carrier kinds, and malformed select-materialization edge indexes.

## Suggested Next

Continue Step 3 by adding the next shared prepared object-consumer query that
target backends need before connection, likely value-home or move-bundle
planning over the traversal events and select/join-transfer classification.

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
- RV64 `object_emission.cpp` remains untouched; target consumers should only be
  connected after shared tests and hooks cover the needed contract.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepared_object_consumer_contract|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
