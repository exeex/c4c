# RV64 Object Route Residual Local Memory Boundaries Runbook

Status: Active
Source Idea: ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md

## Purpose

Repair the remaining RV64 object-route local-memory lowering boundaries that
appear after prepared local aggregate and union frame-slot facts are coherent.

## Goal

Make the residual `unsupported_local_memory_access` representatives consume
explicit prepared local-memory facts, or split any newly proven missing producer
fact to a separate owner.

## Core Rule

RV64 object emission may consume prepared base, offset, size, alignment, and
range facts. It must not reconstruct C aggregate or union layout, fabricate
frame-slot facts, weaken diagnostics, or special-case torture filenames.

## Read First

- `ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md`
- `ideas/closed/405_prepared_local_aggregate_slot_layout_facts.md`
- `ideas/closed/400_rv64_object_route_local_memory_addressing_edges.md`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- `tests/c/external/gcc_torture/src/ieee/mul-subnormal-single-1.c`
- Current prepared dumps and RV64 probe logs for the two representatives

## Current Targets

- `src/20020225-2.c`: still reports
  `RV64 object route requires prepared frame-slot or pointer-value
  base-plus-offset local memory addressing` after `%lv.a.0` is published as an
  8-byte in-bounds frame slot.
- `src/ieee/mul-subnormal-single-1.c`: still reports
  `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory
  accesses` after `%lv.u.0` is published as a 4-byte in-bounds frame slot.
- Nearby same-family local-memory cases selected by the supervisor or found in
  the current RV64 gcc_torture backend artifacts.

## Non-Goals

- Do not reopen idea 405 unless a fresh prepared dump again shows the same
  contradictory one-byte aggregate or union extent defect.
- Do not absorb broad stack-frame, callee-saved, or parameter-home admission
  work owned by `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- Do not absorb global data, string symbol, or direct global-symbol memory work
  owned by `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`.
- Do not implement generic instruction, terminator, or move-bundle lowering
  owned by ideas 395, 396, or 397.
- Do not mark cases unsupported, weaken allowlists, or claim progress from
  expectation rewrites.

## Working Model

The producer-side slot extent family is repaired. A valid implementation should
start from the prepared local-memory facts already present in the dumps and
determine why the RV64 object route still rejects them. If the current
diagnostic is stale or hides a more precise target-side condition, improve the
classification only as part of a semantic lowering or split decision.

If inspection proves a representative still lacks a required prepared fact,
record the evidence in `todo.md` and request a producer-side split instead of
teaching RV64 object emission to infer the missing fact.

## Execution Rules

- Keep each packet tied to one concrete local-memory fact boundary.
- Prefer semantic RV64 lowering for reusable prepared frame-slot or
  pointer-value base-plus-offset forms.
- Use prepared dumps to prove that base, offset, width, alignment, and range
  facts are coherent before changing object emission.
- When a case object-compiles and links, include qemu comparison in the proof.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat filename-specific matching, unsupported downgrades, and diagnostic-only
  churn as route failures.

## Step 1: Classify Residual Local Memory Rejections

Goal: identify the exact prepared local-memory facts that each representative
has, and the exact RV64 object-route condition that rejects them.

Actions:

- Reproduce or inspect the current allowlisted RV64 probe for
  `src/20020225-2.c` and `src/ieee/mul-subnormal-single-1.c`.
- Inspect prepared dumps for both representatives and record each rejected
  local-memory access: base kind, frame slot or pointer source, offset, size,
  alignment, range verdict, and address-exposure/home facts.
- Map each rejection to target-side code that emits the current
  `unsupported_local_memory_access` diagnostic.
- Decide whether each representative is a valid target-emission lowering gap,
  a stale/imprecise diagnostic, or a newly discovered producer-fact gap.
- Search the current backend artifacts for at least one nearby same-family
  local-memory case when available.

Completion check:

- `todo.md` names the concrete target-side fact family for the first executor
  packet and states whether the two representatives share one repair route or
  need separate packets.
- Any producer-fact gap is stopped and routed for lifecycle review instead of
  patched in RV64 object emission.

## Step 2: Repair Valid Frame-Slot Or Pointer-Value Base-Plus-Offset Lowering

Goal: lower reusable prepared local-memory addressing forms that already carry
coherent base-plus-offset facts.

Actions:

- Update the RV64 object local-memory route to consume valid prepared
  frame-slot or pointer-value base-plus-offset facts for the classified family.
- Preserve existing range, alignment, stack-frame, and home-slot checks.
- Do not infer source aggregate or union containment from instruction shape.
- Add or update focused backend coverage on the local-memory route where the
  repo already has matching prepared-object tests.

Completion check:

- `src/20020225-2.c` no longer fails with the same generic prepared
  frame-slot or pointer-value base-plus-offset diagnostic when its prepared
  dump shows coherent frame-slot facts.
- Existing local-memory and aggregate/union frame-slot tests still pass.

## Step 3: Repair Valid Local-Memory Width Admission

Goal: handle the residual local-memory width boundary when the prepared access
width is valid and explicit.

Actions:

- Inspect the rejected access in `src/ieee/mul-subnormal-single-1.c` and any
  same-family case to determine whether the width diagnostic is stale,
  imprecise, or tied to a real unsupported target width.
- Repair RV64 object emission only for prepared widths whose size, alignment,
  and range facts are explicit and valid under the prepared contract.
- Keep invalid, dynamic, or missing-width forms rejected with precise
  diagnostics.

Completion check:

- `src/ieee/mul-subnormal-single-1.c` no longer fails with the same width
  diagnostic for coherent 4-byte local overlay accesses, or a separate
  non-duplicate producer gap is documented for lifecycle review.
- The repair does not broaden unsupported memory widths without semantic RV64
  lowering.

## Step 4: Prove Representatives And Route Residuals

Goal: prove the local-memory object-route family advanced without hiding
runtime or ownership failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for both
  representatives and any same-family additions.
- If a representative object-compiles and links, inspect qemu comparison rather
  than stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same local-memory family, a distinct
  target-emission family, or a producer-fact gap that needs its own source
  idea.

Completion check:

- The two representatives either object-compile and pass runtime comparison or
  advance to distinct later boundaries with owners.
- `todo.md` records exact proof commands, results, and any split
  recommendations.
- No expectation rewrites, unsupported downgrades, allowlist filtering, or
  filename-specific fixes are used as acceptance evidence.
