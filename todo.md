Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Four-test Reproducer

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Classify The Four-test Reproducer", by running
the supervisor-selected narrow reproducer once and preserving the combined
build plus CTest output in `test_after.log`.

Per-test narrow-run classification:

- `c_testsuite_aarch64_backend_src_00040_c`: passed under the narrow command;
  the rejected-candidate timeout did not reproduce in this run.
- `c_testsuite_aarch64_backend_src_00119_c`: failed under the narrow command
  with `[RUNTIME_NONZERO]`, exit `48`.
- `c_testsuite_aarch64_backend_src_00123_c`: failed under the narrow command
  with `[RUNTIME_NONZERO]`, exit `48`.
- `c_testsuite_aarch64_backend_src_00195_c`: failed under the narrow command
  with `[RUNTIME_MISMATCH]`; expected `12.340000, 56.780000`, actual
  `0.000000, 0.000000`.

The narrow command selected all four requested tests. Current evidence shows a
reproducing three-test core (`00119`, `00123`, `00195`) and no reproduced
`00040` timeout. Because this packet ran the fixed command once, the three
failures look immediately reproducible but still need repeat-run or
cross-commit classification before being called deterministic noise-free
regressions.

## Suggested Next

Execute `plan.md` Step 3: use the same narrow reproducer across candidate
commits or lifecycle slices to identify where the reproducing
`00119`/`00123`/`00195` family first appears, while treating the non-reproduced
`00040` timeout as a separate noise-or-history question.

## Watchouts

- `00119`, `00123`, and `00195` reproduce under the current narrow command;
  `00040` passed and should not be grouped as deterministic without repeat-run
  or historical proof.
- The earliest historical regressive candidate found after the accepted
  baseline is `76bfe950d`, not `9c1add9c`, and its failure set is not identical
  to the rejected candidate. Treat this as an evidence gap for Step 3, not a
  first-bad answer.
- Keep `test_baseline.new.log` rejected until a non-regressive full-suite
  candidate exists. Do not weaken c_testsuite expectations.

## Proof

Ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

Result: build succeeded, CTest selected 4/4 requested tests, 1 passed and 3
failed. The command exited nonzero because `00119`, `00123`, and `00195`
failed. Combined output is preserved at `test_after.log`.
