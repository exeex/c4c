Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Tighten Spill And Reload Pseudo Records

# Current Packet

## Just Finished

Step 4 tightened AArch64 target-MIR spill/reload pseudo records in
`module/module.hpp` and `module/module.cpp`.

Completed work:

- Added an explicit target-MIR spill/reload pseudo semantic for store-from-register,
  reload-to-scratch, and rematerialize-to-scratch records.
- Preserved prepared spill/reload register class, bank, site, slot id, prepared
  stack-offset snapshot, register name, contiguous width, occupied scratch
  registers, and `PreparedSpillReloadOp` provenance.
- Linked each prepared spill/reload carrier with a structured
  `TargetRegisterRecord` scratch authority when the prepared op publishes a
  register name.
- Updated the focused AArch64 frame/control test to observe representative
  spill and reload records with structured slot ids, prepared stack-offset
  snapshots, pseudo semantics, and scratch authority without final instruction
  selection.

## Suggested Next

Proceed to Step 5 by updating the gap ledger and relevant AArch64 markdown
consumers so later slices consume these structured move, ABI, parallel-copy,
spill, and reload records instead of local allocation or assembly-text recovery.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.
- Step 5 owns `BIR_PREPARED_GAP_LEDGER.md` and markdown consumer updates.
- `PreparedAbiBinding` does not currently publish a destination value id or
  destination slot id; Step 3 preserved only the destination carrier facts the
  prepared ABI-binding record directly provides.
- `ParallelCopyStepRecord::has_target_move_record` is false when the prepared
  out-of-SSA move bundle cannot be uniquely matched; consumers should treat
  that as missing prepared target-move authority, not as permission to recover
  from raw BIR or printed text.
- `PreparedSpillReloadOp` publishes register bank rather than register class;
  Step 4 records the class derived from that bank and keeps the prepared bank as
  the direct source fact.

## Proof

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log. The `^backend_` subset
reported 100% tests passed for the enabled backend tests.
