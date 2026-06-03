# Prealloc Synthetic Helper Call ABI Authority Runbook

Status: Active
Source Idea: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md

## Purpose

Make the ABI authority for synthetic i128 and f128 runtime helper calls explicit
when prealloc plans helper calls that have no corresponding source BIR
`CallInst` ABI carriers.

## Goal

Classify and prove the helper-call ABI authority boundary for i128 helpers,
f128 arithmetic/cast helpers, and f128 comparison result bridging.

## Core Rule

Do not treat synthetic runtime helper calls as duplicate source-call direct
callee authority. These helpers are prealloc runtime legalization artifacts;
the work is to make their ABI binding and result bridge explicit without moving
physical clobber, preservation, carrier marshaling, or register/stack movement
out of prealloc.

## Read First

- `ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md`
- `src/backend/prealloc/regalloc/runtime_helpers.cpp`
- `src/backend/prealloc/regalloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/regalloc/f128_runtime_helpers.cpp`
- Existing helper ABI, f128, i128, and call-boundary tests under
  `tests/backend/`

## Current Targets

- i128 helper callee synthesis and arg/result ABI binding.
- f128 arithmetic and cast helper callee synthesis and ABI binding.
- f128 comparison helper `I32` result to BIR `I1` comparison-result bridge.
- Prepared-only helper ABI authority versus any structured helper-call carrier
  that the implementation proves necessary.

## Non-Goals

- Do not model synthetic helper callee selection as a source BIR direct callee.
- Do not move clobber, preservation, carrier marshaling, or register/stack
  movement authority out of prealloc.
- Do not rewrite unrelated special-carrier analysis.
- Do not change helper names or expectations without capability proof.
- Do not broaden into unrelated runtime-helper or regalloc rewrites.

## Working Model

Prealloc selects libgcc and soft-float helper callees from BIR opcode/type
facts, then synthesizes helper arg/result ABI bindings and call-boundary policy
for helper calls that do not exist as source BIR calls. That may be legitimate
prepared-only authority, but it needs a named, reviewable contract. F128
comparison helpers additionally bridge an `I32` helper result into the BIR `I1`
comparison result; that semantic bridge must be explicit and covered.

## Execution Rules

- Keep inventory and classification notes in `todo.md`.
- Prefer named helper contracts over broad metadata redesign.
- If prepared-only authority is retained, document its exact scope and proof
  surface.
- If a structured helper-call carrier is introduced, keep it narrow to helper
  ABI facts and do not move physical call planning out of prealloc.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation if shared regalloc, helper, or call-boundary interfaces
  change.

## Step 1: Inventory Synthetic Helper ABI Producers And Consumers

Goal: identify the current helper ABI authority surfaces and result ownership
paths for i128 and f128 helper calls.

Primary targets:

- `src/backend/prealloc/regalloc/runtime_helpers.cpp`
- `src/backend/prealloc/regalloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/regalloc/f128_runtime_helpers.cpp`
- Existing backend helper tests

Actions:

- Trace how i128 helpers are selected and where arg/result ABI bindings are
  synthesized.
- Trace how f128 arithmetic and cast helpers are selected and where arg/result
  ABI bindings are synthesized.
- Trace f128 comparison helper result ownership from helper `I32` result to
  BIR `I1` comparison result.
- Separate semantic helper ABI facts from physical clobber, preservation,
  carrier movement, and register/stack placement decisions.
- Record candidate implicit contracts and proof gaps in `todo.md`.

Completion check:

- `todo.md` names each helper family, producer/consumer surface, and implicit
  result bridge.
- Analysis proof confirms no implementation diff under `src/backend/prealloc`
  for the inventory packet.

## Step 2: Classify Helper ABI Authority

Goal: decide whether synthetic helper ABI remains prepared-only or needs a
structured helper-call carrier.

Actions:

- Compare Step 1 findings against the source idea acceptance criteria.
- Decide the narrow authority model for i128 helpers, f128 arithmetic/cast
  helpers, and f128 comparison helpers.
- Name the F128 comparison helper-result-to-BIR-result bridge contract.
- Reject routes that only rename structures while leaving helper ABI authority
  or result bridging implicit.
- Identify focused proof cases for i128 helper ABI binding, f128 helper ABI
  binding, and f128 comparison result bridging.

Completion check:

- `todo.md` records a concrete authority classification, named bridge contract,
  implementation targets, and proof targets.
- Analysis proof confirms no implementation diff unless the packet explicitly
  starts implementation.

## Step 3: Implement Or Document The Contract

Goal: make helper ABI binding and f128 comparison result bridging reviewable at
the chosen authority surface.

Actions:

- Add or refine named helper predicates, plan fields, or contract helpers for
  synthetic helper ABI authority.
- Keep helper callee synthesis scoped to prealloc runtime legalization.
- Make arg/result ABI binding explicit for representative i128 and f128 helper
  routes.
- Make the f128 comparison `I32` helper-result to BIR `I1` result bridge
  explicit and locally auditable.
- Avoid moving physical clobber, preservation, carrier marshaling, or movement
  out of prealloc.

Completion check:

- The default build passes.
- The diff exposes the helper ABI authority and f128 result bridge through
  named reviewable code, not just renamed structures.
- No unrelated runtime-helper or regalloc rewrite is introduced.

## Step 4: Add Focused Helper Proof

Goal: prove representative i128, f128 arithmetic/cast, and f128 comparison
helper routes against the named contract.

Actions:

- Add or strengthen tests for i128 helper ABI arg/result binding.
- Add or strengthen tests for f128 arithmetic or cast helper ABI binding.
- Add or strengthen tests for f128 comparison helper result bridging from
  helper `I32` to BIR `I1`.
- Prefer prepared-plan, contract, or instruction-record assertions that expose
  the authority boundary directly.

Completion check:

- The default build passes.
- Focused backend helper tests pass.
- Tests do not weaken expectations, mark supported paths unsupported, or prove
  only one helper name while leaving adjacent helper families unexamined.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant backend helper/call-boundary subset.
- Include broader `^backend_` validation if shared helper, regalloc, or
  call-boundary interfaces changed.
- Update `todo.md` with final proof, retained prepared-only authority details,
  and close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows coverage for i128 helper ABI binding, f128 helper ABI
  binding, and f128 comparison result bridging.
- The source idea acceptance criteria are satisfied without unrelated helper or
  regalloc rewrites.
