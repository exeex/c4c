# AArch64 Markdown Driven Backend Case Bringup Runbook

Status: Active
Source Idea: ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md

## Purpose

Reopen AArch64 backend case coverage through markdown-owned backend feature contracts instead of testcase-shaped implementation.

## Goal

Build and maintain a matrix that maps each relevant `tests/backend/case` file to its AArch64 markdown owner, required structured implementation owner, machine-node/operator coverage, smoke route, and current support status.

## Core Rule

Do not enable or strengthen a backend case until the matching AArch64 markdown feature has a structured `.cpp/.hpp` implementation path. Tests validate architecture progress; they do not dictate ad hoc code shape.

## Read First

- `ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md`
- `ideas/closed/216_aarch64_machine_node_asm_printer_external_smoke.md`
- `ideas/closed/218_aarch64_structured_asm_encoder_linker_contract.md`
- `ideas/closed/219_aarch64_natural_operator_naming_and_printer_spelling.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/globals.md`
- `tests/backend/case`

## Current Targets And Scope

- Audit AArch64-relevant backend case files under `tests/backend/case`.
- Classify each case by markdown owner and structured implementation owner.
- Identify the first next feature group with a clear `.md -> .cpp/.hpp -> case` route.
- Keep case smoke tests on the public c4cll route with external assembler/compiler validation where applicable.
- Keep internal record/model coverage in focused `tests/backend/backend_aarch64_*` C++ tests.

## Non-Goals

- Do not implement all AArch64 backend features in this runbook.
- Do not enable a large case batch before markdown owners have structured implementation.
- Do not weaken expectations or mark supported paths unsupported to claim progress.
- Do not add testcase-shaped MIR, codegen, printer, or selection shortcuts.
- Do not use external `.s` or `.ll` as backend input.
- Do not implement the internal assembler, encoder, object writer, linker, or binary output route.
- Do not touch `ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md` from this plan.

## Working Model

The allowed progress path is:

```text
AArch64 markdown feature contract
  -> structured .cpp/.hpp owner
  -> target MIR records
  -> machine instruction nodes and natural operator spelling
  -> terminal .s printer smoke validation
  -> tests/backend/case enablement
```

The bring-up matrix is the durable routing artifact for future feature-specific ideas. It should make deferred cases explicit instead of hiding unsupported behavior behind expectation churn.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source intent in the idea file; only repair durable lifecycle metadata there.
- Prefer small semantic feature groups over one-off fixture handling.
- For docs or matrix-only steps, use text-search proof that the expected owners, statuses, and reject rules exist.
- For code-changing steps, run `cmake --build --preset default` plus the narrow backend case or focused AArch64 test subset affected by the step.
- Escalate to broader backend validation before closure when case wiring or shared backend helpers change.
- Treat testcase overfit as a blocking failure even if a narrow case passes.

## Steps

### Step 1: Audit Backend Cases And Markdown Owners

Goal: inventory AArch64-relevant `tests/backend/case` files and map each one to its markdown feature owner.

Primary targets:
- `tests/backend/case`
- `src/backend/mir/aarch64/**/*.md`
- existing backend test manifests and CMake wiring

Actions:
- Inspect backend case files and identify which are relevant to AArch64 bring-up.
- Group cases under markdown owners such as `returns.md`, `alu.md`, `comparison.md`, `memory.md`, `calls.md`, `prologue.md`, and `globals.md`.
- Record whether each case is enabled, deferred, or blocked.
- Record the specific reason for every deferred or blocked case.
- Identify current test wiring and external assembler/compiler smoke entry points without changing them.

Completion check:
- `todo.md` records the audited case groups, markdown owners, status labels, and any missing source needed to build the matrix.
- A search proof captures the case files, markdown owners, and test wiring surfaces inspected.

### Step 2: Commit The Bring-Up Matrix

Goal: add the durable matrix that future AArch64 feature ideas can consume without rediscovering the case strategy.

Primary targets:
- A markdown matrix location under the AArch64 backend docs or test docs, chosen to match nearby repo conventions.
- `todo.md`

Actions:
- Create or update a matrix with columns for markdown owner, structured `.cpp/.hpp` owner, dependent machine-node/operator coverage, case files, external smoke expectation, status, and deferred reason.
- Mark enabled, deferred, and blocked cases honestly.
- Keep the matrix focused on `tests/backend/case` bring-up; do not convert it into a generic backend roadmap.
- Include explicit anti-overfit and expectation-downgrade reject rules.

Completion check:
- The matrix is committed in repo docs and can be found by searching for the markdown owners and case statuses.
- The matrix identifies at least one first next feature group with a clear `.md -> .cpp/.hpp -> case` route.

### Step 3: Validate Existing Public Smoke Route

Goal: prove the current public backend case smoke route is understood before enabling more cases.

Primary targets:
- c4cll backend test invocation
- `tests/backend/case`
- external assembler/compiler validation hooks established by idea 216

Actions:
- Locate the current backend case runner and its external smoke validation behavior.
- Run the narrow command that exercises currently enabled backend case smoke coverage.
- Record whether the route uses public c4cll output and external assembler/compiler proof.
- Do not add internal assembler, encoder, object, linker, or binary output expectations.

Completion check:
- `test_after.log` contains the narrow backend case smoke proof or a precise blocker if the current route cannot run.
- `todo.md` records the command and the smoke-route facts needed by the next executor.

### Step 4: Select The First Feature Group

Goal: choose the first next AArch64 markdown-owned case group to implement in a follow-up feature idea or bounded packet.

Primary targets:
- committed bring-up matrix
- matching AArch64 markdown owner
- current structured implementation owner

Actions:
- Pick one feature group whose markdown owner and structured implementation route are clear.
- Identify required C++ owners, machine-node/operator coverage, focused unit tests, and backend case smoke tests.
- Write only the minimal lifecycle or todo handoff needed for the supervisor to choose the next execution packet.
- If the work is a separate implementation initiative, create or request a separate `ideas/open/` entry instead of silently expanding this matrix plan.

Completion check:
- `todo.md` names the selected first feature group and its proof ladder.
- Any separate initiative needed for implementation is recorded under `ideas/open/` by lifecycle action, not hidden inside this plan.

### Step 5: Final Validation And Close Readiness

Goal: verify the matrix and strategy satisfy idea 221 without drifting into implementation overreach.

Primary targets:
- matrix artifact
- `todo.md`
- relevant backend case smoke command

Actions:
- Re-run the final docs/matrix search proof.
- Run the selected backend smoke or focused AArch64 validation command if test wiring changed.
- Confirm the matrix names enabled, deferred, and blocked cases with reasons.
- Confirm the route rejects testcase-shaped implementation and expectation downgrades.
- Confirm future ideas can pick one markdown owner at a time.

Completion check:
- The source idea acceptance criteria are satisfied.
- The plan owner can close idea 221 after regression guard passes for the chosen scope.
