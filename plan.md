# Backend Regex Failure Family Inventory

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Classify the main build `ctest -R backend` failures, split semantic repair
directions into focused ideas, and switch to one focused idea before coding.

## Core Rule

This is an umbrella inventory plan. Do not implement fixes here.

## Known Context

- Main build path: `/workspaces/c4c/build`
- User-observed command:

```bash
ctest -j10 -R backend
```

- Current backend regex test count:

```text
352 tests selected by `ctest -R backend`
212 are `c_testsuite_aarch64_backend_*`
about 80 were observed failing
```

## Steps

### Step 1: Capture Current Backend Regex Inventory

Run the backend regex test from the main build with output capture, then check
for stale runtime processes.

Completion check: `todo.md` records the command, log path, pass/fail count,
and stale-process check result.

### Step 2: Classify Failure Sources

Separate local backend/unit/CLI failures from AArch64 c-testsuite backend
runtime failures, frontend handoff failures, and timeout/hang cases.

Completion check: `todo.md` records failure buckets and representative tests.

### Step 3: Compare Against Closed Owners

Compare the buckets against closed AArch64 owners 285 through 294 before
reopening or splitting any follow-up.

Completion check: `todo.md` records which closed owners remain valid and which
new owner candidates exist.

### Step 4: Split Focused Ideas

Create focused `ideas/open/*.md` files for tractable semantic repair families.

Completion check: at least one focused idea exists, or `todo.md` explains why
no semantic owner is ready.

### Step 5: Switch To Focused Idea

Deactivate this umbrella runbook and activate the highest-value focused repair
idea.

Completion check: implementation work is no longer attached to this umbrella
inventory.
