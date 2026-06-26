Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the First Semantic Reload Repair

# Current Packet

## Just Finished

Step 2 implementation completed for the first semantic RV64 prepared-call reload
shape: when a GPR call argument has a prepared `PriorPreservation` source
selection with complete single-width callee-saved register authority, RV64
textual prepared-call emission and RV64 object emission now source the outgoing
ABI argument from the preserved register instead of falling through to the stale
original ABI argument register. Added focused RV64 object-emission coverage that
proves the later call reloads `s1 -> a0` from the prepared prior-preservation
selection, and that incomplete preserved-register facts plus stack
prior-preservation remain fail-closed on the existing unsupported instruction
diagnostic.

## Suggested Next

Run Step 3 against the representative `src/20000112-1.c` RV64 GCC torture
backend path to determine whether this repair makes the case pass or advances
it to a distinct next owner.

## Watchouts

This packet intentionally admits only complete single-width GPR callee-saved
`PriorPreservation` selections. Stack prior-preservation and
wider/register-bank variants remain unsupported in the RV64 object route until
separately planned. The object-route fixture verifies the reload from the
preserved home, but it does not claim broader call-boundary preservation-home
population coverage.

## Proof

Supervisor-delegated proof passed and was written to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_(obj_)?runtime_rv64_(two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call))$' | tee test_after.log`

Result: build completed with no work needed after the prior compile, and all 13
selected tests passed, including `backend_riscv_object_emission`,
`backend_prepare_frame_stack_call_contract`, `backend_call_boundary_effect_plan`,
idea 379 select-publication route/runtime tests, and the focused RV64
local-argument runtime/object cases.
