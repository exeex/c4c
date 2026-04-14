# C4C Regression Guard Policy

Use this policy when evaluating whether a patch is acceptable for handoff.

## Hard Rules

1. Compare matching before/after logs from the same test command.
2. Mid-task subset checks are allowed when they directly prove the current
   slice.
3. Full-suite comparison is still appropriate for broader acceptance, final
   validation, or when the packet explicitly requires it.
4. Keep the result strictly monotonic by default:
   - Total passed tests must increase.
   - No previously passing test may become failing.
5. Never suppress failures by editing vendored tests in `tests/c-testsuite/` or `tests/llvm-test-suite/`.
6. Keep regression logs as artifacts for supervisor review; do not delete them
   during the validation flow.

## Baseline Rule

Use existing executor-produced logs first when they already cover the needed
scope.

If the worktree is already dirty and you need a true pre-change baseline:

1. `git stash push --include-untracked`
2. build and run the chosen test scope to create `test_before.log`
3. `git stash pop`
4. rebuild and rerun the same scope to create `test_after.log`

If the worktree is clean before execution starts, capture `test_before.log`
directly without stashing.

## Runtime Rule

Treat any test runtime above `30s` as suspicious unless explicitly documented.
Use `--enforce-timeout` in the checker when runtime policy must be hard-gated.

## Standard Commands

```bash
ctest --test-dir build -j --output-on-failure -R backend > test_before.log
# ... implement patch ...
ctest --test-dir build -j --output-on-failure -R backend > test_after.log

python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log

python3 .codex/skills/c4c-regression-guard/scripts/report_fail_categories.py \
  --log test_after.log
```
