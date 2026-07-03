Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Scalar Integer Select Lowering

# Current Packet

## Just Finished

- Step 4 of `plan.md` repaired RV64 object emission for carrier-authorized
  select-materialized predecessor edge publication where the edge source is an
  integer compare result and the join carrier is a GPR.
- The generic move-bundle path now accepts select edge publications whose
  binary source producer is either the join block producer or the predecessor
  edge producer, while preserving carrier-kind, compare, operand,
  destination-register, edge-publication, and carrier-alias authority checks.
- The out-of-SSA predecessor parallel-copy path now uses the same carrier-alias
  authority for join-block compare producers with `.phi.sel*` carrier aliases,
  and emits the booleanized compare into the selected join carrier instead of
  falling through to a raw register copy.

## Suggested Next

- Classify the next representative RV64 object-route blocker at
  `logic.rhs.end.116 -> logic.end.117` (`%t122 -> %t126`) before deciding
  whether it is the next Step 4 select-publication slice or needs a plan-owner
  split.

## Watchouts

- The focused Step 3 predecessor-compare fixture is green without expectation
  weakening. Keep rejecting unsupported-marker, allowlist, representative-name,
  value-name, block-name, or disassembly-offset shortcuts.
- The representative case advanced past the classified `%t80 -> %t84` first
  blocker and now fails later in the nested select chain. Do not report the
  representative as fully repaired yet.

## Proof

- Proof command:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`
- Result: build succeeded; focused `backend_riscv_object_emission` CTest passed.
- Proof log: `test_after.log`.
- Representative command:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20030408-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4/src_20030408-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4/src_20030408-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4/src_20030408-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > /workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4/src_20030408-1.c/object-route.log 2>&1`
- Representative result: failed later at
  `logic.rhs.end.116 -> logic.end.117` for `%t122 -> %t126`; the original
  classified `%t80 -> %t84` blocker is no longer first.
- Representative log:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4/src_20030408-1.c/object-route.log`.
