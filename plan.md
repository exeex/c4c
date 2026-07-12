# Block-Entry Publication Identity Completion Runbook

Status: Active
Source Idea: ideas/open/718_block_entry_publication_identity_completion.md
Activated after parking: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md

## Purpose

Restore exact agreement between available prepared block-entry publication
facts and the BIR semantic publication identity view.

## Goal

Preserve exact successor, destination, type, PHI instruction, and proof
attribution across the prepared-to-BIR semantic publication boundary.

## Core Rule

Resolve publication identity from attributed semantic evidence. Never recover
it by source order, nearest PHI, display name alone, or target-emission facts.

## Read First

- `ideas/open/718_block_entry_publication_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/query.h`
- `src/backend/mir/query.cpp`

## Current Scope

- Prepared current-block entry publication readiness.
- `find_bir_block_entry_publication_identity` and its semantic evidence path.
- Exact successor, destination value ID/name/type, PHI instruction index, and
  proof attribution.
- Positive and fail-closed proof across multiple publication shapes.

## Non-Goals

- Do not change prepared-call plans, argument materializability, or ABI policy.
- Do not change join-source or edge-publication identity.
- Do not change target materialization, storage hooks, register spelling, move
  order, or emitted publication policy.
- Do not restore route numbers or alter printer/debug vocabulary.

## Execution Rules

- Establish the earliest prepared-to-BIR identity divergence before repair.
- Generalize beyond the currently failing call-contract fixture.
- Preserve typed unavailable results for incomplete or inconsistent evidence.
- Use only supervisor-delegated build, focused, and broader proof commands.

## Ordered Steps

### Step 1: Localize the prepared-to-BIR publication divergence

Goal: identify the earliest point where available prepared publication facts
lose or change semantic successor, destination, instruction, or attribution
identity.

Actions:

- Trace prepared current-block entry readiness into
  `find_bir_block_entry_publication_identity`.
- Map successor, destination ID/name/type, PHI instruction index, and proof
  attribution to their owning facts.
- Classify missing, wrong-successor, wrong-destination, wrong-type, stale,
  duplicate, and unattributed paths before selecting a repair seam.

Completion check:

- `todo.md` records the first divergence, owning helper, authority map, and a
  general repair rule without an implementation change.

### Step 2: Establish the proof-bearing semantic query seam

Goal: establish a typed query seam that can carry exact available publication
identity through the smallest common prepared-to-BIR semantic boundary.

Actions:

- Repair the smallest common boundary at the first incorrect fact.
- Preserve exact successor, destination ID/name/type, PHI instruction index,
  and proof attribution.
- Reject incomplete or inconsistent evidence without synthesizing identity.
- Keep prepared-call and target publication behavior outside the repair.

Completion check:

- The query seam exposes exact semantic identity and typed unavailable results
  to its callers. This step is provisional infrastructure, not source-idea
  completion, until the production consumer uses the seam and ambiguity is
  owned by complete semantic identity.

### Step 3: Prove the query seam across nearby shapes

Goal: lock the semantic publication contract beyond the first call-contract
fixture.

Actions:

- Add focused positive assertions across multiple publication shapes.
- Add missing, wrong-successor, wrong-destination, wrong-type, stale,
  duplicate, and unattributed proof.
- Run the supervisor-delegated build and focused publication subset.

Completion check:

- Focused helper tests prove exact available identity and typed fail-closed
  behavior at the query seam. This does not establish acceptance while the
  production consumer and named frame/stack contract remain on the old path.

### Step 4: Wire exact identity into the production block-entry consumer

Goal: make the real block-entry publication consumer use proof-bearing BIR
identity and repair the named frame/stack contract at that same boundary.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

Actions:

- Route the production consumer through the exact query using its available
  BIR block and destination value evidence.
- Preserve exact successor, destination instruction/PHI/value, type, prepared
  instruction coordinate, and attribution; fail closed when they disagree.
- Replace the named frame/stack fixture's manual completion of publication
  readiness with producer/query-derived attributed facts.
- Do not change prepared-call policy, join/edge identity, target
  materialization, or emitted publication policy.

Completion check:

- Repository callers include the production consumer, and the named
  frame/stack contract obtains its complete publication identity through the
  repaired production/query route without manually setting the completion
  record or proof bit.

### Step 5: Move ambiguity authority to complete semantic identity

Goal: remove display-name-only duplicate classification from the adapter and
make the authoritative BIR semantic view own ambiguity.

Actions:

- Key duplicate agreement by the complete destination semantic identity,
  including the relevant type, identity, coordinate, and proof attribution.
- Do not independently rescan PHIs by display name in the prepared-to-BIR
  adapter.
- Add a nearby same-spelling/nonmatching-identity collision that remains
  available and a true semantic duplicate that remains ambiguous.

Completion check:

- Same display spelling alone cannot produce `ProofAmbiguous`; true duplicate
  semantic evidence does, and neither case selects by source order or
  proximity.

### Step 6: Run focused production and contract proof

Goal: prove the corrected semantic boundary through both the lookup coverage
and the named production-facing contract.

Actions:

- Run the supervisor-delegated build.
- Run a matching focused command that includes
  `backend_prepared_lookup_helper` and
  `backend_prepare_frame_stack_call_contract`.
- Record fresh canonical proof without reusing or mislabeling a missing log.

Completion check:

- Both focused tests are green after the production and ambiguity repairs,
  with no expectation downgrade, manual proof completion, or fixture-shaped
  recovery.

### Step 7: Run broader acceptance proof and resume parked work

Goal: establish closure-quality evidence and return to idea 719 acceptance.

Actions:

- Audit the complete diff for fixture, display-name, source-order,
  nearest-PHI, proof-bit, and target-emission shortcuts.
- Run the supervisor-selected broader backend before/after comparison with
  matching canonical `test_before.log` and `test_after.log` scope.
- Record whether idea 719 can resume its focused and broader acceptance checks
  before routing downstream idea 717 or idea 716.

Completion check:

- Focused production/contract proof and broader proof are green, reviewer
  reject signals are absent, and lifecycle routing can return to idea 719
  acceptance.
