# Iterator Enablement Plan

## Goal

Define and implement the minimum iterator-related C++ language support needed
to unblock STL-like container code in this project.

This plan is not about shipping `<iterator>` or full STL iterator taxonomy.
It is about enabling user-defined iterator types and the container APIs that
return them.

## Compiler vs Library Boundary

### What belongs to the compiler

- class/struct definitions for iterator types
- member functions and `this`
- templates used by iterator/container types
- operator overloading support needed by iterators:
  - `operator*`
  - `operator->`
  - `operator++`
  - `operator==`
  - `operator!=`
- type checking and overload resolution for those operators
- object construction/copy/destruction rules used by iterator values
- member `begin()` / `end()` calls compiling correctly
- eventual `range-for` desugaring logic

### What belongs to library / user code

- concrete iterator classes
- concrete container `begin()` / `end()` implementations
- helper aliases such as `iterator`, `const_iterator`, `value_type`
- `std::begin` / `std::end`
- iterator traits / tags / adaptors / ranges helpers

### Important practical rule

`begin()` / `end()` are not compiler builtins.
The compiler only needs to support:

- declaring them
- calling them
- type-checking their return values
- later, using them during `range-for` desugaring

## Why This Needs To Be Its Own Plan

The STL umbrella depends heavily on iterators, but "iterator support" spans
multiple language mechanisms that should be made explicit:

- operator overloading
- object semantics
- const and non-const methods
- nested type aliases
- member-returned helper objects
- optional `range-for`

If we keep iterator work buried inside a larger container plan, it becomes hard
to tell whether failures come from operator support, object model gaps, or
missing iteration semantics.

## Scope Boundary

### In scope

- custom iterator class support
- member `begin()` / `end()` returning iterator objects
- const and non-const iterator entry points
- plain loop traversal using iterator operations
- a small internal iterator/container smoke surface

### Out of scope for the first milestone

- full standard iterator category hierarchy
- `<iterator>` parity
- ranges library support
- sentinel-heavy designs
- reverse iterators
- full `std::begin` / ADL fallback behavior

## Current Baseline

What appears to exist already:

- basic struct methods
- template struct methods
- implicit `this` lowering for methods
- some basic C++ template / consteval infrastructure

What appears to be missing or not yet isolated:

- user-defined operator overloading needed by iterators
- dedicated iterator tests
- dedicated member `begin()` / `end()` test coverage
- explicit iterator value/object-model validation
- `range-for` syntax support

## Design Principle

Treat iterators as ordinary user-defined types first.

The compiler should not special-case "iterator" as a magic concept.
Instead, we should enable the ordinary language features that make the
following shape work:

- a class storing traversal state
- `operator*`
- `operator++`
- `operator!=`
- optional `operator->`
- container methods returning iterator values

Only after that should `range-for` build on top of the same surface.

## Recommended Phases

## Phase 0: Iterator model and test contract

### Objective

Define the smallest iterator shape we will consider "supported" for the first
milestone.

### Contract for the first milestone

- member-based iterator type
- copyable iterator object
- `operator*`
- prefix `operator++`
- `operator!=`
- member `begin()` / `end()`
- manual loop:
  - `for (auto it = c.begin(); it != c.end(); ++it) { ... }`

### Add these smoke tests

- `iterator_contract_basic.cpp`
- `container_begin_end_manual_loop.cpp`

### Success criterion

- We have a crisp definition of the first iterator slice and corresponding
  smoke tests.

## Phase 1: Iterator object basics

### Objective

Make iterator objects viable as ordinary class values returned from methods.

### Dependencies

- object-lifecycle support from the STL umbrella
- enough method support to define iterator member functions

### Required work

- iterator object declaration and copyability
- returning iterator objects from methods
- using iterator objects in local variables
- const/non-const method basics where needed

### Add these positive tests

- `iterator_class_basic.cpp`
- `iterator_return_from_method.cpp`
- `iterator_copy_basic.cpp`
- `iterator_const_method_basic.cpp`

### Success criterion

- Iterators behave like normal small class values in user code.

## Phase 2: Operator-dependent iterator surface

### Objective

Enable the operator subset an iterator actually needs.

### Dependency

- `operator_overload_plan.md`

### Required operators

- `operator*`
- `operator++`
- `operator!=`
- optional `operator==`
- optional `operator->`

### Add these positive tests

- `iterator_deref_basic.cpp`
- `iterator_preinc_basic.cpp`
- `iterator_neq_basic.cpp`
- `iterator_eq_basic.cpp`
- `iterator_arrow_basic.cpp`

### Add these negative tests

- `bad_iterator_deref_shape.cpp`
- `bad_iterator_compare_shape.cpp`

### Success criterion

- Manual iteration loops work over a custom iterator type.

## Phase 3: Container integration

### Objective

Connect iterators to container-like types through `begin()` / `end()`.

### Required work

- container nested iterator type or external iterator type
- member `begin()` / `end()`
- const and non-const overloads if feasible
- interaction with fixed storage container state

### Add these positive tests

- `begin_end_member_basic.cpp`
- `const_begin_end_member.cpp`
- `iterator_pair_loop.cpp`
- `container_iterator_smoke.cpp`

### Success criterion

- A small container can expose iterators and be traversed with a plain iterator
  loop.

## Phase 4: Iterator/container ergonomics

### Objective

Cover the extra pieces that make iterators feel usable in real container code.

### Possible scope

- `operator->`
- `const_iterator` variants
- nested aliases:
  - `iterator`
  - `const_iterator`
  - `value_type`
- simple arithmetic helpers only if the tests truly need them

### Add these positive tests

- `iterator_alias_basic.cpp`
- `const_iterator_basic.cpp`
- `iterator_arrow_chain_basic.cpp`

### Success criterion

- The iterator surface is coherent enough to support non-trivial container
  examples, not just one-off loops.

## Phase 5: Range-for integration

### Objective

Build `range-for` on top of the iterator/container surface once the underlying
mechanics are already working.

### Dependency

- future `range_for_plan.md`

### Notes

- `range-for` is compiler syntax support, not iterator magic
- the compiler should reuse member `begin()` / `end()` semantics instead of
  inventing a separate path

### Add these positive tests later

- `range_for_basic.cpp`
- `range_for_const.cpp`

### Success criterion

- `range-for` becomes a thin language layer over an already-working iterator
  model.

## Recommended sequencing

1. finish the first useful slice of `operator_overload_plan.md`
2. land Phase 0-3 here
3. only then start `range-for`
4. integrate the results back into the STL umbrella

## Definition of done

This plan is successful when:

- custom iterator classes compile and behave predictably
- containers can return iterators from `begin()` / `end()`
- manual iterator loops work without ad hoc compiler hacks
- `range-for` can later build on the same surface rather than requiring a new
  iterator model

## Non-goals

- full `<iterator>` implementation
- complete ranges support
- standard-library-level iterator categories on the first pass
