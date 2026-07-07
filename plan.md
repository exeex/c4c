# RV64 Stack-Destination Move-Bundle Authority Contract Runbook

Status: Active
Source Idea: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Activated from: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md

## Purpose

Define and implement a narrow authority contract for prepared move bundles that
fan multiple register sources into one stack destination, then teach the RV64
prepared-object consumer to accept only supported authority-backed fan-in.

## Goal

Move RV64 stack-destination multi-source fan-in from a fail-closed diagnostic
gap into an explicit prepared authority contract with positive and negative
coverage.

## Core Rule

RV64 must consume prepared authority facts. It must not infer fan-in legality
from source order, value ids, function names, block names, filenames, emitted
offsets, or diagnostic strings.

## Read First

- Source idea:
  `ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md`
- Prior closed route:
  `ideas/closed/579_rv64_prepared_stack_destination_move_bundle_authority.md`
- Existing diagnostic to preserve as the missing-authority blocker:
  `producer_authority_missing_for_register_fan_in_stack_destination`
- Existing authority surfaces to inspect before editing:
  `PreparedMoveAuthorityKind`, `PreparedMoveBundle`,
  `PreparedMoveResolution`, prepared move-bundle classifiers, and RV64
  prepared-object move materialization helpers.

## Current Scope

- Prepared/prealloc move-bundle authority taxonomy for stack-destination
  multi-source fan-in.
- Focused diagnostics or prepared object evidence recording:
  destination stack slot/value id, source value ids and homes, producer owner,
  authority kind, and missing-authority reason.
- RV64 prepared-object consumer behavior for supported authority-backed
  stack-destination fan-in.
- Focused tests for one supported legal shape and one missing/unknown authority
  shape.
- Representative rerun of `tests/c/external/gcc_torture/src/20000605-1.c`
  after the authority contract is implemented.

## Non-Goals

- Do not weaken, bypass, or disable the ambiguous move-bundle classifier.
- Do not reconstruct producer authority inside RV64 from CFG shape, source
  order, ids, names, diagnostics, or instruction offsets.
- Do not add filename-, function-, block-, or value-id-specific handling for
  `src/20000605-1.c`, `render_image_rgb_a`, `for.cond.2`, `value_id=22`,
  `value_id=23`, or `value_id=24`.
- Do not rewrite expectations, unsupported markers, allowlists, route
  classification, or diagnostics as a substitute for capability repair.
- Do not expand into select, call, pointer arithmetic, floating-point,
  variadic helper, F128, or runtime comparison repair unless this authority
  contract is satisfied and focused evidence identifies the next owner.

## Working Model

- Missing or unknown authority is a first-class state and remains fail-closed.
- At least one legal authority kind must be represented and supported, such as
  equivalent carrier, exclusive control flow, ordered copy, or parallel copy.
- A multi-source register-to-one-stack-destination bundle is accepted only when
  the prepared producer publishes a supported authority kind plus enough owner
  facts for the RV64 consumer to trust it.
- Genuinely ambiguous stack-destination bundles continue to fail closed with a
  specific diagnostic.

## Execution Rules

- Keep each code-changing packet behavior-focused and test-backed.
- Prefer adding contract fields or helpers where prepared move-bundle authority
  is produced and classified; keep RV64 as a consumer of those facts.
- Preserve existing fail-closed behavior before adding positive acceptance.
- For each packet, run a fresh build or compile proof plus the supervisor's
  delegated narrow test command.
- Escalate to broader backend validation when both producer taxonomy and RV64
  consumption have changed.

## Ordered Steps

### Step 1: Map The Existing Authority Surface

Goal: Identify the smallest existing prepared/prealloc and RV64 surfaces that
own move-bundle authority, diagnostics, and stack-destination materialization.

Primary targets:
- `src/backend/prealloc/`
- RV64 prepared-object emission code under `src/backend/`
- focused backend tests that already exercise prepared move bundles and RV64
  stack-destination edge publication

Actions:
- Inspect `PreparedMoveAuthorityKind`, move-bundle storage, diagnostic/status
  enums, and RV64 prepared move consumption.
- Locate where the current
  `producer_authority_missing_for_register_fan_in_stack_destination` diagnostic
  is produced.
- Identify the focused positive and negative test files to extend.
- Record in `todo.md` the chosen producer-side owner, RV64 consumer surface,
  test targets, and delegated proof command result.

Completion check:
- The next executor can name the exact files/functions to edit for taxonomy,
  diagnostics, RV64 consumption, and tests without redoing broad discovery.

### Step 2: Define The Prepared Authority Taxonomy

Goal: Add the minimal explicit authority taxonomy and evidence fields needed
for stack-destination multi-source fan-in.

Actions:
- Represent missing/unknown authority and one supported legal authority kind.
- Ensure prepared evidence can carry destination stack slot/value id, source
  value ids and homes, producer owner, authority kind, and missing-authority
  reason.
- Keep unknown or unsupported authority status distinct from supported legal
  authority.
- Add focused contract-level tests for taxonomy spelling and fail-closed
  missing authority.

Completion check:
- Prepared/prealloc can describe stack-destination fan-in authority without
  RV64 guessing and without accepting unknown authority.

### Step 3: Publish Legal Producer Authority

Goal: Teach the prepared producer/classifier to attach the supported authority
kind only for the legal fan-in shape chosen in Step 1 and Step 2.

Actions:
- Populate the supported authority kind at the producer or classifier surface
  that owns the semantic proof.
- Preserve missing-authority diagnostics for ambiguous or unsupported shapes.
- Add positive producer-side coverage for the legal fan-in shape and negative
  coverage for genuinely ambiguous or missing authority.

Completion check:
- Tests prove both explicit legal authority publication and fail-closed
  missing/unknown authority before RV64 acceptance is broadened.

### Step 4: Consume Authority In RV64

Goal: Let the RV64 prepared-object consumer materialize only supported
authority-backed stack-destination fan-in.

Actions:
- Make RV64 read the prepared authority facts and owner evidence from the
  prepared object surface.
- Emit the supported register-source to stack-destination moves only when the
  authority kind is supported.
- Keep unknown, missing, contradictory, or genuinely ambiguous authority
  fail-closed with a specific diagnostic.
- Add RV64 positive and negative tests that remove or alter authority facts to
  prove there is no local rediscovery.

Completion check:
- RV64 accepts the supported authority-backed fan-in shape and rejects the same
  shape when authority is absent, unknown, or unsupported.

### Step 5: Rerun The 579 Representative Route

Goal: Determine whether `src/20000605-1.c` advances past the current
stack-destination fan-in blocker or remains blocked by a named upstream
authority owner.

Actions:
- Rerun the representative route for
  `tests/c/external/gcc_torture/src/20000605-1.c`.
- If the fan-in is legal, record that it advances past
  `producer_authority_missing_for_register_fan_in_stack_destination`.
- If it is still unsupported, record the exact producer authority still missing
  and the upstream owner.
- Do not patch around the representative by name or by value id.

Completion check:
- `todo.md` records the representative result, artifacts, and whether the next
  owner is RV64, prepared/prealloc producer authority, or a separate idea.

### Step 6: Broader Backend Regression Guard

Goal: Prove the touched prepared/RV64 object-emission bucket did not regress.

Actions:
- Run the supervisor-selected matching before/after regression subset or full
  backend validation command.
- Ensure canonical `test_before.log` and `test_after.log` reflect the matched
  command when regression guard is requested.
- Confirm existing genuinely ambiguous stack-destination bundles still fail
  closed.

Completion check:
- Matching backend proof passes, and no expectation downgrade or testcase
  overfit is present.
