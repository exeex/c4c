# C4C Regression Guard Policy

Use this policy when evaluating whether a patch is acceptable for handoff.

## Hard Rules

1. Compare full-suite logs (`ctest --output-on-failure`) before and after the patch.
2. Keep the result strictly monotonic by default:
   - Total passed tests must increase.
   - No previously passing test may become failing.
3. Never suppress failures by editing vendored tests in `tests/c-testsuite/` or `tests/llvm-test-suite/`.

## Runtime Rule

Treat any test runtime above `30s` as suspicious unless explicitly documented.
Use `--enforce-timeout` in the checker when runtime policy must be hard-gated.

## Standard Commands

```bash
ctest --test-dir build -j --output-on-failure > test_fail_before.log
# ... implement patch ...
ctest --test-dir build -j --output-on-failure > test_fail_after.log

python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_fail_before.log \
  --after test_fail_after.log

python3 .codex/skills/c4c-regression-guard/scripts/report_fail_categories.py \
  --log test_fail_after.log
```
