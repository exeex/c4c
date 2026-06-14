# 252 Phase F3 edge publication source-producer blocker map

## Idea Type

analysis-only.

## Goal

Map the blocker set for
`PreparedFunctionLookups::edge_publication_source_producers` before any BIR
producer-index or adapter implementation idea is allowed.

## Why This Exists

Phase F2 found no safe exit path for source-producer lookup authority. Current
evidence lacks direct x86/riscv source-producer consumers or explicit
non-applicability, and existing AArch64/prepared helper consumers keep the
public prepared surface observable.

## In Scope

- Inventory source-producer lookup helpers and prepared/AArch64 consumers.
- Determine whether x86 and riscv have direct consumption, indirect
  consumption, or explicit non-applicability for the producer/source relation.
- Define duplicate/conflict/mismatch fail-closed requirements for producer
  rows.
- Name compatibility strings, helper/oracle names, fallback names, and
  publication/output rows that must remain stable.

## Out Of Scope

- Implementing a BIR producer index.
- Deleting, privatizing, or wrapping source-producer lookup helpers.
- Inferring readiness from Route 5 edge/source evidence alone.
- Rewriting publication/output baselines or prepared helper/oracle names.

## Acceptance Criteria

- The result is a blocker map that names each consumer bucket and whether it is
  migrated, retained by compatibility, explicitly non-applicable, or still
  unknown.
- No implementation packet is proposed unless one producer/source sub-slice has
  named x86/riscv parity or exclusion evidence plus fail-closed cases.
- Public prepared source-producer authority remains retained until the map
  proves otherwise.

## Reviewer Reject Signals

- Reject named-case shortcuts that match a single producer name, helper call,
  or publication row while claiming source-producer ownership transfer.
- Reject unsupported expectation downgrades, weaker source-producer helper
  contracts, or output-baseline rewrites without explicit approval.
- Reject helper renames, compatibility-label rewrites, or classification-only
  maps claimed as implementation progress.
- Reject broad publication, producer-index, AArch64, x86, or riscv rewrites
  outside this blocker map.
- Reject any route that retains the old prepared source-producer failure mode
  while hiding it behind a new BIR producer-index name.

## Completion Notes

Closed as a completed analysis-only blocker map. The selected relation is
same-edge CFG publication source-producer identity, with `LoadLocal` requiring
Route 5 `memory_source` plus Route 3 source-memory identity agreement.

Producer-adapter readiness is not safe:

- x86 is blocked because no direct or indirect x86 consumer joins prepared
  edge-publication source-producer rows to the same Route 5/BIR
  source-producer identity and rejects disagreement fail-closed.
- RISC-V has diagnostic agreement evidence only; non-agreeing Route 5/Route 3
  facts can still preserve prepared fallback/output while clearing diagnostic
  agreement booleans.
- Public prepared source-producer lookup authority, helper/oracle names,
  compatibility strings, fallback names, prepared statuses, target policy
  rows, and output boundaries remain compatibility-owned.

Any implementation work must be a separate narrow follow-up idea for one target
and one producer/source sub-slice, with focused fail-closed proof for
duplicate, conflict, mismatch, missing/absent, prepared-only, fallback,
`LoadLocal` memory-source, immediate-producer, and policy-sensitive rows.
