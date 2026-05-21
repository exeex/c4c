Status: Active
Source Idea Path: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Selected Aggregate Address Writeback

# Current Packet

## Just Finished

Executed Steps 2 and 3 for idea 371 as one implementation packet. Added focused
AArch64 backend coverage for selected aggregate snapshot values before
consumers:

- a dynamic local-array selected `store_local` value whose true arm is a
  same-block `mul`, proving the selected value is materialized and published to
  its stack home before the concrete lane writeback reloads it;
- a dynamic global-array selected load feeding a fused compare/branch, proving
  the branch compares the materialized selected load rather than the stale
  selected-value stack home.

Implemented the selected snapshot value publication repair in AArch64 dispatch.
`StoreLocal` lowering now recognizes same-block select producers, publishes the
selected value before local lane writeback consumers, and handles address-
materialized store paths without bypassing that publication. Same-block `mul`
producers are now publishable so selected store values such as `Count * Count`
can be materialized through the general scalar publication path. Fused compare
publication now recognizes selected operands and forces current memory-load
materialization for the selected chain before branch lowering, so dynamic
global-array loads are selected from the live global lanes instead of stale
prepared homes.

The representative assembly confirms the repaired `00176` boundary: in
`partition`, after loading the loop index from `[sp, #12]`, the generated select
chain compares that index against lane numbers, loads the selected `array+N`
global lane, and then emits `cmp w9, w13` at `00176.c.s:3839`. The previous bad
compare that consumed unmaterialized `%t14.outer0` at stack offset 260 is no
longer the controlling value. `00157` now passes, proving the local selected
store-local writeback path no longer overwrites array lanes from unmaterialized
selected-store homes.

## Suggested Next

Supervisor can review and commit the Steps 2/3 packet, then run the normal
acceptance guard or advance idea 371 to Step 4 if this subset remains the
chosen representative proof.

## Watchouts

Do not broaden this packet into unrelated residual buckets. Closed idea 348
remains archived; this slice is the follow-up selected snapshot value
publication boundary, not a replay of the earlier selected-address/writeback
route. The repair is semantic, but it does add another selected-chain
publication path; supervisor-side broader guard remains appropriate before
closing the source idea.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00176_c)$' > test_after.log 2>&1
```

Result: build completed; CTest selected 145 tests, 145 passed, 0 failed.
`backend_.*`, `c_testsuite_aarch64_backend_src_00157_c`, and
`c_testsuite_aarch64_backend_src_00176_c` are green. Proof log is
`test_after.log`.
