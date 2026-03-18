# Rvalue Reference Enablement Plan

## Goal

Add the minimum C++ rvalue-reference support needed to unblock modern object
semantics and future STL-like container work.

This plan is not about full move-semantics completeness on day one.
It is about making `T&&` a first-class language feature with enough semantic
support to build on later.

## Why This Needs To Be Its Own Plan

Several upcoming language slices depend on `T&&` directly or indirectly:

- move construction / move assignment
- efficient container mutation paths
- forwarding-style helper APIs
- modern iterator/container interfaces
- eventual support for `std::move` / `std::forward`-like patterns

Without rvalue references, C++ support stays stuck in an older object model
even if templates, consteval, and basic methods work.

Also, `T&&` is not just parser sugar.
It affects:

- declarator parsing
- type representation
- reference binding rules
- overload resolution
- value category propagation
- constructor / assignment selection later on

That cross-cutting nature makes it a poor fit for being hidden inside a larger
STL or iterator plan.

## Scope Boundary

### In scope

- parsing `T&&` in declarations and parameter lists
- representing rvalue-reference types in the frontend
- basic binding rules for lvalues vs rvalues
- function parameters of type `T&&`
- return types of type `T&&` only if straightforward
- enough overload resolution to distinguish `T&` and `T&&` in simple cases

### Out of scope for the first milestone

- full move constructor / move assignment synthesis
- perfect reference collapsing parity
- complete forwarding-reference semantics in every template context
- `std::move` / `std::forward` library implementation
- all xvalue/prvalue corner cases
- guaranteed-copy-elision modeling beyond what existing lowering already does

## Current Baseline

What we have today:

- basic lvalue-reference support via `int&`
- a negative test rejecting lvalue-reference binding from an rvalue:
  - `tests/cpp/internal/negative_case/bad_lvalue_ref_bind_rvalue.cpp`
- a positive lvalue-reference smoke test:
  - `tests/cpp/internal/postive_case/lvalue_ref_basic.cpp`

What appears to be missing:

- explicit `T&&` syntax support
- dedicated rvalue-reference tests
- move-oriented overload selection
- any visible `std::move`-style usage

## Design Principle

Implement rvalue references in layers:

1. syntax and type identity
2. reference-binding rules
3. overload selection in simple non-template cases
4. optional template/reference-collapsing extensions later

That keeps the first milestone useful without overcommitting to the hardest
C++ corner cases immediately.

## Recommended Phases

## Phase 0: Syntax and type-system foundation

### Objective

Teach the parser and internal type model to distinguish `T&&` from `T&`.

### Required work

- parse `&&` in declarators and parameter declarations
- represent rvalue-reference types explicitly in `TypeSpec` / canonical type flow
- ensure AST dump / diagnostics print `&&` correctly
- keep this separate from logical `&&` expression parsing

### Add these positive tests

- `rvalue_ref_decl_basic.cpp`
- `rvalue_ref_param_basic.cpp`
- `rvalue_ref_return_decl.cpp`

### Add these negative tests

- `bad_rvalue_ref_syntax.cpp`
- `bad_mixed_ref_declarator.cpp`

### Success criterion

- The frontend can parse and preserve `T&&` types distinctly from `T&`.

## Phase 1: Basic binding rules

### Objective

Apply the minimum semantic rules that make rvalue references meaningful.

### Required work

- allow `T&&` to bind rvalues
- reject binding plain lvalues to `T&&` unless explicitly cast/moved later
- preserve existing `T&` rejection of rvalues
- produce readable diagnostics for incorrect binding

### Add these positive tests

- `rvalue_ref_bind_literal.cpp`
- `rvalue_ref_bind_temp.cpp`
- `rvalue_ref_param_call_basic.cpp`

### Add these negative tests

- `bad_rvalue_ref_bind_lvalue.cpp`
- `bad_rvalue_ref_param_lvalue.cpp`

### Success criterion

- Simple `T&&` declarations and parameter passing follow the basic lvalue/rvalue
  split expected by users.

## Phase 2: Overload distinction for `&` vs `&&`

### Objective

Make overload sets able to distinguish lvalue-reference and rvalue-reference
 parameters in simple cases.

### Required work

- overload resolution for:
  - `f(T&)`
  - `f(T&&)`
- consistent ranking for simple lvalue vs rvalue call arguments
- keep scope narrow to non-template functions first

### Add these positive tests

- `ref_overload_lvalue_vs_rvalue.cpp`
- `ref_overload_method_basic.cpp`
- `ref_overload_const_lvalue_vs_rvalue.cpp`

### Add these negative tests

- `bad_ref_overload_ambiguous.cpp`

### Success criterion

- The compiler picks the expected overload between `&` and `&&` for ordinary
  calls in straightforward cases.

## Phase 3: Move-oriented object model hooks

### Objective

Prepare for move-aware classes without requiring full STL machinery.

### Required work

- user-declared move constructor parsing
- user-declared move assignment parsing
- basic lowering/codegen path for calling those functions if selected
- enough semantics to support small test classes

### Add these positive tests

- `move_ctor_basic.cpp`
- `move_assign_basic.cpp`
- `move_ctor_over_copy.cpp`

### Add these negative tests

- `bad_deleted_move_ctor_call.cpp`
- `bad_move_assign_const.cpp`

### Success criterion

- Small classes can declare and use move operations in controlled test cases.

## Phase 4: Utility-layer enablers

### Objective

Add the minimum extra pieces needed before iterator/container work starts to
lean on modern C++ value categories.

### Possible scope

- builtin or library-smoke support for `move(x)`-style helper patterns
- first pass at reference collapsing in narrow template cases
- forwarding-reference experiments only if needed by real tests

### Add these positive tests

- `move_helper_basic.cpp`
- `template_rvalue_ref_param_basic.cpp`

### Add these negative tests

- `bad_forwarding_ref_bind.cpp`

### Success criterion

- Enough utility-level support exists to write realistic move-aware helper code
  without pretending the entire modern reference model is done.

## Recommended priority

The highest-payoff first batch is:

- `rvalue_ref_decl_basic.cpp`
- `rvalue_ref_param_basic.cpp`
- `rvalue_ref_bind_literal.cpp`
- `bad_rvalue_ref_bind_lvalue.cpp`
- `ref_overload_lvalue_vs_rvalue.cpp`

Only after those pass should we move into:

- move constructor / move assignment behavior
- helper utilities
- template-heavy forwarding cases

## Interaction With Other Plans

This plan feeds directly into:

- future object-lifecycle work in `stl_plan.md`
- move-aware container semantics in `stl_plan_todo.md`
- possible iterator/container helper APIs

It is adjacent to, but separate from:

- `operator_overload_plan.md`
- future `iterator_plan.md`
- future `range_for_plan.md`

Recommended sequencing:

1. land Phase 0-1 here
2. decide whether move ctor/assign is immediately needed for STL work
3. then resume object-lifecycle phases in `stl_plan_todo.md`

## Definition Of Done

This plan is successful when:

- `T&&` parses and prints correctly
- basic binding rules behave correctly
- `&` vs `&&` overloads resolve correctly in simple cases
- move-oriented class tests can be added without architectural hacks

## Non-goals

- full C++ standard conformance for value categories
- full reference-collapsing parity on the first pass
- immediate support for every `std::move` / `std::forward` use case
- complete STL-ready move semantics in one step
