Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Scalar Integer Select Lowering

# Current Packet

## Just Finished

- Step 4 of `plan.md` advanced the later representative blocker at
  `logic.rhs.end.116 -> logic.end.117`.
- RV64 object emission now resolves select-edge publication through the
  semantic parallel-copy source/destination when stale numeric move ids would
  otherwise select the wrong indexed publication. It also authorizes nested
  select-chain roots when the binary source is proven to feed only the
  destination select chain, while preserving direct carrier-alias fail-closed
  coverage.
- The focused proof `backend_riscv_object_emission` is green.
- The representative no longer fails at the `%t122 -> %t126` pre-terminator
  move bundle. It now reaches the next scalar select materialization blocker:
  `unsupported_instruction_fragment ... block=logic.end.117; block_index=20;
  instruction_index=2; instruction_kind=SelectInst;
  owner=i32 %t126.phi.sel0`.

## Suggested Next

- Continue active plan 573 Step 4 by implementing direct scalar integer select
  materialization for `%t126.phi.sel0`-style join-chain selects in
  `logic.end.117`.

## Watchouts

- The focused Step 3 predecessor-compare fixture is green without expectation
  weakening. Keep rejecting unsupported-marker, allowlist, representative-name,
  value-name, block-name, or disassembly-offset shortcuts.
- The stale diagnostic ids (`33 -> 34`) still appear in the object-route error
  text for this representative family; use the parallel-copy labels and value
  names from prepared output for semantic classification.
- Direct carrier-alias fail-closed coverage must stay green; nested select-chain
  authority should not admit a deliberately broken one-hop carrier-alias shape.
- The remaining blocker is still scalar integer select/phi-select lowering, not
  a branch, call, pointer, runtime-comparison, or expectation problem.

## Proof

- Build freshness command:
  `cmake --build --preset default`
- Focused proof command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
- Focused proof result: passed.
- Representative object-route command:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20030408-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4c/src_20030408-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4c/src_20030408-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4c/src_20030408-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > build/agent_state/573_rv64_select_phi_select_lowering/step4c/src_20030408-1.c/object-route.log 2>&1`
- Representative result: failed later at direct `SelectInst`
  `%t126.phi.sel0`, proving the old `%t122 -> %t126` publication blocker is
  bypassed.
- Representative log:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4c/src_20030408-1.c/object-route.log`.
- Proof log: `test_after.log`.
