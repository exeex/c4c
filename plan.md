# HIR Compile-Time Rendered Registry API Retirement Audit Runbook

Status: Active
Source Idea: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md

## Purpose

Make HIR compile-time rendered registry APIs stop looking like normal semantic
authority for callers that already have structured declaration, TextId, owner,
or value-binding metadata.

Goal: audit the remaining rendered registry entry points, convert or fence one
metadata-rich path, and prove complete structured misses do not silently recover
through rendered names in the covered path.

## Core Rule

Do not remove display strings or diagnostic spelling. This plan is about
semantic lookup authority: metadata-rich paths must use structured keys, while
raw rendered lookup may remain only as explicit compatibility with a named
owner and removal condition.

## Read First

- `ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md`
- `src/frontend/hir/compile_time_engine.hpp`
- Current users of `find_template_def(string)`,
  `find_template_struct_def(string)`, `find_consteval_def(string)`, rendered
  enum/const-int maps, and rendered template specialization mirrors.
- Existing HIR tests around template, consteval, enum, and const-int
  compile-time lookup behavior.

## Scope

- Inventory public rendered lookup APIs in the HIR compile-time engine.
- Identify call sites that still select compile-time entities through rendered
  names.
- Convert one metadata-rich rendered lookup path to structured declaration,
  TextId, or key authority, or explicitly fence it if conversion is not yet
  justified.
- Prove the covered structured miss fails closed instead of falling back to a
  rendered registry name.
- Record remaining compatibility APIs and their owner/removal condition.

## Non-Goals

- Do not redesign template instantiation semantics.
- Do not remove every rendered map in one route.
- Do not classify diagnostic or display strings as semantic authority.
- Do not start backend implementation work.
- Do not weaken tests or mark supported behavior unsupported to claim closure.

## Working Model

Rendered registry APIs are compatibility mirrors. They may still exist for
callers with no structured identity, but metadata-rich callers must not be able
to miss a structured key and then recover through the rendered spelling. The
safe outcome is either a structured lookup success, an explicit no-metadata
compatibility path, or a fail-closed diagnostic/assertion for complete metadata
misses.

## Execution Rules

- Keep source-idea edits out of routine execution; use `todo.md` for packet
  progress.
- Prefer small route-local API renames/fences over broad compile-time engine
  rewrites.
- If a selected call path needs more than one implementation packet, record the
  subdivision in `todo.md` rather than rewriting this plan immediately.
- A code-changing packet needs fresh build or focused test proof. A milestone
  close needs broad validation or an explicit regression-guard baseline
  decision.

## Step 1 - Inventory Rendered Registry Authority

Goal: identify every public or reachable rendered registry lookup that can act
as semantic authority.

Actions:
- Inspect `src/frontend/hir/compile_time_engine.hpp` for string-keyed lookup
  APIs and rendered-map exposure.
- Search call sites for rendered template, template-struct, consteval, enum,
  const-int, and specialization lookup.
- Classify each use as structured-authority mirror, display/diagnostic,
  no-metadata compatibility, or candidate metadata-rich authority bug.
- Select one candidate route for Step 2.

Completion check:
- `todo.md` contains a ledger of rendered registry APIs, call-site classes, and
  the selected Step 2 route.

## Step 2 - Convert Or Fence One Metadata-Rich Route

Goal: make the selected route stop treating rendered names as ordinary semantic
lookup authority.

Actions:
- Convert the selected metadata-rich caller to structured declaration, TextId,
  owner, or value-binding key lookup when the metadata is complete.
- If immediate conversion is not correct, rename or fence the rendered API so it
  is visibly compatibility-only and cannot be chosen by metadata-rich code by
  accident.
- Keep raw rendered lookup available only for an explicit no-metadata
  compatibility boundary.
- Avoid behavior changes outside the selected route.

Completion check:
- The selected path either uses structured authority or an explicit
  compatibility API with documented owner/removal condition.
- Focused build or test proof is recorded in `todo.md`.

## Step 3 - Prove Structured Miss Fail-Closed Behavior

Goal: demonstrate the covered metadata-rich path cannot recover through a stale
or rendered registry name after a complete structured miss.

Actions:
- Add or update focused tests/assertions for structured success and stale
  rendered fallback rejection in the selected path.
- Preserve diagnostics/display output behavior.
- Confirm no expectation downgrade or testcase-shaped shortcut is being used as
  progress.

Completion check:
- Focused tests prove structured success and fail-closed stale rendered lookup
  behavior for the covered route.

## Step 4 - Record Remaining Compatibility Surface

Goal: leave a closure ledger for rendered registry APIs that remain after the
selected conversion/fence.

Actions:
- Record which rendered APIs remain public or reachable.
- For each remaining no-metadata compatibility path, record the owner and
  removal condition.
- Identify whether any remaining surface requires a separate follow-up idea.

Completion check:
- `todo.md` contains a remaining-surface ledger and any follow-up recommendation.

## Step 5 - Validate And Prepare Closure Evidence

Goal: prove the source idea acceptance criteria are satisfied.

Actions:
- Run the focused proof for the converted/fenced route.
- Run broader validation appropriate for this HIR compile-time milestone, or
  prepare matching `test_before.log` and `test_after.log` for regression-guard
  comparison.
- Record final acceptance evidence and whether remaining broad retirement work
  is out of scope or needs a new open idea.

Completion check:
- `todo.md` records final validation, regression status, and whether idea 192 is
  ready for plan-owner close review.
