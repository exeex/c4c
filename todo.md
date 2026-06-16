Status: Active
Source Idea Path: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Capture the Failing AArch64 Local-Memory Shape

# Current Packet

## Just Finished

Lifecycle activation created this active execution skeleton for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce the AArch64 failure, capture the exact
local-memory load/addressing shape, compare it with the passing x86 route, and
record the selected repair surface plus proof command here.

## Watchouts

- Do not fix this by changing expected dump text, CTest labels, or support
  classification.
- Do not add shortcuts for `00204.c`, `myprintf`, `movi`, or specific HFA
  struct names.
- Keep implementation progress and proof notes in this file unless the runbook
  itself needs a real route correction.

## Proof

Not run; lifecycle activation only.
