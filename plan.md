# Template Arg Transport De-Stringify Runbook

Status: Active
Source Idea: ideas/open/51_template_arg_transport_de_stringify_refactor.md

## Purpose

Replace string-carried template-arg transport with structured semantic transport
 across parser and HIR so deferred template work stops depending on mangled text
 decode.

## Goal

Refactor template arg transport until `TypeSpec::tpl_struct_arg_refs` is fully
 removed and no semantic path depends on string-prefix decoding for template arg
 meaning.

## Core Rule

The old `tpl_struct_arg_refs` field is not allowed to survive as a compatibility
 escape hatch. This plan is only complete when the field is deleted from
 `TypeSpec`, all readers/writers are migrated, and any remaining string output
 is derived debug/identity text rather than the primary semantic payload.

## Read First

- [ideas/open/51_template_arg_transport_de_stringify_refactor.md](/workspaces/c4c/ideas/open/51_template_arg_transport_de_stringify_refactor.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)
- [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- [parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
- [parser_types_template.cpp](/workspaces/c4c/src/frontend/parser/parser_types_template.cpp)
- [types_helpers.hpp](/workspaces/c4c/src/frontend/parser/types_helpers.hpp)
- [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

## Current Targets

- `TypeSpec::{tpl_struct_origin, deferred_member_type_name}` transport rules
- replacement for `TypeSpec::tpl_struct_arg_refs`
- parser substitution/rebuild flows that currently pass through `new_arg_parts`
- deferred NTTP evaluation that reparses mangled type strings
- HIR materialization and pending-type identity code that currently encodes
  template args into text

## Non-Goals

- do not redesign template specialization selection beyond what is needed to
  migrate arg transport
- do not rewrite all mangled-name identity schemes in one slice
- do not broaden into unrelated EASTL bug fixing except where the bug directly
  blocks this migration
- do not keep `tpl_struct_arg_refs` as a deprecated compatibility field

## Working Model

The target shape is:

- `TypeSpec` carries structured template args directly
- each template arg explicitly records whether it is a type arg or value arg
- type args retain full `TypeSpec` information, including enum/struct/typedef
  and cv/ref state
- any debug or cache-key text is derived from structured args, not the other
  way around

Illustrative direction:

```cpp
enum class TemplateArgKind : uint8_t {
  Type,
  Value,
};

struct TemplateArgRef {
  TemplateArgKind kind = TemplateArgKind::Type;
  TypeSpec type{};
  long long value = 0;
  const char* debug_text = nullptr;
};

struct TemplateArgRefList {
  TemplateArgRef* data = nullptr;
  int size = 0;
};

struct TypeSpec {
  TypeBase base;
  const char* tag;
  const char* tpl_struct_origin;
  TemplateArgRefList tpl_struct_args;
  const char* deferred_member_type_name;
};
```

## Execution Rules

- Prefer small, test-backed slices. Each slice should remove one class of
  string transport or one major reader/writer family.
- Every slice must move code toward deleting `tpl_struct_arg_refs`, not merely
  wrapping it.
- New code must branch on structured enum/fields, not on string prefixes like
  `enum_`, `struct_`, or `union_`.
- If a temporary encoder/decoder remains necessary, centralize it and document
  exactly which compatibility callers still need it.
- Keep the source idea authoritative for scope. If the work expands into a
  broader canonical-type or specialization-identity redesign, split a new idea.

## Ordered Steps

### Step 1: Audit And Pin The Existing String Transport Surface

Goal:
- Identify every writer, reader, and identity encoder that still depends on
  `tpl_struct_arg_refs` or equivalent text-carried template arg reconstruction.

Primary targets:
- [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- [parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
- [parser_types_template.cpp](/workspaces/c4c/src/frontend/parser/parser_types_template.cpp)
- [types_helpers.hpp](/workspaces/c4c/src/frontend/parser/types_helpers.hpp)
- [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

Actions:
- enumerate all `tpl_struct_arg_refs` read/write sites
- classify each site as construction, transport, identity, debug formatting, or
  semantic decode
- add focused regression tests that pin current behavior for:
  `__is_enum(ns::Color)`-style deferred traits, inherited
  `integral_constant<bool, trait(T)>`, and any existing alias/member-typedef
  cases that still depend on this transport

Completion check:
- the repo has an explicit migration map for every `tpl_struct_arg_refs`
  dependency
- focused regressions exist for the currently known pressure cases

### Step 2: Add Structured Template Arg Storage To `TypeSpec`

Goal:
- Introduce the new structured arg container on `TypeSpec` and related parser
  helper types without deleting the old field yet.

Primary targets:
- [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

Actions:
- add `TemplateArgKind`, `TemplateArgRef`, and `TemplateArgRefList` or an
  equivalent structure
- define initialization/copy/reset rules for the new fields
- document which fields remain semantic transport versus debug-only text

Completion check:
- the tree builds with both old and new transport fields present
- new structured arg fields are available on `TypeSpec` and parser/HIR helper
  types

### Step 3: Populate Structured Args At Parser Construction Sites

Goal:
- Ensure parser-side template type construction fills structured args at the
  earliest practical moment instead of only writing `tpl_struct_arg_refs`.

Primary targets:
- [parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
- [parser_declarations.cpp](/workspaces/c4c/src/frontend/parser/parser_declarations.cpp)
- [types_helpers.hpp](/workspaces/c4c/src/frontend/parser/types_helpers.hpp)

Actions:
- update template arg parsing/build helpers to produce typed arg lists
- update alias-template substitution and base-template reconstruction so they
  preserve structured args
- keep any necessary string rendering derived from typed args rather than as the
  source of truth

Completion check:
- newly built `TypeSpec` template instances carry populated structured args
- parser transport no longer needs ad hoc string decode for the migrated paths

### Step 4: Switch Deferred NTTP Evaluation To Structured Args

Goal:
- Remove semantic dependence on reparsing mangled arg text inside deferred NTTP
  evaluation.

Primary targets:
- [parser_types_template.cpp](/workspaces/c4c/src/frontend/parser/parser_types_template.cpp)

Actions:
- feed structured type/value args into deferred builtin trait evaluation
- delete string-prefix semantic recovery that becomes redundant after typed arg
  transport exists
- keep only narrow text parsing for true source-text expressions, not internal
  template arg transport

Completion check:
- deferred trait/value evaluation consumes structured args in the common path
- qualified enum and alias-backed regressions pass without prefix-based type
  recovery glue

### Step 5: Switch HIR Materialization And Pending-Type Identity

Goal:
- Move HIR template arg materialization and pending-type identity off
  `tpl_struct_arg_refs`.

Primary targets:
- [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- related compile-time helper files touched by the migration

Actions:
- update HIR template arg materialization to read structured args directly
- update deferred member-type resolution to carry structured owner args
- replace pending-type key generation that currently serializes
  `tpl_struct_arg_refs` with a structured or centrally derived identity path

Completion check:
- HIR materialization no longer requires `tpl_struct_arg_refs` as semantic input
- pending/deferred type identity no longer reads the legacy field directly

### Step 6: Delete `tpl_struct_arg_refs` And Remove Compatibility Decode

Goal:
- Remove the old field and any remaining direct semantic dependence on
  string-carried template arg meaning.

Primary targets:
- all remaining `tpl_struct_arg_refs` references under `src/frontend/`

Actions:
- delete `TypeSpec::tpl_struct_arg_refs`
- remove all remaining readers and writers
- reduce any remaining string production to debug/diagnostic or explicit
  identity-rendering helpers only
- clean up helper comments and invariants so the new architecture is the only
  supported one

Completion check:
- the symbol `tpl_struct_arg_refs` no longer exists in the source tree
- `rg -n "tpl_struct_arg_refs" src/frontend` returns no matches
- the repo builds and targeted regressions still pass

### Step 7: Broad Validation And Closeout

Goal:
- Prove the new transport architecture preserves or improves behavior without
  regressing the suite.

Actions:
- run the focused transport/trait regressions
- run `eastl_type_traits_simple_workflow`
- run a full `ctest --test-dir build -j8 --output-on-failure`
- compare pass/fail state against the pre-refactor baseline
- update the source idea with completion notes only when the field-removal
  criterion is satisfied

Completion check:
- no new failing tests
- `eastl_type_traits_simple_workflow` is at least as healthy as before the
  refactor, ideally improved
- full-suite state is monotonic
- `tpl_struct_arg_refs` is fully gone, which is required for acceptance

## Acceptance

This plan is not complete until all of the following are true:

- `TypeSpec::tpl_struct_arg_refs` has been deleted
- no parser, HIR, or compile-time semantic path depends on string-prefix
  matching to recover template arg meaning
- structured template arg transport is the only supported semantic transport
  path
- targeted deferred-trait regressions pass
- broad validation remains monotonic
