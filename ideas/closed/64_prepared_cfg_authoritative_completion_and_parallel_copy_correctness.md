# Prepared CFG Authoritative Completion And Parallel-Copy Correctness

Status: Closed
Created: 2026-04-20
Last-Updated: 2026-04-20
Closed: 2026-04-20
Disposition: Completed; authoritative prepared control-flow publication now covers block-only functions and prepared join ownership, and cyclic phi-edge parallel-copy temp-save semantics survive into consumer-visible move resolution without reopening heuristic CFG recovery.
Depends-On:
- idea 62 prealloc CFG generalization and authoritative control-flow facts
- idea 63 complete phi legalization and parallel-copy resolution

## Why This Was Closed

Idea 64 existed to finish the authoritative prepared CFG route on three
specific gaps: block-only function publication, prepared join ownership, and
end-to-end temp-save handling for cyclic phi-edge parallel copies. Those gaps
are now closed on the idea's stated scope, and the route no longer depends on
raw-BIR join rediscovery or plan-only cycle markers for the covered path.

## What Landed Before Closure

- `PreparedControlFlowFunction` publication now covers functions that have
  authoritative block or edge metadata even when branch-condition and transfer
  records are absent
- join ownership is now sourced from prepared control-flow facts instead of
  downstream raw-BIR CFG reconstruction
- cyclic prepared parallel-copy bundles now carry
  `SaveDestinationToTemp` through `PreparedMoveResolution` as an explicit
  consumer-visible operation kind
- backend liveness coverage proves the temp-save record survives from prepared
  phi-edge planning into the move-resolution handoff

## Validation At Closure

Closure used the existing backend-scoped regression logs and the close-time
guard:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- regression guard passed
- before and after both reported `70` passed / `4` failed / `74` total
- the same four pre-existing backend route failures remained:
  - `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`

## Follow-On Context

- `ideas/open/57_x86_backend_c_testsuite_capability_families.md` remains the
  correct umbrella route for the carried backend-route failures that still sit
  outside this prepared-CFG completion scope
- closing idea 64 does not claim broader x86 semantic-lowering completion
  beyond the authoritative prepared CFG publication and parallel-copy
  correctness route defined here

## Intent

Recent prealloc work moved prepared control-flow ownership onto `TextId`-backed
records and added `PreparedControlFlowBlock` plus per-edge
`PreparedParallelCopyBundle` data. That is the right direction, but the route
is still not fully authoritative or fully correct.

This idea closes the remaining gap so shared prepare publishes complete
authoritative CFG facts and parallel-copy semantics that downstream consumers
can trust without falling back to heuristic CFG recovery or half-materialized
cycle handling.

## Problem

Three residual issues remain:

- cyclic phi-edge parallel copies are planned with
  `SaveDestinationToTemp` steps, but the temporary-save semantics are not yet
  carried through into the consumer-facing move-resolution path
- `PreparedControlFlowFunction` can still be omitted when a function has only
  block/edge facts and no branch-condition, join-transfer, or parallel-copy
  records, leaving authoritative block-edge metadata unpublished
- join ownership is still partially reconstructed from raw BIR CFG shape rather
  than being derived directly from the prepared control-flow model

These gaps mean the prepared CFG route is improved but not yet complete.

## Goal

Finish the authoritative CFG route so control-flow publication and phi-edge
parallel-copy correctness are both complete enough for backend consumers to
treat prepared control-flow as the sole source of truth.

## Core Rule

Do not patch one failing CFG shape at a time. Fix the missing authoritative
model and the missing cycle-correct transfer semantics instead of growing more
heuristics around the old route.

## Primary Surfaces

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- x86 prepared consumers that currently assume the prepared CFG handoff is
  already authoritative

## Desired End State

- prepared control-flow functions are published whenever authoritative block
  metadata exists
- join ownership is derived from prepared CFG facts rather than raw-BIR
  linear-chain rediscovery
- cyclic parallel-copy bundles have a real end-to-end temporary-save contract
  rather than a plan-only marker
- downstream consumers no longer depend on silent fallback when prepared CFG
  data should be authoritative

## Acceptance Shape

This idea is satisfied when prepared control-flow publication is complete for
covered functions, join ownership is sourced from prepared CFG facts, and
parallel-copy cycles are resolved correctly through the move-resolution path.

## Non-Goals

- reopening string-based naming for prepared metadata
- broad target-specific emitter rewrites unrelated to authoritative CFG
  publication
- adding more select/short-circuit shape matchers as a substitute for general
  prepared CFG ownership
