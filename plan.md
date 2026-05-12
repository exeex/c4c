# Frontend-to-BIR Legacy String Lookup Closure Gate Runbook

Status: Active
Source Idea: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md

## Purpose

Close the second frontend-to-BIR legacy string lookup audit as the final
pre-backend-restart authority gate.

Goal: produce a milestone closure decision showing whether backend restart may
proceed after parser, sema, HIR, LIR, and BIR retained strings are classified.

## Core Rule

This is a closure gate, not backend restart work. Do not implement new backend
routes or accept expectation downgrades, unsupported markings, or
testcase-shaped shortcuts as progress.

## Read First

- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/closed/188_lir_bir_freeze_closure_gate.md`
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`
- `ideas/closed/193_hir_constructor_member_owner_structured_lookup_closure.md`
- `ideas/closed/194_bir_global_memory_provenance_linknameid_expansion.md`

## Current Scope

- Review completed ideas 188 and 190-194.
- Build a frontend-to-BIR closure ledger covering parser token spelling, sema
  consteval/type utilities, HIR compile-time state, HIR object/member owner
  lookup, HIR-to-LIR call/type metadata, LIR call payload, and BIR lowering.
- Confirm no high-risk metadata-rich path recovers semantic identity through
  rendered strings after a complete structured miss.
- Confirm retained raw/no-id compatibility has an explicit owner and removal
  condition.
- Run milestone-appropriate broad validation before closure.

## Non-Goals

- Do not start backend restart implementation.
- Do not remove strings required by printers, assemblers, diagnostics, or
  route-local SSA/slot/block handles.
- Do not rewrite parser, sema, or HIR type systems.
- Do not weaken supported behavior or change tests to hide compatibility gaps.

## Working Model

Retained strings are acceptable only when they are structured-authority
mirrors, display/output text, diagnostics, route-local handles, ABI/final
spelling, or explicit no-metadata compatibility. Metadata-rich generated paths
must not recover semantic identity from rendered spelling after a complete
structured miss.

## Execution Rules

- Keep this runbook focused on audit, ledger, closure proof, and any narrow
  blocker split required before backend restart.
- If a real blocker remains, create or request a separate `ideas/open/*.md`
  initiative instead of silently expanding this gate.
- Prefer evidence from source inspection, closed idea notes, and matching
  validation logs over expectation rewrites.
- Treat testcase-overfit changes as route failure, not closure progress.

## Ordered Steps

### Step 1: Reconstruct Closure Inputs

Goal: establish the completed dependency evidence and the frontend-to-BIR
surfaces this gate must classify.

Actions:

- Read the linked closed ideas and extract their closure facts, retained
  compatibility boundaries, and validation notes.
- Inspect the relevant parser, sema, HIR, LIR, and BIR lookup surfaces named by
  the source idea.
- Identify any remaining raw or no-id compatibility paths that still need owner
  and removal-condition classification.

Completion check:

- `todo.md` records the dependency evidence reviewed, the inspected surface
  list, and any candidate blockers requiring deeper audit.

### Step 2: Produce the Closure Ledger

Goal: classify retained frontend-to-BIR string surfaces without changing source
intent or starting backend work.

Actions:

- Write a ledger that classifies each retained string surface as
  structured-authority mirror, display/output text, diagnostic text,
  route-local handle, ABI/final spelling, or explicit no-metadata
  compatibility.
- For every explicit compatibility path, name the owner and removal condition.
- Mark any unclassified high-risk metadata-rich path as a blocker instead of
  treating it as closed.

Completion check:

- The active execution notes contain a ledger sufficient for reviewer scrutiny,
  and every retained compatibility path has an owner and removal condition or a
  blocker record.

### Step 3: Audit High-Risk Generated Paths

Goal: prove metadata-rich generated paths fail closed or are already under
structured authority.

Actions:

- Check HIR rendered registry and constructor/member owner fallback paths
  called out by the source idea.
- Check HIR-to-LIR call/type metadata, LIR call payload, and BIR lowering paths
  for stale rendered-string recovery after complete structured misses.
- Add only narrow follow-up ideas for uncovered blockers; do not broaden this
  gate into unrelated implementation.

Completion check:

- No known high-risk metadata-rich path remains unclassified; any remaining
  blocker has a separate open idea before this gate proceeds to closure.

### Step 4: Run Milestone Validation and Decide Restart Readiness

Goal: finish the closure gate with broad proof and an explicit backend restart
decision.

Actions:

- Run milestone-appropriate broad validation, normally the full suite unless
  the supervisor chooses a different regression-guard scope.
- Compare canonical regression logs under the repo policy when baseline changes
  exist.
- State whether backend restart is allowed next or blocked by a new narrow
  open idea.

Completion check:

- Validation proof is recorded, the restart-readiness decision is explicit,
  and the source idea is ready for close review if no blockers remain.
