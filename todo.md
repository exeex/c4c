Status: Active
Source Idea Path: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Classify Residuals

# Current Packet

## Just Finished

Executed Step 4 narrow proof/classification for idea 371 on implementation
baseline `f0e556f6f`. The representative subset is green: `00157` and `00176`
both pass under the delegated command.

Assembly classification confirms the selected snapshot publication repair:

- `00157`: in the first dynamic local array writeback loop, the selected store
  value is materialized before the concrete lane store consumes it. For example,
  `00157.c.s:53-62` compares the dynamic index, selects either the old lane or
  recomputed square (`mul w9, w9, w10`), stores the selected value to its home
  with `str w9, [sp, #92]`, then reloads that published home at
  `00157.c.s:63` before the lane writeback `str w9, [sp]` at `00157.c.s:64`.
  The same pattern repeats for later lanes (`#100`, `#108`, etc.), so the
  previous unmaterialized selected-store-home overwrite is removed.
- `00176`: in `partition`, the dynamic global-array selected load feeding
  `array[i] < pivotValue` is rebuilt before the compare. The generated code
  loads the loop index from `[sp, #12]`, compares it against lane numbers,
  materializes the selected `array+N` lane through `adrp`/`add`/`ldr` branches,
  and reaches `cmp w9, w13` at `00176.c.s:3839`. The old stale `%t14.outer0`
  stack-home load at `sp + 260` is no longer the compare input; the controlling
  value is the selected live global lane in `w9`.

No residual first bad fact remains for the idea 371 representatives. Broader
backend-regex/full-suite guard before closure remains supervisor-owned.

## Suggested Next

Supervisor can run the broader backend-regex/full-suite regression guard and
decide whether idea 371 is ready for lifecycle close.

## Watchouts

This Step 4 packet intentionally did not modify implementation files. Closed
idea 348 remains archived. Treat any broader-suite failures as supervisor
classification work rather than residuals proven by the `00157`/`00176`
representative subset.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00176_c)$' > test_after.log 2>&1
```

Result: build completed with no work to do; CTest selected 2 tests, 2 passed,
0 failed. `c_testsuite_aarch64_backend_src_00157_c` and
`c_testsuite_aarch64_backend_src_00176_c` are green. Proof log is
`test_after.log`.
