# 251 Phase F3 Route 4/5 edge publication parity blocker map

## Idea Type

x86/riscv parity proof.

## Goal

Prove or explicitly block the narrow Route 4/5 edge-publication identity
boundary needed before public `edge_publications` lookup helpers can become
compatibility mirrors.

## Why This Exists

Phase F2 found that edge-publication readiness is split between route/BIR
semantic identity and target-owned emission policy. Public prepared
publication lookup, dispatch/status, and exact output rows remain authority
until x86 and riscv validate the same Route 4/5 fact through adapters and
prepared answers only mirror that fact.

## In Scope

- Select one edge/source publication identity fact.
- Name the x86 dispatch/status/module-output evidence and riscv
  emission/status evidence required for that fact.
- Preserve `EdgePublicationMoveIntentStatus`, x86 dispatch/status names, riscv
  prepared publication statuses, fallback publication rows, exact riscv
  instruction text, helper/oracle names, and module output rows.
- Require fail-closed behavior for duplicate, mismatch, unsupported,
  prepared-only, fallback, and policy-sensitive cases.

## Out Of Scope

- Deleting or privatizing `edge_publications`.
- Moving move selection, register choice, carrier/helper selection, layout, or
  instruction spelling into BIR.
- Rewriting riscv instruction output or x86 dispatch/status strings.
- Broad publication, edge lowering, or emission-policy rewrites.

## Acceptance Criteria

- The packet proves one route/BIR edge-publication identity with named x86 and
  riscv evidence, or records exactly why the boundary remains blocked.
- Prepared publication lookup/status rows stay stable and agree with the
  route/BIR fact or fail closed on mismatch.
- Target emission policy remains separate from the semantic identity fact.
- The result states whether a later one-adapter implementation idea is safe.

## Reviewer Reject Signals

- Reject named-case shortcuts that only special-case one edge label,
  publication kind, or target-specific output row while claiming shared
  Route 4/5 parity.
- Reject unsupported expectation downgrades, weakened publication status
  contracts, or riscv/x86 baseline rewrites without explicit approval.
- Reject helper renames, output rewrites, or classification-only claims as
  edge-publication capability progress.
- Reject broad publication lowering, register allocation, carrier/helper,
  layout, or instruction-emission rewrites outside the selected identity fact.
- Reject any route that leaves prepared `edge_publications` as the old
  semantic source behind a new adapter name.
