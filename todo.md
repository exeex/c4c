Status: Active
Source Idea Path: ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Byte-Offset Drift Row Facts

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and initialized this executor-compatible
scratchpad for Step 1. No implementation work has been performed.

## Suggested Next

Execute Step 1 from `plan.md`: identify and record the exact RISC-V backend
consumer, prepared memory row, positive Route 3 memory/source authority fact,
prepared byte offset, positive Route 3 byte offset, drift Route 3 byte offset,
expected fail-closed status or agreement flag, and all byte-stable
compatibility outputs.

## Watchouts

- Do not edit code or tests before the Step 1 facts are recorded.
- Do not use hand-built stale prepared state unless the runbook is stopped and
  replaced with a narrower fixture or testability idea.
- Do not hide, demote, delete, privatize, wrap, or rename
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken the `lw a1, 12(s2)` compatibility output or adjacent status,
  helper/oracle, fallback, route/debug, prepared-printer, wrapper, exact target,
  or baseline output.

## Proof

No proof has been run. Activation is lifecycle-only.
