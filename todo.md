Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Shared-Contract Tests

# Current Packet

## Just Finished

Step 2 added focused shared-contract coverage for prepared object-consumer
traversal and edge-copy placement. The new
`backend_prepared_object_consumer_contract` test covers the canonical
`Label -> BlockEntryCopies -> Instruction -> PreTerminatorCopies -> Terminator`
schedule, successor-entry vs predecessor-terminator edge-copy placement, and
fail-closed handling for unplaced critical-edge copies.

The minimal shared helper now lives in `src/backend/prealloc` as
`prepared_object_traversal.{hpp,cpp}`. It is target-independent and returns
prepared/BIR event records plus matching prepared move-bundle and
parallel-copy ownership pointers without touching target object emission.

## Suggested Next

Execute Step 3 by extending the shared prepared object-consumer helpers around
the new traversal surface. The next coherent packet should add the next query
contract that target consumers need, preferably select/join-transfer
classification or value-home transfer planning, with focused shared tests first.

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
- RV64 `object_emission.cpp` remains untouched; target consumers should only be
  connected after shared tests and hooks cover the needed contract.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepared_object_consumer_contract|prealloc_block_entry_publications|prepared_lookup_helper|prepared_printer|aarch64_module_skeleton_contract|aarch64_function_traversal)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
