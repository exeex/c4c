Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair General RV64 Indirect-Store Or Writeback Lowering

# Current Packet

## Just Finished

Step 3 continued the RV64 prepared pointer-value store repair for the
representative `loop-2e.c` callee shape where old `%t9` is register-home `s1`
and the local cursor writeback shares `%lv.param.q`'s frame-slot home.

Evidence:

- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
- `build/agent_state/657_step3_representative_pointer_value_store/focused_dump_ctest.after.log`
- `build/agent_state/657_step3_representative_pointer_value_store/focused_object_runtime_ctest.after.log`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.bir.txt`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.prepared.txt`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.s`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.o`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.objdump.txt`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.runtime.o`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.runtime.objdump.txt`
- `build/agent_state/657_step3_representative_pointer_value_store/loop-2e.runtime_compare.log`
- `test_after.log`

Representative facts:

- The callee still has an explicit prepared pointer-value store fact:
  `access block=block_1 inst_index=7 base=pointer_value stored=%t8 pointer=%t9
  offset=0 size=8 align=8`.
- Store-source freshness for `%t8` remains selected.
- The representative object now preserves the local cursor writeback to
  `%lv.param.q`'s frame-slot home with `sd t1,0(sp)`.
- The caller-visible indirect store now uses the pointer register home with
  `sd t1,0(s1)` for the explicit `base=pointer_value stored=%t8 pointer=%t9`
  access.
- Representative `loop-2e.c` RV64 object-runtime comparison now passes against
  clang.

## Suggested Next

Return to Step 4 representative proof/classification for idea 657. Refresh
`loop-2e.c` end-to-end evidence after the representative pointer-value store
repair and decide whether the source idea is closure-ready or whether a new
precise downstream owner remains.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- The Step 3 implementation consumes explicit prepared pointer-value access
  facts; it does not infer destination authority from runtime behavior, source
  names, final assembly, `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- The representative shape now emits `sd t1,0(s1)` for the caller-visible
  indirect store while keeping the cursor writeback as `sd t1,0(sp)`.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.
- `backend_riscv_object_emission` remains in the known red backend subset.

## Proof

Build:

`cmake --build --preset default`

Focused contracts:

- `ctest --test-dir build --output-on-failure -R '^backend_dump_riscv64_indirect_store_postincrement_callee_contract$'`
- `ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract$'`

Representative runtime compare:

`cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=clang -DQEMU_RISCV64=qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/loop-2e.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/657_step3_representative_pointer_value_store/loop-2e.clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/657_step3_representative_pointer_value_store/loop-2e.runtime.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/657_step3_representative_pointer_value_store/loop-2e.c4c.bin -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=20 -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

Result: pass, `-- [PASS][rv64-gcc-torture-backend-obj]
/workspaces/c4c/tests/c/external/gcc_torture/src/loop-2e.c`.

Delegated proof run exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: CTest exited `8` with the known red backend subset; `test_after.log`
reports `91% tests passed, 32 tests failed out of 368`. The focused dump
contract passed inside the subset.
