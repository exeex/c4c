# Codegen LIR Aggregate Type Identity Metadata Runbook

Status: Active
Source Idea: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md

## Purpose

Execute the downstream carrier split exposed by the `TypeSpec::tag` deletion
probe after frontend/parser/Sema/HIR-owned residuals were cleared.

## Goal

Give codegen and LIR aggregate type routes structured identity metadata so
`TypeSpec::tag` can be removed without losing record, union, layout, call,
`va_arg`, or ABI behavior.

## Core Rule

Do not replace `TypeSpec::tag` with another rendered-string semantic key.
Rendered spelling may remain only as final LLVM text, diagnostics, dumps,
mangling, ABI/link-visible text, or explicitly named no-metadata compatibility.

## Read First

- `ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md`
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/lir/hir_to_lir/lvalue.cpp`
- `src/codegen/llvm/calling_convention.cpp`

## Current Targets

- Codegen-owned `TypeSpec::tag` consumers and producers under `src/codegen`.
- LIR aggregate type refs, struct-name interning, structured layout lookup,
  field-chain/lvalue lowering, call argument/return type refs, `va_arg`,
  const-init aggregate handling, and ABI classification.
- The deletion-probe first boundary at
  `src/codegen/shared/llvm_helpers.hpp:444` and related LIR/codegen `.tag`
  residuals.

## Non-Goals

- Reopening parser/Sema/HIR migration unless a codegen route proves a missing
  upstream producer carrier is still required.
- Removing final emitted LLVM names, diagnostics, dumps, mangling, ABI/link
  spelling, or other output text merely because it is string-shaped.
- Replacing `TypeSpec::tag` with a renamed rendered-string semantic field.
- Broad backend rewrites unrelated to aggregate type identity carriers.
- Weakening tests, marking supported cases unsupported, or adding named-case
  shortcuts.

## Working Model

- Classify codegen `.tag` uses before deleting the field.
- Preserve final LLVM spelling separately from semantic identity.
- Prefer existing codegen carriers such as `StructNameId`, `LirTypeRef`,
  structured layout metadata, and LIR-module-owned aggregate refs before adding
  new carriers.
- If a residual proves a distinct BIR/backend carrier gap outside LIR/codegen
  aggregate identity, create another follow-up idea instead of expanding this
  runbook.
- Use temporary `TypeSpec::tag` deletion only as a probe; do not commit a
  broken deletion build.

## Execution Rules

- Keep implementation packets small and buildable.
- Update `todo.md` after each executor packet with the migrated family, proof
  command, and deletion-probe residual boundary.
- For code-changing packets, run `cmake --build --preset default` plus the
  supervisor-selected focused codegen/LIR subset.
- At the field-removal checkpoint, rerun the deletion probe and classify any
  new first boundary before continuing.

## Step 1: Inventory Codegen Tag Surfaces

Goal: classify every codegen-owned `TypeSpec::tag` read/write before behavior
changes.

Primary Target: `src/codegen` `.tag` users.

Actions:
- Search `src/codegen` for `.tag`, `TypeSpec::tag`, `StructNameId`,
  `LirTypeRef`, structured layout lookup, and aggregate helper routes.
- Classify each use as semantic aggregate identity, layout lookup, call/ABI
  type ref, field/member access, final LLVM spelling, diagnostic text, or
  compatibility.
- Identify existing carriers that can replace semantic `tag` reads and the
  first concrete migration target.

Completion Check:
- `todo.md` records the classified surface map and first executable migration
  target.
- No field deletion or broad behavior change is required in this step.

## Step 2: Establish Aggregate Identity Carrier Contract

Goal: define the codegen/LIR aggregate identity carrier used by migrated
consumers.

Primary Target: LIR aggregate refs, struct-name interning, and structured
layout lookup helpers.

Actions:
- Prefer existing `StructNameId`, `LirTypeRef`, LIR module intern tables, and
  structured layout metadata where they already carry identity.
- Add only narrow carrier fields or helper APIs needed to avoid semantic
  lookup through rendered `TypeSpec::tag`.
- Keep final LLVM type spelling generation separate from semantic lookup.

Completion Check:
- At least one aggregate identity path can be represented without reading
  `TypeSpec::tag`.
- The carrier contract distinguishes semantic identity from final spelling.

## Step 3: Migrate Layout And Field Access Consumers

Goal: stop layout and field-chain lowering from depending on rendered tags when
structured aggregate metadata is present.

Primary Target: `src/codegen/lir/hir_to_lir/types.cpp`,
`src/codegen/lir/hir_to_lir/lvalue.cpp`, `src/codegen/lir/hir_to_lir/core.cpp`,
and `src/codegen/shared/llvm_helpers.hpp`.

Actions:
- Migrate structured layout lookup, field-chain discovery, anonymous member
  descent, base-class descent, and LLVM aggregate type helper calls to the
  chosen carrier.
- Preserve final LLVM struct spelling only at output boundaries.
- Add or adjust focused LIR/codegen tests where structured metadata must win
  over rendered spelling.

Completion Check:
- Covered layout and field access routes no longer use `TypeSpec::tag` as
  semantic authority when structured metadata is available.
- `cmake --build --preset default` and the supervisor-selected focused subset
  pass.

## Step 4: Migrate Call, VaArg, Const Init, And ABI Consumers

Goal: move aggregate call/return refs, `va_arg`, const init, and ABI
classification off semantic `TypeSpec::tag` reads.

Primary Target: `src/codegen/lir/hir_to_lir/call/target.cpp`,
`src/codegen/lir/hir_to_lir/call/args.cpp`,
`src/codegen/lir/hir_to_lir/call/vaarg.cpp`,
`src/codegen/lir/hir_to_lir/const_init_emitter.cpp`, and
`src/codegen/llvm/calling_convention.cpp`.

Actions:
- Route aggregate call signatures and return type refs through structured LIR
  aggregate metadata.
- Route `va_arg`, const-init aggregate lookup, and ABI classification through
  structured carriers or explicit final-spelling payloads.
- Keep compatibility fallbacks narrow and named.

Completion Check:
- Covered call, `va_arg`, const-init, and ABI routes no longer use
  `TypeSpec::tag` as semantic authority when structured metadata is available.
- `cmake --build --preset default` and the supervisor-selected focused subset
  pass.

## Step 5: Probe TypeSpec Tag Removal Boundary

Goal: confirm codegen aggregate identity residuals are cleared or split.

Primary Target: temporary `TypeSpec::tag` deletion build.

Actions:
- Temporarily delete or disable `TypeSpec::tag` and run
  `cmake --build --preset default`.
- Record the first remaining failure boundary and classify it by ownership.
- Revert only the probe edit.
- If remaining failures are outside this idea's aggregate identity scope,
  create narrower follow-up ideas instead of expanding this runbook.

Completion Check:
- `todo.md` records the probe result and any follow-up ideas created.
- The worktree is returned to a buildable state after the probe.

## Step 6: Validate And Hand Back To Field Removal

Goal: make the downstream carrier split acceptance-ready and unblock the parent
field-removal route.

Primary Target: validation logs and lifecycle state.

Actions:
- Confirm retained codegen strings are classified as final LLVM spelling,
  diagnostics, dumps, mangling, ABI/link text, or explicit compatibility.
- Run supervisor-selected broader validation for the migrated codegen/LIR
  routes.
- Report whether parent idea
  `ideas/open/141_typespec_tag_field_removal_metadata_migration.md` can resume
  final `TypeSpec::tag` field removal.

Completion Check:
- `cmake --build --preset default` passes.
- Broader validation selected by the supervisor passes.
- Any remaining downstream blockers are represented as open ideas rather than
  hidden inside this runbook.
