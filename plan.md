# Deferred Consteval Late-Information Fixpoint

Status: Active
Source Idea: ideas/open/06_deferred_consteval_late_information_fixpoint.md

## Purpose

Teach c4c's compile-time engine to revisit deferred `consteval` work when
later translation-unit information unlocks results that were unavailable at
first contact.

## Goal

Implement a bounded late-information fixpoint for deferred `consteval` work, or
add explicit diagnostics for unsupported cases, without regressing existing
deferred-consteval behavior.

## Core Rule

Do not silently collapse unresolved compile-time queries to wrong defaults,
"not constant", or other final states before the final fixpoint rules have had
an opportunity to revisit them.

## Read First

- [ideas/open/06_deferred_consteval_late_information_fixpoint.md](/workspaces/c4c/ideas/open/06_deferred_consteval_late_information_fixpoint.md)
- [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)
- [tests/cpp/internal/postive_case/deferred_consteval_chain.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/deferred_consteval_chain.cpp)
- [tests/cpp/internal/postive_case/deferred_consteval_multi.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/deferred_consteval_multi.cpp)
- [tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp](/workspaces/c4c/tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp)

## Current Targets

- deferred `consteval` scheduling and revisit logic in
  [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)
- any HIR or frontend bookkeeping that decides whether pending compile-time
  work is retained, retried, finalized, or diagnosed
- targeted internal regression coverage for late-information cases

## Non-Goals

- redesign all template semantics
- promise support for every late-information C++ pattern
- expand into backend or general codegen optimization work

## Working Model

- Some compile-time queries should remain pending when required semantic
  information is not yet available.
- Later semantic events within the same translation unit may unlock those
  queries.
- The implementation should iterate toward convergence and stop with either:
  a resolved constant result, or an explicit final unresolved outcome with a
  coherent diagnostic or bounded rejection.

## Execution Rules

- Preserve currently passing deferred-consteval behavior.
- Keep the implementation narrow and centered on late-information revisit
  semantics.
- When a candidate case is intentionally unsupported, reject it explicitly
  rather than leaving accidental fallback behavior in place.
- Prefer small, observable slices backed by focused tests.

## Step 1: Locate The Current Loss Point

Goal: identify where unresolved `consteval` work is currently finalized too
early or dropped.

Primary target:
- [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)

Actions:
- inspect how deferred `consteval` evaluation is represented today
- identify where incomplete types, late specializations, or late-known facts
  become permanent failure or non-constant states
- trace what events currently trigger retries, if any

Completion check:
- the exact control points for pending, retry, finalize, and diagnose behavior
  are documented in code comments or captured in follow-up edits

## Step 2: Define Revisit Semantics

Goal: establish the minimal set of semantic events that should re-queue pending
`consteval` work.

Actions:
- decide which unresolved states are safe to keep pending
- define what should cause another pass over pending work
- ensure unsupported patterns still terminate cleanly

Completion check:
- the code has a single coherent rule for when pending work is retried and when
  it becomes a final unresolved result

## Step 3: Implement A TU Fixpoint Loop

Goal: make deferred `consteval` work converge when later translation-unit
information becomes available.

Actions:
- update scheduling or bookkeeping so pending work survives until a later pass
- re-run pending work when tracked semantic progress occurs
- stop when the system reaches a stable fixpoint

Completion check:
- supported late-information cases move from pending to resolved during later
  passes, and the pass structure terminates deterministically

## Step 4: Bound Unsupported Cases

Goal: keep semantics explicit where the fixpoint model should not extend.

Actions:
- add final-pass handling for still-unresolved queries
- ensure diagnostics or rejection paths do not masquerade as successful constant
  evaluation

Completion check:
- unresolved end-state behavior is explicit, bounded, and not represented as a
  fabricated constant

## Step 5: Add Targeted Coverage

Goal: lock in both preserved behavior and new late-information behavior.

Actions:
- keep existing deferred-consteval tests passing
- add focused cases derived from
  [tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp](/workspaces/c4c/tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp)
- prefer tests that distinguish early failure from later convergence

Completion check:
- regression tests cover at least one supported late-information path and any
  intentionally bounded unsupported path

## Step 6: Validate Narrowly

Goal: prove the change improves behavior without introducing new internal
`consteval` regressions.

Actions:
- run the existing deferred-consteval internal tests
- run the focused new late-information tests
- inspect HIR dumps, stats, or equivalent evidence if needed to confirm pending
  work was revisited

Completion check:
- no newly failing internal `consteval` regressions are introduced, and the new
  behavior is demonstrated by tests or equivalent compiler evidence
