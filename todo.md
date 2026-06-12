Status: Active
Source Idea Path: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Completed Step 3 - Prove Public Fallback And Nearby Same-Feature Cases.

Added focused public-boundary coverage in
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` for the
selected fused-compare operand-producer row. The new selected/folded fixture
lowers through `lower_prepared_conditional_branch_terminator(...)`, proves the
baseline Route 7 selected/folded facts match the public compare operand row,
then mutates retained BIR while preserving prepared lookup authority and proves
the public row falls back unchanged for:
- absent/withheld Route 7 fused-compare evidence;
- duplicate/conflicting selected operand evidence;
- mismatched Route 7 fused-compare operands;
- unfused/unavailable Route 7 branch-condition evidence;
- partial non-agreement from duplicate rhs producer evidence.

The existing private proof in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` remains the explicit
invalid-reference seam proof: invalid producer references, stale references,
wrong-key/wrong-role consumer records, duplicate/conflicting Route 7 records,
unavailable evidence, mismatched operands, unfused evidence, and partial
non-agreement all fail closed before public fallback can accept Route 7 facts.

Added a nearby prepared-only public-boundary fused-compare row check for the
register/immediate case so the proof is not shaped only around the
select-plus-folded-constant fixture. No backend implementation, expected
strings, wrappers, branch-control output policy, machine-printer output,
suffix mapping, fused-legality, hazard checks, target policy, or emitted-output
policy changed.

## Suggested Next

Start Step 4 - Validate And Prepare Acceptance Notes. The next packet should
review the Step 3 diff for proof scope, keep the invalid-reference coverage
classified as private explicit proof unless a new public injection seam is
introduced by the supervisor, and prepare acceptance notes from the delegated
proof log.

## Watchouts

- The public boundary does not expose a Route 7 index injection seam; invalid
  producer-reference coverage remains explicit private-reader proof from
  `backend_prepared_lookup_helper_test`.
- The new public tests mutate retained BIR without moving the prepared producer
  instructions, because prepared source-producer lookups validate pointer and
  instruction-index identity against the current block.
- Keep any follow-up proof semantic. Do not convert this into expected-string,
  wrapper, branch-control output, machine-printer output, suffix, legality,
  hazard, target-policy, or emitted-output policy work.

## Proof

Ran delegated proof:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`

Result: passed after the Step 3 public-boundary test additions. Build reported
no work to do after the branch-control target rebuild; ctest passed
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`, and `backend_prepared_lookup_helper`.
Proof log: `test_after.log`.
