# LIR/BIR Vararg and va_arg Contract Completion Runbook

Status: Active
Source Idea: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md

## Purpose

Complete the shared LIR/BIR/prepared contract for variadic functions,
vararg entry state, and `va_arg` consumption before any target object emitter
tries to lower those shapes directly.

## Goal

Define, implement, and test a target-independent vararg contract that gives
target backends explicit vararg entry and `va_arg` plans or precise unsupported
diagnostics.

## Core Rule

Do not implement vararg semantics directly in RV64 object emission as a
shortcut. Target code may only consume explicit prepared vararg/value-home
information or report a structured unsupported diagnostic.

## Read First

- `ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md`
- frontend representation for variadic function signatures and `va_*`
  builtins
- LIR, BIR, and prepared-module representations for calls, arguments,
  locals, value homes, and unsupported diagnostics
- representative tests around `src/920908-1.c` and `src/20030914-2.c`

## Current Scope

- Audit how variadic identity and `va_*` operations move through frontend,
  LIR, BIR, and prepared representations.
- Define the shared contract for vararg entry state and `va_arg` consumption.
- Add focused tests for the shared contract and diagnostic path.
- Leave RV64-specific ABI materialization to a later target hook slice unless
  the shared contract requires a narrow hook boundary.

## Non-Goals

- Do not complete RV64-specific vararg object emission before the shared
  contract exists.
- Do not improve GCC torture scan counts by expectation downgrades, skips, or
  testcase-name filtering.
- Do not broaden into globals/data sections, FPR ABI expansion, vector ABI, or
  unrelated instruction coverage.
- Do not hide `va_arg` behind an opaque blob with no target ABI hook boundary.

## Working Model

The shared pipeline should expose enough information for target backends to
answer these questions without re-inspecting source or guessing ABI state:

- which functions are variadic;
- where fixed argument homes end and variable argument state begins;
- how `va_start`, `va_arg`, `va_copy`, and `va_end` are represented;
- which layer owns argument-save-area and overflow-area layout;
- what value-home or target hook plan each `va_arg` consumes;
- how unsupported argument classes or aggregate varargs are diagnosed.

## Execution Rules

- Keep changes semantic and layer-correct; no named torture-case shortcuts.
- Prefer focused contract tests before relying on full RV64 gcc torture scans.
- When adding unsupported handling, make diagnostics precise enough to
  distinguish missing shared vararg contract from missing target ABI emission.
- Keep each implementation packet small enough for build proof plus the
  relevant narrow tests.
- Escalate to broader validation after shared representation changes land
  across more than one compiler layer.

## Steps

### Step 1. Audit Current Vararg Representation

Goal: map how variadic functions and `va_*` operations are currently
represented from frontend through prepared output.

Primary targets:

- frontend parse/sema lowering for variadic signatures and `va_*` forms
- LIR call/function/argument representation
- BIR function entry, locals, value homes, calls, and unsupported diagnostics
- prepared-module emission and object-consumer admission checks

Concrete actions:

- Inspect existing code and tests for variadic function signatures,
  `va_start`, `va_arg`, `va_copy`, `va_end`, and `va_list`.
- Run narrow probes for existing representative tests or minimal cases that
  isolate each operation.
- Record the first missing or ambiguous contract boundary in `todo.md`.

Completion check:

- The next executor can name the current representation for variadic identity,
  fixed argument homes, vararg state, each `va_*` operation, and the first
  layer that loses required information.

### Step 2. Define the Shared Vararg Contract

Goal: introduce or document the target-independent contract that prepared
consumers should receive for vararg entry and `va_arg` consumption.

Concrete actions:

- Define the data carried for variadic function entry, fixed argument homes,
  variable argument state, and unsupported argument classes.
- Define the target ABI hook boundary for materializing or diagnosing each
  `va_arg`.
- Keep unsupported diagnostics structured and layer-specific.
- Add focused tests that assert the contract shape without requiring RV64
  object emission to guess source semantics.

Completion check:

- Tests prove that prepared consumers receive explicit vararg plans or precise
  diagnostics, and no RV64 object-emission code is needed to infer shared
  vararg semantics.

### Step 3. Wire Contract Through LIR, BIR, and Prepared Output

Goal: carry the contract through the compiler layers that currently own
function entry, calls, locals, and value-home planning.

Concrete actions:

- Implement the minimal representation and lowering changes needed for the
  contract from Step 2.
- Preserve existing non-vararg behavior.
- Add regression tests for fixed arguments, simple variadic calls, `va_start`,
  `va_arg`, `va_copy`, `va_end`, and unsupported vararg forms where supported
  by the current frontend.

Completion check:

- Narrow compiler tests pass for the touched frontend/LIR/BIR/prepared
  surfaces, and representative vararg cases progress to either a target ABI
  hook requirement or a precise upstream diagnostic.

### Step 4. Confirm RV64 Boundary Without Target-Layer Overfit

Goal: verify that RV64 object emission consumes only explicit prepared vararg
information or reports a structured target-hook requirement.

Concrete actions:

- Probe `src/920908-1.c` and `src/20030914-2.c` through the RV64 gcc torture
  backend runner or narrower equivalent.
- Ensure any remaining failure is classified as a shared-contract diagnostic,
  a target ABI hook requirement, or a separate unsupported aggregate/argument
  class.
- Do not add testcase-specific RV64 lowering.

Completion check:

- The representative cases no longer fail as generic prepared-object shape
  rejections for missing shared vararg semantics, or the residual diagnostic
  precisely names the target hook/unsupported class still needed.

### Step 5. Milestone Validation and Handoff

Goal: prove the shared vararg contract is stable enough for later target ABI
  emission work.

Concrete actions:

- Run the narrow build and test subset covering every touched layer.
- Run the representative RV64 torture allowlist subset for:
  - `src/920908-1.c`
  - `src/20030914-2.c`
- Document any remaining target-specific ABI work in `todo.md`; create a new
  open idea only if the remaining work is a distinct initiative.

Completion check:

- Baseline tests for touched layers are green, representative cases have
  structured outcomes, and follow-up target work is clearly separated from the
  completed shared contract.
