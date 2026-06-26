Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Residual Semantic Repair

# Current Packet

## Just Finished

Step 2 implemented the residual RV64 semantic repair for supported GPR
callee-saved prior-preservation. RV64 prepared text emission and RV64 prepared
object emission now consume prepared `PreservationHomePopulation` effects before
call argument setup and `PreservationRepublication` effects after result
publication. The accepted shape is deliberately narrow: available
callee-saved-register preservation, GPR register endpoints, single-register
width, and RV64-known physical register names. The RV64 object route now also
uses prepared `frame_plan.saved_callee_registers` for supported single-width GPR
callee-saved prologue spills and return-epilogue restores, including saved slot
extents when sizing the object stack frame. The RV64 object test now proves the
supported `s1` save/restore around the prior-preserved `a0 -> s1` population
plus `s1 -> a0` republication/reload shape and keeps missing endpoint, stack
preservation, and unsupported saved-register slot routes rejected.

## Suggested Next

Supervisor should review the revised Step 2 diff for acceptance and decide
whether to move to lifecycle/commit handling.

## Watchouts

Stack-slot, multi-register, FPR, byval, local-frame-address, unknown-route, and
missing preservation-endpoint cases still fail closed in this slice. Saved
callee entries also fail closed unless they publish a complete 8-byte GPR
callee-saved slot placement with an RV64-known register and immediate-reachable
stack offset. The repair does not add testcase-name matching or external
allowlists.

## Proof

Passed the supervisor-selected proof command and preserved output in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_(obj_)?runtime_rv64_(two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call))$' | tee test_after.log`

The refreshed subset ran 13 tests and all passed after adding RV64 object
callee-saved save/restore support.
