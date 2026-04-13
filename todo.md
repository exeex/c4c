# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Source Plan: plan.md

## Current Active Item

- active route:
  semantic BIR is the new truth surface, `prepare` owns target legality, and
  target backends should eventually ingest prepared BIR only
- runbook repair:
  `plan.md` now carries the ordered remaining capability backlog so future
  executor packets can be chosen from a durable semantic queue rather than
  rediscovering the next testcase family from chat
- current capability family:
  backlog item 4, broader global data and addressed globals
- current packet shape:
  extend the scalar global lane toward addressed global data, starting with
  extern/global array reads and other simple non-scalar global-address forms
  that still belong to honest semantic BIR
- candidate proving surface:
  `tests/c/internal/backend_case/branch_if_eq.c`
  `tests/c/internal/backend_case/extern_global_array.c`
  `tests/c/internal/backend_case/extern_global_array_def.c`

## Immediate Target

- keep packet selection attached to the ordered semantic backlog in `plan.md`
- continue from scalar globals into addressed global data instead of jumping to
  unrelated one-off cases
- avoid reintroducing testcase-shaped routing while broadening the global lane

## Done Condition For The Active Packet

- `branch_if_eq.c` still lowers to clean BIR
- the first addressed-global / extern-global-array read path lowers through
  semantic BIR instead of LLVM-text fallback
- no new direct route, rendered-text matcher, or tiny case-family special path
  is introduced

## Parked While This Packet Is Active

- broader call-lane expansion beyond the current minimal direct-call coverage
- intrinsic/runtime operation lowering
- real stack layout, liveness, and regalloc
- target backend ingestion rewrites beyond the shared BIR/prepare spine
