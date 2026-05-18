# AArch64 Dynamic Stack Lowering Plan

Status: Active
Source Idea: ideas/open/280_aarch64_dynamic_stack_lowering.md

## Purpose

Implement real AArch64 lowering for prepared dynamic-stack operations that
currently fail with the truthful diagnostic:
`AArch64 dynamic-stack helper lowering is not implemented`.

## Goal

Make supported prepared stack-save, dynamic-allocation, and stack-restore
operations lower to valid AArch64 machine output while preserving fail-closed
diagnostics for unsupported forms.

## Core Rule

Lower the prepared dynamic-stack operation family semantically. Do not restore
unresolved LLVM helper calls, match only `00207.c`, or weaken diagnostics,
allowlists, expected output, unsupported classifications, or stage accounting.

## Read First

- `ideas/open/280_aarch64_dynamic_stack_lowering.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Existing dynamic-stack preparation and plan records found from the Step 1
  trace.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Scope

- AArch64 backend lowering for prepared stack save, dynamic allocation, and
  stack restore.
- Focused backend tests that model prepared dynamic-stack operations directly.
- AArch64 c-testsuite backend proof for `00207.c` and a broader
  `aarch64_backend` scan after repair.

## Non-Goals

- Do not configure or require an AArch64 runtime runner.
- Do not claim `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not reopen completed call-boundary move, `.LBB...` label, or helper-symbol
  authority work unless the exact old diagnostics return.
- Do not make broad frontend, ABI, scalar-printer, or unrelated backend rewrites
  unless tracing proves prepared dynamic-stack facts are insufficient before
  AArch64 lowering.
- Do not emit unresolved `llvm.stacksave`, `llvm.dynamic_alloca.*`, or
  `llvm.stackrestore` references in generated AArch64 output.

## Working Model

Idea 279 already prevents LLVM stack helpers from leaking as external symbols.
The remaining capability gap is that prepared dynamic-stack operations reach
AArch64 backend lowering and are rejected before machine output. The repair
should consume the prepared records and emit target-machine behavior for the
supported forms.

Unsupported dynamic-stack records must still fail before machine output with a
specific owner-layer diagnostic. A truthfully unsupported form is better than
invalid assembly, missing stack restoration, or unresolved helper symbols.

## Execution Rules

- Keep each packet bounded to one trace, proof, or implementation step.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets.
- For code-changing packets, run the narrow backend proof before the relevant
  AArch64 c-testsuite backend subset.
- Escalate to broader `-L aarch64_backend` only after focused backend and
  `00207.c` proof show the owned failure mode has moved.

## Steps

### Step 1: Reproduce And Trace Dynamic-Stack Records

Goal: identify the exact prepared dynamic-stack operation records consumed by
AArch64 lowering.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- dynamic-stack preparation code discovered by symbol search
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00207.c.s`, if produced

Actions:

- Reproduce `c_testsuite_aarch64_backend_src_00207_c`.
- Confirm the current failure is the dynamic-stack unsupported diagnostic, not
  unresolved LLVM helper references.
- Trace where stack save, dynamic allocation, and stack restore are represented
  before AArch64 lowering.
- Record the concrete prepared record fields that the implementation can rely
  on.

Completion check:

- `todo.md` names the owner layer, relevant files/functions, prepared record
  fields, and the exact focused c-testsuite proof command for `00207.c`.

### Step 2: Add Focused Backend Proof For Supported Forms

Goal: create or extend a backend test that models the prepared dynamic-stack
operation family independently of `00207.c`.

Primary target:

- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:

- Add a focused test covering prepared stack save, dynamic allocation, and stack
  restore in one representative function.
- Assert that generated machine output contains target-owned AArch64 behavior
  and contains no unresolved LLVM helper call references.
- Preserve or add an assertion that unsupported dynamic-stack forms fail before
  machine output with a specific diagnostic.

Completion check:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  fails only on the new old-path proof, with the expected dynamic-stack
  diagnostic or helper-leak assertion.

### Step 3: Implement Supported AArch64 Dynamic-Stack Lowering

Goal: lower supported prepared dynamic-stack operations into valid AArch64
machine output.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- adjacent AArch64 codegen helpers if Step 1 proves a narrower owner file
- existing prologue/stack helpers only if the prepared records require them

Actions:

- Implement stack-save lowering for the supported prepared representation.
- Implement dynamic allocation lowering for the supported prepared
  representation, preserving stack alignment requirements.
- Implement stack-restore lowering for the supported prepared representation.
- Keep unsupported forms fail-closed before machine output with a clear
  owner-layer diagnostic.
- Avoid filename-specific or helper-spelling shortcuts.

Completion check:

- `^backend_` passes.
- The focused backend proof from Step 2 passes without unresolved LLVM helper
  references.

### Step 4: Prove `00207.c` Advances Past The Owned Failure

Goal: show that the representative c-testsuite case no longer fails at AArch64
dynamic-stack helper lowering.

Actions:

- Run:
  `ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'`
- Inspect the generated assembly or proof log for absence of
  `llvm.stacksave`, `llvm.dynamic_alloca.*`, and `llvm.stackrestore`.
- Classify the next failure layer truthfully.

Completion check:

- `00207.c` advances beyond the current dynamic-stack unsupported diagnostic.
- No unresolved LLVM stack helper references appear in generated AArch64 output
  or link input.
- Any remaining failure is recorded in `todo.md` and `[RUNTIME_UNAVAILABLE]` is
  not counted as pass evidence.

### Step 5: Broader AArch64 Backend Scan

Goal: verify the repair did not reintroduce prior helper-symbol leakage and
identify any remaining owner layers.

Actions:

- Run the configured AArch64 backend scan:
  `ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend`
- Group failures by stage and record counts in `todo.md`.
- Confirm there are zero unresolved `llvm.stacksave`,
  `llvm.dynamic_alloca.*`, or `llvm.stackrestore` references.

Completion check:

- Broad scan evidence is recorded with stage counts.
- Dynamic-stack helper leakage is absent.
- Runtime-unavailable cases remain blockers for runtime proof, not pass
  evidence.

### Step 6: Completion Review

Goal: decide whether idea `280` is complete under its source acceptance
criteria.

Actions:

- Compare the implementation and proof evidence against
  `ideas/open/280_aarch64_dynamic_stack_lowering.md`.
- Confirm focused backend proof, `00207.c` owner-layer movement, and broad scan
  helper-reference absence.
- If complete, request plan-owner closure review.
- If a distinct new failure family appears, record it as a follow-up idea
  instead of expanding this runbook.

Completion check:

- `todo.md` clearly says whether the source idea is ready for closure review or
  names the exact blocker/follow-up owner layer.
