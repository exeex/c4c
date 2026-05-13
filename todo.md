Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Tighten Move, Parallel-Copy, And ABI-Binding Records

# Current Packet

## Just Finished

Step 2 tightened AArch64 target-MIR value-home/register-location records in
`module/module.hpp` and `module/module.cpp`.

Completed work:

- Added explicit `AllocationLocationKind` and `AllocationAuthorityKind` fields
  to distinguish physical-register locations, structured spill slots,
  non-register locations, future virtual-register placeholders, value-home
  facts, regalloc assignments, spill authority, storage plans, and deferred
  placeholders.
- Added typed AArch64 `RegisterReference` storage, explicit
  `is_reserved_mir_scratch`, and allocation-authority classification to
  `TargetRegisterRecord`; reserved scratch registers now stay distinct from
  long-lived-home eligibility.
- Added structured spill-slot metadata on `OperandRecord` (`spill_slot_id`,
  size, alignment, fixed-location flag) from prepared frame slots, keeping this
  separate from prepared stack-offset snapshots.
- Added local propagation of prepared contiguous-width and occupied-register
  metadata into `MoveRecord`, `AbiBindingRecord`, and `SpillReloadRecord`
  because it was trivial and did not select final instructions.
- Updated focused backend tests to assert representative physical-register,
  spill-slot, non-register/storage-plan, deferred future-virtual, and
  reserved-scratch facts without assembly text.

## Suggested Next

Proceed to Step 3 by tightening `MoveRecord`, `ParallelCopyRecord`, and
`AbiBindingRecord` with direct movement-record authority and source/destination
carrier facts, especially destination slot ids and per-step parallel-copy
details that are still only reachable through source prepared pointers.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.
- Keep `BIR_PREPARED_GAP_LEDGER.md` edits for Step 5 unless the supervisor
  explicitly wants a docs-only ledger correction earlier; Step 2 can rely on
  `todo.md` for the stale-ledger finding.
- Typed AArch64 register references are now present on `TargetRegisterRecord`;
  move/ABI-binding destination registers still carry string snapshots plus
  width/occupied-register metadata and should be tightened in Step 3 before
  instruction selection consumes them.
- `ParallelCopyRecord` currently exposes only counts plus `source_bundle`.
  Later movement work should decide whether direct per-step target records are
  needed before instruction selection consumes it.

## Proof

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
