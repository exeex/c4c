Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Tighten Move, Parallel-Copy, And ABI-Binding Records

# Current Packet

## Just Finished

Step 3 tightened AArch64 target-MIR move, parallel-copy, and ABI-binding records
in `module/module.hpp` and `module/module.cpp`.

Completed work:

- Added destination slot snapshots to `MoveRecord` when prepared value-home,
  regalloc, or storage-plan facts can prove the destination value's slot.
- Added `authority_kind` to `AbiBindingRecord` so ABI bindings remain explicit
  movement records without encoding call/return instruction-selection policy.
- Added per-move and per-step `ParallelCopyRecord` details, including source
  and destination BIR values, carrier kind, storage name, cycle-temp usage, and
  matched target move facts for out-of-SSA parallel-copy steps.
- Updated the focused AArch64 frame/control test to cover representative
  prepared call moves, out-of-SSA parallel-copy moves, cycle-temp steps, target
  move provenance, destination slot ids, and ABI bindings without assembly text.

## Suggested Next

Proceed to Step 4 by tightening spill/reload records and any remaining
move-adjacent frame-slot facts needed before instruction selection consumes
prepared allocation records.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.
- Keep `BIR_PREPARED_GAP_LEDGER.md` edits for Step 5 unless the supervisor
  explicitly wants a docs-only ledger correction earlier; Step 2 can rely on
  `todo.md` for the stale-ledger finding.
- `PreparedAbiBinding` does not currently publish a destination value id or
  destination slot id; Step 3 preserved only the destination carrier facts the
  prepared ABI-binding record directly provides.
- `ParallelCopyStepRecord::has_target_move_record` is false when the prepared
  out-of-SSA move bundle cannot be uniquely matched; consumers should treat
  that as missing prepared target-move authority, not as permission to recover
  from raw BIR or printed text.

## Proof

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log. The `^backend_` subset
reported 100% tests passed for the enabled backend tests.
