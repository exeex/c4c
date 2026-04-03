# Rvalue Reference Completeness Runbook

Status: Active
Source Idea: ideas/open/rvalue_reference_completeness_plan.md
Supersedes: ideas/open/eastl_container_bringup_plan.md
Activated from: prompts/SWITCH_PLAN.md

## Purpose

Use the current EASTL/libstdc++ pressure to turn partial C++ reference support
into a deliberate language-completeness track centered on `&&`, forwarding
references, and deduction-aware `auto&&` / `auto&`.

## Goal

Establish correct parser, sema, and HIR behavior for rvalue references,
forwarding references, and `auto`-deduced reference forms across focused
internal regressions and real library-facing probes.

## Core Rule

Treat this as a generic C++ language feature initiative, not as a library-
specific workaround track: land reusable parser/sema/HIR fixes, and keep
EASTL/STL header pressure only as evidence and validation.

## Read First

- [`prompts/EXECUTE_PLAN.md`](/workspaces/c4c/prompts/EXECUTE_PLAN.md)
- [`ideas/open/rvalue_reference_completeness_plan.md`](/workspaces/c4c/ideas/open/rvalue_reference_completeness_plan.md)
- [`src/frontend/parser`](/workspaces/c4c/src/frontend/parser)
- [`src/frontend/sema`](/workspaces/c4c/src/frontend/sema)
- [`src/frontend/hir`](/workspaces/c4c/src/frontend/hir)
- existing reference-heavy tests under [`tests/cpp/internal`](/workspaces/c4c/tests/cpp/internal)

## Scope

- audit current support for `T&&`, `const T&&`, `auto&&`, and `auto&`
- classify existing tests by earliest failing stage
- add focused regressions for missing or under-specified reference behavior
- fix parser declaration/expression disambiguation gaps first
- tighten sema binding rules and HIR value-category propagation afterward
- keep EASTL/libstdc++ probes as validation pressure, not as the primary unit of work

## Non-Goals

- broad container bring-up as the primary goal while this plan is active
- runtime/ABI ownership unrelated to reference semantics
- header-name special cases or testcase-specific parsing exceptions
- trying to finish every STL/EASTL issue under one reference-semantics slice

## Working Model

- Start with the narrowest internal reduction that proves one reference bug.
- Confirm whether the failure is parser, sema, HIR, or runtime.
- Compare against Clang when binding or value-category behavior is unclear.
- Re-run one or two real library-facing probes after each meaningful fix.
- Keep `auto&&` / `auto&` under the same umbrella as `T&&` because the missing
  behavior is shared through deduction, collapsing, and value-category rules.

## Execution Rules

- Work from the highest-priority incomplete item in [`todo.md`](/workspaces/c4c/todo.md).
- Add or update the narrowest validating test before implementing a fix.
- Keep parser-only slices separate from sema/HIR behavior slices.
- Preserve the resumable EASTL tuple frontier in `todo.md` as a secondary validation probe.
- If a newly found issue is library-specific rather than language-generic, record it back in the EASTL idea instead of growing this plan sideways.

## Ordered Steps

### Step 1: Inventory current reference support

Goal: classify what the tree already supports for `&&`, forwarding references,
and `auto`-deduced reference forms.

Primary targets:
- [`tests/cpp/internal`](/workspaces/c4c/tests/cpp/internal)
- [`src/frontend/parser`](/workspaces/c4c/src/frontend/parser)
- [`src/frontend/sema`](/workspaces/c4c/src/frontend/sema)
- [`src/frontend/hir`](/workspaces/c4c/src/frontend/hir)

Actions:
- inventory existing positive and negative tests for `T&&`, move operations,
  ref-qualified members, `auto&&`, and `auto&`
- identify where parser, sema, and HIR each currently special-case reference behavior
- record the biggest missing coverage clusters in `todo.md`

Completion check:
- the current `&&` / `&` reference surface is classified by stage and missing coverage areas are explicit

### Step 2: Parser completeness for reference-shaped declarations

Goal: stabilize parser behavior for deduction guides, conversion operators,
block-scope declarations, and other declaration/expression boundaries under
reference pressure.

Primary targets:
- [`src/frontend/parser`](/workspaces/c4c/src/frontend/parser)
- focused parse regressions under [`tests/cpp/internal/postive_case`](/workspaces/c4c/tests/cpp/internal/postive_case)

Actions:
- add focused parse regressions for missing `auto&&`, `auto&`, and reference-heavy declaration forms
- fix grouped-declarator, local-declaration, operator, and deduction-guide ambiguities one root cause at a time
- validate with the current EASTL/libstdc++ tuple probe after each parser slice

Completion check:
- the current parser blockers for representative `&&` / `auto&&` / `auto&` shapes are either fixed or reduced to later stages

### Step 3: Semantic binding and overload rules

Goal: make binding rules and overload choice behave correctly for lvalues,
xvalues, and prvalues.

Primary targets:
- [`src/frontend/sema`](/workspaces/c4c/src/frontend/sema)
- focused positive/negative regressions under [`tests/cpp/internal`](/workspaces/c4c/tests/cpp/internal)

Actions:
- verify direct binding rules for `T&&`, `const T&&`, `auto&&`, and `auto&`
- tighten overload selection between `T&`, `const T&`, and `T&&`
- compare against Clang for contentious cases such as named rvalue refs and forwarding references

Completion check:
- key binding and overload cases match Clang on the focused regression set

### Step 4: HIR value-category and forwarding behavior

Goal: preserve correct value category and reference semantics into lowering.

Primary targets:
- [`src/frontend/hir`](/workspaces/c4c/src/frontend/hir)

Actions:
- audit HIR handling of named rvalue refs, casts to `T&&`, `std::move`, `std::forward`, and `auto`-deduced refs
- fix any places that still collapse behavior into a simple lvalue/non-lvalue split where xvalue matters
- add behavior-level regressions when parser/sema already succeed

Completion check:
- HIR lowering preserves the intended reference/value-category behavior for the current focused cases

### Step 5: Library-facing validation

Goal: confirm that the generic fixes survive real template/header pressure.

Primary targets:
- `tests/cpp/eastl/eastl_tuple_simple.cpp`
- `tests/cpp/eastl/eastl_memory_simple.cpp`
- selected libstdc++-facing reductions

Actions:
- re-run the current Step 3 EASTL tuple/memory probes
- record whether the active frontier moved later, changed stage, or exposed a new generic reduction
- only then decide whether to stay on this plan or switch back to EASTL bring-up

Completion check:
- the current real-header reference frontier is either cleared or reduced to a new explicit later-stage blocker
