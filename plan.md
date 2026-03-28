# Positive Template/Sema Regression Runbook

Status: Active
Source Idea: ideas/open/template_positive_sema_regressions_plan.md

## Purpose

Fix the current positive C++ template/sema regressions with small,
mechanism-focused frontend patches and focused regression coverage.

## Goal

Drive the six named positive cases to passing without broad parser or lowering
cleanup:

- `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
- `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
- `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
- `cpp_positive_sema_template_arg_deduction_cpp`
- `cpp_positive_sema_template_mixed_params_cpp`
- `cpp_positive_sema_template_type_subst_cpp`

## Core Rule

Fix one shared mechanism at a time. Do not mix broad refactors, unrelated
frontend cleanups, or testcase-specific EASTL hacks into a regression slice.

## Read First

- [ideas/open/template_positive_sema_regressions_plan.md](/workspaces/c4c/ideas/open/template_positive_sema_regressions_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Scope

- Frontend template argument parsing
- Template deduction and substitution
- Dependent expression handling needed by the named positive cases
- Call-result value-category handling if required by the listed failures

## Non-Goals

- Broad `ast_to_hir.cpp` cleanup
- General parser refactors not required by a current failing mechanism
- Codegen or runtime changes unless a targeted case proves the issue is not
  frontend-owned
- EASTL-specific special cases that do not generalize

## Working Model

1. Start from the highest-priority incomplete item in `plan_todo.md`.
2. Capture the first failing frontend mechanism for the selected testcase.
3. Add the narrowest regression test that isolates that mechanism.
4. Implement the smallest frontend fix that addresses the mechanism.
5. Re-run targeted tests and then the full suite.
6. Record progress and the next slice in `plan_todo.md`.

## Execution Rules

- Prefer one mechanism family per commit.
- Compare behavior against Clang when the failure shape is ambiguous.
- If a testcase requires a new language feature rather than a bug fix, stop and
  spin that work into a separate open idea.
- Keep patches local to the smallest relevant frontend surface.
- Preserve existing passing behavior; full-suite results must be monotonic.

## Ordered Steps

### Step 1: Characterize The Active Failure Cluster

Goal:
- Identify the first failing mechanism and choose the narrowest target slice.

Primary targets:
- positive-case tests named in this runbook
- frontend surfaces implicated by the first failure

Actions:
- run the full suite and record the baseline
- inspect the failing output for the highest-priority remaining listed case
- group the failure as parse, deduction, substitution, dependent-expression, or
  value-category related
- record the selected slice in `plan_todo.md`

Completion check:
- one failing mechanism family is selected and written down as the active slice

### Step 2: Add Focused Regression Coverage

Goal:
- pin the mechanism with the smallest test that fails for the right reason

Actions:
- add or update a focused frontend/HIR regression near the failing behavior
- keep the original positive testcase as end-to-end confirmation
- avoid relying only on large EASTL probes when a smaller case can isolate the
  bug

Completion check:
- a focused regression exists and fails before the implementation fix

### Step 3: Implement The Narrow Frontend Fix

Goal:
- correct the selected mechanism with the smallest behavior-driven change

Primary targets:
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- adjacent parser/sema/template helpers only if the failure demands it

Actions:
- patch the narrowest code path that owns the failing behavior
- keep template parsing, substitution, and lvalue-category fixes separate when
  they are distinct mechanisms
- avoid cleanup-only motion

Completion check:
- the focused regression and the original targeted positive case pass

### Step 4: Validate Monotonic Regression Behavior

Goal:
- prove the slice did not regress the broader suite

Actions:
- run nearby subsystem tests
- rebuild from scratch
- run the full `ctest` suite
- compare `test_before.log` and `test_after.log`

Completion check:
- no previously passing test regresses
- pass count is unchanged or improved

### Step 5: Record And Handoff

Goal:
- leave the active plan resumable after context loss

Actions:
- update `plan_todo.md` with completed work, the next slice, and any blockers
- keep `plan_todo.md` aligned with the committed code and tests
- commit exactly one mechanism-focused slice

Completion check:
- `plan_todo.md` names the next intended slice and the repo is committed
