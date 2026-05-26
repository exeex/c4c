# x86 Prepared Edge Publication Remaining Home Coverage Runbook

Status: Active
Source Idea: ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md

## Purpose

Continue the x86 prepared edge-publication consumer work from ideas 21 and 22
by covering remaining nearby source and destination home combinations before
any RISC-V consumer readiness claim.

## Goal

Land at least one additional x86 edge-publication home combination or nearby
edge shape that consumes shared prepared edge-publication facts through the
existing shared lookup authority.

## Core Rule

Shared prepare remains the semantic authority for edge-publication facts.
x86 may consume
`x86::ConsumedPlans::shared_function_lookups()->edge_publications`, but must not
rediscover edge-copy facts locally or move x86 emission policy into shared
prepare, BIR, or target-neutral helpers.

## Read First

- `ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md`
- Recent code and tests from the closed idea 22 handoff, as needed for local
  context only.
- x86 prepared edge-publication consumption paths that already cover the idea
  21 stack-source to register-destination and idea 22 register-source to
  register-destination cases.

## Current Targets

- Remaining nearby x86 source/destination home combinations, especially
  destination homes intentionally left unsupported after idea 22.
- Focused tests that prove x86 reads shared `edge_publications` rather than a
  target-local rediscovery path or named-case shortcut.
- Fail-closed behavior for homes that are still outside the safe x86 lowering
  surface.

## Non-Goals

- Do not implement a RISC-V consumer in this plan.
- Do not claim RISC-V readiness until the final handoff evaluates remaining x86
  coverage.
- Do not attempt full x86 codegen completion or broad control-flow lowering
  rewrites.
- Do not create x86-local edge-copy fact tables, predecessor/successor scans,
  or fallback semantic authority.
- Do not weaken supported-path expectations or mark targeted paths unsupported
  as a substitute for capability work.

## Working Model

- Shared prepared lookup data owns edge-publication facts and indexing.
- x86 lowering owns scratch selection, clobber policy, physical register
  spelling, register-class constraints, stack operand syntax, move spelling,
  branch/control-flow emission, and final assembly formatting.
- Unsupported homes should remain explicit and fail closed until x86 can emit
  them safely.

## Execution Rules

- Prefer a small semantic home-family implementation over testcase-shaped
  matching.
- Keep tests focused enough to fail if shared `edge_publications` are ignored.
- Preserve existing AArch64 behavior and shared prepared lookup contracts.
- Run build or compile proof for each code-changing step, then the narrow x86
  tests and relevant shared/backend subset chosen by the supervisor.
- Escalate to reviewer or plan-owner if the route requires changing authority
  boundaries or downgrading supported-path expectations.

## Ordered Steps

### Step 1: Inventory Remaining x86 Edge Homes

Goal: identify the next auditable x86 prepared edge-publication home family to
cover.

Primary target: x86 prepared edge-publication consumer code and focused tests
from ideas 21 and 22.

Actions:

- Inspect the existing x86 shared edge-publication consumption path.
- List already-covered homes from ideas 21 and 22.
- Identify remaining nearby source/destination home combinations, with special
  attention to destination homes left unsupported after idea 22.
- Choose the smallest coherent home family whose implementation is semantic,
  testable, and does not require an authority-boundary change.

Completion check:

- `todo.md` records the chosen home family, why it is in scope, and which homes
  stay explicitly unsupported.

### Step 2: Implement Shared-Lookup Consumption for the Selected Home Family

Goal: make the selected x86 home family consume shared prepared
edge-publication facts.

Primary target: x86 lowering and emission code that already reads
`shared_function_lookups()->edge_publications`.

Actions:

- Extend the existing x86 consumer path for the selected source/destination
  home family.
- Keep x86-specific scratch, clobber, physical-register, stack-operand, move,
  and assembly-formatting policy inside x86.
- Reuse the shared lookup authority without adding x86-local fact discovery.
- Preserve fail-closed handling for unsupported homes and missing authority.

Completion check:

- The selected home family lowers through shared `edge_publications`.
- Unsupported homes still produce explicit unsupported or fail-closed behavior.
- The implementation does not add testcase-name, edge-name, or fixture-label
  shortcuts.

### Step 3: Add Focused Coverage and Negative Proof

Goal: prove the new path is real shared lookup consumption, not local
rediscovery or expectation-only progress.

Primary target: focused x86 prepared edge-publication tests and nearby shared
prepared lookup tests.

Actions:

- Add or extend tests for the selected remaining home family.
- Include assertions that would fail if x86 ignored shared
  `edge_publications`.
- Cover missing-authority or still-unsupported-home behavior where relevant.
- Avoid weakening existing supported-path expectations.

Completion check:

- Tests distinguish the new home-family rule from the idea 21 and idea 22
  coverage.
- Tests fail closed for unsupported or missing-authority cases.

### Step 4: Validate the x86 Slice

Goal: provide enough proof for supervisor acceptance without overstating
readiness.

Primary target: build proof, focused x86 tests, shared prepared lookup tests,
and a backend bucket appropriate to touched lowering code.

Actions:

- Run the exact proof command delegated by the supervisor for executor packets.
- Capture the executor proof in the canonical `test_after.log` when delegated.
- Include any compile/build failure details in `todo.md`.
- Do not run broad validation as a substitute for narrow semantic checks.

Completion check:

- Build or compile proof is fresh for the code-changing slice.
- Narrow x86 and relevant shared/backend validation is green, or blockers are
  recorded with exact failure evidence.

### Step 5: Handoff RISC-V Readiness Decision

Goal: decide whether x86 coverage is broad enough to open a separate RISC-V
consumer idea.

Primary target: final `todo.md` handoff and, if needed, a future source idea
created through lifecycle routing.

Actions:

- Summarize covered x86 edge/home combinations after this plan.
- Name remaining unsupported homes and whether they fail closed.
- State whether a separate RISC-V consumer idea is now justified.
- If another x86 coverage slice is needed, recommend that as follow-up instead
  of claiming readiness.

Completion check:

- The handoff makes a concrete readiness decision grounded in covered and
  remaining x86 homes.
- No RISC-V implementation work has been folded into this plan.
