Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Capture Failure Shape

# Current Packet

## Just Finished

Step 1 `Reproduce And Capture Failure Shape` reproduced both target AArch64
runtime failures with the delegated CTest subset:

- `c_testsuite_aarch64_backend_src_00216_c`: `[RUNTIME_NONZERO]`,
  `exit=Segmentation fault`, empty combined stdout/stderr.
- `c_testsuite_aarch64_backend_src_00204_c`: `[RUNTIME_NONZERO]`,
  `exit=Segmentation fault`, empty combined stdout/stderr.

Minimal debug artifact for root-cause work: `test_after.log` captures the fresh
CTest runner failure shape for both tests.

## Suggested Next

Execute the next root-cause packet for Step 2: compare the generated AArch64
runtime path for `00216` and `00204` against a nearby passing same-feature case
without changing expectations or adding testcase-shaped special cases.

## Watchouts

- Do not weaken, skip, or reclassify `00216` or `00204`.
- Do not add filename-shaped or expected-output-shaped special cases.
- Keep printer-only differences out of scope unless they prove a
  construction-time side effect.
- The fresh failure shape has no stdout/stderr payload, so the next packet will
  need a lower-level debug signal than CTest output alone.

## Proof

Ran:

```sh
ctest --test-dir build -j1 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00(216|204)_c)$' > test_after.log 2>&1
```

Result: failed as expected for the observation packet; both selected tests
segfault at runtime. Proof log: `test_after.log`.
