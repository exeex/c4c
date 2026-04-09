# C-Style Cast Reference Follow-Ups Review

Status: Completed
Last Updated: 2026-04-09

## Goal

Review and reduce likely follow-up bugs around C-style casts that target complex
C++ types, especially reference-qualified and template-qualified forms.

## Why This Idea Exists

Recent parser and sema work around C-style casts exposed a cluster of nearby
risks:

- `template-id` casts such as `(box<int>*)raw` needed parser-side
  disambiguation fixes
- lvalue-reference casts such as `(int&)x` needed sema value-category repair
- rvalue-reference casts such as `(int&&)x` currently parse and validate, but
  deeper alias/writeback behavior may still differ from Clang

These are all signs that the cast pipeline still has pressure points at the
boundaries between parser disambiguation, semantic value-category tracking, and
HIR/lowering reference handling.

## Main Risks To Review

### 1. Reference-qualified cast targets

Examples:

- `(int&)x`
- `(const int&)x`
- `(volatile int&)x`
- `(int&&)x`

Questions:

- does sema assign the correct lvalue/xvalue category?
- do assignment and mutation through the casted result affect the original
  object when they should?

### 2. Typedef / alias-owned reference casts

Examples:

- `using R = int&; (R)x`
- `using RR = int&&; (RR)x`
- `using Alias = ns::box<int>::value_type; (Alias)x`

Questions:

- does typedef expansion preserve cast semantics?
- do parser and sema agree once the type is hidden behind an alias?

### 3. Qualified and dependent cast targets

Examples:

- `(ns::Type*)p`
- `(ns::Type&)x`
- `(typename holder<T>::value_type*)raw`
- `(typename holder<T>::value_type&)x`

Questions:

- does the parser still disambiguate correctly when `::`, `typename`, and
  template-ids are mixed into the type-name?
- are sema and HIR still consistent after dependent or qualified type parsing?

### 4. Template-id declarator suffixes inside C-style casts

Examples:

- `(box<int>*)raw`
- `(box<int>&)x`
- `(box<int>&&)x`
- `(box<int> const*)p`
- `(int (*)(int))raw`

Questions:

- are all declarator suffixes after template-ids handled consistently?
- did fixing pointer forms leave holes for reference or function-pointer forms?

### 5. Value-category propagation after casts

Examples:

- `((int&)x) = 3;`
- `foo((int&)x);`
- `bar((int&&)x);`
- `decltype((int&)x)`

Questions:

- does the cast result behave like the correct reference category in later
  expressions?
- do overload resolution and decltype-like consumers observe the right category?

### 6. Member/base access through casted references

Examples:

- `((S&)obj).field`
- `((Base&)derived).method()`
- `((T&&)x).member`

Questions:

- does member access preserve the owner type and reference semantics?
- do method-call and inherited-member paths stay aligned with cast semantics?

## Proposed Approach

1. Add focused `tests/cpp/internal/postive_case/` and, where appropriate,
   `negative_case/` reductions for each risk family.
2. Separate parser-only acceptance from runtime/behavior checks so the failing
   stage stays obvious.
3. Compare Clang behavior for value-category-sensitive cases before changing
   HIR or lowering.
4. Treat parser, sema, and HIR/lowering regressions as separate root-cause
   slices rather than one large cast umbrella fix.

## Success Criteria

- the likely cast/reference follow-up shapes are covered by targeted internal
  regressions
- known mismatches are classified by earliest failing stage
- no current cast fix relies on an unreviewed neighboring hole staying dormant

## Completion Summary

Closed on 2026-04-09 after finishing the final Step 5 audit and regression
coverage for casted member/base access.

- added
  `tests/cpp/internal/postive_case/c_style_cast_base_ref_qualified_method.cpp`
  to cover inherited ref-qualified method dispatch through `((Base&)derived)`
  and `((Base&&)derived)`
- confirmed the inherited casted-base method slice already matches Clang, so
  the closeout landed as regression-only coverage with no compiler change
- full validation passed at `2877/2877` tests with 0 failures, and the
  regression guard passed against `test_fail_before.log` with zero newly
  failing tests

## Leftover Follow-up

The cast-specific review is complete. Broader non-casted inheritance/layout
gaps remain tracked separately in
`ideas/open/45_inherited_record_layout_and_base_member_access_followup.md`.
