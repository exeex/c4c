Status: Active
Source Idea Path: ideas/open/381_rv64_object_route_short_circuit_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Select/Join Materialization Repair

# Current Packet

## Just Finished

Step 2 implemented the RV64 object-route select/join materialization repair for
the audited prepared shape. When a predecessor-terminator move bundle publishes
an authoritative `select_materialization` edge copy whose source is a prepared
join-local binary compare producer, object emission now admits only the narrow
supported form: fully published select join, GPR destination, compare result,
register/immediate operands, producer located in the join block, and no
non-carrier BIR uses of that producer. The RHS predecessor edge materializes
the compare directly into the select result home before its jump, and the
corresponding join-block producer is suppressed with the select carrier.

Unsupported adjacent shapes remain fail-closed through the same object-route
admission: stack/memory operands, unsupported source producers, ambiguous or
partial publication authority, non-select carriers, cycle/non-move bundles, and
additional non-carrier producer consumers are not admitted by this repair.

Focused coverage was added in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`:

- admitted published select edge source compare materializes on the RHS
  predecessor edge and the join block begins at the return;
- a nearby stack-operand source for that edge compare rejects instead of
  emitting partial object code.

## Proof

Local pre-proof check passed:

`cmake --build --preset default --target backend_riscv_object_emission_test -j2 && ./build/tests/backend/mir/backend_riscv_object_emission_test`

Canonical Step 2 proof passed and its full output is preserved in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_(obj_)?runtime_rv64_(two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call))$' | tee test_after.log`

Result: build passed; 13/13 selected tests passed.

## Suggested Next

After Step 2 proof is green, execute Step 3 by rerunning the `src/20000112-1.c`
representative for idea 381 with the supervisor-selected allowlist command.
