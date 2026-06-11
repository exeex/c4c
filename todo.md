Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove The Final State

# Current Packet

## Just Finished

Completed `plan.md` Step 5, "Prove The Final State".

The focused repair restored the three deterministic Route 3 regressions, and a
fresh full-suite baseline candidate at
`e5218942517a2bb9c3bd0167d810e27bed8273c8` passed 3428/3428 tests. The
candidate was non-regressive relative to the accepted 3427/3427 baseline and
was accepted through `scripts/plan_review_state.py accept-baseline`.

## Suggested Next

Plan-owner lifecycle review: the active runbook appears complete and ready for
close. The final state restored a 100% full-suite baseline candidate without
weakening c_testsuite contracts.

## Watchouts

- `PreparedMemoryAccess` remains the target-addressing/address-policy source;
  no BIR target-address payload was introduced.
- The repair is semantic over FP same-block prepared global loads and does not
  match testcase names, global names, or c_testsuite expectations.
- `00040` remains outside this repair packet.
- `test_baseline.log` now records the repaired 3428/3428 full-suite baseline;
  `test_baseline.new.log` was consumed by the baseline acceptance helper.

## Proof

Narrow repair proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

Result: passed, 5/5 selected tests green:
`backend_aarch64_prepared_memory_operand_records`,
`backend_prepared_lookup_helper`,
`c_testsuite_aarch64_backend_src_00119_c`,
`c_testsuite_aarch64_backend_src_00123_c`, and
`c_testsuite_aarch64_backend_src_00195_c`.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed; resolved the three c_testsuite failures with no new failures.

Full-suite baseline candidate:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure
```

Result: passed, 3428/3428 tests.

Baseline acceptance:

```sh
scripts/plan_review_state.py accept-baseline
```

Result: accepted the repaired full-suite baseline into `test_baseline.log`.
