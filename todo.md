Status: Active
Source Idea Path: ideas/open/381_rv64_object_route_short_circuit_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select/Join Materialization Gap

# Current Packet

## Just Finished

Step 1 audited the RV64 object-route select/join materialization gap. The first
relevant shape is `special_format`'s prepared `logic.end.7` join:
`%t13 = select ne %t0, 0, 1, %t9` is represented as a
`join_transfer` with `kind=phi_edge`, `carrier=select_materialization`, and
`ownership=authoritative_branch_pair`. Its prepared edge transfers are
`logic.skip.5 -> logic.end.7: 1 -> %t13` and
`logic.rhs.end.6 -> logic.end.7: %t9 -> %t13`; prepared move bundles publish
both transfers before predecessor terminators.

The object route partially consumes that authority. It emits the skip-edge
materialization (`li t0,1`) and `fragment_for_prepared_instruction` suppresses
ordinary select lowering when `prepared_join_transfer_edge_copies_are_published`
is true. The missing rule is that RV64 object emission still lowers the
join-block source producer `%t9 = ne ptr %t8, 0` as an ordinary instruction
before the skipped select. On the skip path `%t8` was never produced by the RHS
call, so the emitted join reads `%t8`'s prepared home (`s2`) and overwrites the
already published `%t13` value before the next branch.

## Suggested Next

Execute Step 2 with the narrow semantic repair: when an authoritative
select-materialization join is fully published by predecessor edge copies, treat
join-local pure source materializations used only by the select-carrier edge
transfer as edge-owned. For the admitted shape, materialize the RHS edge source
producer on the predecessor edge and publish directly to the select result home,
then suppress the corresponding join-block source producer and select carrier.

## Watchouts

This is not an idea 380 regression: the current disassembly shows `mv s1,a0`
before the first call and later argument reloads from `s1`; the remaining abort
happens after that, when short-circuit join materialization reads an unproduced
RHS result home on a skip path.

Keep Step 2 fail-closed for adjacent shapes unless they are explicitly
admitted: critical-edge or successor-entry select publications that cannot be
placed before the edge transfer, non-select carriers, ambiguous/missing
`join_transfer` or parallel-copy authority, cycles or non-move parallel-copy
steps, stack/FPR/vector/wide destinations, stack or memory source operands,
non-immediate/non-register source producers, impure or unsupported join-local
source producers, producer values with additional non-carrier consumers, and
any case requiring testcase names, block labels, value ids, literal chars, or
fixed registers.

Concise audit artifact:
`build/agent_state/381_step1_select_join_audit.txt`.

## Proof

Audit-only step; no proof command or root proof log required. Recommended Step
2 focused proof after implementation:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_(obj_)?runtime_rv64_(two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call))$'`

Step 2 should add focused RV64 object tests for the admitted predecessor-edge
select-source materialization shape, plus a fail-closed nearby unsupported
shape. Step 3 should then rerun `src/20000112-1.c` with the supervisor-selected
allowlist command.
