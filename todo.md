Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Stack-Carried Pointer Evidence

# Current Packet

## Just Finished

Step 1, `Refresh Stack-Carried Pointer Evidence`: refreshed prepared-BIR,
object, disassembly, and runtime evidence for the two representative
stack-carried pointer rows.

- Evidence root:
  `build/agent_state/653_step1_stack_carried_pointer_evidence/summary.md`.
- `tests/c/external/gcc_torture/src/20140828-1.c` `%t6`: prepared consumer is
  `main` `entry`, `bir.cond_br i32 %t7, block_2, block_3`, with
  `branch_condition ... compare=ne ptr %t4, %t6`. The selected stack slot is
  `%t6` value id `19`, slot `#16`, object `#17`, stack offset `8`, size `8`,
  align `8`. Source value is source-level `&a[1]`; current prepared facts only
  publish `%t6` as a call-preserved stack-slot value and branch-stack-load
  authority, with no explicit local-frame address materialization/source
  publication for `%t6` itself. Object emission succeeds; linked disassembly
  compares the call result in `t0` with `ld t4,8(sp)` at the first branch.
  Runtime still fails with `[RV64_BACKEND_RUNTIME_MISMATCH]`,
  `clang_exit=0`, `c4c_exit=Subprocess aborted`. First owner remains the
  prepared/RV64 stack-carried pointer source materialization/publication for
  `%t6` into slot `#16`, not terminator admission.
- `tests/c/external/gcc_torture/src/loop-2e.c` `%t23`: prepared consumer is
  `main` `block_6`, `bir.cond_br i32 %t24, block_13, block_14`, with
  `branch_condition ... compare=ne ptr %t20, %t23`. The selected stack slot is
  `%t23` value id `27`, slot `#46`, object `#51`, stack offset `336`, size
  `8`, align `8`. Source value is source-level `(int *)p + 39`; current
  prepared facts only publish `%t23` as a call-preserved stack-slot value and
  branch-stack-load authority, with no explicit address-add source
  publication/materialization for `%t23` itself. Object emission succeeds;
  linked disassembly compares loaded `q[39]` in `t0` with `ld t4,336(sp)`.
  Current runtime runner passes, so `%t23` no longer proves a failing runtime
  owner even though the publication/materialization fact gap remains visible.

## Suggested Next

Executor packet: trace the `%t6` producer/materialization path for
`src/20140828-1.c` from the source-level `&a[1]` value through prepared
selection, call preservation, slot `#16`, and the RV64 branch consumer; identify
the narrow producer fact that should explicitly publish the materialized
stack-carried pointer source.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- `loop-2e.c` now passes the direct runtime runner; do not use `%t23` as the
  first failing runtime proof unless a later packet identifies a still-red
  focused owner.
- The `%t6` row has explicit branch-stack-load authority and call-preserve
  metadata, but no explicit `%t6` source materialization/publication fact. Keep
  the next packet on that producer boundary rather than widening to unrelated
  branch shapes.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed, but the delegated backend subset remains red with 32
failed tests. Failure names match `test_before.log`, so this evidence-only
packet did not introduce a new backend failure set.
