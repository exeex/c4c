# HIR Template Registry Structured Generated Paths Runbook

Status: Active
Source Idea: ideas/open/201_hir_template_registry_structured_generated_paths.md

## Purpose

Close the HIR template-registry authority blocker found by the frontend-to-BIR
closure gate.

## Goal

Ensure metadata-rich generated HIR template paths use structured template
identity when available, and fail closed after a complete structured miss
instead of recovering semantic authority through rendered template spelling.

## Core Rule

Do not let generated call, deduction, seed, collection, or deferred retry paths
recover a metadata-rich template definition through string-only
`find_template_def(name)` or `has_template_def(name)` after structured identity
was available and missed.

## Read First

- `ideas/open/201_hir_template_registry_structured_generated_paths.md`
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

## Current Targets

- Generated template-call lowering and pending consteval replay.
- Template deduction result inference.
- HIR build collection, seeding, and deferred instantiation retry bookkeeping.
- Focused HIR lookup tests proving stale rendered spelling cannot repair a
  complete structured miss on at least one generated call or retry route.

## Non-Goals

- Do not rewrite the full template deduction system.
- Do not remove diagnostic, display, ordering, or preservation-only rendered
  template names.
- Do not touch parser, sema, LIR, BIR, backend, or unrelated compatibility
  retirement ideas.
- Do not weaken supported generated-template behavior, downgrade expectations,
  or mark routes unsupported.

## Working Model

- Structured template lookup has priority whenever a caller has an owning node,
  declaration identity, or other metadata-rich source identity.
- String-only lookup can remain only for explicit no-metadata compatibility,
  display/order text, diagnostics, or final rendered preservation state.
- A complete structured miss means rendered spelling is not allowed to become
  semantic authority for that metadata-rich operation.

## Execution Rules

- Keep each implementation slice narrow and behavior-preserving except for
  intentional fail-closed behavior after structured miss.
- Prefer existing HIR compile-time/template helpers over new registry
  abstractions unless the existing API cannot express the needed boundary.
- Document every retained string-only registry call in `todo.md` with owner,
  limitation, and removal condition.
- Code-changing steps need fresh build plus focused HIR proof. Escalate to a
  broader HIR or full-suite checkpoint before closing the blocker.
- Reject testcase-shaped shortcuts, named-case-only guards, helper renames that
  preserve rendered-name authority, and expectation-only changes.

## Steps

### Step 1: Inventory Template Registry Authority Paths

Goal: classify every generated HIR template registry caller in the source
idea's hot files.

Primary targets:

- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

Actions:

- Locate `find_template_def(...)`, `has_template_def(...)`, and nearby
  structured lookup helpers in the hot files.
- Classify each caller as metadata-rich semantic authority,
  no-metadata compatibility, display/order text, diagnostic text, or
  preservation-only spelling.
- Identify the first implementation slice that can fence rendered fallback
  without broad template-system rewrite.

Completion check:

- `todo.md` records the caller inventory, the first code slice, retained
  compatibility paths, and any ambiguity that needs supervisor review.

### Step 2: Fence Generated Call And Replay Lookup

Goal: prevent generated call lowering and pending consteval replay from using
rendered spelling as semantic authority after structured identity was
available and missed.

Primary targets:

- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

Actions:

- Route metadata-rich generated call and replay lookup through structured
  template identity when the owning node or declaration identity is available.
- Keep string-only lookup only for no-metadata compatibility or display-like
  state.
- Add or update focused tests proving stale rendered template spelling cannot
  repair a complete structured miss on a generated call or replay route.

Completion check:

- `cmake --build --preset default` passes.
- A focused HIR lookup/template proof passes and is written to
  `test_after.log`.
- `todo.md` records changed files, proof command, and remaining registry
  authority callers.

### Step 3: Fence Deduction, Collection, Seed, And Retry Paths

Goal: handle the remaining metadata-capable template registry paths from the
inventory.

Primary targets:

- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

Actions:

- Convert metadata-rich deduction, collection, seed, or deferred retry paths to
  structured lookup where the required identity exists.
- For paths that cannot be converted safely, hard-fence them as explicit
  compatibility with owner, limitation, and removal condition.
- Extend tests if a second route is needed to prove the complete-miss boundary.

Completion check:

- Fresh build plus focused HIR proof passes.
- `todo.md` records every retained string-only registry path and why it is not
  semantic authority for metadata-rich generated work.

### Step 4: Record Closure Evidence

Goal: make the blocker clearance auditable for idea 195.

Actions:

- Summarize the final caller classification from the hot files.
- State whether the HIR template-registry blocker for idea 195 is cleared.
- List any explicit compatibility leftovers with concrete removal conditions.

Completion check:

- `todo.md` contains closure evidence sufficient for the supervisor and
  reviewer to compare against the source idea's acceptance criteria.

### Step 5: Validate And Hand Off Closure

Goal: prove the completed blocker is ready for lifecycle closure.

Actions:

- Run the focused proof used by the implementation steps.
- Run a broader HIR checkpoint or full-suite validation as selected by the
  supervisor.
- Keep canonical regression logs in `test_before.log` and `test_after.log`
  when regression comparison is required.

Completion check:

- Validation results are recorded in `todo.md`.
- The source idea acceptance criteria are satisfied or any remaining blocker is
  clearly identified for lifecycle handling.
