# HIR Compile-Time Template Consteval Dual Lookup Runbook

Status: Active
Source Idea: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md

## Purpose

Move the HIR compile-time, template, and consteval definition registries toward structured identity while preserving rendered-name maps as compatibility, diagnostic, and parity-check paths.

## Goal

Add structured mirrors for compile-time definition lookup where enough metadata exists, then dual-write and dual-read without changing emitted names or current behavior.

## Core Rule

Structured identity is a semantic lookup mirror. Do not remove legacy string maps, change diagnostics, change link/codegen spellings, or add testcase-shaped shortcuts.

## Read First

- `ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/impl/templates/templates.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

## Current Targets

- `CompileTimeState::register_template_def`
- `CompileTimeState::register_template_struct_def`
- `CompileTimeState::register_consteval_def`
- `CompileTimeState::has_template_def`
- `CompileTimeState::find_template_def`
- `CompileTimeState::find_consteval_def`
- HIR lowerer registration paths feeding those APIs
- Template materialization paths that can already carry source identity

## Non-Goals

- Do not remove legacy string maps.
- Do not change emitted names, link names, mangled names, HIR dump spellings, or diagnostics.
- Do not migrate `TypeSpec::tag`, `Module::struct_defs`, struct layout, or codegen type names.
- Do not migrate method, member, static-member, enum constant, or const-int lookup except for read-only parity observation that is required to validate this idea.
- Do not rewrite parser or sema.

## Working Model

- Keep rendered names as the compatibility and output-facing contract.
- Introduce HIR-owned structured keys only where the registration or lookup path has enough source identity.
- Prefer key components from namespace context, global qualification, qualifier segment `TextId`s, unqualified declaration `TextId`, declaration kind, and stable declaration identity when no better bridge exists yet.
- Dual-write new structured maps beside legacy maps.
- Dual-read through structured identity when complete, then fall back to legacy rendered lookup.
- Track mismatch or fallback observations without changing behavior.

## Execution Rules

- Keep each step behavior-preserving.
- Use existing structured-identity helpers and conventions before adding new vocabulary.
- Add a structured path only after identifying both registration and lookup metadata for that surface.
- Preserve missing-metadata fallback as normal compatibility behavior.
- Build after code-changing steps. Run the narrow HIR template/consteval proof before claiming a step complete.
- Escalate to a broader HIR or full CTest checkpoint after multiple registry surfaces are changed.

## Ordered Steps

### Step 1: Inventory compile-time registry surfaces

Goal: identify every compile-time, template, and consteval definition registration and lookup that participates in this migration.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/templates/*.cpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`

Actions:
- Inspect all calls to the `CompileTimeState` registration and lookup APIs named in this runbook.
- Classify each use as semantic lookup, diagnostic/output spelling, compatibility fallback, or parity observation.
- Identify which call sites can provide structured source identity now and which require a temporary declaration-pointer bridge.
- Record the first implementation packet boundary in `todo.md`; do not edit the source idea for routine inventory results.

Completion check:
- `todo.md` names the owned first implementation surface, the fallback policy, and the exact narrow proof command the executor should run next.

### Step 2: Define structured registry keys

Goal: add HIR-owned key types for source-identifiable compile-time registry entries.

Primary target:
- `src/frontend/hir/compile_time_engine.hpp`

Actions:
- Define key types for template function, template struct primary, template struct specialization owner, and consteval function definitions only where metadata supports them.
- Include declaration kind and source identity components needed to avoid collisions.
- Keep rendered names stored and accessible for diagnostics, dumps, and compatibility.
- Avoid coupling the new keys to codegen-facing names.

Completion check:
- The code builds with key types present and no lookup behavior changes.

### Step 3: Dual-write definition registrations

Goal: populate structured mirrors beside existing rendered-name maps.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/templates/templates.cpp`

Actions:
- Extend registration APIs or add overloads so callers with source identity can dual-write structured mirrors.
- Preserve the existing string registration APIs as compatibility paths.
- Add mismatch or fallback counters only where they help prove parity without affecting behavior.
- Keep template struct specialization registration aligned with primary-template identity.

Completion check:
- Existing HIR template and consteval tests still pass, and legacy maps remain populated.

### Step 4: Dual-read lookup paths

Goal: prefer structured identity for semantic lookups, then fall back to rendered-name lookup.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`

Actions:
- Add lookup overloads or helper entry points that accept structured identity.
- Update call sites that already have complete identity metadata to use the structured lookup path.
- Keep string-only call sites on the legacy path until their metadata handoff is real.
- Observe mismatches between structured and legacy hits without changing the chosen semantic result.

Completion check:
- Structured-capable lookups use the mirror first; missing metadata and mismatches still preserve previous behavior through fallback.

### Step 5: Materialization and parity proof

Goal: prove template materialization and consteval behavior remain stable with the dual lookup mirrors enabled.

Primary targets:
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- HIR template and consteval CTest entries under `tests/cpp/internal`

Actions:
- Verify `materialize_hir_template_defs` and deferred compile-time paths still see the same definitions.
- Add or adjust focused tests only to cover structured mirror behavior and fallback behavior; do not weaken expectations.
- Run a narrow HIR template/consteval CTest subset.

Completion check:
- Focused HIR template and consteval proof passes without expectation downgrades or named-case shortcuts.

### Step 6: Broader validation and handoff

Goal: decide whether the source idea is complete or whether remaining HIR compile-time registry work needs a follow-up runbook.

Actions:
- Run the supervisor-selected broader validation checkpoint.
- Summarize implemented mirrors, remaining legacy-only paths, fallback observations, and any deferred metadata blockers in `todo.md`.
- If a separate initiative is discovered, record it under `ideas/open/` through lifecycle rules instead of expanding this plan.

Completion check:
- The supervisor has enough proof to accept the slice, request review, or ask the plan owner to close or replan the active lifecycle state.
