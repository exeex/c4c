Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Attribute The First Bad Step

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Attribute The First Bad Step", by running the
same narrow four-test reproducer across the accepted-good to observed-bad
history bracket in a scratch worktree at `/tmp/c4c-step3-attr`.

First bad commit proven:

- `c8346c7bb052af1ae81c7ca95bee1f71a899ea6d`
  (`migrate FP global-load identity to Route 3`)

Adjacent proof:

- parent `cc4d9e401bc68d96b377b2b399d6451f3aea6b10`
  (`cover Route 3 FP global-load oracle`): all four tests passed.
- child `c8346c7bb052af1ae81c7ca95bee1f71a899ea6d`: `00119`,
  `00123`, and `00195` failed; `00040` passed.

Tested commits and outcomes:

- `af8c2f6efac78a862e7f024cd579d1a2ec487be3`: all four passed.
- `6315a51e992c709eadf5ac621720ac7315a8b0aa`: all four passed.
- `b9b94d8a39db81f09d6bdac81baab438fd07e5ff`: all four passed.
- `cc4d9e401bc68d96b377b2b399d6451f3aea6b10`: all four passed.
- `c8346c7bb052af1ae81c7ca95bee1f71a899ea6d`: `00119`
  `[RUNTIME_NONZERO]` exit `80`, `00123` `[RUNTIME_NONZERO]` exit `32`,
  `00195` `[RUNTIME_MISMATCH]` expected `12.340000, 56.780000` and
  actual `0.000000, 0.000000`; `00040` passed.
- `1985c727e290db7a2440a4e5f93336900064da50`: same three-test failure
  family reproduced; `00040` passed.
- `76bfe950d86dc35d9fdd848c8322023a6105ef26`: same three-test failure
  family reproduced; `00040` passed.

Conclusion: the `00119`/`00123`/`00195` family is deterministic across the
tested bad side of the bracket and first appears in the Route 3 implementation
slice `c8346c7b`. The `00040` timeout did not reproduce at any tested commit
and remains separate noise-or-history evidence, not part of the deterministic
family.

## Suggested Next

Execute `plan.md` Step 4: route a focused repair investigation against the
Route 3 FP global-load identity migration introduced by `c8346c7b`, with the
repair packet starting from the semantic/target-policy boundary that can make
`00119`, `00123`, and `00195` miscompile while parent `cc4d9e40` passes.

## Watchouts

- The first-bad evidence used a scratch worktree with the vendored
  `tests/c/external/c-testsuite` tree copied from the main workspace and
  per-commit configure setup:
  `cmake --preset default -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`.
- The fixed proof command was then run unchanged at each candidate commit.
- `00119` and `00123` fail with varying nonzero exit codes across bad commits,
  but their pass/fail classification is stable across the tested bad side.
- `00195` consistently produces `0.000000, 0.000000` instead of
  `12.340000, 56.780000` across the tested bad side.
- `00040` passed at every tested commit and should not be grouped with the
  deterministic Route 3 failure family without additional evidence.
- Keep `test_baseline.new.log` rejected until a non-regressive full-suite
  candidate exists. Do not weaken c_testsuite expectations.

## Proof

In the scratch worktree, ran the same narrow proof command at each candidate
commit:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

Result: the accepted baseline and parent-side commits passed all four tests;
`c8346c7b` and later tested commits reproduced the same three-test failure
family with `00040` passing.

Scratch log paths:

- `/tmp/c4c-step3-logs/af8c2f6_proof_aarch64.log`
- `/tmp/c4c-step3-logs/6315a51e_proof.log`
- `/tmp/c4c-step3-logs/b9b94d8a_proof.log`
- `/tmp/c4c-step3-logs/cc4d9e40_proof.log`
- `/tmp/c4c-step3-logs/c8346c7b_proof.log`
- `/tmp/c4c-step3-logs/1985c727_proof.log`
- `/tmp/c4c-step3-logs/76bfe950_proof.log`

Per packet instruction, root `test_after.log` was not modified.
