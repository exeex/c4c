# Phase E3 Fused-Compare Operand-Producer Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md

## Purpose

Augment or replace one selected
`find_prepared_fused_compare_operand_producer_facts(...)` helper-oracle
operand-producer row family with Route 7 comparison provenance evidence while
preserving prepared fallback and output authority.

## Goal

Use Route 7 comparison provenance only when the private Route 7 agreement
reader agrees with prepared fused-compare operand-producer data, and prove that
all non-agreement paths retain current prepared helper-oracle behavior.

## Core Rule

This plan may change one fused-compare operand-producer helper-oracle row
family only. Do not rewrite suffix mapping, fused legality, hazard checks,
branch-control output, machine-printer output, wrappers, expected strings,
target policy, emitted-output policy, or broad prepared diagnostic/oracle
ownership.

## Read First

- `ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `ideas/closed/227_phase_e3_branch_target_helper_oracle_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- Existing implementation and tests around:
  `find_prepared_fused_compare_operand_producer_facts(...)` and the private
  Route 7 fused-compare comparison provenance agreement reader

## Current Targets

- One selected helper-oracle row family tied to
  `find_prepared_fused_compare_operand_producer_facts(...)`
- Positive Route 7 comparison provenance under prepared agreement
- Prepared fallback for absent route, invalid references, duplicate or conflict
  cases, mismatch, unfused input, non-agreement paths, and prepared-only data
- Nearby same-feature fused-compare tests that prove this is not shaped around
  one named fixture

## Non-Goals

- Do not replace broad Route 7 comparison diagnostics, aggregate route views,
  branch-control output, machine-printer output, wrappers, suffix mapping,
  fused legality, hazard checks, or final assembler rows.
- Do not move branch policy, target policy, wrapper policy, emitted-output
  authority, or expected-output policy.
- Do not refresh baselines, rewrite expected strings, rename helpers as proof,
  downgrade supported paths, or weaken helper-oracle authority.
- Do not start draft 155, E5, aggregate prepared retirement, broad prepared
  diagnostic/oracle replacement, or broad route facade migration.

## Working Model

- Phase E1 proved Route 7/prepared agreement for the selected comparison
  provenance identity read consumed by AArch64 comparison lowering.
- Phase E2 extracted the private Route 7 agreement reader for the selected
  fused-compare operand-producer fact.
- Phase E3 classified the matching helper-oracle positive row family as ready
  while retaining public prepared fallback, branch-control, machine-printer,
  wrapper, and expected-string authority.
- The implementation should consume the private Route 7 agreement reader for
  the positive row and fail closed to the existing prepared result everywhere
  else.

## Execution Rules

- Start by identifying the exact helper-oracle row family and its existing
  tests before editing code.
- Keep each code change behavior-preserving except for the selected diagnostic
  evidence source under proven agreement.
- Treat prepared output as authoritative whenever Route 7 evidence is absent,
  invalid, duplicate/conflicting, mismatched, unfused, not in agreement, or not
  available through the private agreement reader.
- Prefer semantic use of the private Route 7 agreement reader; do not add
  testcase-shaped matching or fixture-name shortcuts.
- Every implementation step needs fresh build or compile proof plus the
  delegated test subset. Escalate to broader validation before acceptance if
  the diff touches shared comparison lowering, branch-control, machine-printer,
  wrappers, or expected-output surfaces.

## Steps

### Step 1: Locate The Row Family And Proof Surface

Goal: identify the selected fused-compare operand-producer helper-oracle row
family, its current prepared-only behavior, and the tests that prove positive
and fallback cases.

Primary targets:

- `find_prepared_fused_compare_operand_producer_facts(...)`
- The private Route 7 fused-compare comparison provenance agreement reader
- Existing fused-compare helper-oracle, comparison provenance, branch-control,
  machine-printer, and nearby same-feature tests

Actions:

- Inspect the helper implementation and current callers to find the precise
  helper-oracle row family eligible for this plan.
- Map the positive prepared row to the available Route 7 comparison provenance
  evidence from the private agreement reader.
- Inventory current positive, absent-route, invalid-reference,
  duplicate/conflict, mismatch, unfused, non-agreement, prepared-only fallback,
  and public fallback tests.
- Record any proof gaps in `todo.md` before implementation begins.

Completion check:

- `todo.md` names the exact row family, implementation target files, private
  Route 7 reader, and positive/fallback test coverage to use for Step 2.
- No code, baseline, expected-string, wrapper, branch-control, machine-printer,
  suffix, fused-legality, hazard, or output-policy change is made in this step
  unless the executor is explicitly delegated to start implementation.

### Step 2: Add Agreement-Gated Route 7 Evidence

Goal: route the selected positive helper-oracle row through Route 7 comparison
provenance only after prepared agreement.

Primary targets:

- The helper-oracle row family found in Step 1
- The private Route 7 fused-compare comparison provenance agreement reader
- Existing prepared fallback helper behavior

Actions:

- Thread or consume the private Route 7 agreement reader at the selected
  helper-oracle boundary using existing local patterns.
- Use Route 7 comparison provenance for the positive agreement path.
- Preserve the existing prepared helper result for absent route, invalid
  references, duplicates/conflicts, mismatches, unfused input, non-agreement,
  and prepared-only fallback.
- Keep public prepared helper fallback available and byte-stable.

Completion check:

- The selected positive row can be explained by Route 7 comparison provenance
  under prepared agreement.
- All non-agreement paths retain the same prepared helper-oracle authority and
  externally visible behavior.
- The slice builds and the delegated narrow proof passes without expectation or
  baseline rewrites.

### Step 3: Cover Fallback And Nearby Same-Feature Cases

Goal: prove that the implementation is semantic and not shaped around one
named fused-compare testcase.

Primary targets:

- Existing fused-compare helper-oracle tests
- Nearby same-feature comparison provenance and fused-compare cases
- Negative/fallback cases for absent, invalid, conflicting, mismatched,
  unfused, non-agreement, and prepared-only evidence

Actions:

- Add or adjust focused tests only where coverage is missing for the selected
  row family and fallback matrix.
- Prefer tests that assert unchanged helper-oracle behavior while exercising
  the new Route 7 agreement path.
- Do not weaken supported-path expectations, expected strings, wrappers,
  branch-control output, machine-printer output, suffix mapping, fused
  legality, hazard checks, or emitted output.

Completion check:

- Positive agreement and every required fallback path are covered by focused
  tests or explicit proof.
- Nearby same-feature coverage demonstrates the route is not a named-case
  shortcut.
- Expected strings, baselines, wrappers, branch-control output,
  machine-printer output, suffix mapping, fused legality, hazard checks, and
  emitted-output policy remain unchanged.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed helper-oracle row slice and leave clear handoff state
for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact delegated proof command and record the command and result in
  `todo.md`.
- If the change touched shared comparison lowering, branch-control,
  machine-printer, wrapper, expected string, or output-policy surfaces, ask the
  supervisor to choose broader validation before acceptance.
- Summarize the row family changed, fallback guarantees retained, and tests
  covering nearby same-feature behavior.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  authority, and any residual risks.
