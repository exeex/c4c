# Rvalue Reference Completeness Plan

Status: Open
Last Updated: 2026-04-03

## Goal

Turn current partial `&&` support into a deliberate, end-to-end C++ feature
track that covers parsing, template deduction, semantic binding rules,
value-category propagation, HIR lowering, and regression coverage.

## Why This Should Be A Separate Initiative

Recent EASTL Step 3 work uncovered multiple generic `&&`-related parser gaps in
libstdc++ `ranges_util.h`, including:

- deduction guides with forwarding-reference parameters such as
  `subrange(_Rng&&) -> ...`
- conversion operators to template-id types such as
  `operator in_in_result<_IIter1, _IIter2>() &&`

These are not EASTL-specific bugs. They point to a broader compiler-quality
track around rvalue references, forwarding references, and reference-aware
overload/binding behavior.

Keeping this as its own idea avoids silently stretching the current EASTL
bring-up plan into a repo-wide language-completeness effort.

## External Reference Notes

Reference material that should anchor behavior and terminology:

- Microsoft Learn:
  [Rvalue reference declarator: `&&`](https://learn.microsoft.com/zh-tw/cpp/cpp/rvalue-reference-declarator-amp-amp?view=msvc-170)
  highlights move semantics, perfect forwarding, and the reference-collapsing
  table used during template deduction.
- cppreference:
  [Reference declaration](https://en.cppreference.com/w/cpp/language/reference.html)
  distinguishes ordinary rvalue references from forwarding references, notes
  that `const T&&` is not a forwarding reference, explains `auto&&`, and
  reminds that named rvalue-reference variables are lvalues in expressions.

## Current Coverage In The Repo

Current tree support is real but incomplete:

- parser support exists for basic `T&&` declarations, parameters, casts,
  ref-qualified methods, and several template/dependent declaration shapes
- sema rejects obvious invalid bindings such as binding `T&&` to direct lvalue
  arguments in non-dependent cases
- HIR already prefers move over copy in several constructor/operator scoring
  paths and has some forwarding-reference deduction logic for template calls
- targeted regressions already exist for:
  `rvalue_ref_decl_basic`, `rvalue_ref_param_basic`,
  `template_rvalue_ref_param_basic`, `move_ctor_basic`,
  `move_assign_basic`, `ref_overload_*`, `static_cast_rvalue_ref_basic`, and
  newer parser reductions from the EASTL Step 3 frontier

## Missing Or Under-Audited Functionality

### 1. Full value-category modeling

The current implementation still appears too boolean in several places
(`is_ast_lvalue`-style checks and heuristic overload scoring) for a feature set
that really needs reliable lvalue/xvalue/prvalue distinctions.

Risk:

- wrong overload choice between `T&`, `const T&`, and `T&&`
- incorrect behavior for `std::move`, `std::forward`, cast results, and member
  access through xvalues

### 2. Complete reference-collapsing coverage

There is partial support for deducing `T&&` against lvalue arguments, but this
does not yet look fully audited across all substitution sites.

Missing coverage likely includes:

- alias/typedef-mediated collapsing
- qualified and dependent type spellings
- nested template substitutions
- constructor templates and deduction guides
- return-type and intermediate-expression collapsing

### 3. Forwarding-reference completeness

Forwarding-reference support should be validated for all standard shapes, not
just the common `template<class T> f(T&&)` case.

Important missing or under-tested cases:

- `auto&&`
- `decltype(auto)` returns that preserve reference category
- parameter packs such as `Args&&...`
- forwarding through wrappers into constructors, ref-qualified methods, and
  overloaded call operators
- ensuring `const T&&` is treated as a plain rvalue reference, not as a
  forwarding reference

### 4. Binding-rule completeness

The repo has basic direct-binding checks, but broader standard binding behavior
still needs explicit review.

Likely gaps:

- `const T&&`, `volatile T&&`, and cv-qualified variants
- arrays and function types with `&&`
- base/derived and user-defined conversion cases
- conditional/comma-expression results and other nontrivial xvalue producers
- temporary lifetime-extension edges
- named `T&&` variables behaving as lvalues unless explicitly moved/forwarded

### 5. Ref-qualified member semantics

Parsing support for trailing `&` / `&&` is improving, but semantic and lowering
behavior for ref-qualified member overloads still needs broader confidence.

Missing review areas:

- overload choice between `foo() &`, `foo() const&`, and `foo() &&`
- interactions with `operator()`, conversion operators, and inherited methods
- method dispatch after casts, `std::move`, and forwarding wrappers

### 6. Move special members beyond basic happy paths

The tree has early support for move construction and move assignment, but it is
not yet clear that defaulting/deletion rules and selection behavior are fully
standard-like.

Review targets:

- deleted/defaulted move constructors and move assignments
- const objects and const subobjects
- self-move patterns
- copy-vs-move selection under overload pressure
- parser/sema/HIR agreement for defaulted move members

### 7. Local declaration vs expression disambiguation under `&&` pressure

The current EASTL/libstdc++ work keeps surfacing cases where declaration-shaped
syntax is misread as expression syntax, or vice versa.

Known pressure areas:

- deduction guides
- template-id conversion operators
- block-scope `auto&&` / `decltype(...)` declarations
- heavy generic headers with nested templates and `requires`

### 8. Library-facing regression depth

There are useful unit-style regressions already, but not enough end-to-end
coverage proving that language support survives real-world template stacks.

Missing coverage should include:

- `std::tuple`, `std::ranges`, and `std::utility`-style forwarding machinery
- EASTL tuple/utility/function helper paths
- a small set of Clang-comparable perfect-forwarding runtime checks

## Proposed Execution Order

1. Audit and classify all existing `&&` tests by stage: parse, sema, HIR,
   runtime.
2. Add a matrix of focused internal regressions for:
   `T&&`, `const T&&`, `auto&&`, `Args&&...`, ref-qualified members,
   conversion operators, deduction guides, `std::forward`, and
   named-rvalue-reference lvalue behavior.
3. Repair parser declaration/expression disambiguation gaps first, because they
   block real library headers early.
4. Tighten sema binding and overload rules next, using Clang as the reference.
5. Audit HIR value-category propagation and move/forward lowering after parser
   and sema are stable.
6. Re-run library-facing probes in `std`/EASTL headers and only then expand to
   broader container or ranges pressure.

## Success Criteria

- the repo has explicit regression coverage for the major `&&` feature matrix,
  not just a few happy-path move tests
- forwarding references and ordinary rvalue references are distinguished
  correctly in parser, sema, and HIR
- overload choice and binding behavior match Clang on a representative set of
  lvalue/xvalue/prvalue cases
- current EASTL/libstdc++ `&&` blockers are reduced to later-stage issues or
  cleared entirely

## Non-Goal

This idea should not silently absorb unrelated STL/EASTL bring-up work. If a
library header reveals a broader `&&` language issue, fix the generic language
root cause here and keep container-specific progress in the active EASTL plan.
