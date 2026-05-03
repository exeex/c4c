# Codegen LIR Aggregate Type Identity Metadata

Status: Open
Created: 2026-05-03

Parent Ideas:
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`

## Goal

Give LIR, LLVM helper, and backend-facing aggregate type routes a structured
type identity carrier so `TypeSpec::tag` can be removed without losing record,
union, layout, call, va_arg, or ABI lookup behavior.

## Why This Idea Exists

Step 4 of the `TypeSpec::tag` removal runbook cleared the parser/Sema/HIR-owned
deletion-probe residuals. The next first deletion-probe boundary moved into
codegen at `src/codegen/shared/llvm_helpers.hpp:444`, with related `.tag` uses
across LIR lowering and LLVM/backend helper code.

The parent idea explicitly treats downstream LIR/BIR/backend carrier gaps as
separate follow-up work instead of silently expanding the frontend/HIR migration
or resurrecting rendered spelling as semantic authority on `TypeSpec`.

## In Scope

- Inventory codegen-owned `TypeSpec::tag` reads and writes under `src/codegen`
  and classify each as semantic aggregate identity, layout lookup, call/ABI
  type reference, final LLVM spelling, diagnostic text, or compatibility.
- Design or extend LIR aggregate type identity carriers such as
  `StructNameId`, `LirTypeRef`, structured layout lookup metadata, or explicit
  aggregate refs so codegen can resolve struct/union identity without reading
  `TypeSpec::tag`.
- Migrate aggregate LLVM type spelling helpers, field-chain/layout lookup,
  call argument/return type refs, `va_arg` aggregate handling, const init, and
  calling convention routes that currently depend on `TypeSpec::tag`.
- Keep final emitted LLVM spelling and ABI/link-visible names available where
  required, but carry them separately from semantic lookup authority.
- Re-run the `TypeSpec::tag` deletion probe and record the next boundary after
  codegen-owned aggregate identity sites are migrated.

## Out Of Scope

- Parser, Sema, and HIR semantic metadata migration already owned by the parent
  idea unless a codegen route proves a missing HIR producer carrier is still
  required.
- Broad backend rewrites unrelated to aggregate type identity.
- Replacing `TypeSpec::tag` with another rendered-string semantic field.
- Removing diagnostic, dump, final LLVM spelling, mangling, or ABI/link text
  merely because it is string-shaped.
- Weakening frontend/HIR/codegen tests, marking supported cases unsupported, or
  adding named-case shortcuts.

## Candidate Hot Regions

- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/lir/hir_to_lir/lvalue.cpp`
- `src/codegen/lir/hir_to_lir/call/target.cpp`
- `src/codegen/lir/hir_to_lir/call/args.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- `src/codegen/lir/hir_to_lir/core.cpp`
- `src/codegen/llvm/calling_convention.cpp`
- `src/codegen/lir/verify.cpp`

## Acceptance Criteria

- Codegen aggregate identity and layout lookup no longer require
  `TypeSpec::tag` when structured aggregate metadata is available.
- LIR call signatures, return type refs, field-chain/lvalue access, aggregate
  const init, `va_arg`, and ABI classification routes preserve behavior through
  structured aggregate refs or explicitly classified final spelling payloads.
- A `TypeSpec::tag` deletion probe no longer first fails in the codegen
  aggregate identity hot regions named above, or any remaining failures are
  classified into a narrower follow-up idea.
- `cmake --build --preset default` passes after normal, non-probe edits.
- Focused codegen/LIR tests prove aggregate identity still works when rendered
  spelling is not used as the semantic lookup authority.

## Reviewer Reject Signals

- A slice claims progress by introducing a renamed rendered-string field or
  helper that remains the semantic lookup authority for aggregate identity.
- A route keeps resolving struct/union semantics from final LLVM spelling after
  `StructNameId`, `LirTypeRef`, structured layout metadata, HIR declaration
  metadata, or another explicit carrier is available.
- The proof relies only on rewriting expectations, weakening supported tests,
  or marking codegen cases unsupported without explicit user approval.
- A change only handles the current deletion-probe line or one named testcase
  while nearby same-feature call, field-chain, layout, `va_arg`, or ABI routes
  still use the old failure mode.
- The implementation removes diagnostics, final emitted LLVM names, mangling,
  or ABI/link spelling and presents that as aggregate identity progress.
- Broad backend or LIR rewrites are mixed into this idea without proving they
  are necessary for aggregate type identity carrier migration.
