Status: Active
Source Idea Path: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Same-Consumer Prepared Row Facts

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and initialized this executor-compatible
scratchpad for Step 1. No implementation work has been performed.

## Suggested Next

Execute Step 1 from `plan.md`: identify and record the exact RISC-V backend
consumer, prepared memory row, Route 3/Route 5 authority facts, the candidate
prepared-only fail-closed condition, and all byte-stable compatibility outputs.

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
