Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Traversal and Query Helpers

# Current Packet

## Just Finished

Step 3 added the target-independent prepared object frame-slot ownership query
`classify_prepared_object_frame_slot_consumer` in
`src/backend/prealloc/prepared_object_traversal.{hpp,cpp}`. The helper consumes
a prepared stack-slot `PreparedValueHome` plus `PreparedStackLayout`, returns
the owning `PreparedFrameSlot` and slot layout facts when available, and fails
closed for missing value homes, non-stack homes, incomplete stack homes, missing
stack layout, missing or ambiguous frame-slot owners, and frame-slot
function/offset/size/alignment mismatches.

`backend_prepared_object_consumer_contract` now covers all new frame-slot
consumer status names, an available stack-home-to-frame-slot classification,
and the focused fail-closed ownership cases that target consumers need before
using prepared frame layout authority.

## Suggested Next

Continue Step 3 with the remaining shared contract gap: precise diagnostic
query coverage for missing prepared consumer contract pieces before any
target-connection packet consumes these helpers.

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
- RV64 `object_emission.cpp` remains untouched; target consumers should only be
  connected after shared tests and hooks cover the needed contract.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepared_object_consumer_contract|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
