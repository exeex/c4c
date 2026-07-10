Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative Runtime And Backend Regression Proof

# Current Packet

## Just Finished

Step 4 refreshed representative evidence for
`tests/c/external/gcc_torture/src/20140828-1.c` after the completed `%t3`
producer/freshness repair. Evidence directory:
`build/agent_state/656_step4_representative_proof/`.

The prior 656-owned boundary remains repaired:

- semantic/prepared BIR still contains `%t3 = bir.add ptr %t2, 2`;
- prepared BIR still records
  `store_source function=f block=entry inst=6 source=%t3 ... source_producer=binary ... source_freshness_status=selected`.

The prior 653 boundary also remains intact:

- prepared BIR still records
  `branch_stack_load_authority function=main block=entry role=rhs value=%t6 ... pointer_status=proven status=available source_freshness_status=selected`;
- prepared BIR still records frame-slot address materialization for `%t6`
  at offset `6`.

Representative clang RV64 builds and runs with exit `0`. Representative c4c
RV64 object emission fails closed before link/qemu with:

`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

The precise downstream owner is
`rv64_prepared_move_bundle_consumer` for an ambiguous non-parallel
register-source fan-in to one stack destination in `function=f`, before
instruction index `9`.

## Suggested Next

Supervisor should route lifecycle review/closure consideration for idea 656.
This packet produced the requested representative proof/classification without
implementation changes. If the project wants to pursue the remaining
`producer_authority_missing_for_register_fan_in_stack_destination` stop, that
should be activated or split under the parked stack-destination fan-in authority
line, not continued as implementation under idea 656.

## Watchouts

- Preserve the `%t6` pointer-source publication and branch RHS authority from
  idea 653.
- Do not special-case `src/20140828-1.c`,
  `riscv64_callee_result_frame_slot_contract.c`, `%t6`,
  `rv64_advance_and_store`, `f(a, 1, &d)`, or final branch compare shapes.
- Do not change expectations, unsupported markers, allowlists, timeout
  accounting, or runtime policy.
- The original `%t3` producer/freshness gap is repaired; do not re-open it
  unless evidence regresses from `source_freshness_status=selected`.
- The representative object route now fails before runtime execution with the
  same fail-closed RV64 object diagnostic for register fan-in stack destination
  authority.
- Treat the current fan-in failure as downstream classification evidence, not
  as implementation authorization inside idea 656.

## Proof

Representative evidence:

- BIR: `build/agent_state/656_step4_representative_proof/20140828-1.bir.txt`
- Prepared BIR:
  `build/agent_state/656_step4_representative_proof/20140828-1.prepared.txt`
- ASM: `build/agent_state/656_step4_representative_proof/20140828-1.s`
- Object diagnostic:
  `build/agent_state/656_step4_representative_proof/obj.stderr.txt`
- Status summary:
  `build/agent_state/656_step4_representative_proof/status.summary`
- Packet summary:
  `build/agent_state/656_step4_representative_proof/summary.md`

Delegated backend proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: CTest exited `8` with the known red backend subset. `test_after.log`
reports `334 passed, 32 failed, 366 total`, matching `test_before.log` counts.
The focused `backend_dump_riscv64_callee_result_frame_slot_contract` test
passed in the backend subset.
