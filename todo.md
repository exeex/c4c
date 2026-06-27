Status: Active
Source Idea Path: ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Runtime Mismatch Families

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`.

## Suggested Next

Execute Step 1: refresh the current RV64 gcc_torture runtime abort/segfault
bucket for `20000113-1.c`, `20070212-2.c`, and supervisor-selected nearby
runtime mismatch representatives. Classify the first concrete runtime
root-cause family before implementation.

## Watchouts

- Do not weaken qemu comparison or claim compile-only proof for runnable cases.
- Do not mask bad BIR/prepared facts in RV64 lowering; route producer defects
  separately.
- Split abort and segfault representatives if evidence shows distinct root
  causes.
- Do not use filename-specific handling, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.

## Proof

No execution proof yet. This is lifecycle activation only.
