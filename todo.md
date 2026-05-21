Status: Active
Source Idea Path: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Classify Residuals

# Current Packet

## Just Finished

Refined the idea 371 implementation after supervisor full-suite guard found a
new `00181` AArch64 segfault. The regression was in pointer-derived dynamic
indexed load publication exposed by the selected-chain/current-memory-load
repair: address publication built the pointer base in `x9`, then materialized
the `index * 4` scale by writing `mov x9, #4`, clobbering the base before
`add x9, x9, x10`.

Added focused backend coverage for a pointer-derived scaled-index load where
the base and scaled index carriers must remain distinct before the final
`ldr`. Also updated the existing byte-offset scale coverage to accept the more
direct power-of-two lowering.

Implemented the refinement by recognizing non-negative power-of-two immediate
multipliers in scalar publication and direct scalar multiply lowering. These
now publish the scaled value with `lsl` instead of borrowing the other scratch
register for `mov #scale; mul`, avoiding clobber of a live pointer base. This
preserves the existing `00157` local selected store publication and `00176`
selected global load/compare fixes.

Assembly classification for `00181` confirms the repair in `Move`: the first
pointer-derived load now emits `mov x9, x0; ldr w10, [sp, #36]; sxtw x10, w10;
lsl x10, x10, #2; add x9, x9, x10; ldr w13, [x9]` at
`00181.c.s:284-289`, so the pointer base survives scale materialization.
`00157`, `00176`, and `00181` all pass in the delegated proof. No representative
residual remains in this refinement subset; broader backend-regex/full-suite
guard before closure remains supervisor-owned.

Supervisor full-suite baseline review also passed after the refinement. The
accepted baseline before idea 371 had `3347` passed and `28` failed out of
`3375`; the hook-generated candidate at `3bbe5f8f0` has `3350` passed and
`25` failed out of `3375`. The regression guard reports resolved
`c_testsuite_aarch64_backend_src_00157_c`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00183_c`, with no new failing tests. The
canonical `test_baseline.log` was rolled forward to this accepted result.

## Suggested Next

Supervisor can route idea 371 to lifecycle closure.

## Watchouts

Closed idea 348 remains archived. This refinement stays inside the idea 371
selected snapshot/pointer-derived publication fallout and does not change
expectations. Treat any broader-suite failures outside `00157`/`00176`/`00181`
as supervisor classification work.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$' > test_after.log 2>&1
```

Result: build completed; CTest selected 146 tests, 146 passed, 0 failed.
`backend_.*`, `c_testsuite_aarch64_backend_src_00157_c`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00181_c` are green. Proof log is
`test_after.log`.

Supervisor full-suite baseline review compared `test_baseline.log` against
the hook-generated `test_baseline.new.log`:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_baseline.new.log
```

Result: passed, `3347 -> 3350` passed, `28 -> 25` failed, three resolved
tests, and no new failures.
