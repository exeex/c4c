# STL Enablement Umbrella Plan

## Goal

Make STL-like C++ code possible in this project by coordinating the smaller
language-support plans it depends on.

This file is the parent project plan.
It should describe the overall target, the child plans, the dependency graph,
and the milestone order.
Detailed implementation slices belong in the child plan files.

## Why This Is An Umbrella Plan

STL-like support is not one isolated compiler feature.
It sits on top of several cross-cutting language areas:

- object lifecycle and class semantics
- user-defined operator overloading
- iterator model support
- `range-for` syntax/desugaring
- rvalue references and later move-aware object behavior
- template/dependent-name semantics

Trying to hold all of that in one detailed feature plan makes it too hard to
see:

- which missing capability is actually blocking progress
- which work belongs to the compiler vs library/user code
- what can be shipped incrementally

So this file should stay high-level and point to the focused subplans.

## Target Outcome

The first meaningful milestone is not "full STL".
It is the ability to compile and run small, container-shaped C++ code such as:

- a fixed-storage or simple-owned-storage vector-like type
- member `size()`, `empty()`, `data()`, `front()`, `back()`
- `operator[]`
- `begin()` / `end()`
- a small custom iterator type
- manual iterator loops first
- `range-for` later, once iterator and lookup support are ready

The practical goal is to unlock a coherent, testable subset of C++ that feels
like STL-oriented code, without pretending we are implementing libc++.

## Parent / Child Plan Structure

### This parent plan owns

- the overall product goal
- milestone ordering
- dependency management between plans
- the definition of "STL-like support" for this project

### Child plans own

- detailed phases
- concrete test files
- implementation slices
- feature-specific success criteria

## Current child plans

- `operator_overload_plan.md`
- `rvalue_ref_plan.md`
- `iterator_plan.md`

## Expected child plans

- `range_for_plan.md`
- an object-lifecycle / class-semantics plan if that area grows beyond the STL
  todo tracker

## Child Plan Responsibilities

### `operator_overload_plan.md`

Owns the minimum user-defined operator support needed by iterators and
containers, especially:

- `operator[]`
- `operator*`
- `operator->`
- `operator++`
- `operator==`
- `operator!=`
- `operator bool`

### `rvalue_ref_plan.md`

Owns `T&&` as a language feature and the move-oriented object-model hooks that
may later be needed by realistic container code.

### `iterator_plan.md`

Owns the boundary between compiler support and iterator/user-code/library
responsibilities, including:

- custom iterator class viability
- `begin()` / `end()` member usage
- manual iterator loops
- iterator/container integration

### `range_for_plan.md`

Should own:

- `range-for` parsing
- desugaring strategy
- begin/end lookup policy
- the dependency on a pre-existing iterator model

## Dependency Graph

STL-like support depends on several prerequisites that should not be developed
blindly in parallel.

### Hard dependencies

- operator overloading is a prerequisite for realistic iterator support
- iterator support is a prerequisite for meaningful `range-for`
- object/class semantics are a prerequisite for both containers and iterators
- rvalue references are not required for the first minimal milestone, but they
  are an important prerequisite for move-aware container behavior later

### Recommended flow

1. stabilize object/class basics
2. land the first useful slice of operator overloading
3. land iterator support on top of that
4. add `range-for` as syntax sugar over the iterator model
5. extend toward richer container behavior
6. add rvalue-reference-powered move behavior when real tests need it

## Milestones

## Milestone 1: Container skeleton viability

We can define and use a small container-like class with:

- members
- const members
- nested aliases if needed
- fixed storage
- basic element access

This milestone is mostly blocked by object/class semantics, not iterators.

## Milestone 2: Manual iterator loops

We can define:

- a custom iterator type
- member `begin()` / `end()`
- `operator*`
- `operator++`
- `operator!=`

And then compile code in the shape of:

- `for (auto it = c.begin(); it != c.end(); ++it) { ... }`

This milestone depends on:

- `operator_overload_plan.md`
- `iterator_plan.md`

## Milestone 3: Small vector-like usability

We can express and validate a minimal vector-like abstraction with:

- `size()`, `empty()`, `data()`
- `front()`, `back()`
- `operator[]`
- iterator exposure
- simple push/pop-style happy-path operations

This is the first milestone where "STL-like" starts to feel real.

## Milestone 4: Range-for ergonomics

We can compile:

- `for (auto& x : container) { ... }`

But only after the iterator/container model already works in manual-loop form.

This milestone depends on:

- `iterator_plan.md`
- future `range_for_plan.md`

## Milestone 5: Move-aware and modernized behavior

We can start supporting more realistic modern container behavior with:

- `T&&`
- move construction / move assignment where needed
- move-aware helper patterns

This milestone depends on:

- `rvalue_ref_plan.md`

## Scope Boundary

### In scope

- the minimum compiler support needed for STL-like user code
- phased enablement through child plans
- internal tests as the primary source of truth

### Out of scope

- full `std::vector` / `std::array` / `std::string` parity
- exceptions
- RTTI
- full allocator model
- full `<iterator>` / `<ranges>` / `<type_traits>` parity
- shipping a production-quality standard library

## Design Principle

Do not chase "C++ completeness".

Instead:

1. use STL-like code as the product target
2. split prerequisites into focused child plans
3. land the smallest coherent slice in each child plan
4. validate with internal tests
5. integrate the results back at the umbrella level

## Definition Of Done

This umbrella plan is successful when:

- the child plans together cover the actual blockers for STL-like code
- the first vector-like/container-shaped examples compile without awkward gaps
- iterator support is real enough to power both manual loops and later
  `range-for`
- future C++ work can be placed into the correct child plan instead of being
  mixed into one oversized roadmap

## Execution Tracking

Detailed execution state should live in:

- `plan_todo.md`

This file should remain the high-level parent plan rather than turning back
into a detailed task list.
