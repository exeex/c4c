# `types.cpp` Compression Refactor Plan

Status: Complete

Target file: `src/frontend/parser/types.cpp`  
Current size: 5235 lines  
Primary goal: compress the parser by extracting recurring parsing motifs and turning giant mixed-responsibility functions into coordinators.

## Why This File

`types.cpp` currently contains:
- template argument parsing
- dependent `typename` parsing
- base type parsing
- declarator parsing
- struct/union parsing
- enum parsing
- parameter parsing
- many small token-skipping and mangling helpers

The file is large because multiple parser subsystems are stacked into one implementation unit.

## Refactor Objective

Reduce duplication and branch fanout by extracting helpers around recurring token patterns, especially inside:
- `parse_base_type`
- `parse_struct_or_union`
- `parse_declarator`

## Highest-Value Compression Targets

### 1. `parse_base_type` branch decomposition

Hot region: `Parser::parse_base_type`

Symptoms:
- too many token-class branches in one function
- repeated patterns for qualified names, template ids, typedef resolution, and unresolved C++ fallback types

Extract helpers such as:
- `try_parse_dependent_typename_type(...)`
- `try_parse_qualified_typedef_type(...)`
- `try_parse_unresolved_template_type(...)`
- `finalize_resolved_typedef_type(...)`
- `try_apply_alias_template(...)`
- `try_instantiate_template_struct_type(...)`

Goal:
- keep `parse_base_type` as a dispatcher/coordinator

### 2. Template argument parsing family

Hot region: `parse_template_argument_list`

Repeated motifs:
- type-arg vs non-type-arg disambiguation
- expression capture fallback
- template-close handling

Extract helpers such as:
- `try_parse_template_type_arg(...)`
- `try_parse_template_non_type_arg(...)`
- `capture_template_arg_expr(...)`
- `is_clearly_value_template_arg(...)`

Goal:
- isolate the disambiguation policy from the loop skeleton

### 3. Qualified/dependent name consumption

Hot regions:
- `parse_dependent_typename_specifier`
- qualified-name handling inside `parse_base_type`
- template-id skipping in multiple places

Plan:
- consolidate all "consume qualified name with optional `<...>` after each segment" logic
- create one reusable helper for qualified dependent type spelling

Goal:
- avoid future reintroduction of ad hoc `typename` and `::` logic

### 4. Declarator suffix handling

Hot region: `parse_declarator`

Symptoms:
- pointer/ref/array/function/member-pointer/operator-name handling are intertwined
- multiple local lambdas cover distinct grammar families

Extract helpers such as:
- `parse_pointer_ref_qualifiers(...)`
- `parse_array_suffixes(...)`
- `parse_function_suffix(...)`
- `parse_operator_declarator_name(...)`
- `parse_member_pointer_prefix(...)`

Goal:
- separate declarator grammar pieces into composable stages

### 5. Struct/union body parsing

Hot region: `parse_struct_or_union`

Repeated motifs:
- attribute/alignment prelude parsing
- template/member-template clauses
- nested member declarations
- field/method/typedef branch splitting

Extract helpers such as:
- `parse_decl_attrs_for_record(...)`
- `parse_record_template_member_prelude(...)`
- `parse_record_typedef_member(...)`
- `parse_record_method_or_field(...)`
- `skip_record_member_initializer_if_present(...)`

Goal:
- flatten the nesting inside record-body parsing

## Suggested Agent Work Packages

### Package A: `parse_base_type` extraction

Scope:
- only helpers used by `parse_base_type`

Tasks:
- move branch families into `try_parse_*` helpers
- reduce nesting and repeated resolution logic

Validation:
- `typename` / template / qualified-name parse tests

### Package B: declarator compression

Scope:
- `parse_declarator` and related local lambdas

Tasks:
- split suffix parsing into explicit helpers
- keep parsing order unchanged

Validation:
- operator declarator tests
- array/function pointer tests

### Package C: record parsing compression

Scope:
- `parse_struct_or_union`

Tasks:
- isolate member-category parsing
- reduce the main loop to a top-level dispatcher

Validation:
- struct/union/member/template-member parse tests

## Guardrails

- Treat parser behavior as high-risk: refactor in small slices with targeted tests.
- Prefer helper extraction over "smart" grammar rewrites.
- Add or expand focused parse tests before/with each structural cleanup.
- Avoid simultaneous token-policy changes and code movement in one patch.

## Completion Notes

- Completed across bounded refactor slices with targeted parse-only coverage for
  template-argument parsing, qualified/dependent type consumption, declarator
  staging, and record-body dispatch.
- Final completion bar reached:
  - `parse_base_type` now acts as a helper-driven dispatcher
  - `parse_template_argument_list` uses a clear loop skeleton with extracted
    argument parsing helpers
  - qualified/dependent type spelling is centralized enough to avoid repeated
    ad hoc token walks
  - `parse_declarator` is staged through focused prefix/suffix/helper paths
  - `parse_struct_or_union` now dispatches through prebody/tag/definition
    helpers instead of carrying the old nested record-body stack directly
- Final validation on 2026-03-27:
  - clean baseline rebuild/suite: 2209 total, 7 failed
  - clean after rebuild/suite: 2209 total, 7 failed
  - regression guard: passed with no new failures and non-decreasing pass count

## Leftover Issues

- Full `ctest` still has the same pre-existing failures outside this refactor:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
  - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
  - `cpp_positive_sema_template_arg_deduction_cpp`
  - `cpp_positive_sema_template_mixed_params_cpp`
  - `cpp_positive_sema_template_type_subst_cpp`

## Good First Patch

Extract `parse_base_type` sub-branches for dependent `typename`, qualified typedefs, and unresolved template types into `try_parse_*` helpers. This gives high readability gain with comparatively low grammar risk.
