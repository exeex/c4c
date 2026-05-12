# HIR Template Registry Structured Generated Paths

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`

Blocks:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`

## Goal

Retire or hard-fence string-only HIR template registry calls that remain on
metadata-capable generated call, deduction, collection, and deferred retry
paths.

Metadata-rich generated paths should use structured template identity when it
is available and fail closed after a complete structured miss instead of
recovering a template definition through rendered spelling.

## Why This Idea Exists

Idea 195 Step 3 found that `CompileTimeState::find_template_def(const Node*,
rendered_name)` has a structured miss boundary, but representative generated
paths still call string-only `find_template_def(name)` or
`has_template_def(name)`.

The unproven paths include template-call lowering, deduction result inference,
HIR collection/seed/deferred instantiation, and deferred retry bookkeeping.
Those paths are metadata-capable enough that treating the string-only registry
as ordinary authority would leave the closure gate unable to prove frontend
generated template identity is structured.

## In Scope

- Audit generated HIR template registry callers in:
  - `src/frontend/hir/impl/expr/call.cpp`
  - `src/frontend/hir/impl/templates/deduction.cpp`
  - `src/frontend/hir/hir_build.cpp`
  - `src/frontend/hir/impl/compile_time/engine.cpp`
- Convert metadata-rich call, deduction, seed, or retry paths to structured
  template lookup when the owning node or declaration identity is available.
- Preserve string-only lookup only for explicit no-metadata compatibility,
  display/order state, diagnostics, or final rendered preservation state.
- Add or update focused tests proving a stale rendered template name cannot
  repair a complete structured miss on at least one generated call or retry
  route.
- Document any retained compatibility path with owner, limitation, and removal
  condition.

## Out Of Scope

- Rewriting the full template deduction system.
- Removing diagnostic or preservation-only rendered template names.
- Parser, sema, LIR, BIR, or backend compatibility retirement.
- Weakening template support, marking supported routes unsupported, or changing
  expected output only to avoid the structured miss.

## Acceptance Criteria

- Metadata-rich generated template paths do not call string-only HIR template
  registry APIs as semantic authority after a complete structured miss.
- Any remaining string-only registry path is explicitly fenced as no-metadata
  compatibility, display/order text, diagnostic text, or preservation-only
  spelling with a concrete removal condition.
- Focused tests cover stale rendered spelling and show the structured generated
  path fails closed or resolves by structured identity.
- The closure notes state whether idea 195's HIR template-registry blocker is
  cleared.

## Closure Notes

Closed after the generated call, replay, deduction, collection, seed, and retry
authority paths were fenced or documented as no-metadata compatibility. Focused
HIR proof passed `frontend_hir_tests` and `frontend_hir_lookup_tests` 2/2 in
`test_after.log`; the close-time regression guard passed with
`--allow-non-decreasing-passed` against matching 2/2 `test_before.log` evidence.
The accepted full-suite baseline for implementation commit `a07fa8929` passed
3137/3137 in `test_baseline.log`.

This clears idea 195's HIR template-registry blocker. Broader frontend-to-BIR
closure and backend restart readiness remain separate lifecycle decisions.

## Reviewer Reject Signals

- A slice claims progress while generated call, deduction, seed, or deferred
  retry paths still recover a metadata-rich template through
  `find_template_def(name)` or `has_template_def(name)`.
- The fix only renames the string-only helper or moves it behind a wrapper
  while preserving the same rendered-name authority.
- Tests only update diagnostics, dump text, or expected rendered names.
- A supported generated template path is downgraded, skipped, or marked
  unsupported without explicit user approval.
- The route expands into broad HIR template-system rewrites or backend restart
  work instead of the narrow registry-authority blocker.
- The exact old complete-structured-miss recovery behavior remains available
  behind a new abstraction name.
