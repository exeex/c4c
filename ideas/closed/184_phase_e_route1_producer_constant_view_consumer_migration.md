# 184 Phase E Route 1 producer/constant view consumer migration

## Goal

Switch one AArch64 scalar value materialization or publication subpath to Route
1 producer and integer-constant facts.

## Why This Exists

Phase D identified Route 1 producer/constant facts as a bounded semantic view
for selected scalar materialization or publication consumers. The slice should
prove route-first reads without moving target materialization policy into BIR.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One scalar producer, integer-constant, or value-publication subpath that
  currently queries same-block scalar producer or integer constant helpers.
- A narrow Route 1 view keyed by consuming BIR value or instruction-local
  operand reference.
- Prepared same-block producer, integer-constant, scalar operand, and
  value-publication helpers as comparison/fallback answers.

## Out Of Scope

- Deleting prepared producer, constant, or value-publication APIs.
- Moving value homes, storage encodings, register choice, rematerialization
  spelling, emitted machine records, or move generation into BIR.
- Migrating unrelated scalar materialization paths.

## Acceptance Criteria

- The selected consumer can read same-block producer and integer-constant facts
  from Route 1 where available.
- Tests compare route-first answers with prepared helpers for same-block
  producer, constant, no-producer, non-constant, and cross-block cases.
- Existing Route 1 oracle coverage remains at least as strong.

## Reviewer Reject Signals

- The change special-cases a named testcase or materialization shape.
- Prepared value-home, storage, register, or machine-record policy leaks into
  the Route 1 view.
- The slice claims route-wide or API contraction from one selected consumer.
- Oracle tests are weakened or replaced with expectation-only changes.
- Missing Route 1 facts are hidden by broad BIR scans.

## Closure Notes

Closed after migrating the selected publication-source consumer,
`value_publication_may_read_register_index(...)`, to prefer the Route 1
publication producer view for complete same-block source-producer facts while
preserving prepared fallback and oracle surfaces for incomplete or out-of-scope
answers. Focused proof covered the selected route-first behavior, prepared
fallback/oracle visibility, no-producer and missing-producer fail-closed cases,
future-producer fallback, recursive operand register dependencies without
prepared source-producer indexes, and unrelated-register rejection.

This closure does not claim route-wide consumer migration or prepared API
contraction. Prepared producer, constant, scalar operand, value publication,
target register, home, storage, move, and machine-record policy surfaces remain
intentional guardrails for future slices.
