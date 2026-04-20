# Complete Phi Legalization And Parallel-Copy Resolution

Status: Closed
Created: 2026-04-19
Last-Updated: 2026-04-20
Closed: 2026-04-20
Disposition: Completed; phi destruction now publishes authoritative typed edge transfers and parallel-copy bundles, and downstream move resolution consumes that prepared contract instead of depending on residual IR shape or slot-only fallback as the correctness path.
Depends-On:
- idea 64 shared text identity and semantic name table refactor
- idea 62 prealloc CFG generalization and authoritative control-flow facts

## Why This Was Closed

Idea 63 was about finishing phi legalization as a deliberate shared-prepare
contract rather than a growing pile of select-shaped recovery or slot-backed
fallback. That route is now complete on its stated scope: prepared control-flow
publishes typed per-edge transfer facts and canonical parallel-copy bundles,
cyclic copies are represented explicitly, and regalloc consumes that
authoritative transfer data when recording phi move resolution.

## What Landed Before Closure

- prepared control-flow now stores explicit `PreparedEdgeValueTransfer` records
  on typed predecessor and successor labels
- shared prepare builds `PreparedParallelCopyBundle` steps so multi-move and
  cyclic phi edges no longer rely on incidental lowering order
- the public contract preserves typed identity boundaries on functions, blocks,
  values, and slots instead of widening back to raw string-keyed handoff data
- regalloc-facing move resolution reads prepared phi-transfer and parallel-copy
  data directly rather than reconstructing transfer meaning from legalized IR
  shape
- reducible select materialization remains an optimization path, not the sole
  correctness route for supported phi handling

## Validation At Closure

Closure used a backend-scoped regression guard:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- regression guard passed
- before and after both reported `68` passed / `4` failed / `72` total
- the same four pre-existing backend route failures remained:
  - `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`

## Follow-On Context

- `ideas/open/57_x86_backend_c_testsuite_capability_families.md` remains the
  umbrella route for the broader pre-handoff backend capability debt named in
  the closure guard
- `ideas/open/59_generic_scalar_instruction_selection_for_x86.md`,
  `ideas/open/60_prepared_value_location_consumption.md`, and
  `ideas/open/61_stack_frame_and_addressing_consumption.md` remain separate
  follow-on routes and should not be silently folded back into this closed phi
  legalization slice

## Intent

The current phi handling in `src/backend/prealloc/legalize.cpp` is not a fully
general SSA-destruction route. It can materialize some reducible phi trees into
`bir.select`, and otherwise falls back to slot-backed lowering that is good
enough for the current pipeline but not a complete or principled answer for
arbitrary CFG and phi interaction.

This idea makes phi legalization complete enough for general backend use by
moving from pattern-first recovery to a deliberate phi destruction strategy
with explicit edge transfers, critical-edge handling where needed, and
parallel-copy-safe resolution, all expressed through the typed symbolic
identity/control-flow substrate established by ideas 64 and 62.

## Problem

Current phi lowering is limited by:

- materialization logic that depends on funnel/select-shaped CFG discovery
- fallback lowering that uses local slot stores/loads instead of a fully
  general move/edge-transfer model
- no single authoritative route for resolving phi cycles and parallel copies
  independent of shape
- symbolic identities for blocks, values, and slots should converge on the
  semantic-id substrate from idea 64 rather than freezing more string-shaped
  contracts here

Those constraints make correctness and future backend reuse harder than they
need to be.

## Goal

Provide a general phi legalization path that preserves SSA edge semantics for
arbitrary supported CFG while producing prepared transfer data that downstream
passes can consume without guessing.

## Core Rule

Do not treat one additional phi shape as success. The route must converge on a
general phi destruction model, not a larger pile of named-case materializers.

## Primary Surfaces

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- any extracted shared helpers for edge splitting or parallel-copy planning

## Desired End State

- phi elimination has a general path independent of select/funnel recovery
- edge transfers can represent multiple incoming moves without clobber hazards
- critical edges are handled explicitly when the legalization route requires it
- regalloc and later consumers use authoritative phi-transfer data instead of
  reconstructing phi moves from surviving IR shape

## Concrete Direction

Possible end-state pieces include:

- generalized edge-transfer planning per predecessor/successor edge
- explicit parallel-copy bundles or an equivalent canonical transfer record
- critical-edge splitting only where required by the legality model
- keeping select materialization as an optional optimization instead of the
  correctness path

## Acceptance Shape

This idea is satisfied when phi legalization correctness no longer depends on
select-shaped recovery or slot-only fallback, and downstream move resolution can
consume a deliberate, general transfer model.

## Non-Goals

- changing frontend SSA formation rules
- broad target-specific codegen rewrites unrelated to phi transfer semantics
- folding unrelated branch/join ownership work back into ad hoc x86 helpers
