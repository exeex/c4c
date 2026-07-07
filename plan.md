# RV64 Pointer Arithmetic Lowering Runbook

Status: Active
Source Idea: ideas/open/575_rv64_pointer_arithmetic_lowering.md

## Purpose

Repair RV64 object-route handling for pointer-valued BIR binary arithmetic where
a pointer base is combined with a scaled byte offset and the result must be
published as a pointer owner.

## Goal

Make the observed loaded-base plus scaled-offset pointer add shape either lower
successfully with correct pointer owner publication or fail closed with a
specific pointer-arithmetic diagnostic instead of the generic
`unsupported_instruction_fragment`.

## Core Rule

Implement semantic pointer arithmetic lowering, not filename-specific handling
for `src/20000819-1.c` or exact-value matching for `%t4`.

## Read First

- `ideas/open/575_rv64_pointer_arithmetic_lowering.md`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/object-route.log`

## Current Targets

- RV64 object emission for pointer-valued binary add/subtract forms.
- Pointer base plus scaled integer offset address arithmetic.
- Publication of the resulting pointer owner for later memory use.
- Focused diagnostics for unsupported pointer arithmetic forms.

## Non-Goals

- Do not perform a broad integer ALU rewrite.
- Do not mix in same-module calls, inline asm carriers, select lowering, FP
  binary lowering, runtime comparison, or unrelated instruction-fragment work.
- Do not rewrite BIR producers unless focused evidence proves the pointer
  arithmetic representation itself is semantically wrong.
- Do not change expected outputs, unsupported markers, allowlists, or runtime
  comparison behavior as proof of progress.

## Working Model

The failing representative reaches RV64 object emission with a pointer-valued
`BinaryInst` owner. The object route needs either a real lowering path for
pointer address arithmetic or a narrower fail-closed classification that names
pointer arithmetic and the unsupported operand shape.

## Execution Rules

- Keep each packet scoped to one owner boundary: classification, focused test,
  lowering, representative route, then broader proof.
- Add or adjust focused backend tests before relying on the GCC torture
  representative as the only proof.
- Preserve existing generic unsupported diagnostics for unrelated instruction
  families.
- Treat expectation rewrites and named-case shortcuts as route drift.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  separate artifact.

## Ordered Steps

### Step 1: Reproduce And Classify Pointer Arithmetic Shape

Goal: Confirm the current first unsupported owner and operand shape for the
pointer-valued binary arithmetic representative.

Primary target:
- `src/20000819-1.c` RV64 object route and prepared-BIR evidence.

Actions:
- Inspect the existing 570 evidence listed above.
- Rerun the representative route if current behavior needs confirmation.
- Record the first owner, operation, operand types, and current diagnostic in
  `todo.md`.
- Decide whether the BIR/prepared-BIR representation is already sufficient for
  RV64 object lowering.

Completion check:
- `todo.md` names the first semantic owner and whether Step 2 should proceed at
  RV64 object emission or split to a producer-side follow-up idea.

### Step 2: Add Focused Backend Coverage

Goal: Encode pointer base plus scaled integer offset behavior in focused tests
before changing lowering.

Primary target:
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:
- Add coverage for loaded pointer base plus scaled integer offset producing a
  pointer owner.
- Add fail-closed coverage for at least one unsupported pointer arithmetic form
  if the current lowering surface cannot support it.
- Assert a specific diagnostic for unsupported pointer arithmetic instead of
  relying on generic `unsupported_instruction_fragment`.

Completion check:
- Focused tests fail for the missing capability or missing narrow diagnostic
  before implementation, then become the narrow proof target for Step 3.

### Step 3: Implement RV64 Pointer Arithmetic Lowering

Goal: Lower supported pointer-valued add/subtract forms as integer address
arithmetic while preserving pointer owner publication.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- Adjacent RV64 object-emission helpers only when needed by the existing local
  structure.

Actions:
- Locate the RV64 object-emission handling for BIR binary instructions and
  owner publication.
- Add a semantic path for supported pointer add/subtract with integer offset
  operands.
- Preserve the resulting pointer owner for later memory operations.
- Keep unsupported pointer arithmetic forms fail-closed with a specific
  diagnostic naming the operation and operand category.

Completion check:
- `cmake --build --preset default` passes.
- Focused backend test coverage from Step 2 passes.
- Unrelated unsupported instruction families retain their existing diagnostics.

### Step 4: Prove Representative Route Advancement

Goal: Show the GCC torture representative no longer fails first on the old
generic pointer arithmetic fragment.

Primary target:
- `src/20000819-1.c` RV64 object route.

Actions:
- Rerun the representative route using the repo-native RV64 object-route
  command.
- Compare the result with the 570 evidence.
- If the route still fails, classify whether it is a later distinct owner or
  the same pointer arithmetic owner.
- If a later distinct owner appears, record it in `todo.md` and recommend a
  separate follow-up idea rather than expanding this runbook.

Completion check:
- The representative route no longer reports the old generic
  `unsupported_instruction_fragment` as the first failure for the loaded-base
  plus scaled-offset pointer add shape, or it reports a narrower pointer
  arithmetic diagnostic for an unsupported form.

### Step 5: Broader Backend Proof And Closure Readiness

Goal: Validate that the pointer arithmetic change did not regress nearby RV64
object emission behavior.

Actions:
- Run the focused backend test target after the implementation packet.
- Run a broader backend subset chosen by the supervisor when the slice is ready
  for acceptance.
- Record proof commands and results in `todo.md`.

Completion check:
- Build and backend proof are green.
- `todo.md` records the focused proof, representative route result, and any
  later-owner follow-up recommendation.
- The source idea acceptance criteria can be evaluated without relying on
  expectation rewrites or unsupported-marker changes.
