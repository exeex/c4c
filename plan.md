# RV64 Floating-Point Binary Lowering Runbook

Status: Active
Source Idea: ideas/open/574_rv64_floating_point_binary_lowering.md

## Purpose

Repair the RV64 object-route lowering gap for scalar floating-point BIR binary
operations, starting with the double-precision division owner identified by the
570 unsupported-instruction diagnostics.

## Goal

Make scalar double floating-point binary operations materialize and publish
their result through RV64 object emission without falling through to the generic
unsupported instruction path.

## Core Rule

Implement semantic FP binary lowering and diagnostics; do not match
`src/20000605-1.c`, `render_image_rgb_a`, `%t5`, or any other testcase-shaped
identifier.

## Read First

- `ideas/open/574_rv64_floating_point_binary_lowering.md`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/object-route.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- RV64 object emission for scalar double BIR binary operations, beginning with
  the observed `bir.sdiv double 1.0, %t4` shape.
- Operand materialization and result publication for the FP binary owner.
- Focused backend coverage for supported double FP binary lowering and
  fail-closed unsupported FP binary forms.

## Non-Goals

- Do not implement F128 or long-double lowering.
- Do not mix in floating-point casts, truncation, comparisons, libcall policy,
  pointer arithmetic, select lowering, inline asm, or call ABI repairs unless a
  later first-owner investigation creates a separate source idea.
- Do not change expected runtime output, unsupported markers, allowlists, or
  route classification as the proof of progress.
- Do not perform broad floating-point rewrites beyond the object-emission
  support required by this source idea.

## Working Model

- The current representative first unsupported owner is a BIR `BinaryInst` with
  double operands and a double result.
- Lowering should map supported scalar FP binary operations to the appropriate
  RV64 floating-point instructions when the target features and operand types
  permit it.
- Unsupported FP binary forms should produce a narrower diagnostic or fail
  closed through the existing unsupported path rather than silently generating
  incorrect code.
- If the representative advances to a later cast, truncation, comparison, or
  runtime mismatch, record that later owner for a follow-up instead of widening
  this runbook.

## Execution Rules

- Keep each code-changing step paired with focused backend proof.
- Prefer existing RV64 object-emission helpers and publication patterns before
  adding new abstractions.
- Preserve existing integer, pointer, select, inline asm, and call behavior.
- Treat diagnostic-only edits as insufficient unless they accompany real
  semantic lowering or a deliberately narrower fail-closed path.
- Keep `todo.md` as the packet scratchpad; do not edit the source idea unless
  source intent changes or a separate follow-up must be recorded.

## Ordered Steps

### Step 1: Reproduce And Localize The FP Binary Owner

Goal: Confirm the current FP binary failure shape and the relevant object
emission entry points before implementation.

Primary target: RV64 object route for `src/20000605-1.c`.

Actions:

- Inspect the 570 evidence files named above.
- Reproduce the representative object route or an equivalent focused dump if
  the existing evidence is stale.
- Identify the object-emission function that rejects or skips the double
  `BinaryInst`.
- Record in `todo.md` the exact owner, operation, operand sources, and current
  failure path.

Completion check:

- `todo.md` names the current FP binary owner and the implementation surface
  for Step 2 without changing code.

### Step 2: Add Focused FP Binary Object-Emission Coverage

Goal: Pin the desired supported and unsupported behavior before the lowering
change.

Primary target: `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Actions:

- Add a focused test for a supported scalar double FP binary operation that
  requires operand materialization and result publication.
- Add or preserve focused fail-closed coverage for an unsupported FP binary
  form or type.
- Keep tests semantic and operation/type based, not tied to the representative
  filename or value names.

Completion check:

- The focused backend test target exposes the missing supported behavior or
  verifies the intended fail-closed diagnostic contract.

### Step 3: Implement Double FP Binary Lowering

Goal: Emit RV64 object code for the supported scalar double FP binary shape and
publish the result for later consumers.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`.

Actions:

- Reuse existing scalar/FPR materialization and publication helpers where
  possible.
- Lower supported double FP binary operations through RV64 floating-point
  instructions with correct operand registers and result ownership.
- Keep unsupported operations, unsupported types, and unmaterializable operands
  fail-closed.
- Avoid changing unrelated integer, pointer, select, call, or inline asm paths.

Completion check:

- Focused backend object-emission tests pass and the implementation does not
  rely on testcase-shaped identifiers.

### Step 4: Prove The Representative Route

Goal: Verify the original 570 representative no longer fails first on the
generic FP binary owner.

Primary target: RV64 object route for `src/20000605-1.c`.

Actions:

- Rerun the representative object route and save the log under
  `build/agent_state/574_rv64_floating_point_binary_lowering/`.
- Compare the new result with the 570 object-route evidence.
- If the route advances to a later cast, truncation, comparison, or runtime
  mismatch, record that as a distinct later owner in `todo.md`.

Completion check:

- The old generic unsupported FP binary fallback is gone for the representative
  shape, or the failure is narrowed to a specific unsupported FP binary
  operation/type diagnostic.

### Step 5: Backend Guard And Closure Readiness

Goal: Establish that the FP binary slice is acceptance-ready without
regressing nearby backend coverage.

Primary target: backend CTest subset.

Actions:

- Run `cmake --build --preset default`.
- Run the focused backend object-emission test.
- Run the supervisor-selected broader backend subset, expected to be at least
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` before
  closure.
- Write proof results to `test_after.log` when delegated by the supervisor or
  executor protocol.

Completion check:

- Focused and broader backend proof is green, `todo.md` records any follow-up
  owner, and the source idea acceptance criteria can be evaluated for closure.
