# LIR Direct HIR Family Construction and Array Composition

Status: Closed
Type: first-owner producer construction and recursive array composition
Matrix Rows: M3, M11
Dependencies: 838, 839, 840, 841, 842 at explicit value boundaries

## Goal

Replace semantic construction from `llvm_ty()` and rendering with producer-
specific construction from `TypeSpec`, canonical owners/layout, vector facts,
and signature facts; retain arrays as recursive typed composition.

## In Scope

- Migrate bounded HIR-to-LIR producer groups, `emit_rval_*`, and `coerce`.
- Make array length and typed element refs recursive facts; prove GEP consumes
  typed elements.

## Out Of Scope

- Parsing printer output, migrating global/extern mirrors, collector scans, or
  deleting compatibility adapters whose named producer is not migrated.

## Acceptance Criteria

- Fresh build plus scalar/vector/array/nested aggregate/signature lowering,
  GEP verification, and proof no rendered output is reparsed.
- Delete each runtime-text/rendered aggregate/array factory only after its
  exact producer emits a family ref; retain parser/extern/asm adapters by
  named consumer.

## 866 Reconciliation And 734 Return

Idea 866 keeps this as the ordered producer successor for direct HIR family
construction and recursive array composition. Return to 734 only after an exact
family construction handoff identifies one typed aggregate/vector/array row
that Raw BIR can receive without rendered-text recovery.

## Closure Note

Disposition: intentionally concluded as a bounded selected indexed local-array
GEP producer slice, not terminal completion of every HIR construction family.

Accepted commits:

- `0803da789` docs: select HIR array GEP producer target
- `3e7691199` lir: construct indexed array GEP refs from TypeSpec
- `67e5e8a69` test: prove indexed array GEP structured facts
- `c7d7267ce` docs: conclude indexed array GEP retirement check

Accepted proof:

- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`
- Focused subset passed with `frontend_lir_call_type_ref` and
  `frontend_lir_extern_decl_type_ref`, 2/2 passing.
- Final no-code retirement check used `git diff --check` and passed.

Delivered capability:

- Selected the indexed local-array GEP element-type construction route as the
  bounded producer target.
- Added recursive `TypeSpec` to `LirTypeRef::array` construction for selected
  local array object authority and indexed GEP element refs.
- Proved nested static local-array GEP element refs carry structured recursive
  array shape facts and do not reparse stale compatibility text.

Deferred work:

- Broader `emit_rval_*`, `coerce`, scalar/vector/signature producer migration,
  global/extern mirrors, collector scans, Raw-BIR receipt, and terminal
  compatibility deletion remain outside this bounded route.
- `hir_rendered_indexed_gep_element_type_text` remains for unsupported indexed
  GEP element shapes and unmigrated named consumers.

734 handoff:

- No direct 734 receiver handoff was accepted from this idea. A future return
  to 734 still requires one exact typed aggregate/vector/array row accepted by
  a named successor.

## Reviewer Reject Signals

- Reject `llvm_ty()`/printer text as authority, flattened recursive children,
  broad producer rewrites, or factory deletion before named-consumer parity.
