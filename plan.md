# HIR Generated Member Payload Structured Miss Runbook

Status: Active
Source Idea: ideas/open/202_hir_generated_member_payload_structured_miss.md
Activated from: blocked active gate `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`

## Purpose

Clear the generated-member HIR blocker that prevents idea 195 from closing.

Goal: ensure metadata-rich generated-member lookup cannot recover semantic
identity through rendered owner/member spelling after a complete structured
miss.

## Core Rule

Fix the generated-member authority boundary, not the testcase surface. Do not
downgrade supported generated-member behavior, weaken expectations, or hide the
fallback behind renamed rendered-string helpers.

## Read First

- `ideas/open/202_hir_generated_member_payload_structured_miss.md`
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `src/frontend/hir/impl/expr/scalar_control.cpp`

## Current Scope

- Audit the generated-member route in `scalar_control.cpp`.
- Identify where structured `HirStructMemberLookupKey` lookup succeeds, misses,
  and falls through to rendered owner/member lookup.
- Prevent metadata-rich generated-member paths from using rendered
  owner/member spelling as semantic authority after complete structured misses.
- Preserve rendered names only for diagnostics, final spelling, display text,
  or explicit no-metadata compatibility.
- Add focused stale rendered spelling coverage for the generated-member path.
- Record any retained compatibility fallback with owner, limitation, and
  removal condition.

## Non-Goals

- Do not rewrite the full HIR member lookup model.
- Do not remove diagnostic, final-output, or display-only member spelling.
- Do not touch parser, sema, LIR, BIR, or backend restart work.
- Do not mark supported generated-member cases unsupported or change expected
  output only to avoid the fallback.

## Working Model

The generated-member path currently starts from structured owner evidence, then
derives member candidates from rendered qualified spelling and can fall through
to rendered `find_struct_static_member_decl` or
`find_struct_static_member_const_value`. A metadata-rich path must resolve by
structured owner/member authority or fail closed; rendered fallback is allowed
only when it is explicitly no-metadata compatibility or non-semantic text.

## Execution Rules

- Keep implementation changes narrowly scoped to the generated-member route and
  directly supporting helpers/tests.
- Prefer a semantic guard or structured lookup contract over named-case checks.
- Make retained rendered lookup sites visibly fenced with owner, limitation,
  and removal condition.
- Treat testcase-shaped shortcuts and expectation downgrades as route failure.
- After this blocker is proven, report whether idea 195's generated-member
  payload blocker is cleared; do not close idea 195 from this runbook.

## Ordered Steps

### Step 1: Trace Generated-Member Authority

Goal: establish the exact structured and rendered lookup paths before editing.

Actions:

- Inspect the generated-member branch in
  `src/frontend/hir/impl/expr/scalar_control.cpp`.
- Trace `structured_owner_key_from_qualified_ref`, generated member candidate
  extraction, structured `HirStructMemberLookupKey` lookup, and rendered
  `find_struct_static_member_decl` / `find_struct_static_member_const_value`
  fallback.
- Identify whether any rendered fallback is truly no-metadata compatibility or
  display/final/diagnostic spelling.

Completion check:

- `todo.md` records the traced authority path and names the exact fallback that
  must be removed, guarded, or fenced before implementation proceeds.

### Step 2: Enforce Structured-Miss Fail-Closed Behavior

Goal: prevent metadata-rich generated-member lookup from recovering through
rendered owner/member spelling after complete structured misses.

Actions:

- Adjust the generated-member route so structured owner/member metadata either
  resolves through structured authority or fails closed on complete miss.
- Keep rendered owner/member lookup only where the path is explicitly
  no-metadata compatibility or non-semantic text.
- Avoid broad HIR member model rewrites and unrelated lookup behavior changes.

Completion check:

- The metadata-rich generated-member path no longer reaches rendered semantic
  lookup after a complete structured miss, and any retained rendered fallback is
  explicitly fenced.

### Step 3: Add Focused Coverage

Goal: prove stale rendered owner/member spelling cannot repair the structured
generated-member miss.

Actions:

- Add or update the smallest focused test that exercises stale rendered
  owner/member spelling on the generated-member path.
- Confirm the test fails before the authority fix if practical and passes after
  the fix.
- Keep expectations behavioral; do not rely on dump text or diagnostic spelling
  as the proof.

Completion check:

- Focused test coverage proves the structured generated-member path fails
  closed or resolves by structured identity rather than rendered fallback.

### Step 4: Validate and Report Blocker Status

Goal: produce executor proof and state whether this blocker is cleared for
idea 195.

Actions:

- Run the supervisor-delegated build and focused test subset.
- Escalate validation if the implementation touches shared HIR lookup behavior
  beyond `scalar_control.cpp`.
- Record any retained compatibility owner, limitation, and removal condition.
- State whether idea 195's generated-member payload blocker is cleared.

Completion check:

- Build/test proof is recorded in `todo.md`, no testcase-overfit signals
  remain, and the handoff clearly says whether idea 195 may treat this blocker
  as resolved.
