# `types.cpp` Compression Refactor Plan

## Plan Metadata

- Status: Active
- Source Idea: [ideas/open/types_compression_plan.md](/workspaces/c4c/ideas/open/types_compression_plan.md)

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook.
Do not rewrite `types.cpp` in one pass.
Do not mix parser cleanup, token-policy changes, and grammar redesign into one patch.
Do not start by reorganizing unrelated parser files.


## One-Sentence Goal

Compress [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp) by extracting recurring parsing motifs and turning giant mixed-responsibility functions into coordinators, while preserving parser behavior through small, test-backed refactor slices.


## Core Rule

Treat this as a behavior-preserving compression refactor:

1. extract helpers around repeated parsing motifs
2. reduce branch fanout in oversized functions
3. keep parsing order and token policy unchanged unless a focused test requires otherwise

But do **not** turn this into a broad grammar rewrite.

Instead:

- keep the current parser behavior as the default contract
- move one responsibility cluster at a time into helpers
- validate each slice with the narrowest relevant parse tests


## Read First

Before editing, open these exact files:

1. [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
2. [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
3. [plan.md](/workspaces/c4c/plan.md)

Then identify the current implementations of:

1. `Parser::parse_base_type`
2. `parse_template_argument_list`
3. dependent `typename` parsing helpers
4. `parse_declarator`
5. `parse_struct_or_union`

Only open broader parser surfaces after you localize the exact repeated motif you are extracting.

Do not begin in:

- unrelated semantic-analysis files
- codegen or HIR lowering code
- EASTL/libstdc++ bring-up cases
- broad parser-wide naming cleanups


## Current Compression Targets

Keep these areas in mind throughout the work:

- [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- `Parser::parse_base_type`
- `parse_template_argument_list`
- dependent / qualified name consumption helpers
- `parse_declarator`
- `parse_struct_or_union`


## Non-Goals

Do not do these as part of this run:

- redesign the C++ type grammar
- change template-argument disambiguation policy globally without focused evidence
- split `types.cpp` across many new files in one patch
- refactor unrelated parser modules just for consistency
- combine compression work with speculative bug fixes


## Working Model

You are building toward this parser structure:

- `parse_base_type` acts as a dispatcher
- template-argument parsing keeps one loop skeleton with extracted disambiguation helpers
- qualified/dependent name consumption shares reusable logic
- declarator parsing is split into composable grammar stages
- record-body parsing uses top-level member-category dispatch helpers

You are building toward this refactor model:

- extract one repeated motif at a time
- keep behavior stable with targeted tests
- prefer helper extraction over smart rewrites


## Execution Rules

Follow these rules exactly:

1. One edit slice should target one compression cluster.
2. Preserve parsing behavior unless a narrow test proves a correction is required.
3. Do not combine code movement and token-policy changes in the same patch.
4. Keep the coordinator function readable after each extraction.
5. After each step, rerun only the smallest relevant parser tests first.
6. If a helper name hides real control flow, simplify the helper before continuing.


## Step 1. Freeze The Refactor Surface

Goal:

- identify the exact oversized functions and recurring motifs before editing

Do this:

1. inspect [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
2. confirm the main compression candidates are:
   - `Parser::parse_base_type`
   - `parse_template_argument_list`
   - dependent / qualified name parsing helpers
   - `parse_declarator`
   - `parse_struct_or_union`
3. note repeated token patterns, local lambdas, and duplicated fallback paths
4. do not edit until the first extraction slice is clearly isolated

Completion check:

- the initial refactor frontier is explicit and bounded


## Step 2. Compress `parse_base_type`

Goal:

- turn `parse_base_type` into a coordinator instead of a mixed-responsibility branch wall

Primary target:

- `Parser::parse_base_type`

Do this:

1. isolate repeated branches for dependent `typename`, qualified typedefs, unresolved template types, and typedef finalization
2. extract narrow helpers such as:
   - `try_parse_dependent_typename_type(...)`
   - `try_parse_qualified_typedef_type(...)`
   - `try_parse_unresolved_template_type(...)`
   - `finalize_resolved_typedef_type(...)`
   - `try_apply_alias_template(...)`
   - `try_instantiate_template_struct_type(...)`
3. keep parsing order unchanged
4. do not redesign type-resolution policy while extracting helpers

Completion check:

- `parse_base_type` reads as a dispatcher/coordinator and relevant parse tests remain green


## Step 3. Compress Template Argument Parsing

Goal:

- isolate template-argument disambiguation policy from the loop skeleton

Primary target:

- `parse_template_argument_list`

Do this:

1. extract the repeated type-arg vs non-type-arg decision points
2. isolate expression capture fallback and template-close handling
3. add helpers such as:
   - `try_parse_template_type_arg(...)`
   - `try_parse_template_non_type_arg(...)`
   - `capture_template_arg_expr(...)`
   - `is_clearly_value_template_arg(...)`
4. do not change the existing acceptance of ambiguous cases unless a focused test requires it

Completion check:

- template-argument parsing has one clear loop skeleton with helper-driven disambiguation


## Step 4. Consolidate Qualified And Dependent Name Consumption

Goal:

- avoid ad hoc `typename`, `::`, and template-id handling across multiple parsing paths

Primary targets:

- dependent `typename` parsing helpers
- qualified-name handling inside `parse_base_type`

Do this:

1. locate all current "consume qualified name with optional `<...>` per segment" logic
2. extract one reusable helper for qualified dependent type spelling
3. make `parse_base_type` and dependent-type parsing reuse the same consumption path where practical
4. do not broaden the accepted grammar beyond what the current parser already supports

Completion check:

- qualified/dependent name handling is centralized enough to prevent repeated ad hoc token walks


## Step 5. Compress Declarator Suffix Handling

Goal:

- separate declarator grammar families into composable stages

Primary target:

- `parse_declarator`

Do this:

1. identify intertwined handling for pointer/ref qualifiers, arrays, function suffixes, member pointers, and operator declarator names
2. extract helpers such as:
   - `parse_pointer_ref_qualifiers(...)`
   - `parse_array_suffixes(...)`
   - `parse_function_suffix(...)`
   - `parse_operator_declarator_name(...)`
   - `parse_member_pointer_prefix(...)`
3. preserve parsing order and existing precedence
4. do not rewrite declarator grammar semantics during the extraction

Completion check:

- `parse_declarator` becomes a staged coordinator with unchanged behavior on declarator tests


## Step 6. Compress Struct/Union Body Parsing

Goal:

- flatten the nesting inside record-body parsing

Primary target:

- `parse_struct_or_union`

Do this:

1. isolate repeated record-member preludes such as attributes, alignment, and template/member-template setup
2. split member-category handling into focused helpers such as:
   - `parse_decl_attrs_for_record(...)`
   - `parse_record_template_member_prelude(...)`
   - `parse_record_typedef_member(...)`
   - `parse_record_method_or_field(...)`
   - `skip_record_member_initializer_if_present(...)`
3. keep the main record loop as a top-level dispatcher
4. do not change record-member parsing policy while compressing the function

Completion check:

- `parse_struct_or_union` reads as a record-body dispatcher instead of a deeply nested branch stack


## Step 7. Keep Refactor Risk Contained

Goal:

- ensure compression work does not silently change parser behavior

Do this:

1. rerun focused parser tests after every extraction slice
2. prefer adding or expanding targeted parse tests alongside risky refactors
3. if behavior changes unexpectedly, reduce the slice and restore the previous control flow shape before proceeding
4. keep helper extraction and policy changes in separate patches whenever possible

Completion check:

- each completed slice is behavior-backed rather than readability-only guesswork


## Suggested Work Packages

Use this grouping if the work is split into separate passes:

1. Package A: `parse_base_type` extraction only
2. Package B: `parse_declarator` compression only
3. Package C: `parse_struct_or_union` compression only

Validation focus:

1. Package A: `typename` / template / qualified-name parse tests
2. Package B: operator declarator and array/function-pointer tests
3. Package C: struct/union/member/template-member parse tests


## Suggested Test Order

Use this order after each relevant edit:

1. the smallest parser test covering the slice you touched
2. nearby parser tests for the same grammar family
3. broader `types.cpp` parser coverage only after the focused tests stay green


## Good First Patch

Start here unless a narrower hotspot is more urgent:

1. extract the `parse_base_type` sub-branches for dependent `typename`
2. extract qualified typedef handling
3. extract unresolved template type handling

Why this first:

- high readability gain
- comparatively low grammar risk
- good leverage for later extractions


## What “Done” Means For This Plan

This execution plan is complete when all of the following are true:

1. `parse_base_type` is primarily a dispatcher with extracted helper families
2. template-argument parsing has clear helper-based disambiguation
3. qualified/dependent name consumption is consolidated enough to avoid repeated ad hoc logic
4. `parse_declarator` is split into readable grammar stages
5. `parse_struct_or_union` is flattened into a top-level dispatcher with focused helpers
6. the relevant parser tests remain green or any behavior change is explicitly intended and documented


## If You Get Stuck

Do this:

1. stop broad extraction
2. confirm the current slice still has a single responsibility
3. compare the helperized path against the original control flow
4. make one smaller extraction
5. rerun only the nearest relevant parser tests

Do not do this:

- do not hide grammar changes inside readability refactors
- do not rename everything just because code moved
- do not split the file structure broadly before the internal helper boundaries are proven
