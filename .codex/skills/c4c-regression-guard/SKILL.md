---
name: c4c-regression-guard
description: Enforce c4c regression guardrails using matching CTest before/after logs. Use when a user asks to validate that a patch did not regress tests, to compare `test_before.log` vs `test_after.log`, or when the supervisor needs to generate missing regression logs itself. Mid-task subset runs such as backend-only coverage are allowed when the before and after commands match exactly.
---

# C4C Regression Guard

## Goal

Run a deterministic before/after comparison for the same test scope and reject
regressive patches.

Use this skill for both:

- narrow in-task validation such as backend-only subsets
- broader acceptance checks when the current slice requires them
- supervisor-side fallback when executor logs are missing

Prefer script execution over ad-hoc log reading so the decision stays
consistent.

This skill assumes compile/build happens before both the `before` and `after`
test captures. A test-only comparison without a fresh rebuild is not an
acceptance-grade regression check.

Canonical filenames are fixed:

- `test_before.log`
- `test_after.log`

Before invoking this skill, the supervisor should normalize any executor output
to those names and remove stray root-level `.log` files.

## Ownership Rule

Normal path:

- supervisor chooses the proving command and prepares `test_before.log`
- executor runs the packet's tests into `test_after.log`
- supervisor reviews those logs

Fallback path:

- if the needed before/after logs do not exist, the supervisor runs this skill
  to generate them
- generated `test_before.log` and `test_after.log` are durable artifacts and
  should not be deleted as part of the guard flow

## Scope Rule

The `before` and `after` logs must come from the same test command.

Allowed:

- backend subset before vs backend subset after
- one owned test binary before vs the same binary after
- full suite before vs full suite after

Not allowed:

- backend subset before vs full suite after
- one regex-filtered run before vs a different regex-filtered run after

## Escalation Triggers

Prefer a broader or full-scope guard, not just a narrow bucket, when:

- shared compiler pipeline, parser, sema, HIR, IR, codegen, ABI, or lowering
  code changed
- build scripts, presets, test harnesses, or validation infrastructure changed
- the current route has accumulated several narrow-only packets and needs a
  stronger checkpoint
- the user or supervisor asked for acceptance-grade confidence
- the slice is being treated as a closure-quality milestone

Prefer the smallest broader scope that matches the risk. Use full-tree
acceptance only when narrower buckets no longer give credible coverage.

## Current Useful CTest Subsets

These are the currently visible CTest subsets from `ctest --test-dir build -N`
in this repo. Counts are a snapshot of the current build and may change over
time, but the regexes are the intended handles for narrow guard runs.

- `^tiny_c2ll_tests$` : 1 test
  good for the tiny driver smoke binary
- `^backend_` : 243 tests
  good default for backend-only work; covers backend unit, route, contract, IR,
  LIR, and target-specific backend checks
- `^frontend_cxx_` : 5 tests
  good for C++ frontend stage and preprocessor smoke work
- `^frontend_hir_tests$` : 1 test
  good for focused HIR-only validation
- `^negative_tests_` : 86 tests
  good for C negative diagnostic and rejection work
- `^verify_tests_` : 5 tests
  good for verify-surface diagnostics
- `^positive_sema_` : 34 tests
  good for C semantic positive-path work
- `^abi_` : 3 tests
  good for ABI-specific focused checks
- `^cpp_negative_tests_` : 46 tests
  good for focused C++ negative-path work
- `^cpp_` : 1097 tests
  broad C++ validation bucket; use only when the current slice really needs it
- `^ccc_review_` : 9 tests
  good for review/regression harness checks
- `^preprocessor_` : 1 test
  good for the standalone preprocessor smoke path
- `^positive_split_llvm_` : 1 test
  good for the split LLVM positive smoke path
- `^c_testsuite_` : 440 tests
  broad C testsuite coverage; expensive relative to targeted unit buckets
- `^clang_c_external_` : 38 tests
  external C compatibility coverage
- `^clang_cpp_external_` : 1 test
  external C++ compatibility smoke coverage
- `^llvm_gcc_c_torture_` : 1467 tests
  very broad torture-suite coverage; reserve for broad acceptance, not routine
  in-task guard checks

When in doubt, pick the smallest subset that directly proves the owned slice.

## Baseline Acquisition

Choose one of these flows.

### Clean Worktree Before The Change

If you are about to start execution and the worktree is already clean, capture
the baseline directly:

```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_before.log
```

### Dirty Worktree Mid-Task

If you are already in the middle of work and need a true pre-change baseline,
stash the current worktree first, run the baseline, then pop the stash:

```bash
git stash push --include-untracked -m c4c-regression-guard
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_before.log
git stash pop
```

After restoring the worktree, rebuild and rerun the same command for the after
log:

```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_after.log
```

If `git stash pop` conflicts, stop and resolve the tree state before trusting
the comparison.

## Workflow

1. Check whether matching `test_before.log` and `test_after.log` already exist
   from executor validation.
2. If they exist and match the intended scope, use them directly.
3. If they do not exist, decide the test scope that actually proves the current
   slice and generate the logs yourself.
4. Do not accept alternate `.log` filenames. Rename them to the canonical pair
   before comparison.
5. Capture `test_before.log` using either the clean-worktree flow or the
   stash/pop flow.
6. Apply the patch or finish the current slice.
7. Rebuild.
8. Run the exact same test command to produce `test_after.log`.
9. Ensure both logs refer to the same scope.
10. Run the checker script:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log
```
11. Interpret result:
   - Exit `0`: guard passed.
   - Exit `1`: regression guard failed (new failing tests, non-increasing passes, or timeout-policy issue if enforced).
   - Exit `2`: parse/input problem.
12. If guard fails, inspect:
   - Newly failing tests list.
   - Pass-count delta.
   - Timeout violations if `--enforce-timeout` is used.
13. If guard passes, the supervisor should roll `test_after.log` forward to
    `test_before.log` for the next slice.
14. Apply the smallest fix and rerun the same scope first.
15. Only escalate to a broader suite when the packet, plan, or user asks for
    it, or when the escalation triggers above apply.

## Commands

Backend-only example:
```bash
git stash push --include-untracked -m c4c-regression-guard
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_before.log
git stash pop

cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_after.log

python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log
```

Clean-worktree shortcut:
```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_before.log
# ... make change ...
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R backend > test_after.log
```

Full acceptance example:
```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure > test_before.log
# ... make change ...
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure > test_after.log
```

Failure category report from after log:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/report_fail_categories.py \
  --log test_after.log
```

Enforce suspicious-timeout policy (`>30s`):
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --timeout-threshold 30 \
  --enforce-timeout
```

Allow equal pass count for maintenance or pure refactor checks:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```

## Policy Reference

Read `references/regression-policy.md` when policy language needs to be restated in a review or handoff.

Use this skill whenever regression status is part of acceptance criteria.
