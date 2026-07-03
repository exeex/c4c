Status: Active
Source Idea Path: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Guard Adjacent Regressions

# Current Packet

## Just Finished

- Step 5 Guard Adjacent Regressions completed after commit `49b0763d2`
  repaired the RV64 select compare publication path.
- Guarded 572 same-module call/result behavior with the focused
  `backend_riscv_object_emission` bucket, which includes same-module direct
  call symbol emission, scalar/null/byval/sret/FPR same-module calls, prior
  call-result multi-GPR arguments, and the nested ptrtoint same-module call
  object test.
- Guarded adjacent prepared metadata with
  `backend_riscv_prepared_edge_publication` and
  `backend_prepare_frame_stack_call_contract`.
- Guarded representative object-route behavior with RV64 gcc-torture runs for
  `src/20000412-2.c` and `src/20000622-1.c`. Both now pass under qemu.

## Suggested Next

- Delegate lifecycle review to the plan owner. The active runbook completion
  criteria appear satisfied: focused coverage is green, the representative no
  longer aborts through stale select publication, and adjacent 572/577 guard
  routes remain green.

## Watchouts

- The repo-wide hook candidate baseline still fails the unrelated
  `string_authority_guard`; supervisor rejected that candidate baseline after
  committing Step 4.
- Full CTest was not used as the acceptance proof because of that known
  unrelated baseline failure. The selected guard subset targets the RV64
  object-route, prepared edge-publication, frame-stack call, 572, and 577
  surfaces touched by this plan.

## Proof

- Proof command:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_riscv_prepared_edge_publication|backend_prepare_frame_stack_call_contract)$'; } > test_after.log 2>&1`
- Result: green. Build completed and all 3 selected tests passed.
- Proof log: `test_after.log`.
- Representative command template:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=<case> -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=<artifact>/clang.bin -DOUT_OBJECT=<artifact>/c4c.o -DOUT_C4C_BIN=<artifact>/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > <artifact>/representative.log 2>&1`
- `src/20000412-2.c` result: green. Log:
  `build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step5/20000412-2/representative.log`.
- `src/20000622-1.c` result: green. Log:
  `build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step5/20000622-1/representative.log`.
