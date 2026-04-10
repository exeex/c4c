# Template Trait Normalization And Owner Resolution Follow-Up

Status: Complete
Last Updated: 2026-04-10

## Completion Summary

Completed on 2026-04-10.

This follow-up is now closed. The planned generic mechanism work landed across
all four target families:

- alias-template substitution now keeps pack/default argument mapping and
  dependent member-typedef ownership aligned through substitution
- trait and variable-template evaluation now normalizes direct member-typedef
  operands consistently across `_v`, `::value`, and `typename ...::type` forms
- scoped enums preserve explicit underlying types through semantic layout and
  emitted storage decisions
- out-of-class method owner recovery now resolves through namespace context and
  canonical record tags instead of suffix or unqualified-name guessing

At closure time:

- the focused alias, trait, scoped-enum, and owner-resolution regressions pass
- the new qualified namespaced out-of-class owner regression passes
- the full repo suite passes at `3306/3306`

## Leftover Notes

- This plan intentionally stopped at generic owner recovery for out-of-class
  methods. Any future work on richer namespace aliasing, inline-namespace
  corner cases, or broader record-identity plumbing should start as a separate
  `ideas/open/*.md` initiative if it becomes active priority.
- The trait/alias work here fixed the targeted normalization gaps without
  attempting a broader template-system rewrite. Additional generic template
  expansion should be tracked separately if new pressure exposes it.

## Goal

Turn the recent EASTL/type-trait unblock work into a more complete generic
feature pass so current fixes stop depending on narrow regressions and
heuristic fallbacks.

The immediate goal is to finish the mechanism families that are only partially
repaired today:

1. alias-template substitution beyond the empty-pack/default-arg happy path
2. trait/value normalization for alias-transformed and member-typedef-backed
   template operands
3. scoped-enum semantics beyond parse acceptance
4. namespace-aware owner resolution for out-of-class methods without suffix
   guessing


## Why This Idea Exists

Recent EASTL bring-up commits clearly improved the main path, but the follow-up
review shows several areas are still thin:

- alias-template pack handling now accepts `void_t<>`, but substitution still
  assumes a mostly positional one-arg-per-parameter mapping
- variable-template trait support currently recognizes only a small hard-coded
  set of trait families
- scoped enums with explicit underlying types are accepted syntactically, but
  the underlying type is still discarded instead of modeled
- out-of-class method owner recovery still falls back to suffix/unqualified
  name heuristics when direct namespace-context recovery fails

These patches are good unblockers, but they are not yet a stable end state for
generic C++ support. Leaving them as-is risks repeating the same failure shape
when EASTL or future library pressure arrives through a slightly different
syntax form.


## Main Objective

Promote the recent "repair the current EASTL case" work into complete generic
mechanism support with broader regression coverage.

Success means:

- alias-template application behaves predictably for empty and non-empty packs,
  defaults, and member-typedef-backed aliases
- trait-style variable templates operate on normalized semantic types instead
  of ad hoc family-specific shortcuts where feasible
- scoped-enum underlying types influence semantic behavior consistently
- owner lookup for out-of-class methods is driven by explicit namespace/record
  identity rather than best-effort suffix matching


## Proposed Work Areas

### 1. Finish Alias-Template Argument Mapping

Focus:

- pack-aware substitution for alias templates, not just pack-aware acceptance
- defaulted alias parameters mixed with packs
- dependent/member-typedef outputs such as
  `enable_if_t`, `conditional_t`, `remove_reference_t`, and `remove_cv_t`

Target outcome:

- alias application no longer treats non-empty packs or late defaults as a
  parse-only success followed by a semantic collapse


### 2. Generalize Trait / Variable-Template Normalization

Focus:

- reduce reliance on hard-coded trait-family special handling
- normalize alias-transformed types before trait/value evaluation
- keep `*_v` style variable templates aligned with their `::value` /
  `typename ...::type` backing forms

Target outcome:

- `eastl_type_traits_simple.cpp` no longer depends on narrow handwritten
  special cases for only `is_signed`, `is_unsigned`, `is_const`,
  `is_reference`, and `is_same`


### 3. Complete Scoped-Enum Semantics

Focus:

- preserve explicit underlying type information from parsing onward
- add coverage beyond cast-heavy happy paths
- validate size/layout/conversion-sensitive scenarios that library code may
  depend on

Target outcome:

- `enum class E : unsigned char` and similar forms behave as real typed enums,
  not merely "accepted syntax"


### 4. Replace Heuristic Owner Recovery

Focus:

- resolve out-of-class method owners through namespace context and canonical
  record identity first
- reduce or eliminate suffix/unqualified fallback matching where ambiguity is
  possible
- add collision regressions with repeated unqualified record names across
  namespaces

Target outcome:

- member lookup, implicit `this`, and owner tagging stay correct even in
  multi-namespace code with repeated type names


## Suggested Initial Repro Set

Start from the already-known pressure sources:

- `tests/cpp/eastl/eastl_type_traits_simple.cpp`
- `tests/cpp/internal/postive_case/template_variable_alias_member_typedef_runtime.cpp`
- `tests/cpp/internal/parse_only_case/template_alias_empty_pack_default_arg_parse.cpp`
- `tests/cpp/internal/postive_case/scoped_enum_bitwise_runtime.cpp`
- `tests/cpp/internal/postive_case/namespaced_out_of_class_method_context_frontend.cpp`

Then add narrower follow-up coverage for:

- non-empty alias template packs
- alias defaults mixed with packs
- scoped-enum underlying-size behavior
- duplicated owner names across different namespaces


## Guardrails

- do not land EASTL-specific parser/sema/HIR exceptions for these cases
- prefer generic semantic modeling over trait-family whitelists when practical
- keep parser acceptance, semantic correctness, and lowering/runtime behavior
  separated in tests
- if full trait generalization grows too large, split a narrower prerequisite
  idea instead of hiding that expansion inside the active EASTL runbook


## Expected Payoff

This follow-up should reduce repeated "move the blocker one step later" churn
in EASTL and make future STL/EASTL pressure land on more complete C++
mechanisms instead of another round of narrow patching.
