# RV64 Same-Module Call Result Lowering Runbook

Status: Active
Source Idea: ideas/open/572_rv64_same_module_call_result_lowering.md

## Purpose

Repair RV64 object-route handling for ordinary same-module BIR `CallInst`
nodes with GPR arguments and integer or pointer-sized results.

## Goal

Lower same-module calls through the RV64 object route and publish integer/GPR
call results back into the prepared value environment so later users observe
the produced value.

## Core Rule

This plan is for ordinary same-module call/result lowering only. Do not treat
`llvm.inline_asm` carriers as ordinary calls, do not mix in select, floating-
point binary, pointer arithmetic, prepared-authority, or runtime-comparison
work, and do not claim progress through expectation, unsupported-marker,
allowlist, or representative-specific edits.

## Read First

- `ideas/open/572_rv64_same_module_call_result_lowering.md`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/summary.md`
- per-case logs under
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`
- `.codex/skills/c4c-clang-tools/SKILL.md` before broad C++ exploration

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- nearby RV64 operand/materialization helpers used by object emission
- value-environment publication helpers used by RV64 object emission
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- representative external rows:
  - `src/20000412-2.c`
  - `src/20000622-1.c`

## Non-Goals

- Do not change `llvm.inline_asm` carrier handling.
- Do not implement floating-point argument/result ABI, varargs, aggregate
  returns, tail calls, or broad external linkage policy.
- Do not repair select, pointer arithmetic, floating-point binary lowering,
  prepared authority, or runtime comparison behavior.
- Do not edit runtime comparison files, expected outputs, unsupported markers,
  or allowlists.
- Do not add filename-specific or exact-source-shape matching for
  `src/20000412-2.c` or `src/20000622-1.c`.

## Working Model

- The 570 diagnostics classified two retained representatives as failing first
  on ordinary same-module `CallInst` nodes with value owners:
  - `src/20000412-2.c`: `function=main`, `owner=i32 %t0`, callee `f`, and a
    GPR result.
  - `src/20000622-1.c`: `function=baz`, `owner=i64 %t4`, callee `foo`, three
    GPR arguments, and a prior call result.
- These are not inline asm carriers. The object route needs a semantic call
  emission path and result publication, not a diagnostic relabeling.
- Unsupported call ABI forms should fail closed with a narrower call-specific
  diagnostic rather than the generic `unsupported_instruction_fragment`
  fallback.

## Execution Rules

- Keep `todo.md` as the mutable packet state.
- Use focused object-emission tests as the primary proof for argument passing,
  call emission, and result publication.
- Use the two representative RV64 gcc_torture object-route reruns as external
  evidence after focused backend tests pass.
- Preserve fail-closed behavior for unsupported ABI forms.
- Keep changes semantic and call-oriented; reject named-case shortcuts,
  expectation rewrites, and unsupported-marker changes as progress.

## Steps

### Step 1: Inspect Same-Module Call Representation

Goal: Identify the exact BIR, prepared-BIR, and RV64 object-emission
representation for ordinary same-module calls with GPR arguments and integer
results.

Actions:
- Use `c4c-clang-tools` first for symbol and caller/callee queries around RV64
  call emission, `CallInst` handling, argument materialization, and value
  environment publication.
- Inspect the 570 classification and per-case logs for `src/20000412-2.c` and
  `src/20000622-1.c`.
- Record the BIR callee shape, argument/value ownership, result owner type,
  existing prepared value bindings, and the first object-emission branch where
  ordinary calls should be handled before the generic fallback.
- Identify unsupported call ABI forms that must remain diagnostic-only.

Completion Check:
- `todo.md` records the representation inventory, first implementation target,
  value-publication path, and unsupported call forms that should remain
  fail-closed.

### Step 2: Add Focused Same-Module Call Tests

Goal: Establish focused backend tests for ordinary same-module GPR calls and
integer result publication before changing lowering behavior.

Actions:
- Add or extend object-emission tests for a same-module call with immediate or
  null/pointer-sized GPR arguments and an integer result owner.
- Add or extend a test for multiple GPR arguments and a later use of a prior
  call result.
- Assert that supported ordinary calls do not reach the generic
  `unsupported_instruction_fragment` fallback.
- Add a negative focused case for an unsupported ABI form if the local harness
  can express it, and assert a narrower call-specific diagnostic.

Completion Check:
- The new or updated tests fail for the expected call-lowering/result-
  publication reason before the implementation change, or `todo.md` records
  why the existing test harness cannot express that precondition.

### Step 3: Implement RV64 Same-Module Call Emission

Goal: Emit supported same-module calls with GPR arguments through the RV64
object route without broad ABI rewrites.

Actions:
- Add the smallest RV64 object-emission path needed to recognize ordinary
  same-module `CallInst` nodes.
- Materialize supported integer and pointer-sized GPR arguments according to
  the local RV64 object-emission model.
- Emit the call to the same-module target symbol using the existing object
  emission abstractions.
- Route unsupported ABI forms such as floating-point arguments/results,
  varargs, aggregate returns, or unmodeled linkage through a precise
  call-specific diagnostic.
- Avoid changing inline asm handling, select lowering, pointer arithmetic,
  floating-point binary lowering, or runtime comparison behavior.

Completion Check:
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
  passes or any failure is recorded as a blocker in `todo.md`.

### Step 4: Publish Integer/GPR Call Results

Goal: Store supported call results under the `CallInst` owner value so later
instructions can consume the result from the prepared value environment.

Actions:
- Identify the local value-publication helper or pattern used by other
  result-producing RV64 object-emission paths.
- Capture the integer/GPR return value after the emitted call.
- Publish that value under the call instruction owner with the correct integer
  or pointer-sized type information.
- Add or update focused tests so a later user proves the result was published,
  not just that the call instruction emitted.
- Keep unsupported or missing-owner result forms on call-specific diagnostics.

Completion Check:
- Focused backend tests prove both call emission and later use of the integer
  call result.
- `todo.md` records the exact publication path and any still-unsupported call
  result forms.

### Step 5: Rerun The Two Same-Module Call Representatives

Goal: Produce external evidence that the same-module call family no longer
falls into the generic unsupported-instruction fallback for absent call/result
lowering.

Actions:
- Rerun the RV64 gcc_torture object route for:
  - `src/20000412-2.c`
  - `src/20000622-1.c`
- Save compact logs or summaries under a new
  `build/agent_state/572_rv64_same_module_call_result_lowering/` artifact
  directory.
- Classify each representative as lowered, still unsupported with a narrower
  call-specific diagnostic, or blocked by a later non-call owner family.
- Do not edit testcase expectations, unsupported markers, allowlists, or
  runtime comparison files as part of the proof.

Completion Check:
- The artifact directory records both reruns and proves neither representative
  still fails first through the old generic `unsupported_instruction_fragment`
  ordinary-call path.

### Step 6: Review And Close Readiness

Goal: Decide whether the source idea is satisfied by call lowering, focused
tests, and representative evidence.

Actions:
- Confirm the source acceptance criteria are satisfied.
- Confirm focused tests cover GPR argument passing and integer result
  publication for ordinary same-module calls.
- Confirm unsupported call forms retain precise call-specific diagnostics.
- Confirm no inline asm, select, floating-point binary, pointer arithmetic,
  expectation, unsupported-marker, allowlist, or runtime comparison file
  changed.
- Record close readiness and proof state in `todo.md`.

Completion Check:
- `todo.md` records close readiness for plan-owner evaluation and the close
  gate has a clear backend CTest plus representative-rerun validation path.
