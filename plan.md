# Phase E3 Fused-Compare Operand-Producer Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md

## Purpose

Validate the existing agreement-gated Route 7 comparison provenance path for
one selected `find_prepared_fused_compare_operand_producer_facts(...)`
helper-oracle operand-producer row family, then fill the public-boundary
fallback and nearby same-feature proof gaps while preserving prepared fallback
and output authority.

## Goal

Keep Route 7 comparison provenance limited to the existing private agreement
reader path, and prove that public AArch64 helper/caller fallback keeps current
prepared helper-oracle behavior for all non-agreement paths.

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
  `find_prepared_fused_compare_operand_producer_facts(...)`,
  `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`,
  and public AArch64 conditional branch lowering callers

## Current Targets

- One selected helper-oracle row family tied to
  `find_prepared_fused_compare_operand_producer_facts(...)`
- Existing positive Route 7 comparison provenance under prepared agreement
- Prepared fallback for absent route, invalid references, duplicate or conflict
  cases, mismatch, unfused input, non-agreement paths, and prepared-only data
- Public AArch64 helper/caller fallback proof for those non-agreement paths
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
- Step 1 inventory found that
  `src/backend/mir/aarch64/codegen/comparison.cpp` already consumes
  `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)` at
  the private AArch64 helper boundary, accepts converted Route 7 facts only
  when both sides match prepared facts, and falls back to prepared facts on
  non-agreement.
- Remaining work is proof-oriented: verify the existing private path, add or
  adjust focused public-boundary fallback coverage where missing, and prove
  nearby same-feature fused-compare behavior.

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
- Every implementation or test step needs fresh build or compile proof plus the
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

### Step 2: Validate Existing Agreement-Gated Route 7 Evidence

Goal: prove the existing private Route 7 agreement-reader path is the selected
positive helper-oracle evidence source and identify only public-boundary proof
gaps that still require tests.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`
- Existing prepared fallback helper behavior
- Existing private agreement-reader tests

Actions:

- Confirm the private AArch64 helper calls
  `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`
  before returning prepared fused-compare operand-producer facts.
- Confirm the reader converts Route 7 comparison operands to prepared facts and
  accepts them only when both converted sides match prepared facts.
- Confirm all absent route, invalid reference, duplicate/conflict, mismatch,
  unfused, non-agreement, and prepared-only cases return `std::nullopt` from
  the reader or otherwise fall back to prepared helper behavior.
- Inventory which of those fallback cases already have direct private-reader
  proof and which still need public AArch64 helper/caller proof.
- Do not add another agreement-reader implementation path unless inspection
  finds a real semantic gap in the existing one.

Completion check:

- `todo.md` records that the private agreement-reader implementation path is
  present or names the exact semantic gap if one is found.
- The public-boundary fallback and nearby same-feature proof gaps to cover in
  Step 3 are explicit.
- No code is changed unless a real semantic gap is found; no expectation,
  baseline, wrapper, branch-control, machine-printer, suffix, fused-legality,
  hazard, or output-policy rewrite is made.

### Step 3: Prove Public Fallback And Nearby Same-Feature Cases

Goal: prove that the existing agreement-gated implementation is semantic at
the public AArch64 helper/caller boundary and not shaped around one named
fused-compare testcase.

Primary targets:

- Public AArch64 conditional branch lowering tests
- Existing fused-compare helper-oracle tests
- Nearby same-feature comparison provenance and fused-compare cases
- Negative/fallback cases for absent, invalid, conflicting, mismatched,
  unfused, non-agreement, and prepared-only evidence

Actions:

- Add or adjust focused tests only where Step 2 confirms coverage is missing
  for the selected row family and fallback matrix.
- Prefer tests that mutate Route 7 evidence at the public AArch64
  helper/caller boundary and assert unchanged prepared compare operand rows.
- Cover prepared-only fallback at the public boundary when usable Route 7
  evidence is withheld or unavailable.
- Include nearby same-feature fused-compare coverage for at least one case
  beyond the select-plus-folded-constant fixture.
- Do not weaken supported-path expectations, expected strings, wrappers,
  branch-control output, machine-printer output, suffix mapping, fused
  legality, hazard checks, or emitted output.

Completion check:

- Positive agreement and every required fallback path are covered by focused
  public-boundary tests or explicit proof.
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
