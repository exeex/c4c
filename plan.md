# Template Trait Normalization And Owner Resolution

Status: Active
Source Idea: ideas/open/48_template_trait_normalization_and_owner_resolution.md

## Purpose

Turn the recent EASTL/type-trait unblock work into complete generic mechanism
support so current fixes stop depending on narrow regressions and heuristic
fallbacks.

## Goal

Finish alias-template substitution, trait/value normalization, scoped-enum
semantics, and out-of-class owner resolution with test-backed slices that
improve generic C++ behavior without adding EASTL-specific exceptions.

## Core Rule

Implement generic semantic support only. Do not land EASTL-specific parser,
sema, HIR, or codegen exceptions to satisfy the target cases.

## Read First

- [ideas/open/48_template_trait_normalization_and_owner_resolution.md](/workspaces/c4c/ideas/open/48_template_trait_normalization_and_owner_resolution.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Current Targets

- Alias-template argument mapping beyond the empty-pack/default-arg happy path
- Trait and variable-template normalization for alias-transformed and
  member-typedef-backed types
- Scoped-enum underlying-type semantics beyond parse acceptance
- Namespace-aware owner resolution for out-of-class method definitions

## Validation Seeds

- `tests/cpp/eastl/eastl_type_traits_simple.cpp`
- `tests/cpp/internal/postive_case/template_variable_alias_member_typedef_runtime.cpp`
- `tests/cpp/internal/parse_only_case/template_alias_empty_pack_default_arg_parse.cpp`
- `tests/cpp/internal/postive_case/scoped_enum_bitwise_runtime.cpp`
- `tests/cpp/internal/postive_case/namespaced_out_of_class_method_context_frontend.cpp`

## Non-Goals

- Broad generic-template rewrites not required by the four target mechanisms
- EASTL-only compatibility hacks
- Folding unrelated newly discovered initiatives into this runbook

## Working Model

Treat each mechanism family as its own test-backed slice. Start with the
narrowest failing or weakly covered behavior, add focused coverage first,
implement one root-cause fix at a time, then re-run nearby tests before moving
to the next family.

## Execution Rules

- Keep parser acceptance, semantic correctness, and lowering/runtime behavior
  separated in tests when possible.
- Prefer canonical type/owner identity flows over family-specific whitelists
  and suffix guessing.
- If a required fix uncovers adjacent but non-blocking work, record it in the
  source idea instead of expanding the runbook.
- If trait generalization expands beyond this runbook's scope, create a new
  idea rather than hiding the expansion inside this plan.

## Step 1: Stabilize Alias-Template Argument Mapping

Goal: make alias-template substitution behave predictably for empty packs,
non-empty packs, defaults, and member-typedef-backed outputs.

Primary targets:

- parser/sema/HIR surfaces involved in alias-template application
- tests around alias-template packs and defaults

Actions:

- inspect existing alias-template parameter-to-argument mapping behavior
- add focused coverage for non-empty packs and defaults mixed with packs
- cover member-typedef-backed alias outputs such as `enable_if_t`,
  `conditional_t`, `remove_reference_t`, and `remove_cv_t` when they expose the
  current gap
- implement the smallest generic substitution fix that preserves existing happy
  paths

Completion check:

- alias-template tests no longer collapse after parse-only success
- the targeted alias cases pass without special-casing EASTL syntax

## Step 2: Generalize Trait And Variable-Template Normalization

Goal: evaluate trait-style templates on normalized semantic types instead of
narrow hard-coded family handling.

Primary targets:

- trait/value evaluation code paths
- variable-template handling for `*_v` forms
- EASTL/simple type-trait regression coverage

Actions:

- identify where alias-transformed and member-typedef-backed types bypass
  canonical normalization
- add or tighten tests that compare `*_v`, `::value`, and `typename ...::type`
  forms over the same underlying types
- replace narrow family-specific shortcuts where a generic normalized path is
  practical

Completion check:

- `eastl_type_traits_simple.cpp` and focused trait regressions pass because
  normalized semantic types flow through evaluation correctly

## Step 3: Complete Scoped-Enum Semantics

Goal: preserve and use explicit scoped-enum underlying types through semantic
modeling and downstream behavior.

Primary targets:

- parser/sema/HIR record or enum type modeling
- tests for explicit underlying-size and conversion-sensitive behavior

Actions:

- confirm where explicit underlying-type information is currently discarded
- add coverage beyond cast-heavy acceptance, including size/layout-sensitive
  scenarios where practical
- implement the smallest semantic-model fix that keeps typed-enum behavior
  explicit across later stages

Completion check:

- scoped enums with explicit underlying types behave as typed enums, not only
  accepted syntax

## Step 4: Replace Heuristic Owner Recovery

Goal: resolve out-of-class method owners from namespace context and canonical
record identity instead of suffix or unqualified-name guessing.

Primary targets:

- owner lookup for out-of-class method definitions
- member lookup / implicit `this` tagging in repeated-name namespace cases

Actions:

- inspect the current fallback path for out-of-class owner recovery
- add collision coverage using repeated record names across different
  namespaces
- route owner resolution through explicit namespace and canonical record
  identity before any fallback is considered

Completion check:

- namespaced out-of-class method regressions pass without ambiguous suffix
  matching

## Final Verification

- re-run targeted tests for each completed slice
- run nearby subsystem coverage after each fix
- run the full configured regression suite before handoff
- compare before/after suite results and keep them monotonic
