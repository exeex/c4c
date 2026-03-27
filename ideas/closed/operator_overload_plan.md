# Operator Overload Status And Remaining Debt

## Goal

Record the current state of user-defined operator overloading support after the
first STL-enablement slice landed, and call out the remaining parser/sema/HIR
debt that is still blocking broader C++ header support.

This is no longer a greenfield enablement plan.
The member-operator subset needed by iterator/container work is mostly in.
What remains is cleanup of parser special-cases, better semantic integration,
and a few missing C++ forms exposed by EASTL/EABase.

## What Is Landed

Implemented and covered in-tree:

- member operator declaration parsing and canonical naming
- operator expression lowering through the ordinary method/function call path
- member operators used by iterator/container code:
  - `operator[]`
  - `operator*`
  - `operator->`
  - prefix/postfix `operator++`
  - `operator==`
  - `operator!=`
  - `operator bool`
  - simple member `operator+` / `operator-`
- const/non-const member selection needed by iterator/container APIs
- out-of-class operator definitions for ordinary named operators such as
  `S::operator<<`
- out-of-class conversion operator definitions such as
  `S::operator float() const`
- out-of-class constructors
- parser regression coverage for:
  - operator declarator spellings
  - qualified operator names
  - out-of-class conversion operator definitions
  - parse stability around overloaded shift expressions in method bodies

This is enough for the original iterator/container milestone.

## Current Implementation Shape

The current implementation works, but it is not yet a clean unified model.
There are still several targeted workarounds in the parser:

- operator names are recognized by hardcoded token-to-mangled-name mapping
- ambiguous operators use deferred names such as
  `operator_star_pending`, `operator_amp_pending`,
  `operator_inc_pending`, `operator_dec_pending`
  and are finalized later from parameter count
- out-of-class constructors are handled by a dedicated top-level special-case
- out-of-class conversion operators are handled by a second dedicated
  top-level special-case
- qualified-name reconstruction for declarators still depends on handwritten
  `::` stitching logic

That is acceptable for the current milestone, but it is still more
special-case-driven than a full declarator-based solution should be.

## Recommended Internal Model

Keep the C++ surface grammar in the parser, but canonicalize cast-like
operators into a template-specialization-shaped internal form after parsing.

That means:

- the parser should still accept ordinary C++ spellings such as:
  - `operator bool()`
  - `operator unsigned long long()`
  - `operator T*()`
  - `MyType(x)` / constructor syntax
- AST should retain the real C++ meaning:
  - symbolic operator
  - conversion operator
  - constructor

But once we reach HIR/canonicalization, cast-like operations should be lowered
into a unified internal model that behaves like a template specialization:

- conversion operator:
  - `obj.operator T()` -> `CastTo<T>(obj)`
- constructor/direct-init/copy-init:
  - `MyType(x)` -> `CastFrom<MyType>(x)`

This should be treated as a canonical internal representation, not a user-facing
syntax rewrite.

### Why this is a good fit

- it lets us reuse existing template/candidate machinery instead of inventing a
  second ad hoc conversion pipeline
- non-template and template conversion operators fit the same shape naturally
- constructor-based conversions and conversion-operator-based conversions can be
  modeled under one cast-resolution framework
- HIR no longer needs to depend on parser-only mangled strings such as
  `operator_conv_float`

### Important boundary

This model should unify lowering, but not erase the semantic distinction between
the candidate sources:

- `CastTo<T>(obj)` candidates come from the source type's conversion operators
- `CastFrom<T>(args...)` candidates come from the target type's constructors

It also must preserve C++ context rules:

- implicit vs explicit cast context
- direct-init vs copy-init
- `explicit operator bool()` and other explicit conversion restrictions

So the recommendation is:

- preserve C++ grammar in parser/AST
- store structured operator metadata in AST
- canonicalize to `CastTo<T>` / `CastFrom<T>`-style specialized internal nodes
  in HIR/sema
- resolve those through the existing template-like specialization machinery
  where practical

## Workarounds We Should Intentionally Carry For Now

These are acceptable temporary choices, not immediate blockers:

- member-first operator resolution instead of full C++ overload ranking
- canonical internal names like `operator_conv_float` rather than preserving a
  richer first-class conversion-operator AST form
- dedicated parser entry paths for constructor and conversion-operator
  out-of-class definitions
- parse-only regression tests for forms where parser correctness is fixed but
  semantic/lowering support is still incomplete

These should stay in place until real coverage forces a more unified rewrite.

## Remaining Problems

### 1. Template conversion operators are still missing

Current parser support handles:

- `operator bool()`
- `operator unsigned long long()`
- `operator float()`
- other non-template conversion operators

Still missing:

- templated conversion operators such as `template<class T> operator T*()`

This is now the first operator-related blocker seen in the EASTL/EABase path:

- `ref/EABase/include/Common/EABase/nullptr.h:32`
- current failure: `unsupported operator overload token 'T'`

### 2. Out-of-class method bodies still rely on incomplete implicit-member lookup

Parsing is now much better, but semantic behavior is still incomplete.

Known gaps:

- unqualified field access inside out-of-class methods may still fail unless
  written as `this->field`
- unqualified static member calls inside out-of-class methods can still lower
  incorrectly as global function calls

Visible evidence already in-tree:

- `operator_conversion_out_of_class_parse.cpp` is currently suitable as a
  parse regression, but full compilation still fails on unqualified `mPart0`
- `operator_shift_static_member_call_parse.cpp` no longer hits the old parser
  failure, but the static member call path still needs semantic/lowering work

This means parser recovery has moved ahead of member-name resolution.

### 3. Operator parsing is still split across multiple handwritten paths

We currently parse operators in at least three different shapes:

- in-class operator members
- out-of-class ordinary named operators through general declarator parsing
- out-of-class conversion operators through a dedicated special-case

That split is the main reason fixes have recently come in as local patches
instead of one declarator-level model.

Risk:

- future operator grammar additions can easily fix one path but miss another
- bugs tend to appear as “parser drift” where later top-level declarations are
  attached incorrectly after one form is misparsed

### 4. Free-function operators are still intentionally incomplete

Still deferred:

- non-member/free-function operator lookup
- ADL-complete lookup
- Clang-like overload ranking

This is not a bug for the current milestone, but it remains an explicit
boundary of support.

## What Was Just Fixed Recently

Recent fixes worth preserving in mental model:

- qualified declarator names no longer lose `::` during reconstruction
- `alignas` is now tokenized as a keyword instead of an identifier
- out-of-class constructors now parse through a dedicated top-level path
- validator no longer treats qualified methods like conflicting global
  functions
- out-of-class non-template conversion operator definitions now parse as
  functions instead of falling out of the AST and corrupting following
  top-level parsing

The most important consequence is that the old `int128.h` failure around
`OperatorPlus(result, v01 << 32, result)` was not really a `<<` parser bug.
It was downstream damage from earlier operator-definition parse failure.

## Recommended Next Steps

Priority order:

1. Support templated conversion operators.
2. Fix implicit member lookup for out-of-class method bodies.
3. Fix unqualified static member call lowering inside out-of-class methods.
4. Only after that, decide whether to unify constructor/conversion/operator
   parsing under a cleaner declarator-based implementation.

## Definition Of Done For The Next Cleanup Slice

This operator subsystem will feel “stable enough” when:

- non-template and template conversion operators both parse correctly
- out-of-class method bodies can use unqualified member names the same way
  in-class method bodies do
- unqualified static member calls in out-of-class methods resolve as members,
  not globals
- EASTL/EABase no longer exposes new operator-specific parser failures before
  we hit broader template/library gaps

## Non-goals

- full C++ operator overloading parity
- perfect Clang overload ranking parity
- immediate removal of every parser special-case
- ADL-complete non-member operator support unless a real library case demands it
