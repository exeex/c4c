Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Scalar Integer Select Lowering

# Current Packet

## Just Finished

- Step 4 of `plan.md` fixed the `20030408-1.c` RV64 object-route runtime
  abort exposed after scalar integer select materialization advanced past
  `%t126.phi.sel0`.
- The object-route select-edge source dependency walker now rematerializes
  same-block cast producers before edge compare sources, so a compare like
  `%t66 = (%t65 != 66)` no longer reads a stale register when `%t65` is a
  suppressed cast dependency.
- Added focused `backend_riscv_object_emission` coverage requiring the
  suppressed select-edge predecessor setup path to materialize the cast source
  before the edge compare.

## Suggested Next

- Continue active plan 573 Step 4 by choosing the next RV64 scalar-select
  representative or escalating validation for the now-passing
  `20030408-1.c` object-route case.

## Watchouts

- The new select materialization recursion is intentionally limited to scalar
  integer `SelectInst` values in the same block and still requires an ordinary
  move source or a same-block select producer for every condition/value operand.
- No-home same-block select producers are admitted when each producer has one
  later same-block select consumer by exact value identity. Reused producers
  intentionally remain unsupported until object emission has a distinct
  publication strategy for their result.
- Select-edge compare source dependency rematerialization now handles
  same-block binary and cast producers before the selected compare. It still
  fails closed when those producers cannot be emitted by the existing RV64
  scalar helpers.
- No unsupported markers, allowlists, gcc_torture expectations, or runtime
  comparison contracts were changed.
- Diagnostic artifact: before the fix, `test1` in `20030408-1.c` loaded
  `buffer[1]` into `s1` but compared stale `s2` against `66` on the RHS edge;
  the final `step4g` object now emits the `s1 -> s2` sign-extension before the
  compare.

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
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20030408-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4g/src_20030408-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4g/src_20030408-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4g/src_20030408-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > build/agent_state/573_rv64_select_phi_select_lowering/step4g/src_20030408-1.c/object-route.log 2>&1`
- Representative result: passed; no `RV64_BACKEND_RUNTIME_MISMATCH`.
- Representative log:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4g/src_20030408-1.c/object-route.log`.
