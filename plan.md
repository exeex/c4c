# C-Style Cast Reference Follow-Ups Review

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Activated from: prompts/ACTIVATE_PLAN.md

## Purpose

Review the likely follow-up bug surface around C-style casts targeting complex
C++ types, with emphasis on reference-qualified, alias-qualified, and
template-qualified forms that can regress across parser, sema, and later
value-category-sensitive stages.

## Goal

Turn the current cast-follow-up review into an ordered set of narrow,
stage-classified slices with focused regressions and Clang-backed behavioral
checks where value-category behavior matters.

## Core Rule

Keep this plan limited to C-style cast follow-up review. If execution uncovers
a broader language initiative, record it under `ideas/open/` instead of
silently stretching this runbook.

## Read First

- [ideas/open/43_c_style_cast_reference_followups_review.md](/workspaces/c4c/ideas/open/43_c_style_cast_reference_followups_review.md)
- [tests/cpp](/workspaces/c4c/tests/cpp)
- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)
- [src/frontend/hir](/workspaces/c4c/src/frontend/hir)

## Current Targets

- reference-qualified C-style cast targets
- typedef / alias-owned cast targets
- qualified and dependent cast targets
- template-id declarator suffixes inside casts
- downstream value-category propagation after casts
- member/base access through casted references

## Non-Goals

- broad non-cast reference-feature work
- unrelated EASTL or STL bring-up
- silent bundling of parser, sema, and HIR fixes into one unfocused change
- speculative lowering rewrites before the earliest failing stage is clear

## Working Model

- treat each cast family as a separate review lane
- add the smallest regression that identifies the earliest failing stage
- use Clang as the behavioral reference for value-category-sensitive cases
- prefer parser-only or sema-only fixes before touching HIR/lowering

## Execution Rules

- keep `todo.md` current whenever the active slice changes
- add or update tests before implementation changes
- classify each mismatch by earliest failing stage
- compare against Clang before changing behavior for reference-category cases
- spin out broader reference-completeness work into a separate idea when needed

## Ordered Steps

### Step 1: Audit reference-qualified cast targets

Goal: establish current parser/sema/runtime coverage for `&` and `&&` cast
targets and identify the first concrete failing slice.

Primary targets:
- [tests/cpp](/workspaces/c4c/tests/cpp)
- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)

Concrete actions:
- inventory existing cast/reference regressions
- add focused reductions for `(int&)x`, `(const int&)x`, `(volatile int&)x`,
  and `(int&&)x` where coverage is missing
- separate parse acceptance from writeback/value-category behavior checks
- compare Clang behavior for assignment and mutation through casted references

Completion check:
- each reference-qualified cast family has at least one focused regression
- the earliest failing stage for the first unresolved case is explicit in
  `todo.md`

### Step 2: Review typedef, alias, and qualified cast targets

Goal: confirm parser/sema consistency when the cast target is hidden behind
aliases, qualification, or dependent spelling.

Primary targets:
- [tests/cpp](/workspaces/c4c/tests/cpp)
- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)

Concrete actions:
- add focused reductions for typedef/alias-owned reference casts
- add qualified and dependent target forms with `::`, `typename`, and
  template-ids
- classify parser versus sema breakpoints rather than treating them as one bug

Completion check:
- alias-qualified and dependent cast targets have explicit coverage
- unresolved cases are grouped by earliest failing stage

### Step 3: Review declarator suffix and function-pointer cast forms

Goal: verify that template-id and declarator suffix parsing remains stable
across pointer, reference, and function-pointer C-style casts.

Primary targets:
- [tests/cpp](/workspaces/c4c/tests/cpp)
- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)

Concrete actions:
- add narrow parser reductions for pointer, lvalue-reference, rvalue-reference,
  cv-qualified pointer, and function-pointer cast targets
- verify prior template-id parser fixes did not leave suffix-specific holes

Completion check:
- the cast declarator-suffix matrix is covered by focused parser reductions

### Step 4: Validate downstream value-category behavior

Goal: confirm that later semantic consumers observe the correct category after
reference-qualified casts.

Primary targets:
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)
- [src/frontend/hir](/workspaces/c4c/src/frontend/hir)
- [tests/cpp](/workspaces/c4c/tests/cpp)

Concrete actions:
- add focused cases for assignment through casted references, overload
  selection, forwarding, and decltype-like consumers
- compare against Clang before changing sema or HIR behavior
- fix one root cause at a time once the earliest failing stage is clear

Completion check:
- value-category-sensitive cast behavior is covered by targeted regressions
- any landed fix is tied to one demonstrated root cause

### Step 5: Review member/base access through casted references

Goal: ensure member access and method dispatch preserve the owner/reference
semantics of the cast result.

Primary targets:
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)
- [src/frontend/hir](/workspaces/c4c/src/frontend/hir)
- [tests/cpp](/workspaces/c4c/tests/cpp)

Concrete actions:
- add focused cases for field access, inherited member access, and method calls
  through casted references
- compare against Clang for ref-qualified member behavior when needed

Completion check:
- casted-reference member/base access has explicit regression coverage
- remaining failures are classified as sema or HIR/lowering issues

## Acceptance Checks

- the likely cast/reference follow-up shapes are covered by targeted
  regressions
- unresolved cases are classified by earliest failing stage
- Clang-backed behavior is captured before value-category-sensitive fixes land
- no nearby cast fix relies on an unreviewed neighboring hole staying dormant

## Validation

- add the narrowest test for each active slice before code changes
- run targeted parser/sema/HIR tests as appropriate for the slice
- run the full `ctest -j` suite before handoff
- require monotonic full-suite results
