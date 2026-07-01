Status: Active
Source Idea Path: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Completed plan Step 3 coverage acceptance for the generic immediate phi-join
materialization family. The positive coverage still proves a nonzero
semantic immediate-to-register materialization through prepared out-of-SSA
parallel-copy facts, without relying on `src/pr37924.c`, block names, or
immediate value 0.

Kept the new fail-closed source-shape mutation that leaves
`source_immediate_i32=1234` and `phi_join_immediate_materialization` in the move
fact but changes the authoritative parallel-copy source to named `%src`. The
RV64 gate now accepts explicit immediate phi-join materialization only when the
selected prepared parallel-copy move source is also an I32 immediate matching
`source_immediate_i32`.

## Suggested Next

Supervisor review for commit readiness, then either close or select the next
bounded packet for the active idea lifecycle.

## Watchouts

- The materialization is intentionally inside the out-of-SSA parallel-copy path,
  not the select-publication path.
- Missing bundle-level edge coordinates can be rejected by the shared prepared
  consumer classifier before RV64-local lowering runs; RV64 still has a
  fail-closed coordinate gate for any bundle that reaches target lowering.
- Stack destinations, memory destinations, pointer immediates, cycle/temp
  moves, and non-`source_imm_i32` source shapes remain unsupported.
- Do not drop the new source-shape test: it is a non-overfit authority check,
  not a testcase-name or source-file match.

## Proof

Delegated proof command:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. The build completed, the `^backend_` CTest subset passed, and
`git diff --check` passed. Output preserved in `test_after.log`.
