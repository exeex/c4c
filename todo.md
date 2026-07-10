Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Representative Runtime And Backend Regression Proof

# Current Packet

## Just Finished

Step 4 refreshed representative `loop-2e.c` runtime evidence after the Step 3
pointer-value store repair and ran the delegated backend proof.

Evidence:

- `build/agent_state/657_step4_representative_proof/summary.md`
- `build/agent_state/657_step4_representative_proof/loop-2e.bir.txt`
- `build/agent_state/657_step4_representative_proof/loop-2e.prepared.txt`
- `build/agent_state/657_step4_representative_proof/loop-2e.s`
- `build/agent_state/657_step4_representative_proof/loop-2e.o`
- `build/agent_state/657_step4_representative_proof/loop-2e.objdump.txt`
- `build/agent_state/657_step4_representative_proof/loop-2e.runner.o`
- `build/agent_state/657_step4_representative_proof/loop-2e.runner.objdump.txt`
- `build/agent_state/657_step4_representative_proof/runtime-case.stderr.txt`

Representative facts:

- `%t23 = bir.add ptr %t21, 156` remains present in semantic and prepared BIR.
- Prepared branch RHS authority for `%t23` remains selected and available.
- The callee still has an explicit prepared pointer-value store fact:
  `access block=block_1 inst_index=7 base=pointer_value stored=%t8 pointer=%t9
  offset=0 size=8 align=8`.
- Store-source freshness for `%t8` remains selected.
- Clang RV64 runtime exits `0`.
- c4c RV64 runtime compare still fails with `[RV64_BACKEND_RUNTIME_MISMATCH]`,
  `clang_exit=0 c4c_exit=Subprocess aborted`.
- The focused Step 3 backend contract remains green, but the representative
  object still writes both the local cursor update and caller-visible stored
  value to `0(sp)` instead of emitting the expected `sd ..., 0(s1)` pointer-value
  store.
- Idea 657 is not ready for lifecycle closure consideration yet.

## Suggested Next

Review or implement the smallest follow-up packet for the representative RV64
prepared pointer-value store lowering gap: make the Step 3 `base=pointer_value`
repair apply to the `loop-2e.c` callee shape where old `%t9` is register-home
`s1` and the local cursor writeback also targets `%lv.param.q`'s frame-slot
home.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- The Step 3 implementation consumes explicit prepared pointer-value access
  facts; it does not infer destination authority from runtime behavior, source
  names, final assembly, `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Step 4 proves the focused contract passed but the representative shape still
  emits `sd t1,0(sp)` for the caller-visible indirect store. The next packet
  should not claim closure until representative runtime matches clang or fails
  closed at a more precise non-overfit owner.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.
- `backend_riscv_object_emission` remains in the known red backend subset.

## Proof

Representative evidence refresh:

- Build, BIR, prepared BIR, ASM, object, disassembly, and clang runtime all
  exited `0`.
- c4c RV64 runtime compare exited `1` with
  `[RV64_BACKEND_RUNTIME_MISMATCH]`.

Delegated proof run exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: CTest exited `8`; `test_after.log` reports `91% tests passed, 32 tests
failed out of 368`, matching `test_before.log`. The focused dump and focused
RV64 object-runtime contracts for `riscv64_indirect_store_postincrement_callee`
passed inside the subset.
