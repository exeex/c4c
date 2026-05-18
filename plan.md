# AArch64 C-Testsuite Failure Family Inventory

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Goal

Classify the active 212-case AArch64 backend c-testsuite failures, split
semantic repair directions into focused ideas, and switch to one focused idea
before implementation.

## Core Rule

This is an umbrella inventory plan. Do not implement fixes here.

## Known Counts

```text
212 total
 46 PASS
 49 FRONTEND_FAIL
 87 RUNTIME_NONZERO
  7 RUNTIME_MISMATCH
 23 TIMEOUT
  0 BACKEND_FAIL
```

## Steps

### Step 1: Confirm Safe Inventory

Refresh or reuse the 212-case classification. If rerunning, use an explicit
timeout and check for stale generated-runtime processes afterward.

Completion check: `todo.md` records the current counts and log paths.

### Step 2: Inspect Runtime Failure Families

Sample `RUNTIME_NONZERO` and `RUNTIME_MISMATCH` cases, compare source,
expected output, generated assembly, and failure mode, then group by semantic
owner.

Completion check: `todo.md` records grouped runtime families and candidate
focused ideas.

### Step 3: Inspect Frontend Boundary

Sample `FRONTEND_FAIL` cases and decide whether they are true frontend/BIR gaps
or mislabeled backend-route handoff failures.

Completion check: frontend-owned work is separated from backend/runtime repair
ideas.

### Step 4: Defer Or Split Timeout Work

Treat `TIMEOUT` cases as late-stage unless a safe common semantic owner is
obvious.

Completion check: timeout cases are deferred, quarantined, or split into a
safe focused idea.

### Step 5: Switch To Focused Idea

Create one or more focused `ideas/open/*.md` files for tractable semantic
repair directions, then switch lifecycle state to the highest-value focused
idea.

Completion check: this umbrella plan is no longer the active implementation
target.
