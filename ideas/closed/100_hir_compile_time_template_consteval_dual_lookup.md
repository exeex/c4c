# HIR Compile-Time Template Consteval Dual Lookup

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [99_hir_module_symbol_structured_lookup_mirror.md](/workspaces/c4c/ideas/closed/99_hir_module_symbol_structured_lookup_mirror.md)

## Goal

Extend the HIR structured-identity migration from module function/global lookup
into the compile-time, template, and consteval definition registries.

The first implementation should add structured mirrors beside the existing
rendered-name maps, dual-write registrations, dual-read queries, and preserve
legacy string lookup as a compatibility and parity-check path.

## Why This Idea Exists

Idea 99 proved the first HIR module function/global structured lookup mirror,
but deliberately left broader HIR string-keyed registries out of scope. The
next narrow HIR-owned surface is the compile-time engine registry family:

- template function definitions
- template struct definitions and specializations
- consteval function definitions
- compile-time definition lookup helpers used during HIR lowering

These registries are still keyed primarily by rendered strings in
`CompileTimeState` and related lowerer paths. They are semantic lookup surfaces,
not merely diagnostics or codegen spellings, so they should gain structured
identity mirrors before any legacy path is removed.

## Scope

Audit and update the HIR compile-time registry paths around:

- `CompileTimeState::register_template_def`
- `CompileTimeState::register_template_struct_def`
- `CompileTimeState::register_consteval_def`
- `CompileTimeState::has_template_def`
- `CompileTimeState::find_template_def`
- `CompileTimeState::find_consteval_def`
- lowerer registration paths that feed those APIs
- template materialization paths that can already carry source identity

The implementation should introduce HIR-owned structured keys where enough
metadata exists. Candidate components include:

- declaration kind
- namespace context ID
- global-qualified bit
- qualifier segment text IDs
- unqualified declaration `TextId`
- stable AST declaration pointer or declaration ID where no better identity
  exists yet

Rendered names, mangled names, diagnostics, and link/codegen-facing spellings
must remain preserved.

## Out Of Scope

- Removing legacy string maps.
- Changing emitted names, link names, or diagnostics.
- Migrating `TypeSpec::tag`, `Module::struct_defs`, struct layout, or codegen
  type names.
- Migrating method/member/static-member owner lookup.
- Migrating enum constants or const-int value bindings except where a narrow
  compile-time registry proof needs read-only parity observation.
- Parser or sema rewrites.

## Suggested Execution Decomposition

1. Inventory all compile-time, template, and consteval definition registry
   registrations and lookups.
2. Define structured key types for the registry surfaces that have enough HIR
   or AST metadata.
3. Add structured mirrors beside the existing rendered-name maps.
4. Dual-write registration paths.
5. Dual-read lookup paths with legacy fallback and mismatch observation.
6. Preserve existing behavior when structured metadata is missing.
7. Add focused proof for template and consteval behavior without expectation
   downgrades.

## Acceptance Criteria

- Compile-time template and consteval definition registries have structured
  mirrors beside rendered-name maps where metadata is available.
- Registrations dual-write structured and legacy maps.
- Lookups prefer structured identity when complete, then fall back to legacy
  rendered lookup without changing behavior.
- Legacy rendered maps remain available for diagnostics, compatibility, and
  parity proof.
- Focused HIR compile-time/template/consteval tests pass without weakening
  expectations or adding testcase-shaped shortcuts.

## Closure Note

Closed on 2026-04-25 after the active runbook completed Step 6 broader
validation and handoff. The implementation added structured mirrors and
dual-read/dual-write coverage for the source idea's compile-time template,
template struct, specialization-owner, and consteval registry scope while
preserving legacy rendered-name fallback paths. Close-time regression guard
used matching `test_before.log` and `test_after.log` for the broader HIR
template/consteval subset, both with 174 passed and 0 failed tests, and passed
with non-decreasing pass-count policy for this behavior-preserving migration.
