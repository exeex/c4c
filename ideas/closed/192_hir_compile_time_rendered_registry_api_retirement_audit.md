# HIR Compile-Time Rendered Registry API Retirement Audit

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/182_type_identity_migration_closure_gate.md`

## Goal

Audit and retire, fence, or narrow the remaining HIR compile-time registry
APIs that expose rendered string lookup as ordinary-looking entry points.

The target is not to remove every compatibility mirror immediately; it is to
make sure callers with structured declaration, TextId, owner, or value-binding
metadata cannot accidentally select the rendered maps.

## Why This Idea Exists

`CompileTimeState` still exposes string-keyed template, consteval, enum, and
const-int registries alongside structured maps. Many lookups already fail
closed when structured keys are complete, but the raw string overloads and
rendered maps remain easy for new code to misuse.

Before backend restart, HIR compile-time state should stop looking like a
second semantic name system.

## In Scope

- Inventory public rendered lookup APIs in `src/frontend/hir/compile_time_engine.hpp`.
- Find call sites that still use `find_template_def(string)`,
  `find_template_struct_def(string)`, `find_consteval_def(string)`, rendered
  enum/const-int maps, or rendered template specialization mirrors.
- Convert selected metadata-rich call sites to structured declaration/TextId/key
  APIs.
- Rename or fence no-metadata compatibility APIs so their owner and removal
  condition are explicit.
- Add focused tests or assertions for structured miss fail-closed behavior.

## Out Of Scope

- Removing all rendered maps in one slice.
- Rewriting template instantiation semantics.
- Changing diagnostic display keys.
- Starting backend implementation work.

## Acceptance Criteria

- A caller inventory identifies which rendered registry APIs remain and why.
- At least one metadata-rich rendered registry call path is converted or
  explicitly fenced.
- Complete structured misses do not recover through rendered registry names in
  the covered path.
- Any remaining broad retirement work is captured as follow-up only if needed.

## Reviewer Reject Signals

- The audit treats diagnostic/display strings as registry authority.
- Rendered overloads remain public and unqualified with no compatibility label.
- Structured miss fail-closed behavior is weakened.
- The slice expands into template engine redesign.

## Closure Notes

Closed after converting the direct consteval call-lowering route through
`try_lower_consteval_call_expr` to require structured callee identity before
using rendered consteval registry compatibility. The focused HIR guard passed
with `test_before.log` and `test_after.log` both reporting `2/2` tests passing
and no new failures. The close-time regression guard passed on those canonical
logs with no new failures.

Full-suite baseline acceptance for commit `78f3cc95d` compared
`test_baseline.log` and `test_baseline.new.log`; both reported `3137/3137`
passing with no new failures.

Remaining rendered registry APIs are classified as compatibility or follow-up
surface rather than blockers for this idea. The highest-value remaining
metadata-rich consteval route has been materialized as
`ideas/open/196_hir_pending_consteval_structured_identity.md`, covering
structured consteval identity through `PendingConstevalExpr` and recursive
consteval evaluation.
