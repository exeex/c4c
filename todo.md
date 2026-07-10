Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Proven Callee/Frame-Slot Rule

# Current Packet

## Just Finished

Step 3 was blocked before implementation. The focused dump contract still
passes and continues to prove that the callee store-source publication for
`%t3` exists but has no selected producer freshness:

`store_source function=rv64_advance_and_store block=entry inst=5 source=%t3
... source_producer=unknown source_freshness_status=no_candidate`.

The focused BIR contains `bir.store_local %lv.param.base, ptr %t3` with no
preceding semantic producer instruction for `%t3`, and prepared state gives
`%t3` only a register home (`t0`), not a binary or pointer-base-plus-offset
producer. The object runtime still fails with `exit=1 expected=0` because RV64
emission reads `t0` before any prepared-authorized materialization, stores that
unmaterialized value into the callee base slot, and then returns the slot.

Evidence summary:
`build/agent_state/656_step3_callee_result_repair/summary.md`.

## Suggested Next

Plan-owner decision: keep `plan.md` unchanged. Step 3 already authorizes
repairing prepared publication before RV64 consumption for the proven
callee-result/frame-slot owner; no new source idea is needed.

Continue Step 3 with an executor packet that owns the BIR/prepared producer
surface before any RV64 object-emission repair. The packet should publish an
explicit producer fact for `%t3` (for example a semantic binary or coherent
pointer-base-plus-offset fact), prove that the prepared store-source freshness
is no longer `source_freshness_status=no_candidate`, and only then wire RV64
consumption to the explicit fact.

## Watchouts

- Preserve the `%t6` pointer-source publication and branch RHS authority from
  idea 653.
- Do not special-case `src/20140828-1.c`,
  `riscv64_callee_result_frame_slot_contract.c`, `%t6`,
  `rv64_advance_and_store`, `f(a, 1, &d)`, or final branch compare shapes.
- Do not change expectations, unsupported markers, allowlists, timeout
  accounting, or runtime policy.
- Current focused object evidence still stores unmaterialized `t0` into the
  callee base slot and returns that slot via `ld s1,0(sp); mv a0,s1`.
- A backend/object-only repair would have to infer `base++` from neighboring
  local load/store shape. That would be route drift unless an explicit prepared
  producer or pointer-base-plus-offset fact is added first.
- The `*out` route is adjacent evidence only; the focused red runtime result is
  the callee returned pointer.

## Proof

Focused dump contract:
`ctest --test-dir build --output-on-failure -R '^backend_dump_riscv64_callee_result_frame_slot_contract$'`

Result: pass. Log:
`build/agent_state/656_step3_callee_result_repair/focused_dump_ctest.log`.

Focused object-runtime probe used
`tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake` for
`tests/backend/case/riscv64_callee_result_frame_slot_contract.c` with expected
exit `0`.

Result: fail as expected, `exit=1 expected=0`. Log:
`build/agent_state/656_step3_callee_result_repair/focused_object_runtime.log`.

The delegated backend proof command was not run because no implementation
slice was accepted and the focused positive runtime remains blocked by the
missing prepared producer fact. Existing `test_after.log` was left untouched.
