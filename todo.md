# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 1 legacy matcher inventory for the active cleanup-first runbook
- current exact slice:
  finish mapping the remaining `print_llvm()` / `kExpectedModule` /
  exact-text matcher sites in `lir_to_bir`, then choose the next smallest
  coherent family to remove without widening x86 case recovery

## Next Slice

- continue the `memory.cpp` cleanup queue with the next closely related
  structured-rewrite batch
- after the low-risk `memory.cpp` families are reduced, inventory the remaining
  `calls.cpp` exact-text seams
- only return to new source-backed x86 case recovery after the active matcher
  family stops expanding

## Current Iteration Notes

- the active runbook has been reset from bounded case recovery to
  legacy-lowering cleanup
- already-landed matcher cleanups remain valid, but they are now treated as the
  start of a broader shrink-the-legacy-surface lane rather than as incidental
  debt paydown between new testcase recoveries
- broad-suite red buckets still exist outside this cleanup lane, so targeted
  validation remains the default guard unless a broader checkpoint is
  explicitly needed

## Recently Completed

- reset the active runbook to a cleanup-first plan that treats
  `try_lower_to_bir_legacy(...)` and `.phase = "legacy-lowering"` paths as
  shrink targets rather than normal expansion targets
- reset execution state so future slices start from the remaining legacy
  matcher inventory instead of the previous post-`00080.c` recovery queue
