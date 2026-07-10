Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair General RV64 Indirect-Store Or Writeback Lowering

# Current Packet

## Just Finished

Step 3 repaired the general RV64 prepared pointer-value store lowering for the
proven indirect-store/postincrement contract.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `tests/backend/CMakeLists.txt`
- `todo.md`

Completed behavior:

- Added a prepared `Ptr` store path for `base=pointer_value` local-memory
  accesses with explicit `stored_value_name`, pointer-value base authority,
  size/alignment `8`, default address space, non-volatile access, and signed
  12-bit base-plus-offset addressing.
- The focused contract now emits the caller-visible indirect store through the
  old pointer value (`sd ..., 0(s1)`) and keeps the local cursor writeback as a
  separate frame-slot update (`sd ..., 0(sp)`).
- Registered the focused RV64 object-runtime contract only after the manual
  object-runtime probe reached exit `0`.

## Suggested Next

Execute Step 4: refresh representative `loop-2e.c` runtime evidence and run the
supervisor-selected backend regression proof/classification. No further
implementation is currently needed for the focused Step 3 contract.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- The Step 3 implementation consumes explicit prepared pointer-value access
  facts; it does not infer destination authority from runtime behavior, source
  names, final assembly, `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.
- `backend_riscv_object_emission` remains in the known red backend subset; the
  delegated proof count stayed at 32 failed while the total backend test count
  increased by one registered passing object-runtime contract.

## Proof

Focused dump contract:

`ctest --test-dir build --output-on-failure -R '^backend_dump_riscv64_indirect_store_postincrement_callee_contract$'`

Result: pass. Logs:

- `build/agent_state/657_step3_pointer_value_store_repair/focused_dump_ctest.log`
- `build/agent_state/657_step3_pointer_value_store_repair/focused_dump_ctest.after.log`

Manual focused RV64 object-runtime probe before CTest registration:

`cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/backend/case/riscv64_indirect_store_postincrement_callee_contract.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_OBJECT=/workspaces/c4c/build/agent_state/657_step3_pointer_value_store_repair/riscv64_indirect_store_postincrement_callee_contract.o -DOUT_BIN=/workspaces/c4c/build/agent_state/657_step3_pointer_value_store_repair/riscv64_indirect_store_postincrement_callee_contract.bin -DEXPECTED_RUN_CODE=0 -DCASE_TIMEOUT_SEC=10 -P /workspaces/c4c/tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`

Result: pass. Log:
`build/agent_state/657_step3_pointer_value_store_repair/focused_object_runtime.log`.

Registered focused RV64 object-runtime CTest:

`ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract$'`

Result: pass. Log:
`build/agent_state/657_step3_pointer_value_store_repair/focused_object_runtime_ctest.log`.

Object evidence:

- `build/agent_state/657_step3_pointer_value_store_repair/focused_object_runtime_objdump.txt`

Delegated proof run exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: CTest exited `8`; `test_after.log` reports
`91% tests passed, 32 tests failed out of 368`. The newly registered
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract` passed,
and the focused dump contract passed inside the subset.
