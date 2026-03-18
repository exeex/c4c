# Operator Overload Enablement Plan

## Goal

Establish the minimum user-defined operator overloading support needed as a
prerequisite for STL-like iterator and container code.

This is a language-support plan, not a completeness plan.
We do not need full C++ operator overloading up front.
We need the smallest coherent subset that unlocks:

- element access via `operator[]`
- iterator dereference via `operator*`
- iterator member access via `operator->`
- iterator increment via `operator++`
- iterator comparison via `operator==` / `operator!=`
- lightweight state checks via `operator bool`
- optional simple iterator arithmetic via `operator+` / `operator-`

## Why This Must Be A Separate Prerequisite

`stl_plan.md` currently assumes iterator/container APIs such as:

- `operator[]`
- `begin()` / `end()`
- `operator*`
- `operator->`
- pre/post increment
- `==` / `!=`

Those are not just surface syntax.
They require coordinated support across:

- parsing operator-function declarations
- naming and symbol identity for operator functions
- overload resolution at call sites
- method vs free-function dispatch rules
- AST/HIR lowering for rewritten operator calls
- codegen for the resulting ordinary calls

If we do not isolate this work, the STL plan will stall in Phase 2 and Phase 4
on a broad, cross-cutting missing feature.

## Scope Boundary

### In scope

- user-defined overloaded operators needed for minimal containers/iterators
- member operator functions first
- internal positive/negative tests that pin down semantics
- enough overload resolution to distinguish the supported forms

### Out of scope for the first milestone

- every overloadable C++ operator
- full ranking parity with Clang
- operators unrelated to STL-like code such as `operator,`, `operator new`,
  `operator delete`, `operator&&`, `operator||`
- implicit conversion-heavy corner cases
- ADL-complete free-function operator lookup

## Current Baseline

What we appear to have today:

- basic struct methods
- template struct methods
- implicit `this` lowering in HIR
- existing expression parsing for built-in operators

What we do not appear to have yet as first-class user-facing support:

- parsing declarations named `operator[]`, `operator*`, `operator->`, etc.
- dedicated tests for overloaded operator declarations and calls
- overload resolution for user-defined operators
- explicit lowering rules from operator syntax to user-defined function calls

## Design Principle

Treat operator overloading as desugaring plus constrained overload resolution.

For the supported subset, each operator expression should lower as closely as
possible to an ordinary function/method call once semantic resolution succeeds.

Examples:

- `obj[i]` -> `obj.operator[](i)` for supported member form
- `*it` -> `it.operator*()`
- `it->field` -> `it.operator->()->field` or a constrained first-step member call
- `++it` -> `it.operator++()`
- `it++` -> `it.operator++(0)`
- `a == b` -> `a.operator==(b)` or supported non-member equivalent later

The first milestone should strongly prefer member operators because that is
enough for container and iterator prototypes and keeps lookup simpler.

## Recommended Phases

## Phase 0: Syntax and naming foundation

### Objective

Teach the parser and symbol pipeline to recognize operator-function
declarations without yet claiming full call support.

### Required work

- parse operator-function declarator names:
  - `operator[]`
  - `operator*`
  - `operator->`
  - `operator++`
  - `operator==`
  - `operator!=`
  - `operator bool`
  - optionally `operator+`
  - optionally `operator-`
- represent operator kind canonically in AST nodes and symbol identity
- ensure methods can carry operator names without colliding with ordinary names
- expose readable dumps for debugging / HIR inspection

### Add these positive tests

- `operator_decl_subscript_parse.cpp`
- `operator_decl_deref_parse.cpp`
- `operator_decl_arrow_parse.cpp`
- `operator_decl_preinc_parse.cpp`
- `operator_decl_eq_parse.cpp`
- `operator_decl_bool_parse.cpp`

### Add these negative tests

- `bad_operator_unknown_token.cpp`
- `bad_operator_arity_decl.cpp`
- `bad_conversion_operator_with_return_type.cpp`

### Success criterion

- The frontend can parse and retain supported operator-function declarations in
  classes/structs with stable identities.

## Phase 1: Member-call semantic resolution

### Objective

Resolve supported operator expressions to member operator functions.

### Required work

- semantic matching from expression form to member operator candidate set
- arity checks by operator kind
- cv-qualification checks for const member operators
- basic type compatibility on explicit operands
- error reporting when the operator exists but is used with the wrong shape

### Supported first-wave operators

- `operator[]`
- `operator*`
- `operator++` prefix
- `operator==`
- `operator!=`
- `operator bool`

### Add these positive tests

- `operator_subscript_member_basic.cpp`
- `operator_subscript_member_const.cpp`
- `operator_deref_member_basic.cpp`
- `operator_preinc_member_basic.cpp`
- `operator_eq_member_basic.cpp`
- `operator_neq_member_basic.cpp`
- `operator_bool_member_basic.cpp`

### Add these negative tests

- `bad_operator_subscript_arity.cpp`
- `bad_operator_deref_extra_arg.cpp`
- `bad_operator_bool_arglist.cpp`
- `bad_operator_const_call.cpp`

### Success criterion

- Supported operator expressions resolve to the intended member functions and
  fail clearly when the declaration shape is invalid.

## Phase 2: Lowering to ordinary calls

### Objective

Make resolved operator expressions executable by lowering them through the
existing method/function call path.

### Required work

- lower resolved operator expressions to ordinary call-style AST/HIR nodes
- preserve value category and result type
- preserve `this` semantics for member operators
- ensure codegen sees ordinary calls rather than bespoke operator nodes where
  practical

### Add these positive tests

- `operator_subscript_runtime_basic.cpp`
- `operator_deref_runtime_basic.cpp`
- `operator_preinc_runtime_basic.cpp`
- `operator_eq_runtime_basic.cpp`
- `operator_bool_runtime_basic.cpp`

### Add these HIR/debug tests

- `operator_subscript_hir_dump.cpp`
- `operator_deref_hir_dump.cpp`

### Success criterion

- Resolved member operators survive lowering and execute correctly in runtime
  tests.

## Phase 3: Iterator-critical operators

### Objective

Cover the operators specifically needed for iterator usability.

### Required work

- postfix `operator++(int)`
- `operator->`
- stable comparison semantics for iterator pairs
- optional `operator*` const overload

### Add these positive tests

- `operator_postinc_member_basic.cpp`
- `operator_arrow_member_basic.cpp`
- `operator_arrow_chain_basic.cpp`
- `iterator_eq_neq_basic.cpp`
- `iterator_deref_const_basic.cpp`

### Add these negative tests

- `bad_operator_postinc_signature.cpp`
- `bad_operator_arrow_nonpointer_result.cpp`
- `bad_operator_arrow_cycle.cpp`

### Success criterion

- A minimal custom iterator can be incremented, dereferenced, compared, and
  used with arrow access in plain loops.

## Phase 4: Container-facing operators

### Objective

Finish the operator subset directly needed by the STL plan.

### Required work

- robust `operator[]` const/non-const pair support
- `operator bool` for lightweight handles/iterators
- optional simple arithmetic support for iterator stepping

### Add these positive tests

- `container_subscript_pair.cpp`
- `container_bool_state.cpp`
- `iterator_plus_basic.cpp`
- `iterator_minus_basic.cpp`

### Add these negative tests

- `bad_operator_plus_arity.cpp`
- `bad_operator_minus_type.cpp`

### Success criterion

- The operator surface required by fixed-storage containers and simple iterators
  is covered by passing internal tests.

## Phase 5: Optional free-function operator support

### Objective

Decide whether the STL plan needs non-member operators before more complete
iterator ergonomics.

### Notes

- This phase should be deferred unless real tests require it.
- Member operators are enough for the first STL-like milestone.

### Possible scope

- non-member `operator==` / `operator!=`
- non-member arithmetic helpers
- constrained ADL-aware lookup

### Success criterion

- Only pursued if container/iterator tests demonstrate that member-only support
  is too limiting.

## Recommended priority

The highest-payoff first batch is:

- `operator[]`
- `operator*`
- prefix `operator++`
- `operator==`
- `operator!=`
- `operator bool`

Then add:

- postfix `operator++`
- `operator->`

Only after that consider:

- `operator+`
- `operator-`
- non-member operators

## Integration with the STL plan

This plan should be treated as a prerequisite slice for:

- `stl_plan.md` Phase 2: element access and storage
- `stl_plan.md` Phase 4: iterator surface
- `stl_plan.md` Phase 5: small vector-like behavior

Practical sequencing:

1. finish Phase 0-2 here
2. land first operator runtime tests
3. then start `stl_plan_todo.md` Phase 2 and Phase 4 work

## Definition of done

This plan is successful when:

- supported operator declarations parse correctly
- supported operator expressions resolve semantically
- lowering/codegen execute those operators through ordinary call paths
- iterator/container-facing operator tests pass without ad hoc hacks

## Non-goals

- full C++ operator overloading parity
- perfect overload ranking parity with Clang
- move-only / allocator-heavy container semantics
- broad standard library emulation
