# Repeated Stack-Destination Fan-In Order Authority Runbook

Status: Active
Source Idea: ideas/open/622_repeated_stack_destination_fan_in_order_authority.md

## Purpose

Make repeated stack-destination move-bundle handling authority-driven instead
of inferred by the RV64 consumer.

Goal: publish and consume explicit prepared/prealloc authority for legal
repeated stack-destination fan-in or ordering while preserving fail-closed
behavior for ambiguous rows.

## Core Rule

RV64 must not choose destination order, mutual exclusion, or last-writer
semantics for repeated stack destinations unless prepared/prealloc has
published explicit authority facts that justify the move-bundle shape.

## Read First

- `ideas/open/622_repeated_stack_destination_fan_in_order_authority.md`
- `docs/destination_fan_in_authority/`
- Prior closed context when needed:
  - `ideas/closed/607_destination_fan_in_authority_research.md`
  - `ideas/closed/610_rv64_move_bundle_target_materialization.md`

## Current Targets

- Repeated stack-destination move-bundle residuals, including `src/pr71631.c`.
- Prepared/prealloc authority production for legal repeated stack destinations.
- RV64 prepared move-bundle consumption of only explicit supported authority.
- Focused positive and negative backend tests for accepted and rejected
  authority shapes.

## Non-Goals

- Do not reopen direct register-to-register move-bundle materialization from
  idea 610.
- Do not infer ordering, exclusivity, or last-writer behavior in RV64.
- Do not solve select-publication wiring, ABI/call-boundary moves, generic
  before-instruction fragments, terminator lowering, global data, runtime
  mismatch, expectations, unsupported markers, allowlists, timeout policy, or
  accounting.
- Do not add filename-, function-, block-, value-id-, offset-, or row-count
  special cases for `src/pr71631.c`.

## Working Model

- Prepared/prealloc is the authority layer for destination fan-in and ordering
  facts.
- RV64 is a consumer of those facts. It may lower repeated stack-destination
  bundles only when the published authority shape is explicit and supported.
- Missing, unknown, ambiguous, malformed, or unsupported authority remains a
  specific fail-closed diagnostic.

## Execution Rules

- Keep each packet semantic: capability progress must come from authority
  publication or authority-gated consumption, not from expectation rewrites.
- Preserve existing fail-closed diagnostics unless a more precise diagnostic is
  introduced for the same blocker.
- Add negative coverage for each accepted authority shape so malformed or
  ambiguous variants still reject.
- For code-changing steps, prove with a fresh build or compile proof plus the
  supervisor-delegated test subset. Broader matching before/after backend
  regression logs are required before closure.

## Step 1: Classify Repeated Stack-Destination Residuals

Goal: identify the concrete repeated stack-destination row families that block
current RV64 lowering.

Actions:

- Inspect the current diagnostics and object evidence for repeated
  stack-destination move-bundle rows, including `src/pr71631.c`.
- Record which facts are available today: destination stack slot, source homes,
  row owner, candidate order, and current rejection reason.
- Separate legal-looking bundles from bundles that must remain ambiguous or
  malformed until upstream authority exists.

Completion check:

- `todo.md` names the residual families examined and the exact blocker each
  family currently reports.
- The next implementation target is narrowed to one authority fact family, not
  to a single testcase shape.

## Step 2: Publish Prepared/Prealloc Destination Authority Facts

Goal: make legal repeated stack-destination bundles carry explicit authority
facts before RV64 sees them.

Primary target: prepared/prealloc move-bundle authority publication.

Actions:

- Locate the producer responsible for move-bundle destination metadata.
- Add or extend a structured authority fact for repeated stack destinations
  that includes authority kind, owner, destination stack slot, source homes, and
  ordering or fan-in semantics.
- Emit precise missing-authority reasons when the producer cannot prove legal
  destination fan-in or order.
- Keep unsupported or ambiguous bundles fail-closed.

Completion check:

- Prepared diagnostics or object evidence expose the new authority fact for at
  least one legal repeated stack-destination bundle.
- Missing or ambiguous authority produces a specific reason without changing
  RV64 acceptance yet.
- The delegated proof command passes and updates `test_after.log` if requested.

## Step 3: Consume Authority In RV64 Prepared Move Bundles

Goal: allow RV64 lowering only for explicit, supported repeated
stack-destination authority shapes.

Primary target: RV64 prepared move-bundle consumer and diagnostics.

Actions:

- Thread the prepared/prealloc authority facts into the RV64 prepared
  move-bundle path.
- Accept only supported authority kinds and reject missing, unknown, ambiguous,
  malformed, or unsupported facts with precise diagnostics.
- Ensure RV64 does not synthesize destination ordering from row order,
  encodability, offset repetition, or testcase identity.

Completion check:

- At least one legal repeated stack-destination bundle lowers through RV64 via
  explicit authority.
- Ambiguous repeated stack-destination bundles still fail closed with a
  specific diagnostic.
- `src/pr71631.c` is rerun and either advances past the repeated
  stack-destination blocker or reports the exact upstream authority still
  missing.

## Step 4: Add Focused Positive And Negative Backend Coverage

Goal: lock in supported and rejected authority shapes without overfitting.

Actions:

- Add focused positive coverage for a legal authority-backed repeated
  stack-destination bundle.
- Add negative coverage for missing, unknown, ambiguous, malformed, or
  unsupported authority shapes touched by the implementation.
- Avoid weakening supported-path expectations or downgrading tests to
  unsupported without explicit supervisor approval.

Completion check:

- Positive coverage proves authority-backed lowering.
- Negative coverage proves fail-closed behavior for rejected authority shapes.
- The tests are feature-shaped rather than named-case-shaped.

## Step 5: Backend Regression And Closure Readiness

Goal: prove the backend scope and decide whether the source idea is complete.

Actions:

- Run the supervisor-selected backend regression scope with matching
  `test_before.log` and `test_after.log`.
- Confirm diagnostics or object evidence show authority kind, owner,
  destination stack slot, source homes, and rejection reason when authority is
  absent.
- Compare implementation against the source idea's reviewer reject signals.

Completion check:

- Matching before/after backend regression logs pass for the touched scope.
- No testcase-overfit reject signal is present.
- The plan owner can decide whether the active runbook and source idea are
  ready to close.
