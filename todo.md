Status: Active
Source Idea Path: ideas/open/573_rv64_select_phi_select_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rehydrate Select Evidence And Reproduce

# Current Packet

## Just Finished

- Step 1 of `plan.md` rehydrated the saved 570 select evidence and reran the
  current RV64 object-route representative for
  `tests/c/external/gcc_torture/src/20030408-1.c`.
- Saved 570 evidence still classifies the old first unsupported owner as
  `function=test1; block=logic.end.117; instruction_kind=SelectInst;
  owner=i32 %t126.phi.sel0`, and the saved/current prepared BIR shape is a
  nested phi-select publication chain rather than a simple standalone select.
- Current reproduction no longer reaches that saved select owner first. The
  first unsupported route now fails earlier on
  `unsupported_move_bundle_target_shape` for an out-of-SSA
  `pre_terminator_copies` move bundle at `function=test1`,
  `block_label=logic.rhs.end.74`, `instruction_index=0`,
  `fragment_status=generic_move_bundle_materialization_failed`.

## Suggested Next

- Supervisor should decide whether to route the earlier RV64 move-bundle
  blocker through lifecycle state before continuing Step 2 select-boundary
  classification. If the move-bundle blocker is cleared, the refreshed prepared
  BIR still contains the nested `%t126.phi.sel*` publication shape for this
  plan to classify.

## Watchouts

- Do not treat `src/20030408-1.c`, `test1`, `logic.end.117`, or
  `%t126.phi.sel0` as part of the repair contract.
- Keep select lowering separate from same-module call, inline asm carrier,
  floating-point binary, pointer arithmetic, branch/CFG reconstruction, and
  runtime comparison work unless focused evidence proves the first owner moved.
- Expectation rewrites, unsupported-marker edits, allowlist changes, and
  diagnostic-only renames are not capability progress.
- The first owner did move in the current object-route rerun: it is now a
  predecessor-terminator register-to-register move bundle before the saved
  select block. Treating the current representative as select-first would be
  stale evidence.

## Proof

- Build refresh: `cmake --build --preset default --target c4cll` exited 0
  (`ninja: no work to do`).
- Reproduction proof in `test_after.log` and
  `build/agent_state/573_rv64_select_phi_select_lowering/step1/src_20030408-1.c/object-route.log`:
  the delegated RV64 gcc-torture object-route CMake case exited 1 with
  `[RV64_C4C_OBJ_COMPILE_FAIL]` and the current first unsupported diagnostic
  `unsupported_move_bundle_target_shape`.
- Refreshed prepared-BIR evidence:
  `build/agent_state/573_rv64_select_phi_select_lowering/step1/src_20030408-1.c/dump-prepared-bir.txt`.
  The dump command
  `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030408-1.c`
  exited 0; command metadata is in
  `build/agent_state/573_rv64_select_phi_select_lowering/step1/src_20030408-1.c/dump-prepared-bir.command.log`.
