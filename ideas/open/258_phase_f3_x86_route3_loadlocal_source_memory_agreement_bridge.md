# 258 Phase F3 x86 Route 3 LoadLocal source-memory agreement bridge

## Idea Type

x86 semantic agreement bridge.

## Goal

Add or prove the missing x86 Route 3 `LoadLocal` source-memory agreement
bridge needed before prepared `memory_accesses` lookup answers can be treated
as compatibility mirrors for the selected memory/source identity.

## Why This Exists

Idea 250 completed the blocker map for Route 3 memory/source parity and found
that RISC-V has diagnostic semantic evidence for the selected `LoadLocal`
result/source-memory identity, while x86 remains blocked. x86 has prepared
memory consumers, but no consumer or MIR query facade that joins a
same-function, same-block `Route3MemoryAccessRecord` for `LoadLocal` to
`PreparedEdgePublication::source_memory_access` and rejects disagreement.

## In Scope

- Introduce or prove one x86 agreement path for the selected Route 3
  `LoadLocal` result/source-memory identity.
- Compare the Route 3 memory record with the prepared
  `source_memory_access` row before treating prepared `memory_accesses`
  answers as semantic mirrors.
- Fail closed on missing, incomplete, duplicate/conflict, prepared-only, and
  route/prepared mismatch rows.
- Preserve prepared lookup/status observability and exact existing fallback
  names, helper/oracle names, and status strings.
- Keep x86 addressing, frame/global placement, operand spelling, and register
  materialization target-owned.

## Out Of Scope

- Generic memory parity beyond the selected `LoadLocal` source-memory fact.
- RISC-V implementation changes.
- Deleting or privatizing `PreparedFunctionLookups::memory_accesses`.
- Rewriting memory baselines, helper names, status text, or x86 output rows as
  the primary proof of progress.
- Moving x86 addressing, frame layout, storage placement, or register
  materialization into Route 3/BIR.

## Acceptance Criteria

- x86 has a concrete Route 3/BIR agreement consumer or MIR query facade for
  the selected `LoadLocal` source-memory fact.
- The bridge accepts only when the Route 3 record and prepared
  `source_memory_access` row agree on the selected semantic identity.
- Missing, incomplete, duplicate/conflict, prepared-only, unsupported, and
  mismatch cases reject or preserve prepared compatibility behavior without
  synthesizing agreement.
- Focused backend tests prove the agreement path and fail-closed rows without
  weakening existing prepared lookup/status or output contracts.

## Resumed Lifecycle Note

The active runbook reached the implementation bridge in commit `76767c2ac`,
then parked at Step 4 because the current x86 fixture set does not expose a
supported path that naturally publishes a selected `LoadLocal`
`source_memory_access` row into
`render_agreed_route3_load_local_statement_memory_operand(...)`.

Checked surfaces:

- addressed local-slot guard rows provide Route 3 memory records and legacy
  no-publication fallback, but synthetic `join_transfers` are rejected before
  the agreement facade;
- short-circuit rows have real `join_transfers` and prepared frame-slot
  authority, but no available selected `LoadLocal` source-memory publication;
- joined-branch rows publish real non-`LoadLocal` chains, while local-slot rows
  are test carrier rewrites;
- loop-countdown rows use loop-carry rendering outside the addressed
  statement-memory facade.

Follow-up testability work is split to
`ideas/closed/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md`.
That work is now complete. Resume this idea from Step 4 using the supported
joined-branch compare-join `EdgeStoreSlot` selected-`LoadLocal` route, which
naturally carries both the Route 3 `LoadLocal` memory record and prepared
`source_memory_access` publication facts into the x86 agreement facade.

The available proof surface covers positive agreement plus reachable
fail-closed rows for missing source address, join-carrier-only drift, and
incomplete prepared source-memory publication. Do not attempt prepared-only,
stale-publication, byte-offset drift, or cross-publication mismatch rows unless
a supported route can express them without synthetic or stale prepared state.

## Reviewer Reject Signals

- Reject named-case shortcuts that only match one test function, one local
  name, or one x86 output shape instead of comparing the selected Route 3
  `LoadLocal` memory record to the prepared source-memory row.
- Reject unsupported expectation downgrades, status weakening, helper/oracle
  renames, fallback-name rewrites, or output baseline rewrites claimed as
  semantic agreement progress.
- Reject an adapter that leaves prepared `memory_accesses` as the old semantic
  authority behind a new Route 3/BIR accessor name.
- Reject broad memory lowering, frame layout, addressing, storage, or register
  materialization rewrites outside the selected agreement bridge.
- Reject accepting RISC-V diagnostic evidence as sufficient for x86 without an
  x86 consumer, query facade, or fail-closed agreement proof.
