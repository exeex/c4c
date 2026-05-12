# Global Aggregate Layout Structured Boundary

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/173_aggregate_layout_identity_structured_boundary.md`
- `ideas/closed/176_lir_type_ref_structured_equality.md`

## Goal

Extend structured aggregate layout identity from the completed local aggregate
GEP boundary to one bounded global aggregate layout route.

Global aggregate loads, stores, pointer roots, and initializer-derived layout
queries should use structured layout identity when metadata is available.
Rendered `%struct...` spelling and legacy type text may remain output or
no-metadata compatibility payloads, but equal rendered spelling must not be
enough to accept stale or mismatched global aggregate metadata.

## Why This Idea Exists

Idea 173 intentionally selected a local aggregate GEP layout boundary and left
global aggregate layout work out of scope. The next obvious type-identity gap
is the global side of the same family: global struct arrays, nested global
aggregates, pointer roots, and initializer-backed aggregate accesses.

Those paths often pass through LIR/BIR/backend layout surfaces where final text
looks plausible even if the structured owner/layout identity is stale.

## In Scope

- Pick one bounded global aggregate layout family, such as global struct array
  read/write or nested global aggregate pointer roots.
- Carry structured layout identity from HIR/LIR into BIR/backend for that
  family.
- Prefer structured owner/layout facts over rendered struct/type spelling for
  metadata-rich inputs.
- Fail closed or report a clear structured mismatch when generated global
  aggregate metadata is stale, missing, opaque, or mismatched.
- Preserve rendered output spelling for printers and diagnostics.
- Add focused tests proving equal rendered spelling cannot override structured
  global layout identity.

## Out Of Scope

- Rewriting all global aggregate lowering routes in one pass.
- Changing emitted LLVM/assembly spelling for cosmetic reasons.
- Removing legacy no-metadata compatibility for hand-authored or raw LIR/BIR
  inputs.
- Broad ABI classification work not needed for the selected layout route.

## Acceptance Criteria

- The selected global aggregate route has a structured layout identity path.
- Metadata-rich global aggregate misses do not fall back to rendered type text.
- Legacy/no-metadata compatibility remains explicit and fenced.
- Focused backend/LIR/BIR tests cover the selected route and a stale/mismatched
  structured identity case.
- Validation includes a fresh build plus targeted backend route coverage.

## Closure Notes

Completed via the bounded metadata-rich global aggregate initializer/layout
route. Structured `LirTypeRef` identity now drives the selected generated
global aggregate layout path, stale/missing/opaque/parity-mismatched structured
metadata fails closed, and the legacy text-only fallback remains explicit for
no-metadata compatibility.

Closure proof used the focused route command recorded in `todo.md`:
`cmake --build build --target backend_prepare_structured_context_test frontend_lir_global_type_ref_test && ctest --test-dir build -R 'backend_prepare_structured_context|frontend_lir_global_type_ref' --output-on-failure`.
The canonical `test_before.log` and `test_after.log` comparison passed with no
new failures under the accepted non-decreasing baseline guard.

## Reviewer Reject Signals

- The route adds another rendered `%struct` parser branch as the main fix.
- Equal rendered aggregate spelling still accepts stale structured metadata.
- Tests only update snippets without proving layout identity behavior.
- The slice expands into unrelated aggregate ABI or local-slot refactors.
