# Rvalue Reference Completeness Runbook

Status: Active
Source Idea: ideas/open/46_rvalue_reference_completeness_plan.md
Activated from: ideas/open/46_rvalue_reference_completeness_plan.md

## Purpose

Turn the open rvalue-reference completeness note into an execution runbook for
small, test-first compiler slices across parser, sema, HIR, and library-facing
validation.

## Goal

Make `&&`-related language support deliberate and testable across parsing,
deduction, binding, value-category propagation, and ref-qualified member
behavior without turning this plan into general library bring-up.

## Core Rule

Fix generic language behavior for rvalue references and forwarding references.
Do not stretch this runbook into unrelated EASTL or libstdc++ package work.

## Read First

- [ideas/open/46_rvalue_reference_completeness_plan.md](/workspaces/c4c/ideas/open/46_rvalue_reference_completeness_plan.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Current Targets

- parser declaration vs expression disambiguation under `&&` pressure
- reference-collapsing and forwarding-reference coverage gaps
- sema binding and overload selection for `T&`, `const T&`, and `T&&`
- HIR value-category propagation and move/forward lowering
- focused regression depth for library-facing forwarding cases

## Non-Goals

- container-specific EASTL bring-up beyond reproducing generic language bugs
- unrelated STL header cleanup that does not reduce to a `&&` language defect
- broad backend work unless a concrete `&&` issue reaches codegen
- mixing multiple independent library initiatives into this plan

## Working Model

The expected progression is:

1. audit current `&&` coverage and classify it by compiler stage
2. add a focused regression matrix for missing language shapes
3. repair parser blockers that prevent real-world headers from parsing
4. tighten sema binding and overload behavior against Clang
5. audit HIR value-category handling and move/forward lowering
6. re-run library-facing probes and record any bounded follow-on work

## Execution Rules

- use test-first slices and keep each slice narrow
- compare parser behavior and generated IR against Clang when output shape is
  unclear
- prefer generic fixes over header-specific exceptions
- record newly discovered adjacent work in `ideas/open/` instead of silently
  mutating this runbook

## Step 1: Audit Existing `&&` Coverage

Goal: inventory current rvalue-reference support so new work starts from
observed gaps rather than assumptions.

Primary targets:

- `tests/`
- parser, sema, and HIR tests already covering `&&`, `auto&&`, and move paths

Actions:

- classify existing regressions by stage: parse, sema, HIR, and runtime
- identify the highest-value missing shapes that block parser or sema progress
- record the first narrow implementation slice in `todo.md`

Completion check:

- the current regression surface is summarized well enough to choose one
  concrete gap without re-deriving repo state

## Step 2: Add The Missing Regression Matrix

Goal: add focused coverage for missing language shapes before implementing new
 behavior.

Primary targets:

- `tests/cpp/internal/`
- small runtime or parse-only cases that isolate one `&&` rule at a time

Actions:

- add or update focused cases for `T&&`, `const T&&`, `auto&&`, `auto&`,
  `Args&&...`, deduction guides, conversion operators, ref-qualified members,
  `std::forward`, and named-rvalue-reference lvalue behavior
- keep each regression aligned to one root-cause family

Completion check:

- the next parser, sema, or HIR slice is pinned by a failing focused test

## Step 3: Repair Parser `&&` Disambiguation Blockers

Goal: make declaration-shaped and expression-shaped `&&` syntax parse
correctly in the targeted generic cases.

Primary targets:

- `src/frontend/parser/`

Actions:

- inspect declaration/expression ambiguity around deduction guides, conversion
  operators, block-scope `auto&&` or `auto&`, and dependent declarators
- fix the narrowest generic parser root cause first
- keep the change limited to `&&`-driven parsing gaps needed by the focused
  regressions

Completion check:

- targeted parser regressions pass and unblock the next semantic slice

## Step 4: Tighten Sema Binding And Overload Rules

Goal: align reference binding and overload choice with Clang on the targeted
`&&` cases.

Primary targets:

- `src/frontend/sema/`

Actions:

- validate binding for lvalues, xvalues, and prvalues across `T&`, `const T&`,
  `T&&`, and `const T&&`
- audit forwarding-reference deduction vs plain rvalue-reference deduction
- fix one binding or overload root cause at a time

Completion check:

- targeted sema and runtime regressions match Clang for the covered cases

## Step 5: Audit HIR Value-Category Propagation

Goal: preserve the correct value category through casts, forwarding helpers,
member access, and move-aware call sites.

Primary targets:

- `src/frontend/hir/`

Actions:

- inspect lvalue/xvalue/prvalue propagation in the failing targeted cases
- tighten move or forward lowering only where the focused regressions prove a
  generic defect
- avoid broad HIR rewrites that are not justified by the current failing slice

Completion check:

- targeted HIR or runtime regressions pass for the chosen move or forwarding
  path

## Step 6: Validate And Bound Follow-On Work

Goal: confirm the completed slices are stable and record any adjacent but
out-of-scope work.

Actions:

- rerun targeted `&&` regressions and nearby subsystem coverage
- rerun the broader repo validation required by `prompts/EXECUTE_PLAN.md`
- if library-facing probes reveal additional generic `&&` gaps that are not
  required for the current slice, record them in the linked idea or a new open
  idea

Completion check:

- the completed `&&` slice is covered by tests
- broader validation is monotonic
- bounded follow-on work is tracked without silently expanding this plan
