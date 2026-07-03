Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Scalar Integer Select Lowering

# Current Packet

## Just Finished

- Step 4 of `plan.md` classified the later representative blocker at
  `logic.rhs.end.116 -> logic.end.117` after commit `6e6dd0856`.
- The unsupported object-route diagnostic still reports the failing
  predecessor copy as `move[0].from_value_id=33` to `move[0].to_value_id=34`,
  but the refreshed prepared dump resolves the labeled parallel-copy edge as
  `%t122 -> %t126`; the current prepared value-home rows record `%t122`
  as value id 54 and `%t126` as value id 55.
- BIR around the edge:
  `logic.rhs.end.116: bir.br logic.end.117`; `logic.end.117` computes
  `%t122 = bir.ne i32 %t121, 70`, then builds `%t126.phi.sel0` through
  `%t126.phi.sel5`, and finally `%t126 = bir.select ne i32 %t84, 0,
  i32 %t126.phi.sel2, %t126.phi.sel5`.
- Source `%t122` is a binary compare result, not a nested select. Destination
  `%t126` is a nested select materialization root.
- Applicable authority exists for the phi/select join transfer and predecessor
  parallel copy: `join_transfer logic.end.117 result=%t126
  carrier=select_materialization ownership=authoritative_branch_pair` with
  `edge_transfer logic.rhs.end.116 -> logic.end.117 incoming=%t122
  destination=%t126`, plus `parallel_copy logic.rhs.end.116 -> logic.end.117`
  with `move[0] %t122 -> %t126`.
- Publication/select-chain authority exists for the destination:
  `block_entry_publication successor=logic.end.117 status=available to=%t126`
  and `select_chain function=test1 block=logic.end.117 value=%t126
  source_producer=select_materialization`.
- Carrier-alias authority is the missing part:
  `select_carrier_alias_authority function=test1 status=missing_carrier_aliases
  predecessor=logic.rhs.end.116 successor=logic.end.117 destination=%t126
  source=%t122 source_producer=binary ... carrier_alias_candidates=0
  carrier_aliases=0 source_use_closure=no`.

## Suggested Next

- Keep this in active plan 573 Step 4 and target nested select source
  publication for `%t122 -> %t126`: use select-chain authority for nested
  select materialization/publication while retaining the existing compare,
  carrier, edge-publication, register-home, and fail-closed checks.

## Watchouts

- The focused Step 3 predecessor-compare fixture is green without expectation
  weakening. Keep rejecting unsupported-marker, allowlist, representative-name,
  value-name, block-name, or disassembly-offset shortcuts.
- Commit `6e6dd0856` did not cover this later blocker because it repaired
  carrier-authorized compare-source predecessor publication where `.phi.sel*`
  carrier aliases are available, such as the earlier `%t80 -> %t84` edge. This
  edge has a compare source but publishes into a nested select-chain root
  (`%t126`) with no carrier-alias candidates.
- Do not split this yet: the missing boundary is still scalar integer
  select/phi-select publication, not an unrelated branch, call, pointer,
  runtime-comparison, or expectation problem.
- Do not treat this as only direct scalar select materialization unless the next
  implementation proves the nested select-chain root is covered by that path.

## Proof

- Build freshness command:
  `cmake --build --preset default`
- Build result: succeeded; Ninja reported no work to do.
- Prepared dump command:
  `build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir tests/c/external/gcc_torture/src/20030408-1.c > build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/dump-prepared-bir.txt`
- Representative object-route command:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20030408-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/object-route.log 2>&1`
- Representative result: failed as expected at
  `logic.rhs.end.116 -> logic.end.117`; classification completed.
- Classification artifact:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/classification.md`.
- Prepared dump:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/dump-prepared-bir.txt`.
- Representative log:
  `build/agent_state/573_rv64_select_phi_select_lowering/step4b/src_20030408-1.c/object-route.log`.
- Proof log: `test_after.log`.
