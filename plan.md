# Direct Call Signature Metadata Structured Boundary Runbook

Status: Active
Source Idea: ideas/open/184_direct_call_signature_metadata_structured_boundary.md

## Purpose

Bring direct-call signature lowering up to the structured-metadata boundary
already established for bounded indirect calls.

## Goal

Ensure metadata-rich generated LIR direct calls use structured call/function
signature facts for semantic lowering, while rendered signature text remains
only an explicit compatibility or display surface.

## Core Rule

Generated metadata-rich direct calls must not silently fall back to parsing
rendered function signature text. Preserve no-metadata compatibility as a
fenced path, not as normal generated-path authority.

## Read First

- `ideas/open/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/closed/181_function_pointer_signature_type_identity.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `src/codegen/lir`
- `src/backend/bir/lir_to_bir`

## Current Scope

- Inventory the direct-call signature lowering route from generated LIR into
  BIR.
- Identify where return type, parameter types, variadic state, byval, sret, and
  aggregate layout facts are currently derived.
- Select the generated metadata-rich direct-call path and require structured
  signature data there.
- Preserve rendered-signature parsing only for explicit no-metadata
  compatibility fixtures.
- Add focused tests for structured success and stale or missing metadata
  rejection.

## Non-Goals

- Do not rewrite every call ABI path in one slice.
- Do not change printer syntax or final emitted call spelling for cosmetic
  reasons.
- Do not remove raw direct-LIR compatibility fixtures.
- Do not reopen parser or sema function-pointer work unless direct-call
  metadata exposes a concrete missing fact.
- Do not claim backend freeze closure; that remains owned by idea 188.

## Working Model

- Idea 181 closed the indirect function-pointer signature path and left
  direct-call signature metadata as future work.
- Direct-call lowering may keep legacy rendered-signature parsing for raw or
  no-metadata imports.
- Generated metadata-rich direct calls should fail closed when structured
  signature metadata is stale or missing.
- Aggregate-sensitive facts include aggregate return, byval parameters, sret,
  variadic state, and ordinary scalar parameters/returns.

## Execution Rules

- Keep implementation changes scoped to direct-call signature lowering and its
  direct tests.
- Treat new rendered-signature parser branches as route drift.
- Separate generated metadata-rich behavior from raw/no-metadata compatibility
  in code and tests.
- Prefer focused tests that prove semantic lowering behavior, not only printed
  LIR/BIR text.
- For code-changing steps, run build proof plus focused call-signature
  frontend/LIR/BIR/backend coverage selected by the supervisor.

## Ordered Steps

### Step 1: Inventory direct-call signature lowering

Goal: Identify the exact direct-call route and current authority sources.

Primary targets:
- `src/codegen/lir`
- `src/backend/bir/lir_to_bir`

Actions:
- Inspect generated direct-call LIR construction and LIR-to-BIR direct-call
  lowering.
- Locate helpers that parse or trust rendered signature text.
- Locate any structured signature, function type, parameter, return, byval,
  sret, variadic, and aggregate layout facts already available.
- Record the generated metadata-rich route versus raw/no-metadata compatibility
  route in `todo.md`.

Completion check:
- `todo.md` names the direct-call lowering functions, existing structured fact
  sources, and rendered-signature fallback points.

### Step 2: Fence generated metadata-rich direct calls

Goal: Require structured signature metadata on the selected generated direct-
call path.

Primary targets:
- Direct-call signature lowering in `src/backend/bir/lir_to_bir`.
- Structured call/function signature facts from generated LIR.

Actions:
- Add or reuse a route distinction that tells generated metadata-rich direct
  calls apart from raw/no-metadata compatibility imports.
- Make metadata-rich direct calls consume structured return, parameter,
  variadic, byval, sret, and aggregate layout facts where available.
- Make stale or missing structured metadata fail closed on the selected path.
- Keep rendered-signature parsing reachable only through the explicit
  compatibility path.

Completion check:
- Generated metadata-rich direct calls no longer use rendered signature parsing
  as normal semantic authority for the selected path.

### Step 3: Add focused direct-call tests

Goal: Prove structured success, fail-closed rejection, and retained
compatibility behavior.

Primary targets:
- Existing call-signature, LIR-to-BIR, and backend tests.

Actions:
- Add at least one aggregate-sensitive direct-call success case.
- Add at least one stale or missing structured-metadata rejection case.
- Preserve or add a raw/no-metadata compatibility case where rendered
  signature parsing remains intentionally allowed.
- Avoid tests that only prove pretty-printed text without exercising semantic
  lowering behavior.

Completion check:
- Focused tests fail without the structured boundary and pass with it.

### Step 4: Validate and prepare handoff

Goal: Produce acceptance-grade proof for idea 184 without broadening into the
later freeze gate.

Actions:
- Run build proof and the supervisor-selected focused call-signature coverage.
- Escalate to a broader backend or compiler subset if the implementation
  crosses shared lowering or ABI behavior beyond the selected path.
- Update `todo.md` with proof results, residual risks, and the recommended
  lifecycle close/switch decision.

Completion check:
- Idea 184 acceptance criteria are satisfied, and the supervisor has enough
  proof to review and commit the slice.
