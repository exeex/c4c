Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Step 6 proved the AArch64 allocation/move/spill/reload record layer with
focused structured-record backend coverage.

Completed work:

- Confirmed existing focused tests cover physical-register homes, spill-slot
  homes, spill/reload records, prepared moves, parallel-copy records, and
  ABI-binding records through structured target-MIR assertions.
- Required no test changes for this proof packet.
- Regenerated the canonical executor proof log at `test_after.log`.

## Suggested Next

All runbook steps are now complete. Supervisor should route the active plan for
lifecycle review/close decision rather than delegating another implementation
packet under this runbook.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.
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
- If later consumers need carrier details not present in shared preparation,
  split that into a new open idea instead of adding AArch64-local recovery from
  raw BIR, rendered names, LIR strings, parsed assembly, or legacy markdown.

## Proof

Ran `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
The delegated backend subset passed: 100% tests passed for the enabled
`^backend_` tests. Proof log: `test_after.log`.
