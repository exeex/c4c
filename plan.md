# RISC-V Prepared Edge Publication Register Destination Consumer Runbook

Status: Active
Source Idea: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md

## Purpose

Start the RISC-V prepared edge-publication consumer route for the
register-destination subset made ready by the x86 handoff from ideas 21
through 23.

## Goal

Make RISC-V consume shared prepared `edge_publications` for at least one
register-destination source-home family, while preserving explicit fail-closed
behavior for homes outside the scoped surface.

## Core Rule

Shared prepare remains the semantic authority for edge-publication facts and
lookup indexing. RISC-V may consume shared prepared lookup data, but must not
rediscover edge-copy facts locally or move RISC-V emission policy into shared
prepare, BIR, or target-neutral helpers.

## Read First

- `ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md`
- Recent x86 prepared edge-publication consumer commits from ideas 21, 22, and
  23 for shared lookup consumption shape only.
- RISC-V backend code that owns register spelling, move emission,
  scratch/control-flow policy, and final assembly formatting.

## Current Targets

- RISC-V register-destination edge-publication moves.
- Source homes already proven through the x86 handoff where RISC-V can emit
  safely: stack source, register source, and rematerializable immediate source.
- Focused RISC-V tests that fail if shared `edge_publications` are ignored.
- Explicit fail-closed behavior for pointer-base sources and stack-slot
  destinations unless a later lifecycle decision brings them into scope.

## Non-Goals

- Do not implement stack-slot destination edge-publication moves.
- Do not implement pointer-base source homes unless a separate plan proves the
  required RISC-V address materialization and move policy.
- Do not attempt full RISC-V codegen completion or broad control-flow lowering
  rewrites.
- Do not create a RISC-V-local edge-copy fact table, predecessor/successor
  scan, or fallback semantic authority.
- Do not weaken supported-path expectations or mark targeted
  register-destination paths unsupported as a substitute for capability work.
- Do not change x86, AArch64, or shared prepared authority boundaries except
  where validation exposes an existing contract issue that must be routed
  separately.

## Working Model

- Shared prepared lookup data owns edge-publication facts and indexes them by
  prepared edge.
- RISC-V lowering owns source operand rendering, destination register spelling,
  move instruction choice, scratch policy, branch/control-flow emission, and
  final assembly formatting.
- Unsupported homes should remain explicit and fail closed until RISC-V can
  emit them safely.

## Execution Rules

- Prefer a small semantic source-home implementation over testcase-shaped
  matching.
- Keep tests focused enough to fail if RISC-V ignores shared
  `edge_publications`.
- Preserve existing x86, AArch64, and shared prepared lookup behavior.
- Run build or compile proof for each code-changing step, then the focused
  RISC-V tests and relevant shared/backend subset chosen by the supervisor.
- Escalate to reviewer or plan-owner if the route requires changing authority
  boundaries or downgrading supported-path expectations.

## Ordered Steps

### Step 1: Inventory RISC-V Edge Publication Consumer Path

Goal: identify the RISC-V lowering surface that should consume shared prepared
edge-publication facts for register-destination moves.

Primary target: RISC-V prepared lowering, block edge emission, and tests that
exercise prepared edge moves.

Actions:

- Inspect current RISC-V prepared edge or block-transition lowering.
- Identify the shared prepared lookup access point that corresponds to the x86
  `edge_publications` consumer shape.
- List current register-destination source-home handling and unsupported-home
  behavior.
- Choose the smallest coherent source-home family to implement first, without
  expanding beyond register destinations.

Completion check:

- `todo.md` records the chosen RISC-V source-home family, the target files or
  functions, and which homes stay explicitly unsupported.

### Step 2: Implement Register-Destination Shared-Lookup Consumption

Goal: make the selected RISC-V register-destination home family consume shared
prepared edge-publication facts.

Primary target: RISC-V lowering and emission code that can read shared prepared
`edge_publications`.

Actions:

- Add the RISC-V consumer path for the selected source-home family to register
  destinations.
- Keep RISC-V-specific register spelling, move spelling, scratch policy,
  branch handling, and assembly formatting inside RISC-V.
- Reuse shared prepared lookup authority without adding RISC-V-local fact
  discovery.
- Preserve fail-closed handling for unsupported homes and missing authority.

Completion check:

- The selected source-home family lowers through shared `edge_publications` to
  a RISC-V register destination.
- Unsupported homes still produce explicit unsupported or fail-closed behavior.
- The implementation does not add testcase-name, edge-name, fixture-label, or
  prepared-value-id shortcuts.

### Step 3: Add Focused RISC-V Coverage and Negative Proof

Goal: prove the new path is real shared lookup consumption, not local
rediscovery or expectation-only progress.

Primary target: focused RISC-V prepared edge-publication tests and nearby
shared prepared lookup tests.

Actions:

- Add or extend tests for the selected RISC-V register-destination source-home
  family.
- Include assertions that would fail if RISC-V ignored shared
  `edge_publications`.
- Cover missing-authority or still-unsupported-home behavior where relevant.
- Avoid weakening existing supported-path expectations.

Completion check:

- Tests distinguish the new RISC-V rule from existing unsupported or no-op
  behavior.
- Tests fail closed for unsupported or missing-authority cases.

### Step 4: Broaden to Additional Ready Source Homes

Goal: decide whether the remaining x86-proven register-destination source
homes can be added in the same route without expanding authority boundaries.

Primary target: RISC-V register-destination handling for stack source,
register source, and rematerializable immediate source homes.

Actions:

- Compare the already-implemented source home against the remaining ready
  source homes from the idea 23 handoff.
- Add another source-home family only if it uses the same shared lookup model
  and target-local emission policy.
- Keep pointer-base sources and stack-slot destinations fail closed unless a
  separate lifecycle decision changes scope.
- Record any source home that should become a follow-up instead of being
  folded into this runbook.

Completion check:

- `todo.md` records which RISC-V register-destination source homes are covered
  by this plan and which remain unsupported.

### Step 5: Validate the RISC-V Slice

Goal: provide enough proof for supervisor acceptance without overstating RISC-V
coverage.

Primary target: build proof, focused RISC-V tests, shared prepared lookup
tests, and a backend bucket appropriate to touched lowering code.

Actions:

- Run the exact proof command delegated by the supervisor for executor
  packets.
- Capture executor proof in canonical `test_after.log` when delegated.
- Include compile/build or test failure details in `todo.md`.
- Do not run broad validation as a substitute for focused semantic checks.

Completion check:

- Build or compile proof is fresh for the code-changing slice.
- Focused RISC-V and relevant shared/backend validation is green, or blockers
  are recorded with exact failure evidence.

### Step 6: Handoff Covered Homes and Follow-Ups

Goal: close the route with a concrete statement of covered RISC-V homes and
remaining unsupported surfaces.

Primary target: final `todo.md` handoff and, if needed, future source ideas
created through lifecycle routing.

Actions:

- Summarize covered RISC-V source/destination home combinations.
- Name unsupported homes that still fail closed, especially pointer-base
  sources and stack-slot destinations.
- State whether another RISC-V coverage slice, pointer-base policy idea, or
  stack-destination idea is needed.
- Avoid claiming full RISC-V edge-publication support unless all scoped homes
  have been implemented and proven.

Completion check:

- The handoff makes a concrete support and follow-up decision grounded in the
  implemented and validated homes.
