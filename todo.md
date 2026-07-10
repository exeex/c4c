Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Representative Runtime Proof

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed representative `loop-2e.c` evidence after
baseline review. New artifacts live under
`build/agent_state/657_step1_reactivation_runtime_proof/summary.md`.

The delegated proof command passed and preserved output in `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_loop_2e_c$' > test_after.log 2>&1`.
`test_after.log` reports `100% tests passed, 0 tests failed out of 1`.

Fresh BIR, prepared-BIR, ASM, object, disassembly, clang runtime, and c4c RV64
object-runtime comparison evidence were regenerated. The representative RV64
object runtime matches clang; the standalone comparison reports
`[PASS][rv64-gcc-torture-backend-obj]` for
`tests/c/external/gcc_torture/src/loop-2e.c`.

The explicit prepared `base=pointer_value` access fact remains present for the
callee indirect store:
`access block=block_1 inst_index=7 base=pointer_value stored=%t8 pointer=%t9`.
That fact now corresponds to the caller-visible object store `sd t1,0(s1)`.
The local cursor writeback remains separate as `bir.store_local %lv.param.q,
ptr %t10` and object `sd t1,0(sp)`.

`%t23` pointer-source publication remains present as
`%t23 = bir.add ptr %t21, 156`, and the branch compare remains
`%t24 = bir.ne ptr %t20, %t23`. Prepared branch RHS authority remains selected
and pointer-proven for `%t23`.

## Suggested Next

Delegate Step 2 regression proof against the accepted backend baseline context.

## Watchouts

- Do not reopen the completed `%t23` source-publication route unless fresh
  evidence proves a regression.
- Do not special-case `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Preserve the existing explicit prepared `base=pointer_value` access contract.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- Keep stack-destination fan-in, byval, object-emission, AArch64, CLI, and
  LLVM torture work out of this packet unless focused evidence proves the same
  first owner.
- Step 1 evidence is representative/runtime-focused only; Step 2 still owns the
  accepted backend regression comparison.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_loop_2e_c$' > test_after.log 2>&1`.

Proof log: `test_after.log`.
