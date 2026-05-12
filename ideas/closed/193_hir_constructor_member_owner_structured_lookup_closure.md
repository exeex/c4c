# HIR Constructor/Member Owner Structured Lookup Closure

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/177_template_record_owner_structured_identity.md`
- `ideas/closed/182_type_identity_migration_closure_gate.md`

## Goal

Close one remaining HIR owner lookup path where direct constructor calls or
aggregate/member handling can still derive record owner identity from rendered
callee/tag spelling.

Structured record owner metadata should drive metadata-rich constructor and
member lookup. Rendered owner tags may remain only for explicit no-owner
compatibility.

## Why This Idea Exists

The frontend-to-BIR audit found HIR object lowering paths that build a lookup
tag from `callee_name`, `legacy_tag`, or `module_->struct_defs` rendered keys
when structured owner metadata is absent. Some of this is valid compatibility,
but backend freeze should not rely on ambiguous rendered owner recovery for
metadata-rich constructors or aggregate members.

## In Scope

- Audit direct struct constructor lowering and aggregate/member owner lookup in
  `src/frontend/hir/impl/expr/object.cpp` and nearby helper APIs.
- Select one metadata-rich constructor/member route and require structured
  owner identity there.
- Keep rendered tag fallback only for no-owner/no-metadata compatibility.
- Add tests where two rendered-compatible or collision-prone owner spellings
  cannot be confused when structured metadata exists.
- Document any remaining no-owner compatibility path and its removal condition.

## Out Of Scope

- Rewriting all object/member lowering.
- Changing overload resolution policy.
- Removing final display tags from `HirStructDef`.
- Reopening parser tag parsing unless a concrete missing owner carrier is found.

## Acceptance Criteria

- The selected metadata-rich constructor/member route uses structured owner
  identity instead of rendered tag lookup.
- Complete structured owner misses fail closed for the covered path.
- Rendered fallback is explicit no-metadata compatibility.
- Focused HIR tests cover structured success and stale rendered fallback
  rejection.

## Completion Notes

Closed after selecting the direct struct constructor route. The route now treats
complete `record_def` or tag/namespace owner metadata as structured owner
authority through the lowerer registry structured-owner helper, and complete
structured owner misses fail closed instead of recovering through stale rendered
callee spelling.

Focused proof was recorded through `frontend_hir_lookup_tests`, with structured
success and stale rendered fallback rejection coverage. Close review accepted
the canonical focused regression guard comparison in `test_before.log` and
`test_after.log` using `--allow-non-decreasing-passed`, plus the hook-produced
full-suite baseline evidence in `test_baseline.log` at commit `7b8e16a21`
showing 3137/3137 tests passing.

Remaining rendered-owner surfaces were recorded in the Step 4 todo history as
explicit no-owner/no-metadata compatibility or broader follow-up cleanup, not as
unresolved acceptance blockers for this selected-route closure.

## Reviewer Reject Signals

- The implementation only changes comments while still choosing owners by
  rendered `callee_name`.
- Metadata-rich owner mismatches fall back to `module_->struct_defs` string
  lookup.
- Tests only cover simple unqualified constructor names.
- The slice becomes a broad object-lowering rewrite.
