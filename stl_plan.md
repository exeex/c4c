# Minimal STL-Like Container Plan

## Goal

Build the smallest useful C++ language subset needed to support STL-like
containers in this project.

The target is **not** full STL compatibility.
The target is to make small container-style user code practical, testable, and
maintainable.

Concretely, we want enough C++ to express and validate things like:

- a small `vector`-like container with fixed or simple owned storage,
- iterator-style traversal,
- const and non-const element access,
- template-based value types,
- basic compile-time helpers for size/capacity and type plumbing.

This plan treats STL-like support as a **language support plan**, not a
stdlib-porting plan.

## Scope Boundary

### What this plan covers

- the C++ language features needed to implement minimal container abstractions
- internal positive tests that validate those features incrementally
- a phased path from basic class support to iterator/container usability

### What this plan does not cover

- full `std::vector`, `std::array`, `std::string`, or allocator compatibility
- exceptions
- RTTI
- move semantics as a hard requirement for the first milestone
- full operator overloading coverage
- complete `<type_traits>` or `<iterator>` parity
- shipping a production-quality STL implementation

## Why This Plan Exists

For C++, a large percentage of real-world value comes from code that looks like
container code:

- class templates,
- nested types,
- member functions,
- const overloads,
- iterators,
- dependent names,
- element access operators,
- simple compile-time properties.

If these work, a large practical subset of user code becomes possible.
If these do not work, "C++ support" remains narrow even if many isolated
features pass.

So instead of chasing arbitrary language completeness, we should aim for the
smallest coherent subset that unlocks STL-like programming patterns.

## Design Principle

Do not test C++ as an unbounded feature matrix.

Instead:

1. define the minimum container-oriented language slice,
2. add one focused internal positive test per meaningful capability,
3. add a small number of interaction tests where container code naturally
   combines features,
4. grow the suite through bug-driven regression afterward.

The internal suite should be the primary source of truth.
External Clang subsets can be used later as curated validation, not as the main
design surface.

## Target Capability

The first successful target should allow code in this general shape:

- `template <typename T> struct SmallVec`
- fixed internal storage or simple owned storage
- `size()`, `empty()`, `push_back()`, `pop_back()`
- `operator[]`, `front()`, `back()`, `data()`
- `begin()` / `end()`
- a small iterator type
- use with primitive and simple user-defined element types

The second target can extend this toward:

- nested type aliases such as `value_type`, `size_type`, `iterator`
- const overloads
- simple compile-time queries such as `constexpr size()` or capacity helpers
- more dependent-name-heavy member code

## Functional Areas We Need

The missing work is best thought of as a few language buckets.

### 1. Object Lifecycle

Container-like code needs:

- default construction
- value construction
- copy construction
- copy assignment
- destruction
- correct member initialization flow
- use of `this`

### 2. Class And Member Semantics

Container-like code needs:

- member functions
- const member functions
- member initialization lists
- nested classes
- nested type aliases
- member access through object and pointer forms

### 3. Operator Surface

Container-like code often relies on:

- `operator[]`
- `operator*`
- `operator->`
- pre/post increment for iterators
- `==` / `!=`
- simple pointer-difference or arithmetic style operators where needed
- conversion to `bool` for lightweight handle/iterator states

### 4. Template And Dependent-Name Semantics

Container-like code needs:

- class templates with value types
- dependent member calls
- dependent member types
- `typename` on dependent members
- lookup through dependent bases where applicable

### 5. Storage And Access

Container-like code needs:

- array members
- fixed buffer indexing
- pointer-style data access
- const and non-const element access

### 6. Iterator Usability

Minimal container ergonomics need:

- iterator class basics
- `begin()` / `end()`
- const `begin()` / `end()`
- simple iterator loops
- eventually `range-for` if the frontend reaches that level

## Recommended Phases

## Phase 1: Object And Member Basics

### Objective

Make small class-like containers possible at all.

### Add these positive tests

- `default_ctor_basic.cpp`
- `value_ctor_basic.cpp`
- `copy_ctor_basic.cpp`
- `copy_assign_basic.cpp`
- `dtor_basic.cpp`
- `member_init_list_basic.cpp`
- `this_pointer_basic.cpp`
- `const_member_function_basic.cpp`
- `pointer_member_access.cpp`
- `nested_class_basic.cpp`

### Success Criterion

- simple class and struct objects behave predictably
- container skeleton types can be declared, constructed, copied, and used

## Phase 2: Element Access And Storage

### Objective

Support the core mechanics of container storage and indexing.

### Add these positive tests

- `array_member_storage.cpp`
- `fixed_buffer_indexing.cpp`
- `operator_subscript_basic.cpp`
- `operator_subscript_const.cpp`
- `container_front_basic.cpp`
- `container_back_basic.cpp`
- `container_data_basic.cpp`
- `container_size_basic.cpp`
- `container_empty_basic.cpp`
- `container_clear_basic.cpp`

### Success Criterion

- a fixed-storage container can store and retrieve elements
- const and non-const access paths both work

## Phase 3: Template Container Shape

### Objective

Make the container generic over element type in a way that resembles real STL
usage.

### Add these positive tests

- `container_value_type_basic.cpp`
- `size_type_alias.cpp`
- `nested_type_alias.cpp`
- `template_typename_member.cpp`
- `dependent_member_call.cpp`
- `dependent_member_type.cpp`
- `dependent_base_lookup.cpp`
- `container_template_nested_value.cpp`

### Success Criterion

- template container code with nested aliases and dependent names compiles and
  behaves correctly for basic element types

## Phase 4: Iterator Surface

### Objective

Support the smallest useful traversal model.

### Add these positive tests

- `iterator_class_basic.cpp`
- `operator_deref_basic.cpp`
- `operator_arrow_basic.cpp`
- `operator_preinc_basic.cpp`
- `operator_postinc_basic.cpp`
- `operator_eq_basic.cpp`
- `operator_neq_basic.cpp`
- `begin_end_member_basic.cpp`
- `const_begin_end_member.cpp`
- `iterator_pair_loop.cpp`

### Success Criterion

- a container can expose iterators
- iterators can be incremented, dereferenced, and compared in simple loops

## Phase 5: Small Vector-Like Behavior

### Objective

Exercise the minimal API shape users expect from an STL-like container.

### Add these positive tests

- `container_push_back_basic.cpp`
- `container_pop_back_basic.cpp`
- `container_ctor_dtor_balance.cpp`
- `operator_bool_basic.cpp`
- `operator_plus_basic.cpp`
- `operator_minus_basic.cpp`
- `allocator_free_fixed_vector.cpp`

### Success Criterion

- a lightweight vector-like abstraction works for common happy-path operations
- element lifetime is not obviously broken in simple scenarios

## Phase 6: Nice-To-Have Compile-Time Integration

### Objective

Add a few compile-time-oriented features that make the container layer more
pleasant and future-proof.

### Add these positive tests

- `container_constexpr_size.cpp`
- `container_consteval_capacity.cpp`
- `range_for_basic.cpp`
- `range_for_const.cpp`

### Success Criterion

- simple compile-time size/capacity style queries are usable
- if range-for is supported, containers participate in it cleanly

## Recommended Priority

If we want the highest payoff first, the first batch should be:

- `default_ctor_basic.cpp`
- `copy_ctor_basic.cpp`
- `copy_assign_basic.cpp`
- `dtor_basic.cpp`
- `this_pointer_basic.cpp`
- `const_member_function_basic.cpp`
- `array_member_storage.cpp`
- `operator_subscript_basic.cpp`
- `operator_subscript_const.cpp`
- `iterator_class_basic.cpp`
- `begin_end_member_basic.cpp`
- `container_push_back_basic.cpp`
- `container_back_basic.cpp`
- `container_size_basic.cpp`
- `container_empty_basic.cpp`

This batch is enough to tell us whether "small container code" is becoming real
or is still blocked on fundamentals.

## Suggested File Naming Rule

Keep names literal and capability-driven:

- `<feature>_basic.cpp` for isolated single-feature tests
- `container_<api>_basic.cpp` for container-facing behavior
- `operator_<kind>_basic.cpp` for operator support
- `<dependent_or_template_feature>.cpp` for dependent-name cases

The file name should answer:

- what exact language rule is under test
- whether it is container-specific or general infrastructure

Avoid vague names like:

- `class_misc.cpp`
- `template_more.cpp`
- `container_advanced.cpp`

## Definition Of Done

This plan is successful when:

- internal positive tests cover the minimum language subset needed for a
  template-based container with iterators,
- the first vector-like container examples can be expressed without awkward
  feature gaps,
- the test suite reflects capability coverage rather than ad hoc examples,
- new C++ frontend work can be justified by concrete missing container tests.

## Practical Next Step

The most effective next move is:

1. add the Phase 1 and Phase 2 test files first,
2. implement only enough frontend support to make them pass,
3. then move to the Phase 3 dependent-name/template cases,
4. only after that expand iterator and vector-style ergonomics.

That keeps the language surface grounded in a real target instead of turning
into open-ended C++ feature chasing.
