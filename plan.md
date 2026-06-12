# Phase E3 Route 3 Memory/Source Stored-Value Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md

## Purpose

Augment or replace one Route 3 memory/source stored-value helper-oracle
success row with Route 3 memory/source agreement evidence while keeping
prepared diagnostic, target-addressing, wrapper, and expected-string authority
for every non-agreement path.

## Goal

Use Route 3 memory/source agreement for one stored-value helper-oracle success
row only when it matches prepared behavior, and prove prepared fallback remains
authoritative for non-memory, ambiguous, absent, invalid, mismatch, and
target-policy-sensitive cases.

## Core Rule

This plan may change one memory/source stored-value helper-oracle success row
only. Do not migrate prepared addressing printer output, address formation,
relocation, materialization, addressing legality, final operands,
target-addressing policy, wrapper output, helper-oracle strings, expected
strings, or broad Route 3 diagnostic ownership.

## Read First

- `ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Prior Route 3 boundary and fallback context:
  `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`,
  `ideas/closed/193_route3_prepared_policy_boundary_hardening.md`,
  `ideas/closed/202_route3_memory_source_identity_adapter.md`,
  and `ideas/closed/208_route3_memory_source_oracle_printer_row.md`
- Existing implementation and tests around same-block load-local stored-value
  source identity, Route 3 memory access records, prepared lookup helpers, and
  AArch64 prepared memory operand records

## Current Targets

- One stored-value helper-oracle success row for Route 3 memory/source
  agreement.
- Positive Route 3 same-block load-local stored-value or memory/source evidence
  only after agreement with prepared behavior.
- Prepared fallback for non-memory inputs, alias or address ambiguity, invalid
  or absent Route 3 evidence, mismatches, and all non-agreement paths.
- Stability proof for helper-oracle strings, wrappers, address formation,
  materialization, addressing legality, final operands, target-addressing
  behavior, and expected strings.
- Nearby same-feature Route 3 memory/source proof so progress is not tied to
  one named fixture.

## Non-Goals

- Do not replace prepared addressing printer rows or final addressing text.
- Do not move address formation, frame/global/TLS relocation, materialization,
  addressing-mode legality, final operand selection, target wrappers, or
  target-addressing policy into Route 3.
- Do not migrate broad memory/source diagnostics, memory oracle families,
  prepared lookup aggregates, public fallback APIs, or Route 3 route-addressing
  ownership.
- Do not refresh baselines, rewrite expected strings, rename helpers as proof,
  downgrade supported paths, or weaken prepared oracle authority.
- Do not start draft 155, E5, aggregate prepared retirement, broad prepared
  diagnostic/oracle replacement, or wrapper migration.

## Working Model

- Route 3 memory/source records can provide semantic identity for selected
  memory/source facts, including same-block load-local stored-value source
  relationships.
- Prepared memory/source helpers and prepared memory/frame/stack oracle rows
  remain the authority for fallback, target-addressing, final operands,
  printer/debug strings, wrappers, and expected output.
- A valid Route 3 helper-oracle row must fail closed unless Route 3 evidence
  agrees with the prepared stored-value source behavior for the selected row.
- Non-memory, ambiguous alias/address, absent, invalid, mismatch, and
  non-agreement cases must preserve the same prepared diagnostic status and
  output as before.

## Execution Rules

- Start by locating the exact stored-value helper-oracle row and the tests that
  currently own its positive and fallback behavior.
- Treat Route 3 as evidence for semantic memory/source agreement only; prepared
  data remains authoritative for target-specific memory operands and output.
- Prefer existing Route 3 memory/source record queries and agreement checks.
  Do not add testcase-shaped matching, fixture-name shortcuts, or row-text
  matching as capability work.
- Keep every helper-oracle string, wrapper, expected string, prepared printer
  row, and final operand byte-stable unless the supervisor explicitly approves
  a separate source-intent change.
- Every implementation or test step needs fresh build or compile proof plus the
  delegated test subset. Escalate validation before acceptance if the diff
  touches shared memory lowering, prepared lookup helpers, wrapper output,
  expected strings, or target-addressing surfaces.

## Steps

### Step 1: Locate The Stored-Value Helper-Oracle Row And Proof Surface

Goal: identify the exact helper-oracle success row, its current prepared
behavior, the available Route 3 memory/source agreement evidence, and the
positive/fallback tests that must remain stable.

Primary targets:

- Stored-value helper-oracle assertions in `backend_prepared_lookup_helper`
  coverage
- Same-block load-local stored-value source helpers and Route 3 memory/source
  identity readers
- Route 3 memory access record tests and AArch64 prepared memory operand record
  tests that cover nearby same-feature behavior
- Prepared addressing printer, wrapper, final-operand, and expected-string
  surfaces that must remain unchanged

Actions:

- Inspect the helper-oracle row and name the exact implementation and test
  surfaces in `todo.md`.
- Map the row's prepared success behavior to the Route 3 memory/source evidence
  that can prove the same stored-value source under agreement.
- Inventory positive, non-memory, alias/address ambiguity, absent, invalid,
  mismatch, non-agreement, and prepared fallback coverage.
- Identify any nearby same-feature memory/source cases needed to prove this is
  not a named-fixture shortcut.

Completion check:

- `todo.md` names the selected row, the Route 3 evidence path, the prepared
  fallback authority, the target files, and the positive/fallback proof gaps
  for Step 2.
- No code, baseline, expected-string, wrapper, addressing-printer,
  materialization, addressing-legality, final-operand, or target-policy change
  is made unless the executor is explicitly delegated implementation work.

### Step 2: Validate Route 3 Agreement And Fail-Closed Boundaries

Goal: prove the selected row can use Route 3 memory/source evidence only after
agreement, and define the exact fallback matrix before any behavior change.

Primary targets:

- Route 3 memory/source record or identity reader used by the selected row
- Prepared same-block load-local stored-value source behavior
- Prepared memory/frame/stack helper-oracle fallback statuses
- Existing positive and negative tests for Route 3 memory/source records

Actions:

- Confirm Route 3 evidence identifies the same stored-value source as prepared
  for the selected positive row.
- Confirm absent, invalid, ambiguous, mismatched, non-memory, and
  non-agreement Route 3 evidence fails closed to prepared behavior.
- Confirm prepared target-addressing, materialization, addressing legality, and
  final operand behavior are not read from or overridden by Route 3.
- Record whether implementation already has the required agreement gate or
  name the exact semantic gap to fix.

Completion check:

- `todo.md` records the agreement gate, fallback matrix, and any semantic gap
  that Step 3 must address.
- Helper-oracle strings, prepared printer rows, wrappers, expected strings,
  address formation, materialization, addressing legality, and final operands
  remain unchanged.

### Step 3: Wire Or Prove The Selected Helper-Oracle Row

Goal: make the selected helper-oracle success row use Route 3 memory/source
agreement evidence, or prove the existing implementation already does so, while
preserving prepared fallback for all non-agreement paths.

Primary targets:

- The selected helper-oracle row implementation
- Route 3 memory/source agreement reader or query
- Focused positive and fallback tests for the selected row
- Nearby same-feature Route 3 memory/source tests

Actions:

- If Step 2 found a real semantic gap, add the narrow agreement-gated Route 3
  read for the selected row.
- Preserve prepared fallback whenever Route 3 evidence is absent, invalid,
  ambiguous, mismatched, non-memory, or not in agreement.
- Add or adjust focused tests only where Step 2 found proof gaps.
- Include nearby same-feature memory/source coverage beyond the selected
  success fixture.
- Do not change row text, helper names, wrappers, prepared printer output,
  expected strings, baselines, supported-path contracts, address formation,
  materialization, addressing legality, final operands, or target policy.

Completion check:

- The selected success row is explained by Route 3 memory/source agreement.
- Required fallback cases retain prepared authority and byte-stable output.
- Nearby same-feature proof shows the change is semantic rather than
  testcase-shaped.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed stored-value helper-oracle row slice and leave clear
handoff state for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact delegated proof command and record the command and result in
  `todo.md`.
- Summarize the selected row, Route 3 agreement evidence, fallback guarantees,
  unchanged output surfaces, and nearby same-feature proof.
- Ask the supervisor to choose broader validation before acceptance if the
  diff touched shared memory/source lowering, prepared lookup helpers,
  wrapper output, expected strings, or target-addressing surfaces.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  prepared authority, unchanged output surfaces, and residual risks.
