# LIR Composite Type Reference Model Runbook

Status: Active
Source Idea: ideas/open/763_lir_composite_type_ref_model.md
Activated from: 734 post-Step 7.32 receiver exhaustion; 763 is the next
dependency-ordered first owner in the remaining-coverage queue.

## Purpose

Replace string-primary handling for the selected composite LIR type forms with
a structured `LirTypeRef` representation while preserving LLVM emission text
and existing ABI/layout behavior.

## Core Rule

Store supported composite type semantics as ids, enums, and typed nodes. Render
only at output boundaries. Never recover composite semantics by reparsing
`str()` or other rendered LLVM text.

## Read First

- `ideas/open/763_lir_composite_type_ref_model.md`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/print.cpp`
- `src/codegen/lir/verify.cpp`

## Non-Goals

- Remove `runtime_text`, migrate every type-shadow surface, or rewrite HIR
  type lowering.
- Change LLVM output, ABI layout, verifier strictness, Raw-BIR semantics,
  target lowering, or MIR behavior.
- Absorb call/signature or module/global convergence owned by 761 and 762.

## Ordered Steps

### Step 1 - Establish structured composite representation

Goal: extend `LirTypeRef` with queryable array and named struct/union composite
structure while retaining builtin and integer-width forms.

Actions:

- inspect current type storage, construction, equality, and query seams;
- add typed kind/field representation and factories needed for array and named
  struct/union references, designed to admit later vector, aggregate, packed,
  and function forms without string-primary storage;
- retain `runtime_text` as an explicit compatibility escape hatch only.

Completion check: supported composite semantic fields are available without
parsing a rendered type string and existing builtin behavior is preserved.

### Step 2 - Render and migrate the selected layout path

Goal: render structured refs at the LLVM output boundary and migrate the
representative struct-layout padding/storage array path.

Actions:

- add or adapt a boundary render helper for structured `LirTypeRef` values;
- replace the selected `[N x i8]`-like layout field construction with the
  structured array factory;
- keep emitted LLVM text and layout behavior identical.

Completion check: the selected layout path no longer uses `runtime_text` for a
supported array form and produces the same emission text.

### Step 3 - Verify and prove the bounded model

Goal: add nearby focused coverage for structured construction, equality/query,
rendering, and the migrated layout path.

Actions:

- cover builtin, integer-width, named struct/union, and array queries;
- cover array element/length and render behavior without string reparsing;
- build, run focused LIR/frontend/backend tests, then obtain the
supervisor-selected broader proof.

Completion check: focused tests demonstrate structural semantics and unchanged
emission; no test weakens a contract or relies on text parsing as authority.
