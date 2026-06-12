Status: Active
Source Idea Path: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Validate Existing Agreement-Gated Route 7 Evidence

# Current Packet

## Just Finished

Completed Step 1 - Locate The Row Family And Proof Surface.

Selected row family:
`find_prepared_fused_compare_operand_producer_facts(...)` for fused-compare
operand producers feeding AArch64 conditional branch lowering. The row returns
`PreparedFusedCompareOperandProducerFacts` for `lhs`/`rhs`, where each side may
name a same-block producer (`LoadLocal`, `LoadGlobal`, `Cast`, `Binary`,
`SelectMaterialization`) or an immediate/folded integer constant. The selected
observable behavior is the fused-compare operand row consumed by
`make_prepared_conditional_branch_record_impl(...)`, especially RHS immediate
rewriting through `integer_constant` and producer-backed value-home selection.

Implementation surface found:
- Prepared aggregate source:
  `src/backend/prealloc/comparison.cpp` and
  `src/backend/prealloc/comparison.hpp`, implementing
  `prepare::find_prepared_fused_compare_operand_producer(...)` and
  `prepare::find_prepared_fused_compare_operand_producer_facts(...)`.
- AArch64 helper-oracle boundary:
  `src/backend/mir/aarch64/codegen/comparison.cpp`, anonymous-namespace
  `find_prepared_fused_compare_operand_producer_facts(...)` at the conditional
  branch lowering boundary.
- Route 7 evidence provider:
  `src/backend/bir/bir.cpp` and `src/backend/bir/bir.hpp`,
  `route7_build_comparison_condition_index(...)`,
  `route7_find_fused_compare_operand_producer_facts(...)`,
  `route_index_validate_comparison_operand_reference(...)`, and
  `route7_operand_record_to_public(...)`.

Private Route 7 reader and evidence path:
- Private reader:
  `detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`
  in `src/backend/mir/aarch64/codegen/comparison.cpp`.
- Evidence path:
  `BlockLoweringContext::bir_block` plus the prepared fused branch condition
  builds a Route 7 comparison index with
  `bir::route7_build_comparison_condition_index(...)`, reads
  `bir::route7_find_fused_compare_operand_producer_facts(...)`, converts each
  Route 7 `ComparisonOperandProducer` through
  `route7_comparison_operand_producer_to_prepared(...)`, and accepts it only
  through
  `route7_fused_compare_operand_producer_facts_if_agree_with_prepared(...)`
  when both converted sides match the prepared facts. All non-agreement paths
  return `std::nullopt` from the private reader.

Relevant callers:
- AST caller lookup found the AArch64 helper called by
  `lower_prepared_conditional_branch_terminator(...)`.
- That caller passes the result to
  `make_prepared_conditional_branch_record_impl(...)`, which builds the
  `BranchConditionForm::FusedCompare` compare operand row and installs print
  operands for the selected compare branch.
- The private reader is called by the AArch64 helper and by test-only wrappers
  `read_agreeing_route7_fused_compare_operand_producer_facts_for_testing(...)`
  and `agreeing_route7_fused_compare_operand_producer_facts_for_testing(...)`.

Existing positive/fallback proof surface:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_prepared_bir_comparison_condition_producer_equivalence()` covers
  prepared/BIR/Route 7 matching for select plus folded-constant operands,
  immediate operands, rhs-only availability, non-fused/unavailable fail-closed,
  wrong-key/wrong-role route-index validation, consumer-boundary fail-closed
  cases, the private agreement reader positive path, and private fallback for
  absent Route 7, invalid producer references, duplicate/conflicting Route 7
  records, mismatched operands, unfused facts, and partial non-agreement.
- `verify_production_bir_comparison_condition_producer_equivalence()` covers a
  production-prealloc fused compare where prepared facts, BIR facts, Route 7
  branch records, and indexed Route 7 facts agree for both operands.
- `verify_bir_comparison_condition_producer_identity_lookup()` covers the base
  BIR same-block producer and fused-compare fact query.
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
  covers selected fused-compare branch-control output and nearby same-feature
  cases such as i32->i64 extension, folded constant RHS, stack-homed constant
  RHS, loop-header labels, and non-fusable fail-closed behavior.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  covers selected global-load materialization before fused compare branches for
  both lhs and rhs selected operands.

Proof gaps for Step 2/3:
- The private reader has direct fallback coverage for absent route, invalid
  reference, duplicate/conflict, mismatch, unfused, and non-agreement, but the
  public AArch64 helper/caller does not yet appear to have dedicated tests that
  mutate those Route 7 failure modes while proving the conditional branch row
  still uses the prepared fallback result.
- Prepared-only fallback is covered by the prepared aggregate helper and broad
  branch-control output, but there is no focused test that removes/withholds
  usable Route 7 evidence at the public helper boundary and asserts byte-stable
  prepared compare operand rows.
- Nearby same-feature coverage exists for immediate, rhs-only, production
  prealloc, branch-control output, lhs selected global load, and rhs selected
  global load, but there is not yet a focused public agreement-gated
  fused-compare operand-producer row test demonstrating that the implementation
  is not shaped around only the select-plus-folded-constant fixture.

## Suggested Next

Start Step 2 - Validate Existing Agreement-Gated Route 7 Evidence. Confirm the
private AArch64 helper path already calls
`detail::read_agreeing_route7_fused_compare_operand_producer_facts(...)`,
accepts converted Route 7 facts only when both sides match prepared facts, and
falls back to prepared facts on non-agreement. Record any real semantic gap;
otherwise proceed to Step 3 public-boundary fallback and nearby same-feature
proof with no expected-string, baseline, wrapper, branch-control,
machine-printer, suffix, fused-legality, hazard, or output-policy rewrites.

Suggested narrow proof command for Step 2/3:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure`

## Watchouts

- Keep the slice to one fused-compare operand-producer helper-oracle row
  family.
- Preserve prepared fallback for absent route, invalid references,
  duplicate/conflict, mismatch, unfused, non-agreement, and prepared-only
  paths.
- Do not rewrite expected strings, baselines, wrapper output, branch-control
  output, machine-printer output, suffix mapping, fused legality, hazard
  checks, target policy, or emitted-output behavior.
- Reject testcase-shaped matching or fixture-name shortcuts.

## Proof

Discovery-only lifecycle packet. No build/test required by the delegated
packet. Required local checks: `git diff --check -- todo.md` and
`git status --short`.
