# Template Struct Instance Resolution Runbook

Status: Active
Source Idea: ideas/open/16_template_struct_instance_resolution_for_symbol_lookup.md
Supersedes: ideas/open/15_cpp_external_eastl_suite_bootstrap.md

## Purpose

Route template-struct member/symbol lookup through the same
partial-specialization and instantiation machinery that already chooses
concrete struct bodies, so later lowering stops depending on intermediate
emitted-name-like tags.

## Goal

Land a shared template-instance resolution helper, hook late member lookup into
it, and prove the exposed `eastl::pair<int, int>` member path no longer fails
on `pair_T1_T1_T2_T2`.

## Core Rule

Do not accept pair-specific string aliases, emitted-name shims, or testcase
shape matching as progress. The fix must generalize the semantic lookup path.

## Read First

- [ideas/open/16_template_struct_instance_resolution_for_symbol_lookup.md](/workspaces/c4c/ideas/open/16_template_struct_instance_resolution_for_symbol_lookup.md)
- [src/frontend/hir/hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [src/frontend/hir/hir_templates_materialization.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_materialization.cpp)
- [src/frontend/hir/hir_templates_struct_instantiation.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_struct_instantiation.cpp)
- [src/codegen/lir/stmt_emitter_lvalue.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_lvalue.cpp)
- [src/codegen/lir/stmt_emitter_types.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_types.cpp)

## Current Scope

- extract and centralize template-struct instance resolution for primary and
  partial-specialized paths
- use that shared resolver from late member/symbol lookup when the incoming tag
  is not already concrete
- prove the route on the EASTL pair/member failure without expanding EASTL
  suite scope in the same slice

## Non-Goals

- replacing the repo's local emitted naming with a standard ABI mangler
- broad symbol-table redesign beyond the shared template-instance seam
- unrelated EASTL test-suite growth in the same packet

## Working Model

- keep specialization selection in the existing HIR template machinery
- treat emitted mangled names as the final naming surface, not the semantic
  identity used for lookup
- let later lookup request a concrete struct instance first, then perform field
  resolution against `struct_defs`
- prefer one shared helper over new pair-specific or member-specific fallback
  code

## Execution Rules

- reuse the existing partial-specialization selection path instead of cloning it
  in codegen
- keep the new helper structured around family/origin, selected pattern, and
  resolved bindings even if the immediate caller only needs the concrete tag
- avoid widening the patch into method/materialization cleanup unless that is
  required to make the shared resolver correct
- require `build -> narrow repro test` proof before accepting the slice
- escalate to a broader frontend subset if the helper touches common template
  infrastructure beyond the exposed repro path

## Step 1

### Goal

Extract a shared template-struct instance resolution helper from the current
partial-specialization realization path.

### Primary Targets

- [src/frontend/hir/hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [src/frontend/hir/hir_templates_materialization.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_materialization.cpp)
- [src/frontend/hir/hir_templates_struct_instantiation.cpp](/workspaces/c4c/src/frontend/hir/hir_templates_struct_instantiation.cpp)

### Actions

- identify the minimal reusable output needed by later lookup callers
- centralize pattern selection, binding resolution, and concrete instance-tag
  derivation behind one helper
- keep the helper authoritative for both primary-template and
  partial-specialized instantiation paths

### Completion Check

Late lookup callers can request concrete template-instance resolution without
re-implementing specialization logic.

## Step 2

### Goal

Integrate late member/symbol lookup with the shared template-instance resolver.

### Primary Targets

- [src/codegen/lir/stmt_emitter_lvalue.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_lvalue.cpp)
- [src/codegen/lir/stmt_emitter_types.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_types.cpp)

### Actions

- detect when incoming struct tags are not yet concrete enough for
  `struct_defs[tag]` lookup
- resolve those tags through the shared template-instance helper before field
  lookup failure
- keep field lookup generic and semantic; do not add pair-only aliases

### Completion Check

Member lookup resolves against the concrete instance layout even when the
incoming helper path still carries an intermediate template tag.

## Step 3

### Goal

Prove the shared-resolution route on the exposed EASTL pair/member failure.

### Primary Targets

- `/tmp/eastl_pair_member.cpp` or an equivalent focused repro
- `tests/cpp/external/eastl/utility/frontend_basic.cpp` only if the compiler
  fix makes the intended utility-surface proof honest again

### Actions

- keep one focused build/repro command that previously failed on
  `pair_T1_T1_T2_T2`
- confirm the repaired route reaches concrete `pair<int, int>` member layout
  lookup
- decide whether acceptance also needs the narrow EASTL external-suite test
  rerun in addition to the direct repro

### Completion Check

The exposed pair/member route compiles successfully through the repaired shared
lookup path.
