# Block-Entry Publication Proof-Evidence Multiplicity Runbook

Status: Active
Source Idea: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Activated after parking: ideas/open/718_block_entry_publication_identity_completion.md

## Purpose

Decompose the proof-evidence contradiction that blocked idea 718 Step 5 into
explicit identity, claim, collection, and classification contracts.

## Goal

Give authoritative BIR Route4 enough modeled evidence to distinguish zero,
one, inconsistent, and duplicate attributed publication claims.

## Core Rule

Destination semantic identity and individual proof-claim identity are separate
facts. Do not infer either from display name, source order, nearest PHI, or
target-emission behavior.

## Read First

- `ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md`
- `review/idea718_step5_ambiguity_acceptance_review.md`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.h`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Scope

- Destination semantic identity versus attributed proof-claim identity.
- An explicit claim collection and typed cardinality/agreement result.
- Authoritative Route4 classification.
- MIR attribution and prepared-coordinate validation against that result.
- Focused internal backend BIR proof for exact identity and multiplicity.

## Non-Goals

- Do not complete idea 718's production or broader acceptance work here.
- Do not change prepared-call, join/edge, ABI, target materialization, storage,
  or emission policy.
- Do not use source-program cases for state that only internal C++ query probes
  can express directly.
- Do not broaden the redesign to unrelated BIR routes or query families.

## Working Model

- Destination identity answers which semantic publication destination a claim
  concerns.
- Claim identity answers which attributed prepared proof and coordinate makes
  that assertion.
- A collection preserves independently represented claims before agreement or
  ambiguity classification.
- The authoritative result classifies cardinality and disagreement; MIR only
  validates its prepared attribution/coordinate against that result.

## Execution Rules

- Record the current authority map and truth table before implementation.
- Preserve typed unavailable and ambiguous outcomes.
- Add one focused internal probe per contract; do not encode testcase-shaped
  exceptions.
- Run only the supervisor-delegated build and proof commands.
- Treat the switch as route correction, not backend capability progress.

## Ordered Steps

### Step 1: Establish the blocked baseline and modeling truth table

Goal: make the representational contradiction and required result space
explicit before selecting storage or helper changes.

Actions:

- Trace the current prepared attribution, instruction coordinate, destination
  identity, Route4 scan, and MIR validation ownership.
- Record how current code represents zero, one, inconsistent, and duplicate
  claims and where information is missing or collapsed.
- Define a truth table for same-name/same-type distinct identity, true duplicate
  claims, stale coordinate, and missing attribution.
- Identify the smallest shared model boundary that can carry every required
  fact without a MIR rescan.

Completion check:

- `todo.md` records the authority map, blocked baseline, truth table, and one
  proposed generic model boundary without an implementation change.

### Step 2: Define destination, claim, collection, and result contracts

Goal: introduce a typed model that preserves semantic destination identity and
independent attributed claims until authoritative classification.

Actions:

- Define destination semantic identity separately from claim attribution and
  prepared instruction coordinate.
- Represent a collection capable of zero, one, inconsistent, and duplicate
  claims without early deduplication.
- Define precise typed results for available, missing, stale, inconsistent,
  ambiguous, and unattributed evidence.
- Keep unrelated prepared and BIR query contracts unchanged.

Completion check:

- The common seam can represent every truth-table row without display-name or
  source-order recovery, with fresh build proof.

### Step 3: Add focused internal proof rows

Goal: make the model's decisive identity and multiplicity distinctions directly
observable in backend BIR tests.

Actions:

- Add focused internal rows for same-name/same-type distinct destinations, true
  duplicate attributed claims, stale coordinate, and missing attribution.
- Keep each row responsible for one primary contract.
- Explain in test structure or nearby documentation why source-program cases
  cannot express the internal claim collection directly.

Completion check:

- The focused rows distinguish all required result states and fail against the
  old name-plus-type classification rather than merely restating it.

### Step 4: Make Route4 the authoritative classifier

Goal: classify the explicit claim collection in BIR without heuristic identity
recovery.

Actions:

- Route exact destination and attributed claim evidence into Route4.
- Classify zero, one, inconsistent, and duplicate claims from the explicit
  model.
- Remove name-plus-type duplicate authority and avoid source-order, nearest-PHI,
  and target-emission selection.

Completion check:

- Route4 returns the expected typed result for every focused truth-table row,
  and no earlier collapse makes true duplicates unrepresentable.

### Step 5: Bind MIR validation to the authoritative result

Goal: validate prepared attribution and coordinate agreement without creating
a second ambiguity authority.

Actions:

- Consume Route4's authoritative result in the MIR adapter.
- Validate exact prepared attribution and instruction coordinate against the
  selected claim when available.
- Fail closed on stale, missing, inconsistent, or ambiguous evidence.
- Remove or avoid any competing MIR PHI rescan.

Completion check:

- MIR agrees with the authoritative result for every focused row and performs
  no display-name/source-order recovery.

### Step 6: Prove the decomposition seam and hand back to idea 718

Goal: establish focused acceptance for the new model and make the parked
production route resumable.

Actions:

- Run the supervisor-delegated build and focused internal backend BIR subset.
- Audit the diff against the source idea's reviewer reject signals.
- Record the exact resulting seam and remaining idea 718 production/broader
  acceptance work.

Completion check:

- Focused proof is green with no expectation downgrade or heuristic recovery,
  reviewer reject signals are absent, and lifecycle can return to idea 718.
