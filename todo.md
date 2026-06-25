Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Traversal and Query Helpers

# Current Packet

## Just Finished

Step 3 added the target-independent prepared object value-home consumer query
`classify_prepared_object_value_home_consumer` in
`src/backend/prealloc/prepared_object_traversal.{hpp,cpp}`. The helper resolves
named BIR values through prepared names, value-location homes, optional regalloc
identity, and optional value-home lookup indexes, then returns an authoritative
home or precise fail-closed statuses for missing names/locations/homes,
ambiguous homes, conflicting ids/lookups, unsupported home kind, or incomplete
home payloads.

`backend_prepared_object_consumer_contract` now covers the new status names,
available classifications for register, stack-slot, rematerializable-immediate,
and pointer-base-plus-offset homes, plus missing prepared value names, missing
homes, ambiguous homes, `None` homes as unsupported, and incomplete register
homes.

## Suggested Next

Continue Step 3 by adding the next shared prepared object-consumer query for
move-bundle planning over traversal events, or delegate a target-connection
packet that consumes the select and value-home helpers without reopening
target-local CFG/value-home reconstruction.

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
- RV64 `object_emission.cpp` remains untouched; target consumers should only be
  connected after shared tests and hooks cover the needed contract.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepared_object_consumer_contract|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
