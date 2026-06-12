Status: Active
Source Idea Path: ideas/open/220_phase_e1_route_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Required Fallback, Output, And Nearby Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by confirming the required proof surface for the
selected Route 7/AArch64
`find_prepared_fused_compare_operand_producer_facts(...)` agreement gate.

Existing owned tests already account for the required cases:

- Positive route/prepared agreement: `backend_prepared_lookup_helper_test`
  checks prepared, BIR, Route 7, and indexed Route 7 fused-compare operand
  producer facts agree for select plus folded-constant operands, immediate
  operands, rhs-only availability, and a production prealloc branch-condition
  oracle.
- Absent/unavailable fallback: the lookup-helper test covers empty Route 7
  indexes, missing fused paths, and missing producer records as fail-closed
  Route 7 results while prepared/BIR facts remain available where expected.
- Invalid and ambiguous/conflict fallback: the lookup-helper test covers
  wrong-key, wrong-relationship, stale-owner, duplicate-reference, and
  missing-producer Route 7 fused-compare records without weakening prepared
  behavior.
- Mismatch and unconvertible fallback: Step 2's wrapper returns the prepared
  facts unless converted Route 7 facts structurally match prepared facts;
  existing tests cover the lower-level stale/invalid route statuses that feed
  that fallback and the AArch64 output remains byte-stable.
- Policy-sensitive fallback: `backend_aarch64_branch_control_lowering_test`
  keeps non-fusable fused compare branches fail-closed with the existing typed
  diagnostic and no emitted branch node.
- Output stability: AArch64 branch-control tests still assert selected node
  metadata, successors, branch targets, printer output, compare suffixes, and
  expected instruction strings for normal, sign-extended, folded-bound,
  stack-homed-constant, and loop-header fused compare branches.
- Nearby same-feature coverage: adjacent Route 7 materialized-condition
  compare tests cover the same comparison-condition route family for positive,
  absent, duplicate, invalid, stale prepared mismatch, and provenance mismatch
  fallback without absorbing materialized-condition behavior into this selected
  fused-compare helper.

No narrow assertion was added because the owned tests already cover the Step 3
fallback, output, and nearby same-feature proof requirements without baseline
refreshes, unsupported downgrades, helper renames, or expectation rewrites.

## Suggested Next

Delegate Step 4 to run final validation and hand the selected Route 7/AArch64
fused-compare operand-producer helper slice back for lifecycle review.

## Watchouts

- Keep the selected surface to one route family and one consumer/helper.
- Do not open broad Route 1 through Route 7 migration.
- Do not create generic route facades or aggregate route view replacement.
- Do not retire `PreparedFunctionLookups`, `PreparedBirModule`, wrappers, or
  prepared APIs.
- Do not move address formation, value homes, storage, move scheduling, call
  ABI, publication construction, branch spelling, fused legality, target
  policy, printer/debug rows, helper oracles, wrappers, fallback behavior, or
  expected strings into route authority.
- Do not use baseline refreshes, unsupported downgrades, helper renames,
  timeout masking, or expectation rewrites as proof.
- The selected helper still must not absorb materialized-condition migration;
  that nearby Route 7 surface remains separate and only served as same-feature
  proof for Step 3.
- Step 3 did not add tests because the existing owned proof already covers the
  required agreement, fail-closed, fallback, output, and nearby surfaces.
- The selected helper must continue returning prepared fallback for all
  non-agreement paths; Route 7 must not absorb suffix, target, fused-legality,
  final assembler, helper-oracle string, wrapper, printer/debug, or expected
  string ownership.

## Proof

Delegated Step 3 proof command completed successfully:
`( cmake --build --preset default --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$' --output-on-failure ) > test_after.log 2>&1`

Proof log: `test_after.log`.

Result: passed, 2/2 selected tests green.

No additional root-level proof logs were created.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, before 2/2 and after 2/2 with no new failures.
