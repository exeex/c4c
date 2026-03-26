# Templated Member Scope Execution Plan

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook.
Do not redesign the whole frontend in one pass.
Do not merge semantic categories just because they share parsing mechanics.
Do not start in HIR or codegen.


## One-Sentence Goal

Implement parser-side template-scope tracking so that:

- members of class templates inherit enclosing template parameters
- member function templates see both enclosing and local template parameters
- out-of-class member definitions keep the correct owner/template association

while still reusing the same declaration/function parsing pipeline.


## Core Rule

Treat these as different semantic cases:

1. free function template
2. member of class template
3. member function template inside class template

But do **not** build three independent parsing pipelines.

Instead:

- keep one declaration/function parsing pipeline
- drive it with a parser-owned template-scope stack


## Read First

Before editing, open these exact files:

1. [ideas/templated_member_scope_plan.md](/workspaces/c4c/ideas/templated_member_scope_plan.md)
2. [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
3. [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
4. [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)

Do not begin in:

- HIR files
- codegen files
- lexer files
- unrelated C parser paths


## Current Acceptance Targets

Keep these tests in mind throughout the work:

- [templated_member_nested_scope_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/templated_member_nested_scope_parse.cpp)
- [template_member_type_direct_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_member_type_direct_parse.cpp)
- [template_member_type_inherited_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_member_type_inherited_parse.cpp)
- [template_alias_nttp_expr_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_alias_nttp_expr_parse.cpp)
- [template_alias_nttp_expr_inherited_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_alias_nttp_expr_inherited_parse.cpp)
- [member_template_decltype_default_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_template_decltype_default_parse.cpp)
- [member_template_decltype_overload_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_template_decltype_overload_parse.cpp)
- [eastl_slice7d_qualified_declarator_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/eastl_slice7d_qualified_declarator_parse.cpp)


## Non-Goals

Do not do these as part of this run:

- redesign HIR ownership model first
- flatten class-template members into explicit function templates
- rewrite all template parsing logic at once
- refactor unrelated angle-bracket handling unless directly needed


## Working Model

You are building toward this parser model:

- parser remembers active template scopes in a stack
- each scope frame records:
  - names
  - type-vs-NTTP kind
  - source kind:
    - enclosing_class
    - member_template
    - free_function_template
- lookup in declarations/types consults the stack before ad hoc fallback tables

You are building toward this HIR handoff:

- parser preserves ownership/source of template scope
- later stages may normalize function handling
- ownership differences survive as metadata, not parser confusion


## Execution Rules

Follow these rules exactly:

1. One edit slice should target one parser ownership improvement.
2. Always add or keep a reduced regression test before generalizing.
3. If a step needs a new helper, add the smallest helper that supports the next test.
4. Do not remove old ad hoc behavior until the new path demonstrably covers it.
5. After each step, run only the smallest relevant test set first.
6. Only after the reduced tests pass, probe `eastl_slice7d` and then `std_vector_simple`.


## Step 1. Freeze the Current Regression Surface

Goal:

- know exactly which tests are the current guardrails for this work

Do this:

1. open [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
2. verify every test listed in “Current Acceptance Targets” is registered
3. if any listed test is missing from the parse-only set, add it
4. do not change parser code in this step

Completion check:

- every listed reduced regression exists and is registered


## Step 2. Add an Explicit Template-Scope Stack Type

Goal:

- stop representing active template visibility only through scattered sets/maps

Do this:

1. edit [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
2. add a small parser-owned template-scope frame type
3. add a stack field to `Parser`
4. keep the frame minimal:
   - parameter name
   - is_nttp
   - scope kind
5. do not wire in behavior yet

Do not do this:

- do not rewrite all old lookup structures in the same diff
- do not remove existing `active_template_member_type_params_` yet

Completion check:

- parser can compile with an explicit template-scope stack type present


## Step 3. Add Push/Pop Helpers For Template Scope

Goal:

- make scope ownership transitions explicit and localized

Do this:

1. edit [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
2. add small helpers for:
   - push enclosing-class template scope
   - push member-template scope
   - push free-function-template scope
   - pop scope frame(s)
3. implement them in the parser source file that best matches existing parser state helpers
4. keep helper behavior mechanical; no new parsing logic in this step

Completion check:

- there is one obvious API for scope enter/exit


## Step 4. Enter Enclosing-Class Scope When Parsing Templated Struct Bodies

Goal:

- members inside class templates should inherit outer template params

Do this:

1. edit [declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
2. find where templated struct/class declarations hand off into struct-body parsing
3. push an `enclosing_class` template scope before entering the body
4. pop it immediately after leaving the body
5. keep the old ad hoc typedef injection paths alive for now

Completion check:

- parser state now clearly distinguishes “inside templated class body” from ordinary struct body


## Step 5. Enter Member-Template Scope For Templated Members

Goal:

- member templates inside class templates should see both outer and inner scopes

Do this:

1. edit [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
2. find the struct-body path that parses `template <...>` member declarations
3. when the member template parameter list is parsed, push a `member_template` scope
4. pop it immediately after the member declaration has been parsed
5. preserve current behavior for existing injected typedef names until tests prove they are redundant

Completion check:

- there is a nested-scope path where outer class-template params and inner member-template params coexist


## Step 6. Teach Type Lookup To Consult The Scope Stack

Goal:

- make `T`, `Allocator`, `U`, `N` visible because they are in active template scope, not because of scattered special cases

Do this:

1. edit [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
2. update the lookup helpers used by:
   - `is_type_start()`
   - `parse_base_type()`
3. make them consult active template-scope frames in this order:
   - innermost member-template scope
   - enclosing class-template scope
   - older template scopes if still active
4. keep existing typedef/member-typedef lookup after that

Do not do this:

- do not delete old fallback logic in the same step unless the reduced tests already prove it is dead

Completion check:

- `templated_member_nested_scope_parse` still passes with the new lookup order


## Step 7. Unify Existing Member-Template Ad Hoc State With The Stack

Goal:

- reduce duplicated “active template param” mechanisms

Do this:

1. inspect current use of:
   - `active_template_member_type_params_`
   - injected typedefs for template params
2. move one piece of logic at a time onto the template-scope stack
3. after each move, rerun only the reduced tests that cover that behavior
4. keep compatibility bridges if a full deletion would be risky

Completion check:

- the stack is a real source of truth, not dead metadata


## Step 8. Handle Out-of-Class Member Definitions With Owner-Aware Scope

Goal:

- `template <typename T> void S<T>::f(...)` should bind as a templated-class member definition, not as an unrelated free function template

Do this:

1. edit [declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
2. find qualified member definition parsing paths
3. when the owner is a templated class, make the parser re-enter the owner’s template scope before parsing the member signature/body
4. keep ownership metadata intact; do not relabel these as explicit function templates

Completion check:

- out-of-class member definitions can resolve owner-provided template params and member typedefs correctly


## Step 9. Lock The Composite Nested-Scope Case

Goal:

- ensure the mixed outer/inner template scope case is permanently guarded

Do this:

1. run [templated_member_nested_scope_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/templated_member_nested_scope_parse.cpp)
2. if it fails, fix scope-stack ownership before touching any broader EASTL/STL case
3. if it passes, treat it as a hard guardrail from this point onward

Completion check:

- the parser can distinguish:
  - outer `T`
  - inner `U`
  - owner `S<T>`


## Step 10. Revalidate The Existing Template Bring-Up Reductions

Goal:

- make sure the new scope model did not regress earlier template work

Do this:

1. run the reduced tests listed in “Current Acceptance Targets”
2. do not run the full suite first
3. if a regression appears, reduce it before continuing

Completion check:

- all reduced parser regressions listed in this document stay green


## Step 11. Reprobe The EASTL Qualified Declarator Failure

Goal:

- verify whether the scope model resolves or narrows the remaining `eastl_slice7d` failure

Do this:

1. run [eastl_slice7d_qualified_declarator_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/eastl_slice7d_qualified_declarator_parse.cpp)
2. if it still fails, reduce the first remaining failing sub-pattern
3. only then patch the next isolated declarator/parser ownership gap

Completion check:

- either `eastl_slice7d` passes, or you can name the exact next reduced blocker


## Step 12. Reprobe `std_vector_simple`

Goal:

- measure whether this work moved the real libstdc++ frontier

Do this:

1. run [std_vector_simple.cpp](/workspaces/c4c/tests/cpp/std/std_vector_simple.cpp) with `--parse-only`
2. if it still fails, rerun the prefix-localization workflow to find the first real blocker
3. document the new frontier before starting another parser slice

Completion check:

- you know whether the frontier moved and what the next concrete blocker is


## What “Done” Means For This Plan

This execution plan is complete when all of the following are true:

1. parser has an explicit template-scope stack
2. templated struct/class bodies push inherited enclosing-class template scope
3. member templates push nested template scope on top of inherited class scope
4. type/declarator lookup consults the scope stack in the intended order
5. out-of-class member definitions preserve owner/template association
6. all reduced tests listed in this plan stay green
7. `eastl_slice7d` is either fixed or reduced to one smaller remaining blocker
8. the `std_vector_simple` frontier is re-measured after the scope work lands


## If You Get Stuck

Do this:

1. stop broad refactoring
2. write one smaller reduced regression test
3. make one ownership change
4. rerun only the nearest relevant tests

Do not do this:

- do not compensate with more unrelated heuristics
- do not patch HIR to hide parser ownership mistakes
- do not widen scope of the slice until the current reduced case is understood
