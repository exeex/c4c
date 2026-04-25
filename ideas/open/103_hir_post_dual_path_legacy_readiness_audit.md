# HIR Post Dual-Path Legacy Readiness Audit

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [99_hir_module_symbol_structured_lookup_mirror.md](/workspaces/c4c/ideas/closed/99_hir_module_symbol_structured_lookup_mirror.md)
- [100_hir_compile_time_template_consteval_dual_lookup.md](/workspaces/c4c/ideas/closed/100_hir_compile_time_template_consteval_dual_lookup.md)
- [101_hir_enum_const_int_dual_lookup.md](/workspaces/c4c/ideas/closed/101_hir_enum_const_int_dual_lookup.md)
- [102_hir_struct_method_member_identity_dual_lookup.md](/workspaces/c4c/ideas/closed/102_hir_struct_method_member_identity_dual_lookup.md)

## Goal

Audit the HIR structured-identity migration after ideas 99 through 102 and
decide which legacy rendered-name lookup paths are ready to demote, which must
remain as compatibility bridges, and which require downstream HIR-to-LIR or
codegen spelling work before deletion.

This idea is a classification checkpoint. It should not perform broad cleanup
itself.

## Why This Idea Exists

Ideas 99 through 102 intentionally introduced dual-write / dual-read structured
mirrors before deleting legacy rendered-name lookup. That made baseline drift
observable, but it also left many compatibility paths in place.

Before removing legacy APIs, the project needs a fresh audit that separates:

- structured lookup paths that are now authoritative enough to use directly
- legacy rendered lookup paths retained only as parity proof
- string-only APIs that still lack durable structured metadata
- diagnostic, dump, ABI, link-name, or codegen spelling surfaces
- HIR-to-LIR seams that still pass semantic identity as strings

## Audit Scope

Inspect at least:

- module function/global lookup and remaining `find_*_by_name_legacy` callers
- compile-time template, template-struct, specialization, and consteval
  registries
- enum constant and const-int binding maps
- struct definition owner lookup and `Module::struct_defs`
- method, member, and static-member owner lookup helpers
- `TypeSpec::tag` semantic consumers
- HIR dump and diagnostic consumers
- HIR-to-LIR lowering seams under `src/codegen/lir/hir_to_lir/`
- shared codegen helpers that still consume HIR strings, especially struct
  tags, rendered type strings, raw extern names, and fallback symbol spellings

## Classification

Every remaining string or legacy path should be classified as one of:

- `safe-to-demote`: structured lookup is complete enough and parity evidence is
  stable.
- `legacy-proof-only`: retained only to observe mismatches during the migration.
- `bridge-required`: still required because downstream data lacks structured
  identity.
- `diagnostic-only`: user-facing or debug text only.
- `printer-only`: text that should exist only at final textual emission.
- `abi-link-spelling`: rendered spelling required for ABI or linker-visible
  names.
- `blocked-by-hir-to-lir`: HIR has structured identity, but LIR or codegen still
  accepts only rendered strings.
- `needs-more-parity-proof`: structured mirror exists but has insufficient
  validation to demote.

## Required Output

Write a review artifact under `review/`, for example:

```text
review/103_hir_post_dual_path_legacy_readiness_audit.md
```

The report should include:

- table of remaining legacy HIR lookup APIs and call sites
- classification for each string-keyed map or fallback
- explicit HIR-to-LIR seam inventory for string identity handoff
- recommendation for idea 104 safe cleanup scope
- recommendation for idea 105 downstream bridge scope
- proof gaps and suggested focused validation subsets

## Acceptance Criteria

- HIR legacy lookup and string identity surfaces are classified with enough
  precision to avoid accidental baseline drift.
- HIR-to-LIR seams that still pass semantic identity as strings are explicitly
  listed.
- Safe demotion candidates are separated from bridge-required spelling.
- No implementation cleanup is performed as part of this audit.
- Follow-up work can be split cleanly into HIR-internal cleanup and downstream
  bridge cleanup.
