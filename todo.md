Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Connect Target Consumers Through Hooks

# Current Packet

## Just Finished

Step 4 connected RV64 prepared object emission to the shared unplaced
parallel-copy obligation contract in a hook-shaped diagnostic path.
`build_rv64_prepared_text_object_module_with_diagnostics(...)` now consults
`collect_unplaced_prepared_object_parallel_copy_obligations(...)` for each
prepared control-flow function before translating BIR into RV64 text fragments,
and reports the shared `UnsupportedParallelCopyExecutionSite` category plus the
shared critical-edge obligation message from
`diagnose_prepared_object_consumer(...)`.

`backend_riscv_object_emission` now proves that RV64 prepared object emission
rejects a critical-edge parallel-copy obligation through the shared
category/message path, while the legacy optional builder still rejects the shape
without exposing target-local CFG rediscovery.

## Suggested Next

Continue Step 4 with the next target consumer the supervisor selects, or move to
review/closure if AArch64 traversal and RV64 prepared object emission are the
intended Step 4 target hooks for this runbook.

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
- AArch64 emits the shared critical-edge obligation diagnostic from function
  traversal, but the public compile wrapper still discards lowering diagnostics;
  direct traversal/module-lowering tests remain the observable proof point for
  that packet.
- RV64 prepared object emission now has a diagnostic-capable builder alongside
  the legacy optional-return builder; the top-level backend object wrapper still
  maps optional failure to its existing coarse unsupported-shape message.
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

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|aarch64_function_traversal|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
