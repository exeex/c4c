# Prepared-MIR Join-Source Identity Completion

Status: Complete
Type: common prepared-MIR identity repair
Parent: `ideas/closed/703_bir_mir_contract_abstraction_umbrella.md`
Unblocks: `ideas/open/716_prepared_call_plan_cursor_complete_production.md`

## Goal

Make the common prepared-MIR direct-edge publication source query preserve
complete typed join-source authority for every supported move while reporting
unsupported moves explicitly and keeping the BIR semantic join view fail
closed when publication authority is incomplete.

## Why This Exists

`backend_prepared_lookup_helper` reaches the prepared-MIR join fixture with
four edge-copy source facts, but its prepared-MIR/BIR identity contract fails
after unrelated prepared-call assertions have passed. The first bad fact must
be localized in common join-source preparation or adaptation; it predates and
is outside prepared-call plan production.

## In Scope

- Trace prepared current-block join facts into the prepared-MIR direct-edge
  publication source query and the BIR semantic join identity adapter.
- Preserve predecessor, successor, destination, source, producer, freshness,
  publication, and move identity for supported named, immediate, and stack
  sources.
- Represent an unsupported move as a typed unsupported result without erasing
  the other supported source facts.
- Keep the aggregate BIR join identity unavailable when its required
  publication authority is incomplete.
- Add nearby positive and negative proof across more than the existing
  four-row fixture.

## Out Of Scope

- Prepared-call plans or call lookup.
- Block-entry publication identity.
- Target materialization, x86 joined-branch emission, move scheduling, or
  register allocation policy.
- Route-number restoration or diagnostic-text authority.

## Acceptance Criteria

- The first incorrect common join-source fact is documented before repair.
- Supported prepared-MIR direct-edge sources expose complete typed authority;
  unsupported, missing, stale, duplicate, or mismatched evidence remains
  explicit and fail closed.
- The BIR semantic join view does not claim availability when required
  publication authority is unsupported or incomplete.
- Focused prepared lookup/join-source tests and a supervisor-selected broader
  backend comparison are green without expectation changes.

## Reviewer Reject Signals

- A four-row, destination-name, block-label, or assertion-order special case.
- Treating an unsupported move as available, dropping it silently, or choosing
  source authority by vector position.
- Weakening the aggregate fail-closed contract or rewriting expectations to
  accept incomplete identity.
- Helper renames, status reclassification, or diagnostic-only changes claimed
  as capability repair.
- Changes to prepared-call production, block-entry publication, target
  materializers, or move scheduling.

## Completion Note

Completed after the independent BIR CFG prerequisite in idea 719 was restored.
The common prepared-MIR view and BIR semantic adapter now preserve exact typed
edge, destination/source, producer, publication, move, bundle, storage, and
freshness authority for supported named, immediate, and stack sources. Missing,
stale, unsupported, mismatched, incomplete-producer, and semantic-slot conflict
evidence fails closed at the appropriate boundary.

Focused positive and negative proof was accepted by
`review/idea717_step24_final_acceptance_review.md` without expectation
weakening or testcase-shaped authority. The matching broader backend guard
completed with 348 of 400 tests passing and the same 52 known failures before
and after. This closes the common join-source prerequisite and unblocks resumed
acceptance of idea 716.
