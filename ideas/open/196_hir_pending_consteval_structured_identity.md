# HIR Pending Consteval Structured Identity

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`

## Goal

Carry structured consteval identity through pending and recursive consteval
evaluation so metadata-rich replay paths do not recover callees from rendered
function names.

The target is the `PendingConstevalExpr` and recursive consteval evaluation
surface identified while closing the rendered registry audit. Rendered names
may remain as diagnostics, display text, or explicit no-metadata compatibility,
but complete structured consteval metadata should be the semantic authority.

## Why This Idea Exists

Idea 192 converted direct consteval call lowering away from ordinary rendered
registry authority and proved stale rendered fallback rejection for that route.
It left pending consteval replay by `PendingConstevalExpr::fn_name` and
recursive consteval evaluation as the highest-value remaining metadata-rich
consteval surface.

Backend restart should not rely on pending or recursive consteval replay
finding callees by rendered spelling when structured callee identity is
available or should have been carried.

## In Scope

- Audit `PendingConstevalExpr` creation, storage, and replay paths.
- Audit recursive consteval evaluation entry points that can currently choose a
  consteval definition by rendered function name.
- Add or route structured consteval identity through the selected pending or
  recursive path.
- Keep rendered-name lookup only as explicit no-metadata compatibility with a
  documented owner and removal condition.
- Add focused HIR tests for structured success and stale rendered fallback
  rejection in the selected route.

## Out Of Scope

- Rewriting the complete compile-time engine.
- Redesigning template instantiation or specialization lookup.
- Removing diagnostic/display function names.
- Starting backend restart work.
- Treating address-of consteval diagnostics as semantic lookup authority.

## Acceptance Criteria

- The selected pending or recursive consteval route carries structured
  consteval identity instead of relying on rendered `fn_name` lookup when
  metadata is complete.
- Complete structured misses fail closed for the covered route and do not fall
  back to stale rendered registry entries.
- Any remaining rendered consteval lookup is classified as no-metadata
  compatibility, diagnostic/display text, or separate follow-up work.
- Focused HIR proof covers structured success and stale rendered fallback
  rejection.

## Reviewer Reject Signals

- The slice only renames `fn_name` fields or helper APIs while still choosing
  consteval definitions by rendered spelling in metadata-rich replay.
- A complete structured consteval miss falls back to `find_consteval_def(string)`
  or another rendered registry map.
- Tests only cover the direct consteval call-lowering route already closed by
  idea 192.
- The implementation weakens diagnostics, removes supported consteval behavior,
  or marks supported paths unsupported to claim closure.
- The route expands into broad template-engine or backend-restart work.
