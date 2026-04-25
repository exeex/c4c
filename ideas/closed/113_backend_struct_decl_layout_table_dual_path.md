# Backend StructDecl Layout Table Dual Path

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

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

## Closure Notes

Idea 113 is closed after the Step 6 broader parity and regression checkpoint.
The active runbook converted the structured layout table and the Step 3-5
backend consumers to prefer structured layout where module state can provide
it, while keeping legacy `type_decls` parsing available as fallback and parity
source.

The final checkpoint recorded a fresh default build, full `ctest`, and backend
target rebuild:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Recorded result: default build completed, full ctest passed 2980/2980, and
the backend `c4c_backend` target rebuilt successfully. The close-time
regression guard was run read-only using the recorded full-suite baseline
summary and latest full-suite proof with non-decreasing pass semantics:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_before.log --allow-non-decreasing-passed`

Guard result: PASS, with 2980 passed before and after, 0 failed before and
after, and no new failing or suspicious-timeout tests.

Remaining fallback-only backend paths are intentional compatibility surfaces:
legacy public overloads for `resolve_global_gep_address()`,
`resolve_relative_global_gep_address()`,
`resolve_global_dynamic_pointer_array_access()`, and
`resolve_global_dynamic_aggregate_array_access()` still route through
null-table wrappers for callers without module structured layout state. Core
parity and legacy layout functions also remain fallback-only by design where
they serve as the compatibility path under structured lookup miss or absent
table state.
