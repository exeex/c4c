Status: Active
Source Idea Path: ideas/open/220_phase_e1_route_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Agreement-Gated Route Identity Read

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding the smallest Route 7/prepared
agreement-gated identity read inside the selected AArch64
`find_prepared_fused_compare_operand_producer_facts(...)` wrapper in
`src/backend/mir/aarch64/codegen/comparison.cpp`.

The wrapper still computes the existing prepared
`PreparedFusedCompareOperandProducerFacts` result first. It now builds a local
Route 7 comparison condition index for the same `context.bir_block`, same
prepared fused-compare branch lhs/rhs operands, and same
`before_instruction_index`, then returns Route 7-derived prepared-shaped facts
only when Route 7 is available and structurally agrees with the prepared facts.

Agreement checks cover producer availability, producer kind, producer
instruction pointer/index, produced prepared value name, typed producer pointer,
and folded integer constant identity. Absent Route 7 facts, invalid or
unconvertible route facts, route/prepared mismatch, and missing prepared facts
continue to return the current prepared result.

No branch suffix, branch target, fused-legality, materialized-condition,
assembler-row, helper-oracle string, wrapper, printer/debug, or expected-string
behavior was moved to Route 7 authority.

## Suggested Next

Delegate Step 3 to prove required fallback, output, and nearby coverage for the
selected Route 7/AArch64 fused-compare operand-producer identity surface.

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
- The selected helper must not absorb materialized-condition migration; that
  path already has Route 7 agreement gating and is a separate surface.
- Step 3 should decide whether existing Route 7 lookup-helper coverage plus the
  AArch64 branch-control lowering proof is sufficient for positive,
  absent/unavailable, invalid, mismatch, policy-sensitive fallback, output, and
  nearby same-feature coverage, or whether a narrow owned test addition is
  still needed.
- The selected helper must continue returning prepared fallback for all
  non-agreement paths; Route 7 must not absorb suffix, target, fused-legality,
  final assembler, helper-oracle string, wrapper, printer/debug, or expected
  string ownership.

## Proof

Delegated proof command completed successfully:
`( cmake --build --preset default --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$' --output-on-failure ) > test_after.log 2>&1`

Proof log: `test_after.log`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, before 2/2 and after 2/2 with no new failures.

Broader supervisor validation:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 180/180 backend tests green.
