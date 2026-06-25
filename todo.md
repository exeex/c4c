Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Connect Target Consumers Through Hooks

# Current Packet

## Just Finished

Step 4 connected AArch64 prepared function traversal to the shared unplaced
parallel-copy obligation contract. `lower_prepared_functions` now calls
`collect_unplaced_prepared_object_parallel_copy_obligations(...)` for each
prepared function and emits an AArch64 lowering diagnostic carrying the shared
`UnsupportedParallelCopyExecutionSite` category and shared critical-edge
obligation message instead of silently ignoring the obligation or rediscovering
CFG facts locally.

`backend_aarch64_function_traversal` now proves that an AArch64 prepared
function with a critical-edge parallel-copy obligation still traverses normally
and reports the shared target-consumer diagnostic with function/block context.

## Suggested Next

Continue Step 4 with the next target hook connection the supervisor selects,
preferably another narrow target consumer that can use an existing shared
classification/diagnostic helper without target-local semantic reconstruction.

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
- Critical-edge parallel copies now have a shared unplaced-obligation
  side-channel; target consumers should query that contract and surface its
  diagnostic instead of rediscovering skipped copy obligations locally.
- AArch64 now emits the shared critical-edge obligation diagnostic from function
  traversal, but the public compile wrapper still discards lowering diagnostics;
  direct traversal/module-lowering tests remain the observable proof point for
  this packet.
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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_function_traversal|prepared_object_consumer_contract|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
