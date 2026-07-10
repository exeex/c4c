Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove The Owner And Add Focused Contract Coverage

# Current Packet

## Just Finished

Step 2 added focused backend contract coverage for the callee result/frame-slot
owner without changing implementation code:

- `tests/backend/case/riscv64_callee_result_frame_slot_contract.c`
- `backend_dump_riscv64_callee_result_frame_slot_contract`

Evidence summary:
`build/agent_state/656_step2_callee_result_contract/summary.md`.

The prepared producer/consumer facts are now covered outside the GCC torture
row. The callee return consumer is the `before_return` move from `%t10` to the
function return ABI register `a0`; the caller consumer is the `after_call`
`call_result_abi` move from `a0`. The missing producer fact is the callee
`store_source function=rv64_advance_and_store block=entry inst=5 source=%t3`
publication, which still has `source_freshness_status=no_candidate`.

The adjacent indirect writeback route is distinguished in the same prepared
fixture as `access block=entry inst_index=13 base=pointer_value stored=%t9`.
The focused object-runtime probe was kept as evidence, not a committed red
CTest: it fails with `exit=1 expected=0`, meaning the first fixture check
failed on the returned pointer before the later `stored != 1` indirect
writeback check.

## Suggested Next

Execute Step 3 of `plan.md`: repair the general RV64 callee-result propagation
for the proven frame-slot owner. The focused positive coverage should move
from red to green, and the dump contract should continue to name the producer
and consumer facts rather than relying on final assembly shape.

## Watchouts

- Preserve the `%t6` pointer-source publication and branch RHS authority from
  idea 653.
- Do not special-case `src/20140828-1.c`,
  `riscv64_callee_result_frame_slot_contract.c`, `%t6`,
  `rv64_advance_and_store`, `f(a, 1, &d)`, or final branch compare shapes.
- Do not change expectations, unsupported markers, allowlists, timeout
  accounting, or runtime policy.
- Current object evidence still stores an unmaterialized `t0` into the callee
  base slot and returns that slot via `ld s1,0(sp); mv a0,s1`.
- The `*out` route is adjacent evidence only; the focused red runtime result is
  the callee returned pointer.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result after keeping the red object-runtime probe out of the committed CTest
set: CTest exited `8`. `test_after.log` reports `334 passed, 32 failed, 366
total`. The new dump contract passed, and the backend subset has no new
failing tests. Proof log path: `test_after.log`.
