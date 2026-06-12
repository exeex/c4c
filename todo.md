Status: Active
Source Idea Path: ideas/open/220_phase_e1_route_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Route Identity Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by inspecting the Route 1 through Route 7 candidate
identity surfaces and selecting one implementation surface.

Selected route family: Route 7 comparison provenance.

Selected helper/consumer: AArch64
`find_prepared_fused_compare_operand_producer_facts(...)` in
`src/backend/mir/aarch64/codegen/comparison.cpp`, the local wrapper consumed by
`lower_prepared_conditional_branch_terminator(...)`.

Why this is identity-only: the selected surface answers only fused-compare
operand producer identity for the current branch condition: lhs/rhs value
identity, producer kind, producer instruction/index, and folded integer
constant provenance. It does not decide whether a branch may fuse, which
condition suffix to print, which targets to branch to, which hazards or
emitted-register state are legal, or which final assembler rows are emitted.

route/prepared agreement owner: Route 7 `Route7ComparisonConditionIndex`,
`route7_find_fused_compare_operand_producer_facts(...)`, and Route 7 operand
reference validation own the route identity fact only when it agrees with the
prepared `PreparedFusedCompareOperandProducerFacts` oracle for the same
prepared branch condition and BIR block boundary.

retained prepared fallback/policy/output owners: prepared branch-condition
facts, prepared fused-compare operand producer helpers,
`PreparedEdgePublicationSourceProducerLookups`, scalar producer and integer
constant fallback, AArch64 branch-control lowering, branch suffix and
fused-legality policy, branch target labels, machine-printer/debug rows,
helper-oracle strings, wrapper behavior, and expected output remain prepared or
AArch64-owned for absent, invalid, ambiguous/conflict, mismatch, non-fused,
policy-sensitive, and output-sensitive paths.

Current callers/tests/route notes inspected: candidate list and readiness notes
in `ideas/open/220_phase_e1_route_identity_helper_contraction.md` and
`docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`;
Route 7 retained-surface notes in
`docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
and `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`;
Route 7 closure notes `ideas/closed/215_route7_comparison_provenance_consumer.md`
and `ideas/closed/216_route7_comparison_oracle_row.md`; helper definitions in
`src/backend/prealloc/comparison.hpp` and
`src/backend/prealloc/comparison.cpp`; the selected AArch64 wrapper and caller
in `src/backend/mir/aarch64/codegen/comparison.cpp`; Route 7 BIR records and
indexes in `src/backend/bir/bir.cpp`; existing proof coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` and
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.

## Suggested Next

Delegate Step 2 to add a Route 7/prepared agreement-gated identity read inside
the selected AArch64
`find_prepared_fused_compare_operand_producer_facts(...)` wrapper only. The
next packet should prefer Route 7 fused-compare operand facts only when Route 7
is available and agrees with prepared facts; otherwise it should return the
current prepared result.

Recommended narrow proof command for that code-changing packet:
`cmake --build build --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$' --output-on-failure`

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
- The Step 2 implementation should not move branch suffixes, branch targets,
  fused legality, final assembler lines, helper-oracle strings, or expected
  strings into Route 7 authority.
- Existing Route 7 lookup-helper coverage already exercises positive,
  immediate, rhs-only, absent/unavailable, missing-producer,
  duplicate-reference, wrong-key, wrong-relationship, stale-reference, and
  production branch-condition cases; Step 2 should add only the narrow
  production/consumer proof needed for this selected wrapper.

## Proof

Inspection-only packet. No build required. Delegated proof command:
`rg -n "Selected route family|Selected helper|Selected consumer|identity-only|route/prepared agreement|retained prepared|fallback|policy" todo.md`.
