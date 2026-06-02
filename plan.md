# AArch64 Calls Stack And Frame-Slot Operand Owner Runbook

Status: Active
Source Idea: ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md

## Purpose

Extract or consolidate the AArch64-local owner for stack, frame-slot, sret,
aggregate-source, and prior stack-preserved memory operands used by call
lowering.

## Goal

Create a narrow operand-owner boundary that consumes existing prepared
endpoints, value homes, frame plans, and selected source facts without changing
call behavior.

## Core Rule

This route owns AArch64 stack/frame-slot memory operand rendering only. Do not
move source-selection authority, prepared call plans, move bundles,
publication producers, after-call result facts, preservation facts, or aggregate
transport facts into the new owner.

## Read First

- `ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- `tests/backend/mir/CMakeLists.txt`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/CMakeLists.txt`

## Current Targets

- Primary implementation target: `src/backend/mir/aarch64/codegen/calls.cpp`
- Public declaration target only if required by an existing external call path:
  `src/backend/mir/aarch64/codegen/calls.hpp`
- New local helper files are allowed only if they make the operand-owner
  boundary clearer and remain AArch64-local.
- Test/proof targets should stay focused on AArch64 call-boundary, prepared
  frame/stack call contracts, backend route, and c-testsuite cases that cover
  the affected call forms.

## Non-Goals

- Do not rederive prepared call plans, move bundles, publication producers,
  after-call result facts, preservation facts, or aggregate transport facts.
- Do not introduce shared BIR/prealloc ownership for AArch64 frame-slot
  addressing without a separate target-neutral evidence idea.
- Do not rewrite before-call move ordering, after-call republication, aggregate
  byval lane publication, indirect callee materialization, scalar producer
  bridging, call instruction records, or ordinary call-boundary record schema.
- Do not change unsupported diagnostics, selection status, assembly output, or
  frame layout behavior as part of cleanup.
- Do not claim progress through helper renames, file movement, expectation
  rewrites, or line-count reduction alone.

## Working Model

The stack/frame-slot operand owner may consume:

- prepared endpoints and homes
- selected source facts
- prepared frame plans and stack layout facts
- known stack offsets, frame slots, and storage sizes
- diagnostics sinks needed to preserve existing rejection paths

The owner may produce:

- AArch64 memory operands
- frame-slot address materialization instruction lines
- stack-copy address details
- source/destination operand records already expected by the calls-side
  call-boundary machinery

The owner must not decide which call argument, result, preserved value, or
aggregate transport fact should exist.

## Execution Rules

- Keep edits behavior-preserving unless the source idea explicitly requires a
  diagnostic-preserving boundary adjustment.
- Prefer small local helper APIs over a broad `calls.cpp` split.
- Preserve exact diagnostics and record fields unless a focused test update
  proves the old spelling was wrong and the supervisor approves the contract
  change.
- If helper APIs or call-boundary record construction paths change, produce
  matching `test_before.log` and `test_after.log` for the focused subset.
- A code-changing packet needs `cmake --build --preset default` or an
  equivalent fresh build proof before acceptance.
- Escalate to reviewer if the implementation starts selecting facts rather
  than rendering prepared operands.

## Step 1: Confirm Operand Boundary And Proof Subset

Goal: map the current stack/frame-slot operand helpers and choose the focused
proof subset before changing implementation.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Locate these helpers and their call sites:
  `call_boundary_frame_slot_direct_offset_is_encodable`,
  `materialize_call_boundary_frame_slot_address_lines`, `stack_copy_address`,
  `make_selected_frame_slot_source`, `make_sret_memory_return_address_source`,
  `make_selected_local_frame_address_source`,
  `make_stack_call_argument_destination`,
  `make_aggregate_call_argument_source`,
  `make_prior_preserved_call_argument_source`,
  `endpoint_has_stack_storage`, `make_frame_slot_memory_from_endpoint`,
  `find_prior_stack_preserved_value_before_instruction`, and
  `make_prior_stack_preserved_value_source`.
- Record which helpers only render operands and which helpers also touch
  selection, publication, preservation, or record construction.
- Identify the smallest AArch64-local owner boundary that consumes prepared
  facts and emits operands/materialization details.
- Choose a focused proof subset covering stack call arguments, frame-slot
  call-boundary sources, sret or result-related memory sources, aggregate stack
  sources, prior stack-preserved values, and one rejection or non-encodable
  frame-slot path when available.
- Record the proposed helper boundary and proof command in `todo.md`.

Completion check:

- `todo.md` names the helpers to move or consolidate, names helpers that must
  remain in existing calls-side owners, and provides the exact proof command
  the executor will run after implementation.

## Step 2: Introduce The Operand Owner

Goal: create or tighten the local owner API for rendering stack/frame-slot
operands.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Extract a narrow local helper type, namespace, or file boundary around the
  operand-rendering helpers identified in Step 1.
- Keep input parameters tied to prepared endpoints, homes, frame plans,
  selected source facts, and diagnostics.
- Keep outputs limited to memory operands, address materialization lines, and
  source/destination operand records already consumed by call-boundary code.
- Avoid API shapes that accept whole call plans when a selected endpoint or
  home is sufficient.
- Build after the extraction.

Completion check:

- The project builds, and the new boundary makes stack/frame-slot operand
  rendering explicit without changing source-selection or call-boundary
  authority.

## Step 3: Route All Stack/Frame-Slot Call Sites Through The Owner

Goal: make every in-scope calls-side use consume the owner consistently.

Primary target:

- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Route stack call argument destinations through the owner.
- Route selected frame-slot sources, sret/local-frame sources, aggregate
  argument sources, endpoint stack storage, and prior stack-preserved sources
  through the owner.
- Preserve diagnostics, memory operand shape, frame-slot offset handling,
  emitted records, and assembly.
- Leave before-call move ordering, after-call republication, scalar producer
  bridging, and aggregate byval lane publication outside this boundary.
- Build after routing.

Completion check:

- All in-scope call sites use the owner or have a documented reason in
  `todo.md` for staying outside it, and the build is green.

## Step 4: Prove Focused Behavior

Goal: prove the extraction preserved call-boundary behavior for the affected
stack/frame-slot paths.

Primary proof:

- `cmake --build --preset default`
- A focused `ctest --test-dir build -R '<pattern>' --output-on-failure` subset
  chosen in Step 1, expected to include relevant AArch64 call-boundary,
  prepared frame/stack call contract, backend route, and c-testsuite coverage.

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
  call-boundary record construction, new helper files, or CMake wiring.
- Confirm no unrelated calls clusters were swept into this owner.
- Confirm ideas `94` and `95` remain separate follow-up work and were not
  silently absorbed.
- Record any residual risks or intentionally local helpers in `todo.md`.

Completion check:

- The implementation satisfies the source idea acceptance criteria, proof is
  fresh, and remaining work is either out of scope or captured in existing
  follow-up ideas.
