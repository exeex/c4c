# HIR Structured Static-Member Const Lookup Regression Repair

Status: Active
Source Idea: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Activated from: reopened idea 136 after rejected closure baseline review

## Purpose

Repair the idea 136 regression introduced when structured scoped static-member
const lookup became authoritative before preserving the legacy instantiated
trait/static-member evaluation and inherited base lookup behavior.

## Goal

Keep structured HIR owner/member keys as lookup authority while making
structured static-member const-value lookup semantically equivalent to the
legacy string path for instantiated trait and base-recursive cases.

## Core Rule

Do not fix the five failing tests with testcase-shaped shortcuts or expectation
downgrades. The repair must preserve structured lookup authority and restore
the missing instantiated trait/base recursion semantics generally.

## Read First

- `ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md`
- `review/idea136_baseline_regression_review.md`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/hir_types.cpp`
- Focused failing tests:
  - `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  - `cpp_positive_sema_template_variable_trait_runtime_cpp`
  - `cpp_hir_template_inherited_member_typedef_trait`

## Current Targets

- `ScopedStaticMemberLookup` construction for qualified static member
  references.
- Structured `find_struct_static_member_const_value` overloads.
- `struct_static_member_const_values_by_owner_` lookup behavior.
- Legacy static-member const lookup behavior that performs instantiated
  trait/static-member evaluation and inherited base recursion.
- Tests that cover template variable initializers reaching member typedef or
  trait-style inherited static member values.

## Non-Goals

- Do not reopen unrelated HIR lookup families unless the repair proves they
  share the same missing semantic path.
- Do not restore rendered owner/member strings as semantic authority when a
  complete structured key is available.
- Do not edit tests or expected results as the primary repair.
- Do not touch baseline logs or reviewer artifacts from this plan-owner repair.

## Working Model

- The rejected closure means idea 136 is still active, not a separate follow-up.
- The five baseline failures are in the static-member/template trait lookup
  acceptance surface.
- The likely bug is that the structured const-value overload consults the
  owner-key cache and returns before applying the legacy instantiated/base
  fallback semantics.
- A correct repair should make the structured path reuse or mirror the semantic
  fallback behavior while retaining structured owner/member identity as the
  primary key.

## Execution Rules

- Keep source idea changes limited to the reopen note and acceptance surface.
- Update `todo.md` for packet progress and proof.
- Executor packets must run the exact proof command delegated by the
  supervisor.
- Treat a green run of only the five named tests as necessary but not
  sufficient; acceptance also needs nearby idea 136 HIR/frontend coverage and a
  baseline/full-suite decision from the supervisor.
- Escalate to review if the implementation changes fallback precedence across
  template struct lookup, member-symbol lookup, or non-static member lookup.

## Step 1: Reproduce and Localize Structured Const Lookup Failure

Goal: confirm the current failing behavior and identify the structured lookup
branch that bypasses instantiated trait/base recursion.

Primary target:
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/hir_types.cpp`

Actions:
- Trace qualified static member expression lowering into
  `ScopedStaticMemberLookup`.
- Compare the structured `find_struct_static_member_const_value` path with the
  legacy string overload's instantiated trait/static-member evaluation and base
  recursion.
- Record the exact missing semantic branch and the smallest implementation
  packet in `todo.md`.

Completion check:
- The executor can name the structured branch that returns too early and the
  legacy behavior that must be preserved.
- No test expectation is weakened.

## Step 2: Preserve Instantiated Trait and Base Recursion Semantics

Goal: repair structured static-member const-value lookup so structured owner
and member keys remain authoritative without losing legacy semantic fallbacks.

Primary target:
- structured static-member const lookup helpers in
  `src/frontend/hir/hir_types.cpp`

Actions:
- Make the structured const-value path perform the same instantiated
  trait/static-member evaluation and inherited base lookup behavior as the
  legacy path when the by-owner cache lacks a direct value.
- Keep rendered names restricted to compatibility fallback inputs only when
  structured producer data is incomplete.
- Avoid narrow matching for the five test names or specific trait identifiers.
- Add internal guardrails or helper extraction if needed so future structured
  callers cannot bypass the fallback semantics.

Completion check:
- The five focused failing tests pass because structured lookup reaches the
  correct semantic value.
- Nearby static-member const lookups still prefer structured keys when direct
  by-owner data exists.

## Step 3: Validate Idea 136 Static-Member and Template Coverage

Goal: prove the repair covers the regression family and does not reopen nearby
idea 136 behavior.

Actions:
- Build the touched frontend/HIR targets.
- Run the five focused regression tests:
  - `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  - `cpp_positive_sema_template_variable_trait_runtime_cpp`
  - `cpp_hir_template_inherited_member_typedef_trait`
- Run nearby idea 136 static-member/template lookup coverage, including
  inherited static-member lookup, deferred NTTP static-member expression tests,
  template value/static-member trait tests, and template member-owner tests.
- Record exact commands and results in `todo.md` and `test_after.log`.

Completion check:
- The five regression tests are green.
- Nearby idea 136 HIR/frontend static-member and template lookup coverage is
  green.
- No proof relies on editing `test_baseline.log` or `test_baseline.new.log`.

## Step 4: Acceptance Review and Baseline Decision

Goal: decide whether idea 136 can close after repair under the rejected
baseline context.

Actions:
- Ask the supervisor to run the required broader validation or regression guard
  for the repaired slice.
- Compare the repaired result against the accepted baseline failure family,
  including the five tests identified by the reviewer.
- Close idea 136 only if source-idea acceptance criteria are satisfied and the
  regression guard/baseline decision passes.

Completion check:
- Supervisor has fresh acceptance proof that includes the five regression
  tests.
- If any static-member/template regression remains, keep idea 136 open and
  record the next packet in `todo.md`.
