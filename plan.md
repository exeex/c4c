# AArch64 Calls F128 Carrier Operand Owner Runbook

Status: Active
Source Idea: ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md

## Purpose

Extract or consolidate the AArch64 calls-side owner for f128 carrier completion
and q-register operand rendering used by call-boundary moves.

## Goal

Create a narrow operand-owner boundary that consumes existing prepared f128
carrier facts and renders AArch64 q-register operands without changing f128
transport, diagnostics, call records, or assembly behavior.

## Core Rule

This route owns AArch64 calls-side f128 carrier completion and q-register
operand rendering only. Do not move prepared f128 carrier selection, shared
transport authority, ordinary scalar FP handling, aggregate byval transport,
immediate scalar publication, or call-boundary record ownership into the new
owner.

## Read First

- `ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/prealloc/special_carriers.cpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`
- `tests/backend/mir/CMakeLists.txt`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/CMakeLists.txt`

## Current Targets

- Primary implementation target: `src/backend/mir/aarch64/codegen/calls.cpp`
- Public declaration target only if required by an existing external call path:
  `src/backend/mir/aarch64/codegen/calls.hpp`
- New local helper files are allowed only if they make the f128 operand-owner
  boundary clearer and remain AArch64-local.
- Test/proof targets should stay focused on f128 call-boundary carriers,
  prepared f128 carrier dumps or diagnostics, q-register operand rendering,
  and adjacent ordinary scalar FP call-boundary moves.

## Non-Goals

- Do not create shared f128 transport authority or move carrier selection out
  of prepared call/prealloc owners.
- Do not rework aggregate byval lane transport, immediate scalar publication,
  ordinary register operand rendering, or call-boundary record schema except
  where directly coupled to the f128 q-register operand path.
- Do not change f128 assembly output, unsupported diagnostics, q-register
  selection, call-boundary record classification, or printer-side validation as
  a cleanup side effect.
- Do not fold unrelated scalar GP/FP operand helpers into this route unless the
  coupling must be split to isolate f128 carrier rendering.
- Do not claim progress through helper renames, file movement, expectation
  rewrites, or line-count reduction alone.

## Working Model

The f128 carrier operand owner may consume:

- prepared f128 carrier placements
- full-width f128 carrier facts
- constant f128 carrier payload facts
- scalar FP register names when needed to map the f128 q-register view
- diagnostics context needed to preserve existing rejection paths

The owner may produce:

- completed f128 carrier facts for the existing call-boundary path
- AArch64 q-register operands derived from prepared carriers
- scalar FP view conversions directly needed by the f128 q-register path

The owner must not decide which f128 carrier should exist, which call argument
or result should be selected, or which ordinary call-boundary record should be
constructed.

## Execution Rules

- Keep edits behavior-preserving unless the source idea explicitly requires a
  diagnostic-preserving boundary adjustment.
- Prefer a small local helper type, namespace, or file boundary over a broad
  `calls.cpp` split.
- Preserve exact diagnostics and record fields unless a focused test update
  proves the old spelling was wrong and the supervisor approves the contract
  change.
- If helper APIs or f128 call-boundary record construction paths change,
  produce matching `test_before.log` and `test_after.log` for the focused
  subset.
- A code-changing packet needs `cmake --build --preset default` or an
  equivalent fresh build proof before acceptance.
- Escalate to reviewer if the implementation starts selecting f128 carriers or
  changing prepared carrier contracts instead of consuming them.

## Step 1: Confirm F128 Carrier Boundary And Proof Subset

Goal: map the current f128 carrier helpers and choose the focused proof subset
before changing implementation.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Locate these helpers and their call sites:
  `complete_full_width_f128_carrier`,
  `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`,
  `make_f128_q_register_operand_from_carrier`,
  `scalar_fp_view_from_register_name`, and any directly coupled scalar FP view
  conversion helpers used by the f128 path.
- Record which helpers consume prepared f128 carrier facts, which helpers
  render q-register operands, and which scalar FP helpers are shared with
  non-f128 call-boundary moves.
- Identify the smallest AArch64-local owner boundary that consumes prepared
  f128 carrier placements and emits completed carriers or q-register operands.
- Choose a focused proof subset covering full-width f128 carriers, constant
  carriers, prepared carrier lookup, q-register operand rendering, adjacent
  ordinary scalar FP call-boundary moves, and one missing-carrier or rejection
  diagnostic path when available.
- Record the proposed helper boundary and exact proof command in `todo.md`.

Completion check:

- `todo.md` names the f128 helpers to move or consolidate, names shared scalar
  FP helpers that must remain outside or be wrapped narrowly, and provides the
  exact proof command the executor will run after implementation.

## Step 2: Introduce The F128 Carrier Operand Owner

Goal: create or tighten the local owner API for f128 carrier completion and
q-register operand rendering.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Extract a narrow local helper type, namespace, or file boundary around the
  f128 helpers identified in Step 1.
- Keep input parameters tied to prepared carrier facts, scalar register names,
  payload facts, and diagnostics.
- Keep outputs limited to completed carrier facts and q-register operands
  already consumed by the calls-side call-boundary machinery.
- Avoid API shapes that accept whole call plans when a prepared carrier or
  selected register name is sufficient.
- Build after the extraction.

Completion check:

- The project builds, and the new boundary makes f128 carrier completion and
  q-register rendering explicit without changing prepared carrier selection or
  call-boundary authority.

## Step 3: Route F128 Call-Boundary Uses Through The Owner

Goal: make every in-scope calls-side f128 use consume the owner consistently.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Route full-width f128 carrier completion through the owner.
- Route constant f128 carrier completion through the owner.
- Route prepared f128 carrier lookup or lookup consumption through the owner
  only if doing so clarifies the calls-side boundary without taking ownership
  of prepared carrier creation.
- Route q-register operand rendering through the owner.
- Preserve diagnostics, operand shape, q-register selection, emitted records,
  and assembly.
- Leave ordinary scalar FP register moves, aggregate byval lanes, immediate
  scalar publication, and call-boundary record construction outside this
  boundary unless Step 1 documents direct f128 coupling.
- Build after routing.

Completion check:

- All in-scope f128 carrier call sites use the owner or have a documented
  reason in `todo.md` for staying outside it, and the build is green.

## Step 4: Prove Focused Behavior

Goal: prove the extraction preserved f128 call-boundary behavior.

Primary proof:

- `cmake --build --preset default`
- A focused `ctest --test-dir build -R '<pattern>' --output-on-failure` subset
  chosen in Step 1, expected to include relevant AArch64 f128 carrier,
  prepared carrier, q-register operand, adjacent scalar FP call-boundary, and
  diagnostic coverage.

Actions:

- Run the exact focused proof command chosen in Step 1.
- If helper APIs or record fields changed, use matching `test_before.log` and
  `test_after.log`.
- Inspect failures for behavior drift rather than rewriting expectations.
- Record proof commands and results in `todo.md`.

Completion check:

- Focused tests pass with no expectation downgrades, and `todo.md` records the
  proof result and any untested acceptance bullets.

## Step 5: Broader Validation And Close Readiness

Goal: prepare the route for supervisor acceptance and eventual lifecycle close.

Actions:

- Run broader AArch64/backend validation if the implementation touched shared
  scalar FP helpers, call-boundary record construction, new helper files, or
  CMake wiring.
- Confirm no unrelated calls clusters were swept into this owner.
- Confirm diagnostics, f128 assembly, q-register selection, and prepared
  carrier contracts remain equivalent.
- Record close-readiness notes in `todo.md`.

Completion check:

- The supervisor has enough proof to accept the route or delegate lifecycle
  close, and ideas outside the f128 carrier boundary remain separate open work.
