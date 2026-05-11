# Backend Semantic BIR Route Alignment Runbook

Status: Active
Source Idea: ideas/open/163_backend_semantic_bir_route_alignment.md

## Purpose

Drive the 31 backend route semantic BIR failures accepted by idea 162 back to green without weakening the semantic observation contract.

## Goal

Classify every failed backend semantic BIR route, fix route or lowering defects where the actual output is wrong, and update expected snippets only when the actual BIR is semantically correct under the structured `LinkNameId` model.

## Core Rule

Do not mark supported backend route cases unsupported, convert semantic BIR observations into LLVM-output tests, or make expectation rewrites stand in for real route or lowering repair.

## Read First

- Source idea: `ideas/open/163_backend_semantic_bir_route_alignment.md`
- Baseline failure source: `test_baseline.log`
- Backend route tests and harnesses under the existing test tree
- BIR IR and lowering: `src/backend/bir/bir.hpp`, `src/backend/bir/lir_to_bir/`
- BIR validation and printing: `src/backend/bir/bir_validate.cpp`, `src/backend/bir/bir_printer.cpp`
- Backend preparation and route selection code touched by semantic BIR observations

## Current Scope

- Inventory and classify the 31 backend route failures from the current baseline.
- Separate stale semantic snippets from route/harness defects and true lowering bugs.
- Fix semantic BIR tests that receive LLVM IR through the route or harness layer.
- Repair BIR/LIR/backend lowering when actual output is semantically wrong or loses structured symbol authority.
- Preserve aggregate/byval, function pointer, global struct/array, inline asm, builtin memcpy/memset, and dynamic member-array route coverage.
- End with backend route proof and supervisor-selected full-suite baseline proof that no longer carries these 31 failures, unless a residual family is split into a new open idea with explicit justification.

## Non-Goals

- Do not reopen parser, sema, HIR string-table, template-binding, or `LinkNameId` authority decisions from ideas 158 through 162.
- Do not weaken snippets, broaden matching, delete expectations, or mark supported cases unsupported.
- Do not replace semantic BIR tests with LLVM text tests.
- Do not perform large backend feature work unrelated to the failed route families.
- Do not treat temp-number churn as a semantic failure when the BIR is otherwise correct.

## Working Model

- Each failure first receives a semantic classification: stale expected snippet, route/harness output mismatch, or real lowering/backend bug.
- Expected snippets may change only after actual BIR is inspected and judged semantically correct.
- A semantic BIR route that produces LLVM IR is a route-selection or harness defect until proven otherwise.
- `LinkNameId` remains authoritative for covered symbol paths; raw strings can remain only as output text, route-local handles, diagnostics, or explicit no-metadata compatibility.

## Execution Rules

- Keep `todo.md` as the live inventory of classified failures and proof results.
- Work by failure family, not by isolated named testcase shortcuts.
- Before editing an expectation, record why the actual BIR is semantically correct.
- If a route produces LLVM IR for a semantic BIR observation, fix route selection before updating snippets.
- If a family exposes an unrelated substantial backend initiative, record a separate open idea instead of silently expanding this plan.
- Every code-changing or expectation-changing step needs fresh build or targeted route proof; use broader validation before accepting the overall route as complete.

## Ordered Steps

### Step 1: Inventory and Classify the 31 Baseline Failures

Goal: produce a reviewed failure map before any implementation or expectation edits.

Primary targets:
- `test_baseline.log`
- backend route test definitions and expected semantic BIR snippets
- backend route harness code used by semantic BIR observations

Actions:
- Extract the 31 failing backend route test ids and names from `test_baseline.log`.
- Group them into the source idea's initial families: aggregate/byval/function pointer, global struct/array, inline asm readwrite, builtin memcpy/memset, and nested pointer/member/dynamic array.
- For each failed test, record whether the actual output appears to be stale BIR numbering, LLVM-vs-BIR route mismatch, changed inline asm constraint shape, aggregate/global lowering mismatch, or unknown.
- Choose the first narrow family for implementation proof.

Completion check:
- `todo.md` contains the failure inventory, classification table, and first implementation target.
- No implementation files or expected snippets have been edited in this step.

### Step 2: Repair Semantic BIR Route Selection Mismatches

Goal: ensure semantic BIR observation tests receive BIR output, not LLVM IR output.

Primary targets:
- backend route harness and route-selection code
- tests classified in Step 1 as LLVM-vs-BIR mismatches

Actions:
- Trace how each affected semantic BIR test selects the backend observation route.
- Fix harness or route-selection code so semantic BIR tests request and compare BIR output.
- Keep LLVM text routes intact for tests that are explicitly LLVM observations.
- Add or adjust focused route tests only to prove correct observation routing.

Completion check:
- All tests classified as route mismatches now produce semantic BIR for the semantic BIR route.
- Targeted backend route proof passes or remaining failures are reclassified in `todo.md`.

### Step 3: Align Semantically Correct Stale BIR Snippets

Goal: update expected semantic BIR only where actual output is correct under the current model.

Primary targets:
- expected semantic BIR snippets for temp-number, aggregate, global, inline asm, memcpy/memset, and dynamic-member-array routes

Actions:
- For each stale-snippet failure, inspect actual BIR and confirm control flow, data flow, type shape, symbol identity, and side effects are semantically correct.
- Update expected snippets for temp-number or representation churn only after recording the justification in `todo.md`.
- Preserve the original semantic intent of each route, including aggregate/byval, globals, inline asm, builtin memory operations, and dynamic-member-array access.

Completion check:
- Updated expectations are justified by semantically correct actual BIR.
- Targeted route tests for the edited expectations pass.

### Step 4: Fix True BIR/LIR/Backend Lowering Bugs

Goal: repair failures where actual BIR is semantically wrong or loses structured symbol authority.

Primary targets:
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_validate.cpp`
- backend preparation code for affected route families

Actions:
- For each real lowering bug, identify the narrow producer or consumer responsible for the wrong BIR.
- Fix the semantic lowering rule instead of matching a named testcase shape.
- Preserve or restore `LinkNameId` on covered direct-call, global, initializer, and memory-address paths.
- Add focused proof for nearby same-feature cases when a route family is repaired.

Completion check:
- Actual BIR is semantically correct for each repaired family.
- Targeted backend/BIR proof passes for the repaired family and nearby same-feature cases.

### Step 5: Prove Family Coverage and Guard Against Overfit

Goal: show each failure family is repaired as a capability, not as a narrow expectation trick.

Actions:
- Re-run the relevant backend route subset after each family is repaired.
- Check nearby aggregate/byval/function pointer, global struct/array, inline asm, builtin memcpy/memset, and nested pointer/member/dynamic array cases for the same failure mode.
- Add focused tests where coverage is missing for `LinkNameId` transport or structured semantic BIR route behavior.
- Reject any change whose proof relies only on deleting expectations, broadening matching, or named-case special handling.

Completion check:
- The backend route subset covering the original 31 failures passes.
- `todo.md` records proof for each family and any residual work that must be split instead of hidden.

### Step 6: Final Baseline and Closure Readiness

Goal: prepare the active idea for supervisor closure review.

Actions:
- Run the supervisor-selected full-suite or regression-guard validation.
- Confirm the final baseline no longer contains the 31 backend route failures from idea 162.
- Record any residual failure in a new `ideas/open/` file only if it is a distinct initiative with explicit justification and reviewer reject signals.
- Summarize retained compatibility bridges or route limitations in `todo.md`.

Completion check:
- Full-suite or regression-guard proof is available for supervisor review.
- The active source idea acceptance criteria are either satisfied or the exact blocker is recorded for lifecycle decision.
