# BIR Global Memory Provenance LinkNameId Expansion

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/187_bir_memory_provenance_global_handle_cleanup.md`

## Goal

Expand the selected BIR memory/provenance global handle cleanup beyond the
addressed-global pointer path so another global provenance route is
`LinkNameId`-authoritative when metadata is available.

## Why This Idea Exists

Idea 187 intentionally hardened one selected memory/provenance path. The audit
still found many `global_types.find(global_name)` and `GlobalAddress` spelling
flows across provenance, addressing, value materialization, and global
initializer handling. Many are route-local bridges, but at least one more
metadata-rich global path should be closed before backend restart.

## In Scope

- Inventory the remaining global-name keyed memory/provenance consumers after
  idea 187.
- Select one additional route, such as global load/store provenance, pointer
  initializer address resolution, or dynamic global array materialization.
- Require `LinkNameId` consistency for metadata-rich global identity on that
  route.
- Keep raw/global spelling maps only as route-local handles or no-id
  compatibility.
- Add focused tests for matching, stale, and missing global id metadata.

## Out Of Scope

- Removing all string-keyed memory maps in one slice.
- Rewriting local slot, local SSA, or block-label handling.
- Changing BIR dump spelling.
- Broad alias analysis redesign.

## Acceptance Criteria

- One additional global memory/provenance route is no longer spelling-first
  when `LinkNameId` metadata exists.
- Stale or mismatched global ids fail closed in the covered path.
- Route-local string maps are explicitly distinguished from semantic global
  identity.
- Validation includes focused BIR memory/provenance or LIR-to-BIR tests.

## Reviewer Reject Signals

- The slice treats local route names as global semantic symbols.
- Matching final spellings override mismatched `LinkNameId` metadata.
- Tests only exercise the already-hardened 187 path.
- The implementation broadens into unrelated memory lowering changes.
