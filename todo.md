Status: Active
Source Idea Path: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement The Narrow Materialization Path

# Current Packet

## Just Finished

Completed plan Step 2 by extending RV64 prepared out-of-SSA move-bundle
lowering to materialize only explicit
`phi_join_immediate_materialization` moves when the prepared facts prove:
pre-terminator traversal, block-entry `out_of_ssa_parallel_copy` authority,
matching bundle and move predecessor/successor coordinates, predecessor
terminator execution, no cycle/temp step, `op_kind=move`, value register
destination storage with a GPR home, and an explicit `source_imm_i32` fact.

Focused backend coverage was added for the semantic family using a nonzero
immediate, plus fail-closed mutations for missing immediate facts, wrong
reason/source shape, unsupported destination storage, mismatched edge
coordinates, missing step coordinates, cycle-temp steps, and non-move ops.

## Suggested Next

Execute plan Step 3 as a review/coverage packet: confirm the focused backend
coverage is sufficient for the implemented immediate materialization family, or
add any missing non-overfit coverage without weakening unsupported contracts.

## Watchouts

- The materialization is intentionally inside the out-of-SSA parallel-copy path,
  not the select-publication path.
- Missing bundle-level edge coordinates can be rejected by the shared prepared
  consumer classifier before RV64-local lowering runs; RV64 still has a
  fail-closed coordinate gate for any bundle that reaches target lowering.
- Stack destinations, memory destinations, pointer immediates, cycle/temp
  moves, and non-`source_imm_i32` source shapes remain unsupported.

## Proof

Delegated proof command:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
