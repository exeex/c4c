# HIR Enum Const-Int Dual Lookup Runbook

Status: Active
Source Idea: ideas/open/101_hir_enum_const_int_dual_lookup.md

## Purpose

Move HIR enum constant and const-int binding lookup toward structured identity while preserving rendered-name maps as compatibility, diagnostic, and parity-check paths.

## Goal

Add structured mirrors for enum constants and const-int value bindings where source identity is available, then dual-write and dual-read without changing constant folding, consteval, switch/case, template value argument, or diagnostic behavior.

## Core Rule

Structured identity is a semantic lookup mirror. Do not remove legacy string maps, change constant-folding semantics, change emitted names or diagnostics, or add testcase-shaped shortcuts.

## Read First

- `ideas/open/101_hir_enum_const_int_dual_lookup.md`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/consteval.hpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`

## Current Targets

- Lowerer enum constant registration into `enum_consts_`
- `CompileTimeState::register_enum_const`
- Lowerer const-int binding registration into `const_int_bindings_`
- `CompileTimeState::register_const_int_binding`
- Constant-expression lookup paths such as `Lowerer::eval_const_int_with_nttp_bindings`
- Consteval environment handoff through `ConstEvalEnv`
- Switch/case, enum, global const-int, and template value argument paths that depend on these value bindings

## Non-Goals

- Do not remove legacy string maps.
- Do not migrate template or consteval definition registries already covered by idea 100.
- Do not migrate struct layout, `TypeSpec::tag`, `Module::struct_defs`, method lookup, member lookup, or static-member lookup.
- Do not change constant folding semantics.
- Do not rewrite parser or sema unless inventory proves a missing metadata handoff blocks the structured mirror.
- Do not downgrade expectations or introduce named-case shortcuts.

## Working Model

- Keep rendered strings as the compatibility, diagnostic, and debug-output contract.
- Introduce HIR-owned structured keys only where registration and lookup paths have enough source identity.
- Prefer namespace context, qualifier segment `TextId`s, unqualified `TextId`, member owner identity for scoped constants, and stable declaration identity as a temporary bridge when durable owner identity is incomplete.
- Dual-write structured maps beside legacy rendered-name maps.
- Dual-read structured identity when complete, then fall back to legacy rendered lookup.
- Treat missing metadata as compatibility fallback, not as permission to remove behavior.

## Execution Rules

- Keep each step behavior-preserving.
- Inventory both registration and lookup metadata before adding a structured path.
- Preserve string spellings for diagnostics, debug output, and fallback.
- Add parity or mismatch observation only where it helps prove behavior without changing the selected semantic result.
- Build after code-changing steps. Run a focused HIR constant/consteval/template value proof before claiming a step complete.
- Escalate to broader HIR or full CTest validation after multiple value-binding surfaces are changed.

## Ordered Steps

### Step 1: Inventory enum and const-int value-binding surfaces

Goal: identify every enum constant and const-int binding registration, lookup, and consteval/template handoff that participates in this migration.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/consteval.hpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`

Actions:
- Inspect all direct uses of `enum_consts_`, `const_int_bindings_`, `register_enum_const`, and `register_const_int_binding`.
- Classify each string use as semantic lookup, diagnostic/debug spelling, legacy compatibility fallback, parity observation, or bridge-required.
- Identify which enum and const-int call sites can provide structured source identity now.
- Identify lookup paths that are string-only because metadata is not yet handed through.
- Record the first implementation packet boundary and narrow proof command in `todo.md`; do not edit the source idea for routine inventory results.

Completion check:
- `todo.md` names the owned first implementation surface, fallback policy, metadata gaps, and exact narrow proof command the executor should run next.

### Step 2: Define structured value-binding keys

Goal: add HIR-owned key types for source-identifiable enum constants and const-int bindings.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/impl/lowerer.hpp`

Actions:
- Define separate or tagged structured key forms for enum constants and const-int bindings.
- Include source identity components needed to avoid collisions across namespaces, qualifiers, owner types, and declarations.
- Use declaration pointer or declaration ID only as a bridge when durable source identity is incomplete.
- Keep rendered names stored and accessible for diagnostics, dumps, and compatibility.

Completion check:
- The code builds with key types present and no lookup behavior changes.

### Step 3: Dual-write registrations

Goal: populate structured mirrors beside existing rendered-name maps.

Primary targets:
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_types.cpp`

Actions:
- Extend enum constant registration paths so callers with source identity can dual-write a structured mirror and the legacy string map.
- Extend const-int binding registration paths so callers with source identity can dual-write a structured mirror and the legacy string map.
- Preserve existing string-only registration APIs as compatibility paths.
- Keep member-scoped or owner-scoped constants on legacy fallback until owner identity is real enough for a structured key.

Completion check:
- Existing enum, global const-int, consteval, and constant-expression tests still pass, and legacy maps remain populated.

### Step 4: Dual-read semantic lookup paths

Goal: prefer structured identity for semantic value lookup, then fall back to rendered-name lookup.

Primary targets:
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/consteval.hpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`

Actions:
- Add lookup helpers or overloads that accept structured value-binding identity.
- Update call sites that already have complete identity metadata to use the structured lookup path.
- Keep string-only call sites on the legacy path until metadata handoff is real.
- Observe mismatches between structured and legacy hits without changing the chosen semantic result.

Completion check:
- Structured-capable lookups use the mirror first; missing metadata and mismatches still preserve previous behavior through fallback.

### Step 5: Consteval, switch/case, and template value proof

Goal: prove behavior remains stable across the value-binding consumers named by the source idea.

Primary targets:
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/consteval.hpp`
- HIR enum, consteval, constant-expression, switch/case, and template value tests under `tests/cpp/internal`

Actions:
- Verify consteval environment handoff still sees the same enum constants and const-int bindings.
- Verify switch/case and constant-expression lowering still resolve the same values.
- Verify template value argument paths keep existing behavior with structured mirrors enabled.
- Add or adjust focused tests only to prove structured mirror behavior and fallback behavior; do not weaken expectations.

Completion check:
- Focused HIR enum, const-int, consteval, switch/case, and template value proof passes without expectation downgrades or named-case shortcuts.

### Step 6: Broader validation and handoff

Goal: decide whether the source idea is complete or whether remaining value-binding work needs a follow-up runbook.

Actions:
- Run the supervisor-selected broader validation checkpoint.
- Summarize implemented mirrors, remaining legacy-only paths, fallback observations, and metadata blockers in `todo.md`.
- If a separate initiative is discovered, record it under `ideas/open/` through lifecycle rules instead of expanding this plan.

Completion check:
- The supervisor has enough proof to accept the slice, request review, or ask the plan owner to close or replan the active lifecycle state.
