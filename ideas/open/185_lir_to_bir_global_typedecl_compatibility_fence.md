# LIR-to-BIR Global/TypeDecl Compatibility Fence

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md`
- `ideas/open/183_lir_bir_backend_freeze_authority_audit.md`

## Goal

Fence the remaining LIR-to-BIR `GlobalTypes`, `TypeDeclMap`, and structured
layout compatibility tables so generated metadata-rich lowering cannot treat
producer final spellings as semantic authority.

## Why This Idea Exists

LIR-to-BIR still keeps string-keyed tables for global declarations, legacy type
declarations, and aggregate layout bridging. Some of these are intentionally
raw-import/no-id compatibility surfaces. Others may still be used along
metadata-rich generated paths where `LinkNameId`, `LirTypeRef`,
`StructNameId`, or structured layout facts should be authoritative.

Backend restart should begin with this boundary explicit instead of relying on
comments and convention.

## In Scope

- Audit `GlobalTypes`, `TypeDeclMap`, and `BackendStructuredLayoutTable`
  construction and consumers.
- Separate raw/no-id compatibility lookup from metadata-rich generated lookup.
- Prefer `LinkNameId` for global identity and `LirTypeRef` / `StructNameId` /
  structured layout entries for type and aggregate layout identity.
- Make stale or missing structured metadata fail closed for one or more
  selected generated paths.
- Keep printer/final spelling and raw imported LIR compatibility working.

## Out Of Scope

- Removing all legacy type declaration parsing in one pass.
- Rewriting aggregate layout algorithms unrelated to the selected boundary.
- Changing emitted object symbol spelling.
- Treating final display names as unavailable; they may still be rendered from
  ids for output.

## Acceptance Criteria

- The selected generated LIR-to-BIR global/type/layout path no longer uses
  final spelling as normal semantic authority.
- Compatibility tables are documented or encoded as raw/no-id fallback only.
- Focused tests cover structured success, stale/missing metadata rejection, and
  retained raw compatibility where appropriate.
- Validation includes targeted LIR-to-BIR global/type/layout coverage.

## Reviewer Reject Signals

- The fix renames maps but leaves generated metadata-rich lookup string-first.
- Missing `LinkNameId` or `LirTypeRef` metadata silently falls through to a
  string-keyed semantic table.
- Tests only cover hand-authored raw LIR compatibility.
- The slice rewrites unrelated memory lowering behavior.
