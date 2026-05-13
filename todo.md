Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Align AArch64 Markdown Roadmap Artifacts

# Current Packet

## Just Finished

Step 4 of `plan.md` aligned AArch64 markdown roadmap artifacts with
`src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`.

`BACKEND_ENTRY_CONTRACT.md`, `BIR_PREPARED_GAP_LEDGER.md`, and
`CLASSIFICATION_INDEX.md` now name the allocation contract as the authority for
allocation-result shape, long-lived homes, structured spill-slot ids, reserved
MIR scratch policy, call-preservation resources, and future virtual-register
placeholders.

The codegen roadmap artifacts now route future scalar ALU, comparison/branch,
cast, float, global address, i128/f128, intrinsic/vector, memory, call, return,
variadic, inline-asm, asm-emitter, prologue, atomics, and record work through
the shared allocation result instead of letting each slice allocate registers,
invent spills, or patch calling-convention resources locally.

## Suggested Next

Execute Step 5: review the Step 1 audit notes, implemented-surface alignment,
and markdown roadmap updates for missing shared BIR/prepared allocation input
or output carriers. If a required shared carrier is missing, create a separate
`ideas/open/` gap idea; otherwise record that no separate gap blocks the
allocation contract.

## Watchouts

- Historical fixed-register and stack-slot inventories remain in the markdown
  as descriptive evidence; future-direction guidance now points allocation
  authority back to `ALLOCATION_CONTRACT.md`.
- This packet did not edit `ALLOCATION_CONTRACT.md`, implementation files, or
  tests.
- Step 5 should distinguish missing shared BIR/prepared carriers from missing
  target-local AArch64 MIR records. Target-local records are expected later
  work, not automatically shared-carrier gaps.

## Proof

`git diff --check` passed. Proof output is preserved in `test_after.log`; the
log is empty because the command produced no diagnostics.
