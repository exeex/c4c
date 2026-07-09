# Runtime Mismatch Ownership

Source idea: `ideas/open/618_runtime_mismatch_ownership_investigation.md`

This directory records the runtime ownership investigation for the accepted
July 9 RV64 gcc torture backend evidence. The current runtime set has `217`
rows:

- `110` abort or assertion rows.
- `102` segfault rows.
- `0` wrong-output rows.
- `5` timeout rows.

The investigation result is that runtime symptoms are not implementation
owners by themselves. `src/990106-1.c` has direct first-owner evidence for
call lowering through an `ld.so` relocation assertion, and `src/pr79737-2.c`
now has direct first-owner evidence for packed bitfield/global-object layout
and access lowering. The remaining generic abort, segfault, and timeout
families remain mixed-owner evidence lanes that should be rerun after ABI,
layout, local/global memory, call lowering, and control-flow prerequisites.

## Answer Files

- [01_runtime_symptom_map.md](01_runtime_symptom_map.md): groups runtime rows
  by abort/assertion, segfault, wrong output, and timeout, with scan artifacts
  and representative logs.
- [02_likely_first_owner_map.md](02_likely_first_owner_map.md): maps runtime
  families to likely first owners or unresolved rerun lanes.
- [03_followup_implementation_queue.md](03_followup_implementation_queue.md):
  splits recommended follow-up work by owner and proof surface, while labeling
  discussion and policy issues.
- [04_global_residual_runtime_mismatch.md](04_global_residual_runtime_mismatch.md):
  classifies the residual `src/pr79737-2.c` runtime abort as packed
  bitfield/global-object layout and access lowering, not true runtime support.

## Summary

Recommended follow-up work is owner-first:

- Call lowering: direct follow-up or rerun for `src/990106-1.c`.
- ABI: evidence and reruns for argument, return, and varargs-shaped runtime
  rows.
- Layout: evidence and reruns for packed aggregate, bitfield, and field-offset
  rows.
- Local/global memory: evidence and reruns for string, alias, pointer,
  restrict, and global-buffer rows.
- Packed bitfield/global layout: separate implementation follow-up for
  `src/pr79737-2.c`, proving 9-byte packed globals and byte-lane bitfield
  access before rerunning the row.
- Branch/compare/control flow: timeout investigation without changing timeout
  policy.
- True runtime support: only residual rows after the prerequisite owner lanes
  are narrowed.

This documentation answers the source idea and is ready for plan-owner
close-readiness review.
