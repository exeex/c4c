# AArch64 Calls Immediate Scalar Argument Publication Owner Runbook

Status: Active
Source Idea: ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md

## Purpose

Extract or consolidate the AArch64 calls-side owner for immediate scalar call
argument publication while preserving prepared destination/source-home inputs,
instruction spelling, records, diagnostics, and assembly behavior.

## Goal

Create a narrow immediate-publication owner boundary that consumes already
accepted immediate argument facts and does not select publication sources or
prepared call argument plans.

## Core Rule

This route owns immediate scalar call-argument publication only. Do not move
publication-source authority, prepared call argument plans, scalar producer
selection, aggregate byval transport, f128 carrier handling, ordinary
call-boundary record construction, or general constant materialization into
the owner.

## Read First

- `ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `tests/backend/mir/CMakeLists.txt`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/CMakeLists.txt`

## Current Targets

- Primary implementation target: `src/backend/mir/aarch64/codegen/calls.cpp`
- Public declaration target only if required by an existing external call path:
  `src/backend/mir/aarch64/codegen/calls.hpp`
- New local helper files are allowed only if they make the immediate
  publication boundary clearer and remain AArch64-local.
- Test/proof targets should stay focused on integer immediate arguments,
  floating immediate arguments, same-block cast producer lookup, GP and FP
  destination views, unsupported or rejected immediate shapes when available,
  and nearby ordinary before-call moves.

## Non-Goals

- Do not rederive or move publication-source authority, prepared call argument
  plans, source-home selection, or scalar producer selection.
- Do not rework general scalar producer lowering, before-call move ordering,
  aggregate byval lane transport, f128 carrier handling, or ordinary
  call-boundary record construction.
- Do not generalize constant materialization across ALU, memory, and calls
  without a separate owner or evidence idea.
- Do not change supported immediate forms, unsupported diagnostics, assembly
  output, selection status, record fields, or printer behavior as a cleanup
  side effect.
- Do not claim progress through helper renames, file movement, expectation
  rewrites, or unsupported-path downgrades.

## Working Model

The immediate scalar publication owner may consume:

- prepared destination registers
- prepared source homes and same-block cast producer context
- already accepted call argument immediate facts
- integer and floating scalar type widths
- scratch register names needed by the current materialization sequence
- diagnostics context needed to preserve existing rejection paths

The owner may produce:

- AArch64 inline-asm materialization lines for supported immediate cast call
  arguments
- scalar GP and FP destination register views
- the immediate-publication instruction currently consumed by the before-call
  lowering path

The owner must not decide which call argument is selected, which source home
should be used, which scalar producer is authoritative, or whether unrelated
constant materialization should be supported.

## Execution Rules

- Keep edits behavior-preserving unless the source idea explicitly requires a
  diagnostic-preserving boundary adjustment.
- Prefer a small local helper type, namespace, or file boundary over a broad
  `calls.cpp` split.
- Preserve exact diagnostics, records, inline-asm lines, and assembly unless a
  focused test update proves the old spelling was wrong and the supervisor
  approves the contract change.
- Keep `lower_before_call_immediate_binding` as the consumer of prepared
  immediate argument facts; do not weaken or bypass its checks.
- If helper APIs or inline-asm record construction changes, produce matching
  `test_before.log` and `test_after.log` for the focused subset.
- A code-changing packet needs `cmake --build --preset default` or an
  equivalent fresh build proof before acceptance.
- Escalate to reviewer if the implementation starts selecting publication
  sources or changing prepared call argument contracts instead of consuming
  them.

## Step 1: Confirm Immediate Publication Boundary And Proof Subset

Goal: map the current immediate scalar publication helpers and choose the
focused proof subset before changing implementation.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Locate these helpers and their call sites:
  `find_same_block_cast_producer`, `integer_width_bits_for_type`,
  `immediate_integer_bits`, `scalar_fp_register_view`,
  `scalar_gp_register_view`, `append_materialize_fp_immediate`,
  `make_immediate_cast_call_argument_publication_lines`,
  `make_immediate_cast_call_argument_publication_instruction`, and
  `lower_before_call_immediate_binding`.
- Record which helpers belong to same-block cast producer lookup, immediate
  bit extraction, scalar GP/FP destination view rendering, inline-asm
  materialization, instruction construction, and before-call integration.
- Identify the smallest AArch64-local owner boundary that consumes prepared
  destination/source-home inputs and emits immediate-publication lines or the
  existing immediate-publication instruction.
- Name any helpers that are shared with non-immediate call-boundary paths and
  should remain outside or be wrapped narrowly.
- Choose a focused proof subset covering integer immediates, floating
  immediates, same-block cast producer lookup, GP and FP destination views,
  unsupported or rejected immediate shapes when available, and nearby ordinary
  before-call moves.
- Record the proposed helper boundary and exact proof command in `todo.md`.

Completion check:

- `todo.md` names the immediate-publication helpers to move or consolidate,
  names shared helpers that must remain outside or be wrapped narrowly, and
  provides the exact proof command the executor will run after implementation.

## Step 2: Introduce The Immediate Scalar Publication Owner

Goal: create or tighten the local owner API for immediate scalar call-argument
publication.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Extract a narrow local helper type, namespace, or file boundary around the
  immediate scalar publication helpers identified in Step 1.
- Keep input parameters tied to prepared destinations, source homes,
  same-block context, scalar type facts, scratch registers, and diagnostics.
- Keep outputs limited to materialization lines and the immediate-publication
  instruction already consumed by the calls-side before-call machinery.
- Avoid API shapes that accept whole call plans when a destination, source
  home, cast producer, or scalar value fact is sufficient.
- Build after the extraction.

Completion check:

- The project builds, and the new boundary makes immediate scalar publication
  explicit without changing source selection, prepared call argument authority,
  diagnostics, records, or assembly.

## Step 3: Route Immediate Publication Uses Through The Owner

Goal: make every in-scope calls-side immediate scalar publication use consume
the owner consistently.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Route integer immediate materialization through the owner.
- Route floating immediate materialization through the owner.
- Route same-block cast producer lookup or lookup consumption through the
  owner only if doing so clarifies the immediate-publication boundary without
  taking ownership of scalar producer selection.
- Route GP and FP scalar destination view rendering through the owner or a
  narrow wrapper when those views are only needed by immediate publication.
- Route immediate-publication instruction construction through the owner.
- Preserve diagnostics, operand shape, inline-asm spelling, emitted records,
  and assembly.
- Leave ordinary before-call moves, aggregate byval lanes, f128 carriers,
  general constant materialization, and call-boundary record construction
  outside this boundary unless Step 1 documents direct immediate-publication
  coupling.
- Build after routing.

Completion check:

- All in-scope immediate scalar publication call sites use the owner or have a
  documented reason in `todo.md` for staying outside it, and the build is
  green.

## Step 4: Prove Focused Behavior

Goal: prove the extraction preserved immediate scalar call-argument
publication behavior.

Primary proof:

- `cmake --build --preset default`
- A focused `ctest --test-dir build -R '<pattern>' --output-on-failure` subset
  chosen in Step 1, expected to include relevant AArch64 integer immediate,
  floating immediate, same-block cast producer, GP/FP destination view,
  rejected immediate, and nearby ordinary before-call move coverage.

Actions:

- Run the exact focused proof command chosen in Step 1.
- If helper APIs or inline-asm record construction changed, use matching
  `test_before.log` and `test_after.log`.
- Inspect failures for behavior drift rather than rewriting expectations.
- Record proof commands and results in `todo.md`.

Completion check:

- Focused tests pass with no expectation downgrades, and `todo.md` records the
  proof result and any untested acceptance bullets.

## Step 5: Broader Validation And Close Readiness

Goal: prepare the route for supervisor acceptance and eventual lifecycle close.

Actions:

- Run broader AArch64/backend validation if the implementation touched shared
  scalar helpers, before-call move ordering, call-boundary record construction,
  new helper files, or CMake wiring.
- Confirm no unrelated calls clusters were swept into this owner.
- Confirm diagnostics, immediate contracts, inline-asm spelling, records, and
  assembly remain equivalent.
- Confirm ideas outside the immediate scalar publication boundary remain
  separate work unless a supervisor-approved lifecycle switch happens.
- Record close-readiness notes in `todo.md`.

Completion check:

- The supervisor has enough proof to accept the route or delegate lifecycle
  close, and no unrelated owner boundary was absorbed into this idea.
