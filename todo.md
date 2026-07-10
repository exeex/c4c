Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative Runtime And Backend Regression Proof

# Current Packet

## Just Finished

Step 3 published the missing semantic/prepared producer surface for the callee
result store source. RV64 pointer GEP lowering now emits the explicit producer
`%t3 = bir.add ptr %t2, 2` before `bir.store_local %lv.param.base, ptr %t3`.
The focused prepared dump now proves the store source is fact-backed:

`store_source function=rv64_advance_and_store block=entry inst=6 source=%t3
... source_producer=binary source_producer_block=entry source_producer_inst=5
source_freshness_status=selected
source_freshness_authority=producer_rematerialization`.

Changed files:
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `tests/backend/CMakeLists.txt`

Evidence directory:
`build/agent_state/656_step3_producer_surface/`.

## Suggested Next

Proceed to Step 4 as a representative proof/classification packet, not a
destination fan-in implementation packet.

Refresh BIR, prepared-BIR, ASM, object, disassembly, and qemu/runtime evidence
for `tests/c/external/gcc_torture/src/20140828-1.c` after the completed `%t3`
producer/freshness repair. Confirm that the prior 656-owned boundary remains
repaired:

- `%t3 = bir.add ptr %t2, 2` is present before
  `bir.store_local %lv.param.base, ptr %t3`.
- the relevant store source still reports `source_producer=binary` and
  `source_freshness_status=selected`.

If RV64 object emission or runtime now stops at
`producer_authority_missing_for_register_fan_in_stack_destination`, record that
as the precise downstream fail-closed owner for 656 closure consideration. Do
not implement register-source fan-in-to-stack-destination authority under idea
656; that owner belongs to the parked 647/655 line unless the supervisor
explicitly reactivates or splits that work.

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
- The focused object probe now fails before runtime execution with a fail-closed
  RV64 object diagnostic for register fan-in stack destination authority.
- The `*out` route is adjacent evidence only; the focused red runtime result is
  the callee returned pointer.
- `review/656_route_review.md` rejects continuing Step 3 into
  stack-destination fan-in authority as route drift. Treat the current fan-in
  failure as downstream classification evidence, not as implementation
  authorization inside idea 656.

## Proof

Focused dump contract:
`ctest --test-dir build --output-on-failure -R '^backend_dump_riscv64_callee_result_frame_slot_contract$'`

Result: pass after the contract update. Log:
`build/agent_state/656_step3_producer_surface/focused_dump_ctest.after.log`.

Focused object-runtime probe used
`tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake` for
`tests/backend/case/riscv64_callee_result_frame_slot_contract.c` with expected
exit `0`.

Result: blocked before qemu. The object emitter fails closed with
`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
Log:
`build/agent_state/656_step3_producer_surface/focused_object_runtime.after.log`.

Build/backend proof:
`cmake --build --preset default`

Result: pass.

Supervisor then ran the delegated backend proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: CTest exited `8` with the known red backend subset. `test_after.log`
reports `334 passed, 32 failed, 366 total`; the non-decreasing regression
guard found no new failing tests.
