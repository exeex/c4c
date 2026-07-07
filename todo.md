Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Scalar Integer Select Lowering

# Current Packet

## Just Finished

- Step 4 of `plan.md` repaired RV64 object-route no-home same-block scalar
  select-chain admission for duplicated linear select trees whose intermediate
  names are not interned in prepared name tables.
- Prepared BIR value-name equality now compares valid interned ids when both
  sides have them and falls back to exact value spelling for uninterned named
  values, so unrelated invalid ids no longer make a no-home select look reused.
- Added focused `backend_riscv_object_emission` coverage for a duplicated
  tree-shaped nested `i32` select chain, while keeping the reused/DAG no-home
  producer fail-closed.

## Suggested Next

- Continue active plan 573 Step 4 by investigating the runtime abort exposed in
  the `20030408-1.c` RV64 object-route representative after scalar integer
  select materialization succeeds.

## Watchouts

- The new select materialization recursion is intentionally limited to scalar
  integer `SelectInst` values in the same block and still requires an ordinary
  move source or a same-block select producer for every condition/value operand.
- No-home same-block select producers are admitted when each producer has one
  later same-block select consumer by exact value identity. Reused producers
  intentionally remain unsupported until object emission has a distinct
  publication strategy for their result.
- No unsupported markers, allowlists, gcc_torture expectations, or runtime
  comparison contracts were changed.
- The representative next failure is `RV64_BACKEND_RUNTIME_MISMATCH` with
  `c4c_exit=Subprocess aborted`, so the next packet should inspect generated
  object behavior or runtime state rather than broadening select admission.

## Proof

- Build freshness command:
  `cmake --build --preset default`
- Focused proof command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
- Focused proof result: passed via the delegated combined proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
- Supervisor guard result:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  346/346.
- Proof log: `test_before.log` after supervisor roll-forward.
- Representative object-route command:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20030408-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4f/src_20030408-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4f/src_20030408-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4f/src_20030408-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > build/agent_state/573_rv64_select_phi_select_lowering/step4f/src_20030408-1.c/object-route.log 2>&1`
- Representative result: failed later at `RV64_BACKEND_RUNTIME_MISMATCH` with
  `c4c_exit=Subprocess aborted`; the old direct
  `unsupported_instruction_fragment ... owner=i32 %t126.phi.sel0` no longer
  appears first.
- Representative log:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4f/src_20030408-1.c/object-route.log`.
