Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Update Ledgers And Markdown Consumers

# Current Packet

## Just Finished

Step 5 updated the AArch64 prepared gap ledger and markdown consumers so they
reflect the implemented target-MIR allocation/move/spill/reload record layer.

Completed work:

- Updated `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` so the
  prepared-module entry, module/function/block containers, operands, target
  register records, frame records, move/ABI-binding/parallel-copy records, and
  spill/reload records are marked as present structured records instead of
  missing AArch64-local design gaps.
- Pointed later scalar, memory, branch, call, return, prologue, printer,
  encoder, and object work at `module::Module`, `codegen::records`, and
  `ALLOCATION_CONTRACT.md` instead of local allocation, spill-slot invention,
  or assembly-text recovery.
- Updated relevant `codegen/*.md` rebuild guidance for emit, records,
  prologue, memory, calls, returns, and globals to consume structured records
  and treat missing prepared carriers as separate open-idea candidates.

## Suggested Next

Proceed to the next supervisor-selected packet only after review of the Step 5
markdown slice. A coherent next implementation packet would be a later
consumer over the existing structured records, not another markdown correction.

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

No build required. This packet was docs-only and touched only markdown plus
`todo.md`, per the delegated proof contract. No `test_after.log` was produced.
