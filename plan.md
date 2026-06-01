# AArch64 Local Scalar Register Helper Fold-Back Runbook

Status: Active
Source Idea: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md

## Purpose

Fold repeated AArch64-local scalar type, register-view, and compare predicate
helper logic into one narrow target-local helper surface.

## Goal

Remove real duplicate local helper logic while preserving AArch64 record
construction, opcode selection, condition-code spelling, and printer text
formatting.

## Core Rule

Keep the helper boundary AArch64-local. Do not move target instruction
selection, register-view spelling, condition-code spelling, record schemas, or
printer formatting into shared BIR, prealloc, or ABI authority.

## Read First

- `ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

## Current Scope

- Inventory duplicate scalar type, register-view, and compare predicate helper
  logic in the owned AArch64 codegen files.
- Extract only true duplicate helper logic into a focused local helper surface.
- Use BIR type facts and `abi::convert_prepared_register` as inputs instead of
  re-deriving shared semantics.
- Preserve the existing behavior of scalar ALU, comparison, instruction record
  construction, and machine-printer output.

## Non-Goals

- Do not change shared BIR, prealloc, or ABI ownership.
- Do not split `instruction.hpp` by schema family unless duplicate helper
  removal makes that locally necessary.
- Do not rewrite printer formatting, instruction record schemas, call lowering,
  memory lowering, special-carrier paths, or variadic cleanup.
- Do not claim progress through helper renames that leave the same duplication
  in place.

## Working Model

- Treat AArch64 scalar helper duplication as the problem; capability semantics
  must remain unchanged.
- Prefer one local helper file only if it makes ownership clearer than placing
  helpers beside existing instruction construction code.
- Keep target-local spelling and schema decisions at their existing owners.

## Execution Rules

- Start each code-changing step with a focused inventory of the exact duplicate
  helpers being removed.
- Keep patches small enough to prove with targeted build/test commands before
  moving to the next helper family.
- If a proposed helper starts owning shared semantics or printer spelling, stop
  and narrow the boundary.
- Record the before/after duplicate-helper inventory in `todo.md`; do not edit
  the source idea unless source intent changes.
- For acceptance-sized code slices, run regression guard logs before declaring
  the slice ready.

## Ordered Steps

### Step 1: Inventory Duplicate Scalar Helper Logic

Goal: identify the exact duplicate scalar type, register-view, and compare
predicate helper logic across the owned AArch64 files.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:
- Inspect helper functions and local repeated logic related to scalar BIR
  types, prepared-register views, and comparison predicates.
- Separate true duplicated semantic helper logic from target-local opcode,
  record-schema, condition-code, and printer-format decisions.
- Choose the first helper family to fold back and define the local helper
  boundary for that family.

Completion check:
- `todo.md` names the duplicate helpers found, the helper family selected for
  the first fold-back, and any nearby logic intentionally left in place.
- No implementation files need to change for this step unless the inventory
  exposes an obvious zero-risk local cleanup.

### Step 2: Fold Scalar Type And Register-View Helpers

Goal: unify duplicated scalar type and prepared-register view helper logic
behind one AArch64-local helper boundary.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- any new AArch64-local helper file created beside these owners

Actions:
- Extract duplicated scalar type/register-view decisions into the chosen local
  helper surface.
- Pass BIR type facts and `abi::convert_prepared_register` outputs into the
  helper instead of recomputing shared facts.
- Update call sites incrementally, preserving record construction and printed
  assembly output.

Completion check:
- The selected duplicate type/register-view helpers are removed or unified.
- Focused build proof passes.
- Targeted scalar ALU, instruction construction, and machine-printer tests pass
  for the touched helper family.
- `todo.md` records the before/after helper inventory and proof command.

### Step 3: Fold Compare Predicate Helper Logic

Goal: unify duplicated compare predicate helper logic without changing compare
semantics or condition-code spelling.

Primary targets:
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- Extract only repeated compare predicate mapping or validation logic that is
  genuinely duplicated.
- Keep opcode selection, condition-code spelling, and printer formatting in
  target-local owners.
- Prove nearby signed, unsigned, equality, and floating-point comparison paths
  if they are touched.

Completion check:
- Duplicate compare predicate helper logic is removed or unified.
- Focused build proof passes.
- Targeted comparison and machine-printer tests pass for the affected record
  families.
- `todo.md` records the before/after helper inventory and proof command.

### Step 4: Acceptance Review And Regression Guard

Goal: prove the fold-back is behavior-preserving and did not become a shared
semantic rewrite or testcase-shaped cleanup.

Actions:
- Review the final diff against the source idea reject signals.
- Confirm no shared BIR, prealloc, or ABI surface owns AArch64 register-view,
  condition-code, record-schema, or printer-spelling details.
- Confirm no printed assembly, record construction, or compare semantics changed
  without targeted proof.
- Run the supervisor-selected regression guard command for the acceptance slice.

Completion check:
- `test_before.log` and `test_after.log` cover the accepted regression-guard
  scope when the supervisor requests acceptance validation.
- `todo.md` contains the final duplicate-helper inventory and proof summary.
- The source idea can be reviewed for closure or follow-up split.
