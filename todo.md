Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Guard Adjacent Byval And Variadic Repairs

# Current Packet

## Just Finished

Step 5 ran the supervisor-selected adjacent byval and variadic guard subset
after the small byval payload-lane repair.

The selected guard surfaces all remained stable:

- prior byval placement and upper-lane publication coverage:
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_instruction_dispatch`,
  `backend_runtime_byval_helper_payload_8_to_13`, and
  `backend_runtime_byval_helper_payload_9_to_14`
- prepared handoff, fixed-formal, and local/value-home-adjacent coverage:
  `backend_aarch64_prepared_handoff_gate`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- mixed HFA payload helper coverage:
  `backend_runtime_byval_helper_mixed_hfa_payload`
- representative external coverage:
  `c_testsuite_aarch64_backend_src_00204_c`

The proof passed 9/9 with no expectation, allowlist, CTest registration,
runner, timeout/proof-log, implementation, or test changes.

## Suggested Next

Supervisor should decide whether this active plan is ready for lifecycle
closure or whether a final broader acceptance proof is required.

## Watchouts

- Larger stack-passed byval behavior still legitimately materializes an
  address, for example `add x0, sp, #1064; bl fa_s17`; that is not the repaired
  small integer-class register-passed byval lane fault.
- This packet was validation-only and did not inspect for a new external first
  bad fact beyond the selected guard subset.

## Proof

Ran the delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(target_instruction_records|instruction_dispatch|prepared_handoff_gate)|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff(_aarch64_publication)?|backend_runtime_byval_helper_(payload_8_to_13|payload_9_to_14|mixed_hfa_payload)|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`

Result: build was up to date and the selected guard subset passed, 9/9. Proof
log preserved at `test_after.log`.
