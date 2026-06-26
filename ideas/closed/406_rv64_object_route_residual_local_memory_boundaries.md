# RV64 Object Route Residual Local Memory Boundaries

Status: Closed
Type: Follow-up repair idea
Parent: `ideas/closed/400_rv64_object_route_local_memory_addressing_edges.md`
Discovered From: `ideas/closed/405_prepared_local_aggregate_slot_layout_facts.md`

## Goal

Repair the remaining RV64 object-route local-memory lowering boundaries that
appear after prepared local aggregate and union slot facts are coherent.

## Why This Exists

Idea 405 repaired the producer-side one-byte aggregate/union slot extent defect.
The representative prepared dumps now show covering frame slots and
`range_verdict=proven_in_bounds` for the relevant local overlay accesses:

- `src/20020225-2.c`: `%lv.a.0 size=8 align=8` with 8-byte and 4-byte accesses
  selecting `frame_slot=#0`.
- `src/ieee/mul-subnormal-single-1.c`: `u2f` and `f2u` `%lv.u.0 size=4
  align=4` with 4-byte overlay accesses selecting covering frame slots.

The allowlisted RV64 probe still fails both cases with later
`unsupported_local_memory_access` diagnostics:

- `src/20020225-2.c`: `RV64 object route requires prepared frame-slot or
  pointer-value base-plus-offset local memory addressing`.
- `src/ieee/mul-subnormal-single-1.c`: `RV64 object route supports only 1-,
  2-, 4-, and 8-byte prepared local memory accesses`.

Those diagnostics are no longer evidence of the old producer extent bug. They
need a target-emission classification and repair route that consumes explicit
prepared local-memory facts without reconstructing source aggregate or union
layout.

## In Scope

- Classify the residual `unsupported_local_memory_access` cases by the concrete
  prepared local-memory fact that RV64 object emission rejects.
- Repair reusable RV64 object lowering for valid prepared frame-slot or
  pointer-value base-plus-offset local-memory forms when the prepared contract
  already provides coherent base, offset, size, alignment, and range facts.
- Repair reusable RV64 object width admission for valid prepared local-memory
  widths, including cases where the first diagnostic is stale or imprecise
  relative to the prepared dump.
- Use `src/20020225-2.c` and `src/ieee/mul-subnormal-single-1.c` as
  representatives, plus nearby same-family local-memory cases when available.
- Split any newly discovered missing producer fact into a separate producer
  idea instead of compensating in RV64 object emission.

## Out Of Scope

- Reopening the producer aggregate/union slot extent repair closed by idea 405
  unless a prepared dump again shows contradictory one-byte slot facts.
- Broad stack-frame, callee-saved, or parameter-home admission work owned by
  `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- Global data, string symbol, or direct global-symbol memory work owned by
  `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`.
- Generic instruction, terminator, or move-bundle lowering owned by ideas 395,
  396, and 397.
- Expectation rewrites, unsupported downgrades, allowlist-only progress, or
  filename-specific local-memory shortcuts.

## Acceptance

- `src/20020225-2.c` no longer fails with the same generic prepared
  frame-slot or pointer-value base-plus-offset local-memory diagnostic when
  the prepared dump shows coherent frame-slot facts.
- `src/ieee/mul-subnormal-single-1.c` no longer fails with the same local
  memory width diagnostic when the prepared dump shows coherent 4-byte local
  accesses, or the route is split with precise evidence for a distinct
  non-duplicate producer fact gap.
- A refreshed narrow RV64 gcc_torture backend probe shows the repaired
  representatives either object-compile and run through qemu or advance to a
  distinct later boundary owned by another open idea.
- Backend proof includes the representative probe and the supervisor-selected
  backend CTest subset without introducing new backend regressions.

## Reviewer Reject Signals

- Reject changes that special-case `20020225-2.c`,
  `ieee/mul-subnormal-single-1.c`, local variable names, or named union fields.
- Reject RV64 emitter code that reconstructs C aggregate or union layout when
  prepared frame-slot facts are missing or contradictory.
- Reject any slice that claims idea 406 progress while the prepared dump still
  shows the exact one-byte aggregate/union extent defect closed by idea 405.
- Reject clearing the diagnostic by weakening local-memory width, base,
  offset, or range checks without adding semantic lowering for the prepared
  fact family.
- Reject expectation downgrades, unsupported markers, or allowlist filtering.
- Reject object-compilation-only proof when the repaired case reaches qemu and
  can be compared against clang runtime behavior.

## Lifecycle Notes

- 2026-06-26: Closed after commits `5b60f77c` and `b91fc4d5` repaired the
  residual RV64 object-route local-memory boundaries for prepared frame-slot
  sub-width overlays and valid F32 local-memory access admission.
- 2026-06-26: Step 4 representative proof shows neither
  `src/20020225-2.c` nor `src/ieee/mul-subnormal-single-1.c` still reports
  `unsupported_local_memory_access` or the local-memory diagnostics owned by
  this idea. `src/20020225-2.c` now stops at
  `unsupported_instruction_fragment`, routed to
  `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.
  `src/ieee/mul-subnormal-single-1.c` now stops at
  `unsupported_stack_frame`, routed to
  `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- 2026-06-26: Close gate passed with backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
  `test_before.log` and regenerated `test_after.log` both report 326/326
  passing backend tests. The lifecycle-only close comparison used
  `--allow-non-decreasing-passed` because the accepted backend pass count
  remained unchanged.
