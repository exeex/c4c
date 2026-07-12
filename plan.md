# Block-Entry Publication Identity Completion Runbook

Status: Active
Source Idea: ideas/open/718_block_entry_publication_identity_completion.md
Resumed after completing: ideas/closed/720_block_entry_publication_proof_evidence_multiplicity.md

## Purpose

Finish acceptance of exact prepared-to-BIR block-entry publication identity
after idea 720 repaired and proved the semantic evidence-multiplicity seam.

## Goal

Prove the repaired exact-identity route through the production-facing
frame/stack contract and a matching broader backend comparison.

## Core Rule

Resolve publication identity from attributed semantic evidence. Never recover
it by source order, nearest PHI, display name alone, or target-emission facts.

## Read First

- `ideas/open/718_block_entry_publication_identity_completion.md`
- `ideas/closed/720_block_entry_publication_proof_evidence_multiplicity.md`
- `review/idea720_step6_5_handback_audit.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

## Completed Route

- The earliest prepared-to-BIR divergence was localized and the proof-bearing
  semantic query seam was established.
- The seam was proved across nearby positive and fail-closed shapes.
- The AArch64 production consumer and named frame/stack contract were wired to
  exact prepared attribution and BIR destination identity.
- Idea 720 separated destination identity from independently attributed proof
  claims, made Route4 the authoritative classifier, bound MIR validation to
  that result, and proved same-name identity plus duplicate-claim behavior.

## Current Scope

- Fresh focused proof spanning `backend_prepared_lookup_helper` and
  `backend_prepare_frame_stack_call_contract`.
- Audit of the production route against idea 718's reviewer reject signals.
- A supervisor-selected broader backend before/after comparison with matching
  canonical scope.

## Non-Goals

- Do not reopen idea 720's accepted internal decomposition seam.
- Do not change prepared-call plans, argument materializability, or ABI policy.
- Do not change join-source or edge-publication identity.
- Do not change target materialization, storage hooks, register spelling, move
  order, or emitted publication policy.
- Do not absorb unrelated later frame/stack executable failures into this
  route without first proving they are required by idea 718.

## Execution Rules

- Preserve exact successor, destination, type, instruction coordinate, and
  proof attribution across production validation.
- Preserve typed fail-closed results for missing, stale, inconsistent,
  ambiguous, or unattributed evidence.
- Do not weaken expectations or recover identity from display name, source
  order, proximity, or target-emission behavior.
- Use only supervisor-delegated build, focused, and broader proof commands.

## Ordered Steps

### Step 6: Run focused production and contract proof

Goal: prove the corrected semantic boundary through both the lookup coverage
and the named production-facing contract.

Actions:

- Run the supervisor-delegated build.
- Run one matching focused command that includes
  `backend_prepared_lookup_helper` and
  `backend_prepare_frame_stack_call_contract`.
- Confirm the frame/stack contract obtains complete publication identity from
  producer/query-derived attributed facts without manual readiness or proof
  completion.
- If the aggregate executable fails later, localize the first bad fact and do
  not silently broaden idea 718 into an unrelated subsystem.

Completion check:

- Both focused tests are green after the production and ambiguity repairs,
  with no expectation downgrade, manual proof completion, or fixture-shaped
  recovery.

### Step 7: Run broader acceptance proof and close

Goal: establish closure-quality evidence for idea 718 and make its downstream
work resumable.

Actions:

- Audit the complete idea 718 route for fixture, display-name, source-order,
  nearest-PHI, proof-bit, and target-emission shortcuts.
- Run the supervisor-selected broader backend before/after comparison with
  matching canonical `test_before.log` and `test_after.log` scope.
- Confirm the focused production/contract proof and broader comparison cover
  the source idea's acceptance criteria without expectation changes.
- Request lifecycle closure only after reviewer and regression acceptance.

Completion check:

- Focused production/contract proof and broader backend comparison are green,
  reviewer reject signals are absent, and idea 718 can close before routing
  downstream idea 716 or other parked work.
