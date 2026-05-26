# x86 Prepared Edge Publication Consumer Runbook

Status: Active
Source Idea: ideas/open/21_x86_prepared_edge_publication_consumer.md

## Purpose

Turn the idea 19 handoff into the smallest real x86 lowering consumer of shared
prepared edge-publication facts.

## Goal

Make one narrow x86 edge or block-entry publication path read
`x86::ConsumedPlans::shared_function_lookups()->edge_publications` and emit the
target-specific result from x86-owned code.

## Core Rule

Shared prepare owns the semantic edge-publication facts; x86 owns scratch
selection, clobber avoidance, physical register spelling, register-class
constraints, stack operand syntax, move spelling, branch/control-flow emission,
and final assembly formatting.

## Read First

- `ideas/open/21_x86_prepared_edge_publication_consumer.md`
- `ideas/closed/19_x86_riscv_prepared_edge_publication_handoff.md`
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/lowering/lowering.hpp`
- `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` only as a
  consumer-pattern reference, not as an implementation template to copy.

## Current Targets

- Existing shared lookup path:
  `x86::ConsumedPlans::shared_function_lookups()->edge_publications`
- Existing proof fixture:
  `backend_x86_prepared_decoded_home_storage_test.cpp`
- x86-owned prepared/lowering surfaces under `src/backend/mir/x86/`
- Shared lookup helpers under `src/backend/prealloc/prepared_lookups.*`

## Non-Goals

- Do not implement full x86 codegen.
- Do not add a RISC-V consumer.
- Do not rewrite x86 control-flow lowering broadly.
- Do not create an x86-local edge-copy fact table or rediscovery pass.
- Do not move x86 emission details into shared prepare, BIR, or target-neutral
  helpers.
- Do not weaken supported-path tests or expectation contracts.
- Do not perform AArch64 cleanup or layout classification as part of this plan.

## Working Model

- x86 already has enough prepared-module consumption to expose shared function
  lookups through `ConsumedPlans`.
- Idea 19 proved that x86 can read indexed shared edge-publication records from
  the prepared decoded-home fixture.
- This runbook must advance from read-only proof to one narrow x86 lowering
  behavior that depends on those shared records.
- The first implementation should favor a small, auditable block-entry or edge
  publication path over broad dispatch reconstruction.

## Execution Rules

- Keep each code-changing step paired with a build or narrow test proof.
- Prefer adding a named x86 consumer helper before wiring broader emission.
- Tests must fail if the x86 path ignores shared `edge_publications` or derives
  independent edge-copy authority locally.
- Preserve existing AArch64 behavior and shared prepared lookup contracts.
- If the current x86 lowerer lacks the minimum emission surface, record the
  exact blocker in `todo.md` and stop for lifecycle review instead of expanding
  scope into a backend rewrite.

## Step 1: Map the Minimal x86 Consumer Path

Goal: identify the smallest x86-owned surface that can consume one shared
prepared edge-publication record and produce target-owned lowering output.

Primary targets:
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/lowering/*.cpp`
- `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`

Actions:
- Inspect the existing x86 prepared query, dispatch, and lowering surfaces.
- Choose one block-entry or edge-publication path that can read the shared
  indexed publication records without creating x86-local semantic authority.
- Identify the x86-owned output shape for the first consumer proof, such as a
  target move description, lowering decision, or emitted instruction text.
- Record any missing minimum surface in `todo.md` if the path is blocked.

Completion check:
- The selected path is named in `todo.md`, including owned files, the shared
  lookup call site, and the narrow proof command the executor will use next.

## Step 2: Add the x86 Shared-Publication Consumer Helper

Goal: add a small x86-owned helper that turns one shared prepared
edge-publication record into an x86 lowering intent or output.

Primary targets:
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/lowering/*.cpp`

Actions:
- Read publications through
  `ConsumedPlans::shared_function_lookups()->edge_publications` or an x86
  wrapper that delegates to that authority.
- Keep the helper target-local: it may select x86 operand spelling and move
  form, but it must not rediscover predecessor/successor edge semantics.
- Return an explicit unsupported or missing-authority result when the shared
  fact or required x86 home information is absent.
- Avoid copying the AArch64 implementation shape unless the x86 surface already
  has equivalent ownership boundaries.

Completion check:
- The helper compiles, has no independent edge-copy table, and its behavior is
  covered by a focused x86 test that exercises shared lookup consumption.

## Step 3: Wire One Narrow Lowering Behavior

Goal: make one real x86 lowering path depend on the consumer helper.

Primary targets:
- The x86 prepared/lowering files selected in Step 1.
- The focused x86 test fixture selected in Step 1.

Actions:
- Connect the helper to one block-entry or edge-publication lowering path.
- Emit target-specific output from x86 code only.
- Preserve missing-authority behavior instead of inventing fallback edge-copy
  semantics.
- Keep branch/control-flow rewrites out of scope unless the selected path
  already owns them.

Completion check:
- A focused test fails without the shared publication consumer and passes with
  the implementation.
- The diff does not weaken existing expectations or mark supported paths
  unsupported.

## Step 4: Validate Shared and Target Boundaries

Goal: prove the new x86 consumer does not regress the shared lookup contract or
AArch64 behavior.

Actions:
- Run the narrow x86 proof selected in Step 1.
- Run the relevant shared prepared lookup test covering edge-publication
  indexing.
- Run an AArch64-adjacent proof if the implementation touched shared lookup
  helpers or common prepared data.
- Escalate to the supervisor for broader validation if the diff crosses shared
  prepare, BIR, or multiple target lowering surfaces.

Completion check:
- `test_after.log` records the delegated proof command and a passing result.
- `todo.md` lists the exact commands and whether broader validation is still
  recommended.

## Step 5: Handoff and Follow-Up Classification

Goal: leave a clear lifecycle handoff for the next slice.

Actions:
- Summarize the implemented x86 consumer behavior in `todo.md`.
- Identify the next appropriate follow-up: broaden x86 edge coverage, unblock
  RISC-V readiness, or repair a discovered shared contract gap.
- Do not create follow-up ideas unless the supervisor delegates lifecycle work.

Completion check:
- The active source idea acceptance criteria are either satisfied or the
  remaining work is explicit enough for plan-owner review.
