---
name: c4c-regression-guard
description: Enforce c4c full-suite regression guardrails using CTest before/after logs. Use when a user asks to validate that a patch did not regress tests, to compare `test_fail_before.log` vs `test_fail_after.log`, or to gate changes on monotonic pass-rate improvement with zero newly failing tests.
---

# C4C Regression Guard

## Overview

Run a deterministic before/after comparison for c4c test logs and reject regressive patches.
Prefer script execution over ad-hoc log reading to keep decisions consistent.

## Workflow

1. Ensure both logs exist (`test_fail_before.log`, `test_fail_after.log`).
2. Run the checker script:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_fail_before.log \
  --after test_fail_after.log
```
3. Interpret result:
   - Exit `0`: guard passed.
   - Exit `1`: regression guard failed (new failing tests, non-increasing passes, or timeout-policy issue if enforced).
   - Exit `2`: parse/input problem.
4. If guard fails, inspect:
   - Newly failing tests list.
   - Pass-count delta.
   - Timeout violations if `--enforce-timeout` is used.
5. Apply the smallest fix and rerun targeted tests, then full suite and guard check.

## Commands

Baseline run:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_fail_before.log \
  --after test_fail_after.log
```

Failure category report from after-log:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/report_fail_categories.py \
  --log test_fail_after.log
```

Enforce suspicious-timeout policy (`>30s`):
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_fail_before.log \
  --after test_fail_after.log \
  --timeout-threshold 30 \
  --enforce-timeout
```

Allow equal pass count for special maintenance commits:
```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_fail_before.log \
  --after test_fail_after.log \
  --allow-non-decreasing-passed
```

## Policy Reference

Read `references/regression-policy.md` when policy language needs to be restated in a review or handoff.

Use this skill whenever regression status is part of acceptance criteria.
