# RV64 20000622-1 Runtime Abort Runbook

Status: Active
Source Idea: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Activated from: 572 follow-up runtime-mismatch evidence

## Purpose

Classify and repair the RV64 object-route runtime abort for
`src/20000622-1.c` that appears after ordinary same-module call/result lowering
no longer fails through the old generic `CallInst` fallback.

## Goal

Identify the first real post-call-lowering runtime-mismatch family, add focused
coverage for that family, and repair the underlying RV64 object-route semantics
without regressing the same-module call/result work.

## Core Rule

This plan starts with classification. Do not claim progress through expectation
rewrites, unsupported-marker edits, allowlist changes, runtime-comparison
contract changes, or filename-specific handling for `src/20000622-1.c`.

## Read First

- `ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md`
- `ideas/open/572_rv64_same_module_call_result_lowering.md` only for parent
  context when needed
- `build/agent_state/572_rv64_same_module_call_result_lowering/summary.tsv`
- `build/agent_state/572_rv64_same_module_call_result_lowering/src_20000622-1.c/object-route.log`
- `.codex/skills/c4c-clang-tools/SKILL.md` before broad C++ exploration

## Current Targets

- RV64 object-route reproduction for `src/20000622-1.c`
- Artifact capture under a new 577-specific `build/agent_state/` directory
- RV64 backend/object-emission code only after the first semantic mismatch
  family is classified
- Focused backend or runtime coverage for the classified family
- Existing same-module call/result focused tests and representative evidence

## Non-Goals

- Do not reopen ordinary same-module call fallback work unless fresh evidence
  shows the abort is still caused by call argument/result lowering.
- Do not edit testcase expectations, unsupported markers, allowlists, runtime
  comparison behavior, or the gcc_torture runner.
- Do not add filename-specific, exact-source-text, or exact-generated-symbol
  handling for `src/20000622-1.c`.
- Do not perform broad RV64 lowering rewrites before the abort is classified.
- Do not weaken or remove the 572 same-module call/result coverage.

## Working Model

- The 572 rerun moved `src/20000622-1.c` past the old ordinary same-module
  `CallInst` fallback.
- The remaining visible failure is
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, with `clang_exit=0` and
  `c4c_exit=Subprocess aborted`.
- The first task is to determine whether the abort comes from a newly exposed
  lowering family, bad value publication after calls, comparison/control-flow
  semantics, stack/value materialization, or another post-call object-route
  behavior.
- Any repair must address the semantic family, not the representative filename.

## Execution Rules

- Keep `todo.md` as the mutable packet state.
- Preserve source-idea stability; use `todo.md` for classification notes and
  proof details unless the runbook itself must change.
- Capture compact artifacts for each reproduction/classification pass under a
  577-specific `build/agent_state/` directory.
- Add focused coverage before semantic lowering changes whenever the local
  harness can express the classified family.
- Keep validation ladder visible: build, focused backend/runtime test, then the
  representative object-route rerun.
- Treat any expectation rewrite, unsupported-marker change, allowlist edit,
  runtime-comparison contract change, or filename-specific shortcut as route
  drift.

## Steps

### Step 1: Reproduce And Classify The Runtime Abort

Goal: Reproduce the `src/20000622-1.c` RV64 object-route abort and capture the
first observable semantic mismatch after same-module call lowering succeeds.

Actions:
- Rerun the existing gcc_torture object-route command for `src/20000622-1.c`.
- Save logs, command lines, exit codes, and compact summaries under a
  577-specific `build/agent_state/` directory.
- Compare the new artifacts with the 572 summary and case log.
- Identify the earliest point where c4c behavior diverges from the clang
  reference after ordinary call lowering.
- Record whether the abort appears tied to call argument/result publication or
  to a later independent lowering/runtime family.

Completion Check:
- `todo.md` records the reproduction command, artifact paths, exit-code
  comparison, and a concrete first-family classification or a precise blocker
  explaining what evidence is still missing.

### Step 2: Add Focused Coverage For The Classified Family

Goal: Prove the identified semantic family with a focused backend or runtime
case that does not depend on the `src/20000622-1.c` filename.

Actions:
- Choose the narrowest existing harness that can express the classified
  family: focused RV64 object-emission test, runtime test, or a minimal
  representative rerun case.
- Add or extend coverage so it fails for the classified behavior before the
  implementation change when feasible.
- Keep the case shaped around the semantic family, not exact source text or
  generated symbol names from `src/20000622-1.c`.
- Preserve existing 572 same-module call/result tests.

Completion Check:
- Focused coverage exists for the classified family, or `todo.md` records why
  the available harness cannot express the pre-fix failure and what
  representative proof will substitute.

### Step 3: Repair The Underlying RV64 Object-Route Semantics

Goal: Implement the smallest semantic repair for the classified family while
preserving same-module call emission and result publication behavior.

Actions:
- Use `c4c-clang-tools` before broad C++ exploration of backend symbols.
- Modify only the backend/runtime area needed by the classified family.
- Keep unsupported ABI forms fail-closed on their existing precise diagnostic
  surfaces.
- Avoid broad rewrites of unrelated RV64 lowering families.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison behavior, or the gcc_torture runner.

Completion Check:
- `cmake --build --preset default` succeeds.
- The focused test for the classified family passes, or any remaining failure
  is recorded in `todo.md` with the exact blocker.

### Step 4: Rerun The Representative And Preserve 572 Evidence

Goal: Show that `src/20000622-1.c` no longer aborts for the same classified
reason, or that it has advanced to a newly classified later family.

Actions:
- Rerun the RV64 gcc_torture object route for `src/20000622-1.c`.
- Save updated artifacts under the 577-specific `build/agent_state/` directory.
- Confirm the old ordinary same-module `CallInst` fallback does not reappear.
- Run representative 572 same-module call/result focused tests or the
  supervisor-selected subset that protects the parent repair.
- Classify any remaining failure as fixed, advanced to a later family, or still
  blocked by the same family.

Completion Check:
- `todo.md` records the representative result, artifact paths, parent 572 proof
  state, and whether the source idea is ready for close evaluation or needs a
  follow-up packet.

### Step 5: Review And Close Readiness

Goal: Decide whether the source idea is satisfied by classification evidence,
focused coverage, semantic repair, and representative rerun proof.

Actions:
- Confirm the source acceptance criteria are satisfied.
- Confirm the repair is semantic and not representative-specific.
- Confirm no expectation, unsupported-marker, allowlist, runtime-comparison
  contract, or gcc_torture runner changes were used as proof.
- Confirm existing same-module call/result behavior remains intact.
- Record close readiness and recommended close-gate validation in `todo.md`.

Completion Check:
- `todo.md` records close readiness for plan-owner evaluation, including the
  focused proof, representative artifact path, and any known leftover families.
