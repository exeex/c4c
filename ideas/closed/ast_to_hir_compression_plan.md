# `ast_to_hir.cpp` Compression Refactor Plan

Status: Complete
Completed: 2026-03-27

## Completion Summary

- Completed the planned extraction families for:
  - initial program orchestration
  - pending template-struct resolution
  - function/method lowering setup
  - embedded parser isolation
  - layout helper isolation
- Finished the final Step 5 slice by isolating the remaining builtin
  `sizeof`/`alignof` lowering helpers and adding focused HIR coverage for that
  path.
- Final regression guard for the closing slice passed:
  - `test_fail_before.log`: 2230 total, 7 failed
  - `test_fail_after.log`: 2233 total, 7 failed
  - delta: `+3` passed, `0` new failures

## Leftover Notes

- No remaining work from this runbook was left active.
- The standing seven unrelated suite failures remained unchanged and were not
  expanded by this refactor series.

Target file: `src/frontend/hir/ast_to_hir.cpp`  
Current size: 9518 lines  
Primary goal: compress the file by extracting repeated lowering/instantiation/resolution mechanics into helpers without changing lowering order or semantic behavior.

## Why This File

`ast_to_hir.cpp` is the largest file in the tree and currently mixes:
- HIR lowering orchestration
- template instantiation coordination
- pending-template-type resolution
- struct/member typedef resolution
- small expression/text parsers
- layout/type helpers

This makes safe edits expensive because unrelated concerns are interleaved.

## Refactor Objective

Split by repeated mechanism, not by arbitrary line ranges. The win condition is:
- fewer long coordinator functions
- lower duplication of template/type resolution sequences
- smaller helpers with explicit ownership
- no semantic changes to lowering order, seeding, or fixpoint loops

## Highest-Value Compression Targets

### 1. Initial program lowering pipeline

Hot region: `Lowerer::lower_initial_program`

Repeated patterns:
- flatten / collect / seed / realize / verify phases
- per-phase loops over the same `items`
- special handling for template defs, explicit specializations, and consteval defs

Extract helpers such as:
- `flatten_program_items(...)`
- `collect_weak_symbols(...)`
- `collect_type_defs(...)`
- `collect_consteval_defs(...)`
- `collect_template_defs_and_specializations(...)`
- `collect_depth0_instantiations(...)`
- `run_consteval_template_seed_fixpoint(...)`
- `finalize_registry_realization(...)`

Constraint:
- preserve current phase order exactly
- keep `lower_initial_program` as a readable coordinator

### 2. Pending template struct resolution

Hot regions:
- `resolve_pending_tpl_struct_if_needed`
- `resolve_pending_tpl_struct`
- `instantiate_deferred_template_type`
- `instantiate_template_struct_body`
- call sites that do:
  - resolve pending struct if needed
  - resolve member typedef if present
  - re-run resolution on nested type

Extract helpers such as:
- `resolve_type_for_bindings(...)`
- `resolve_struct_member_type_if_needed(...)`
- `resolve_deferred_member_or_template_type(...)`
- `canonicalize_template_owner_primary(...)`

Goal:
- unify the repeated "resolve pending tpl struct, then maybe resolve member typedef" dance

### 3. Function/method lowering setup

Hot regions:
- `lower_function`
- `lower_struct_method`
- parameter/result binding setup
- repeated local context initialization

Extract helpers such as:
- `init_fn_lowering_context(...)`
- `bind_function_params(...)`
- `lower_function_body_into_hir(...)`
- `apply_template_bindings_to_signature(...)`

Goal:
- make function and method lowering share more setup code

### 4. Tiny embedded parsers / text decoders

Hot regions:
- `parse_pack_binding_name`
- mini parser around `parse_identifier`, `parse_number`, `parse_arg_text`, `resolve_arg`, `parse_primary`, `parse_unary`, etc.

Plan:
- isolate this mini parsing subsystem into a dedicated helper section or new file
- expose one narrow entrypoint instead of many local parser routines living beside lowering logic

Goal:
- reduce conceptual switching inside the file

### 5. Struct/type layout helper cluster

Hot regions:
- `type_size_bytes`
- `type_align_bytes`
- `field_size_bytes`
- `field_align_bytes`
- `compute_struct_layout`

Plan:
- move layout-only logic into a dedicated helper unit or at minimum a clearly isolated internal section
- keep ownership clear: layout math should not know about lowering orchestration

## Suggested Agent Work Packages

### Package A: Orchestration compression

Scope:
- only `lower_initial_program` and helpers it directly calls

Tasks:
- extract phase helpers
- preserve loop order
- keep behavior byte-for-byte where possible

Validation:
- existing HIR/lowering tests
- any template/consteval regression tests already in tree

### Package B: Template type resolution compression

Scope:
- pending template struct resolution and member typedef resolution helpers

Tasks:
- identify repeated resolution chains
- extract shared helper(s)
- minimize call-site branching

Validation:
- template struct instantiation tests
- member typedef / deferred type tests

### Package C: Embedded parser isolation

Scope:
- mini parser code only

Tasks:
- move parser routines behind one helper object or internal namespace
- keep call signatures stable where possible

Validation:
- deferred NTTP/default-expression related tests

## Guardrails

- Do not reorder seeding/realization phases unless explicitly intended and separately tested.
- Do not combine "cleanup" with behavior fixes in the same patch.
- Prefer extraction-first refactors before moving code across files.
- Keep each patch reviewable: one mechanism family per patch.

## Good First Patch

Refactor `lower_initial_program` into named phase helpers while preserving exact order and loop contents. This should cut the highest amount of cognitive load with the lowest semantic risk.
