Status: Active
Source Idea Path: ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Completed Step 4 - Validate And Prepare Acceptance Notes.

Validated the completed selected Route 7 fused-compare operand-producer
helper-oracle row family tied to
`find_prepared_fused_compare_operand_producer_facts(...)`.

The proven row family is the selected/folded public AArch64 conditional branch
lowering path that lowers through
`lower_prepared_conditional_branch_terminator(...)` and accepts Route 7
fused-compare operand-producer evidence only when the private agreement reader
converts both operands to prepared facts that match the prepared helper row.
The Step 3 public-boundary tests prove the prepared compare operand row remains
unchanged for:
- absent/withheld Route 7 fused-compare evidence;
- duplicate/conflicting selected operand evidence;
- mismatched Route 7 fused-compare operands;
- unfused/unavailable Route 7 branch-condition evidence;
- partial non-agreement from duplicate rhs producer evidence.

The retained fallback guarantees are backed by the public-boundary tests plus
the existing private-reader proof in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: invalid producer
references, stale references, wrong-key/wrong-role consumer records,
duplicate/conflicting Route 7 records, unavailable evidence, mismatched
operands, unfused evidence, and partial non-agreement all fail closed before
public fallback can accept Route 7 facts.

Nearby same-feature coverage is present through the prepared-only
register/immediate public-boundary fused-compare row check, so the proof is not
limited to the select-plus-folded-constant fixture. This validation packet made
no implementation, test, expected-string, baseline, wrapper, branch-control
output, machine-printer output, suffix mapping, fused-legality, hazard-check,
target-policy, or emitted-output-policy changes.

## Suggested Next

The runbook steps appear complete. Ask the supervisor/plan-owner to decide
whether to close, deactivate, or otherwise transition the active lifecycle
state; do not edit `plan.md` or the source idea from an executor packet.

## Watchouts

- The public boundary does not expose a Route 7 index injection seam; invalid
  producer-reference coverage remains explicit private-reader proof from
  `backend_prepared_lookup_helper_test`.
- The new public tests mutate retained BIR without moving the prepared producer
  instructions, because prepared source-producer lookups validate pointer and
  instruction-index identity against the current block.
- No broader validation was delegated for this packet. Residual risk is limited
  to the supervisor deciding whether this narrow proof is enough for lifecycle
  acceptance.

## Proof

Ran delegated proof:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`

Result: passed. Build reported `ninja: no work to do`; ctest passed
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`, and `backend_prepared_lookup_helper`.
Proof log: `test_after.log`.
