# LIR Module Declaration Type Shadow Convergence Runbook

Status: Active
Source Idea: ideas/open/762_lir_module_declaration_type_shadow_convergence.md

## Purpose

Retire module-level rendered type text as semantic authority only where an
accepted structured carrier already exists, preserving output compatibility.

## Goal

Make structured IDs and type references authoritative for extern declarations,
function signatures, globals, and struct declarations without broad type-model
or backend rewrites.

## Core Rule

Do not recover or choose module identity from rendered declaration/type text.
Legacy strings may remain only as documented compatibility or emission shadows
after an equivalent structured source is checked.

## Read First

- `ideas/open/762_lir_module_declaration_type_shadow_convergence.md`
- `ideas/closed/759_lir_typed_ref_enum_foundation.md`
- `ideas/closed/760_lir_string_constructor_deprecation_migration.md`
- `ideas/closed/761_lir_call_signature_type_mirror_convergence.md`
- `ideas/closed/763_lir_composite_type_ref_model.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and
  `src/codegen/lir/print.cpp`

## Non-Goals

- Full type-tree replacement, wholesale `TypeSpec` lowering, or broad printer
  rewrites.
- Raw-BIR, target lowering, MIR, global-initializer semantic expansion, data
  layout, inline assembly, or template-specialization work.
- Removing a compatibility shadow before all selected producer/consumer routes
  have an equivalent structured replacement.

## Execution Rules

- Select and implement one module-level authority surface at a time.
- Preserve final LLVM output; test stale or misleading shadow text as a
  negative authority case.
- Fail closed when the selected route lacks an exact structured replacement.
- Run a fresh build and nearby LIR/frontend/backend proof for each code step;
  the supervisor selects broader/full checkpoints as shared paths require.

## Steps

### Step 1 - Audit and select one module declaration authority surface — current

Goal: inspect extern declarations, function signatures, globals, and struct
declarations against the accepted 759/760/761/763 structured carriers; select
exactly one surface whose producer, verifier, printer, and consumer seam is
complete.

Actions:

- Trace the selected surface from module recording through verification,
  printing, and backend consumption.
- Record the structured authority fields, legacy shadow fields, stale-shadow
  positive/negative matrix, and excluded surfaces in `todo.md`.
- If no selected surface has an equivalent structured carrier, classify the
  missing carrier and route it as a separate blocker rather than parsing text.

Completion check: one bounded authority contract is selected with an exact
same-feature proof ladder, or a separately scoped blocker preserves this Step
1 return point.

### Step 2 - Implement and prove the selected module declaration surface

Goal: make only Step 1's selected structured fields authoritative and retain
the legacy text solely as a checked compatibility/emission shadow where needed.

Completion check: fresh build, nearby positive and malformed stale-shadow
coverage, matching selected tests, and a supervisor-selected broader checkpoint
prove the row without changing output compatibility or weakening contracts.

### Step 3 - Reassess remaining module-level surfaces

Goal: decide whether the other extern/signature/global/struct surfaces require
another in-scope one-row route or whether the source acceptance criteria are
complete.

Completion check: close only with all source criteria and accepted proof;
otherwise repair this runbook for the next bounded surface or create/switch to
an out-of-scope blocker with an exact return point.
