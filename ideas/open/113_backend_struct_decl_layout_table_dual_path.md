# Backend StructDecl Layout Table Dual Path

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [112_lir_backend_legacy_type_surface_readiness_audit.md](/workspaces/c4c/ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md)

## Goal

Introduce a BIR-facing structured aggregate layout table derived from
`LirModule::struct_decls`, while retaining legacy `module.type_decls` parsing
as a dual-path fallback and parity source.

## Why This Idea Exists

`LirModule::type_decls` is no longer printer authority for LLVM struct
declarations after idea 111, but backend layout consumers still depend on it.
The largest active blocker to demoting `type_decls` is BIR aggregate layout
parsing.

This idea should make backend layout consume structured `StructNameId` /
`LirStructDecl` data where available, but preserve legacy text parsing until
parity is proven across aggregate layout paths.

`src/backend/mir/` is intentionally out of scope because that subsystem is
planned for a full rewrite. MIR must not block this route. If MIR `.cpp` files
block compilation after LIR/BIR cleanup, remove those files from the current
compile target instead of migrating MIR internals.

## Scope

Use the idea 112 audit as the source of truth. Candidate areas include:

- BIR aggregate addressing and memory layout helpers
- helpers that resolve type declarations from `module.type_decls`
- aggregate size/alignment calculation
- HFA/direct-GP/sret aggregate classification
- global initializer and aggregate store/load lowering where backend-owned
  layout is required

Out of scope:

- `src/backend/mir/` migration or cleanup.
- Reworking MIR/aarch64 aggregate layout helpers before the planned rewrite.
- Preserving MIR `.cpp` files in the compile target when they are the only
  blocker for this route.

## Dual-Track Rules

- Do not remove `module.type_decls`.
- Build structured backend layout facts from `module.struct_decls`.
- Prefer structured layout only where legacy parity can be checked.
- Record mismatches between structured and legacy layout.
- Keep all emitted backend output stable.
- If a backend path still accepts only type text, keep that path as fallback
  and record it for later cleanup.
- If the only failing consumer is under `src/backend/mir/`, treat it as a
  compile-target exclusion problem, not a structured-layout blocker.

## Acceptance Criteria

- BIR backend layout code has a structured layout table path sourced from
  `LirStructDecl`.
- Legacy `type_decls` parsing remains available and behavior-preserving.
- Structured-vs-legacy layout parity is checked for the touched backend paths.
- Focused aggregate, HFA, sret, variadic, global-init, and memory-addressing
  proof passes without output drift.
