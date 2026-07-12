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

### Step 6.1: Restore conservative production compatibility

Goal: prevent the surviving production pointer overload from converting
name-scanned evidence into a synthetic available claim or weakening the prior
duplicate fail-closed behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir_route4_publication.cpp`

Actions:

- Trace the non-test pointer-overload caller through the legacy Route4 record
  bridge.
- Prefer routing an explicit attributed claim collection from production when
  the existing model can supply it without expanding this idea's scope.
- Otherwise keep the compatibility overload conservative: evidence recovered
  only by display name/type must not become an available synthetic claim, and
  same-name/same-type duplicates must continue to fail closed.
- Do not add a new name-, order-, fixture-, or proximity-shaped availability
  path.

Completion check:

- The production caller cannot report availability from name-based recovery,
  preserves no-downgrade fail-closed behavior, and the delegated build plus
  focused compatibility proof is green.

### Step 6.2: Complete Route4 instruction-coordinate authority

Goal: make Route4 validate every modeled owner/instruction coordinate before
classifying a claim as available.

Actions:

- Validate `instruction_owner_label_id` against the exact instruction owner
  and destination identity alongside the existing owner pointer, instruction
  pointer, index bound, and pointer-at-index checks.
- Return the precise typed `Stale` result at the Route4 boundary for a stale
  owner label coordinate; do not defer that fact to MIR as `ProofMismatch`.
- Add a focused row that changes only the owner-label coordinate while keeping
  owner pointer, instruction pointer, and index exact.

Completion check:

- Route4 alone classifies the isolated stale-label row as `Stale`, with fresh
  build and focused proof.

### Step 6.3: Prove independent same-name destination availability

Goal: prove that exact attributed identity keeps two same-name/same-type
destinations independently available.

Actions:

- Construct independently selected claim collections for each exact
  same-spelling destination.
- Assert each collection classifies as `Available` for its own destination.
- Retain mismatched-destination rejection as a separate contract rather than
  using it as the positive identity proof.

Completion check:

- Both exact destinations independently classify as `Available`, while the
  mismatched destination remains rejected, under the focused subset.

### Step 6.4: Prove production-overload no-downgrade behavior

Goal: make the actual production compatibility boundary demonstrate that the
repair did not weaken unavailable or ambiguous behavior.

Actions:

- Exercise the production pointer overload, not only a directly constructed
  Route4 classification.
- Cover same-name/same-type duplicate evidence and any name-only compatibility
  input affected by Step 6.1.
- Assert the boundary fails closed and does not manufacture one attributed
  available claim.

Completion check:

- A focused production-overload row would fail against the rejected bridge and
  passes with no expectation downgrade or unsupported reclassification.

### Step 6.5: Run fresh focused acceptance and handback audit

Goal: establish reproducible acceptance for the repaired decomposition seam
before making idea 718 resumable.

Actions:

- Run the supervisor-delegated build and focused internal backend BIR subset,
  recording the exact command in canonical `test_after.log`.
- Confirm the proof covers conservative production compatibility, complete
  Route4 owner-label coordinate validation, independently available same-name
  destinations, and production-overload no-downgrade behavior.
- Audit the complete idea 720 diff against its reviewer reject signals.
- Record the exact resulting seam and remaining idea 718 production/broader
  acceptance work.

Completion check:

- Fresh canonical focused proof is green, the handback audit finds no
  expectation downgrade or heuristic identity recovery, and only then may the
  lifecycle return to idea 718.
