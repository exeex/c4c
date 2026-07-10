Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Publication And Materialization Boundary

# Current Packet

## Just Finished

Step 2, `Locate The Publication And Materialization Boundary`: traced the
`tests/c/external/gcc_torture/src/20140828-1.c` `%t6` path and recorded the
prepared/RV64 boundary in
`build/agent_state/653_step2_t6_pointer_publication_trace/summary.md`.

- `%t6` producer/source: source-level `&a[1]`, meaning local-frame source
  `%lv.a.0 + 2` for `short a[2]`. Prepared `main` has address materialization
  facts for the actual call operands `%lv.a.0` at slot `#5` offset `4` and
  `%lv.d` at slot `#4` offset `0`, but no normal producer/materialization row
  or source publication for `%t6` itself.
- Selected slot: `%t6` value id `19`, slot `#16`, object `#17`, stack offset
  `8`, size `8`, align `8`, carried as
  `preserve value=%t6 ... route=stack_slot`.
- Consumer branch: `main` `entry`,
  `bir.cond_br i32 %t7, block_2, block_3`, fused compare
  `compare=ne ptr %t4, %t6`; branch RHS stack-load authority is available for
  `%t6` with `source_freshness_authority=branch_stack_slot`.
- Current publication/materialization facts: RV64 emits the call argument
  `a` as `addi a0,sp,4` from the existing local-frame address publication, but
  later compares the call result against `ld t4,8(sp)`. No prepared fact proves
  slot `#16` was populated with `%lv.a.0 + 2` before the call or that the
  stack-carried value source was explicitly published.
- Boundary: `src/backend/prealloc/publication_plans.cpp` owns the branch
  stack-load authority and clobber/call-preserve validation, while
  `src/backend/mir/riscv/codegen/object_emission.cpp` consumes only selected
  `BranchStackLoadSource` freshness before reloading stack operands.
  `src/backend/prealloc/call_plans.cpp` already publishes local-frame address
  materialization for call operands, but there is no prepared producer
  authority that binds branch-only `%t6` value id `19`, slot `#16`, to
  `%lv.a.0 + 2`.
- Narrow first implementation surface or missing authority: add or fail closed
  on an explicit prepared stack-carried pointer source
  publication/materialization fact for `%t6` that records source identity,
  materialized value identity, and selected stack home. RV64 terminator
  admission should remain a consumer of that fact, not infer it from final
  stack offsets or source spelling.

## Suggested Next

Executor packet: implement the narrow prepared stack-carried pointer source
publication/materialization rule, or add the precise fail-closed diagnostic if
the producer cannot yet prove `%t6`'s source identity, materialized value, and
slot `#16` home.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- `loop-2e.c` now passes the direct runtime runner; do not use `%t23` as the
  first failing runtime proof unless a later packet identifies a still-red
  focused owner.
- The `%t6` row has explicit branch-stack-load authority and call-preserve
  metadata, but no explicit `%t6` source materialization/publication fact.
  Preserve RV64's fail-closed behavior for missing, stale, ambiguous, and
  mismatched producer facts; do not make RV64 infer the source from slot
  offsets, final assembly shape, or testcase identity.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed, but the delegated backend subset remains red with 32
failed tests. The failed-test list matches `test_before.log`, so this
trace/evidence-only packet did not introduce a new backend failure set.
