# BIR Prealloc Control Publication Lookup Boundary Audit

## Goal

Analyze whether BIR control-flow semantics are duplicated by prealloc
out-of-SSA, control-flow, publication-plan, and prepared-lookup logic, then
produce follow-up ideas only for concrete authority overlaps.

## Why This Exists

BIR owns CFG, terminators, phi/select/comparison semantics, and instruction
values. Prealloc builds out-of-SSA transfers, edge publications, select-chain
and compare-join facts, current-block publication facts, store/source
publication plans, and many prepared lookup APIs.

The AArch64 cleanup work showed that arch backends can consume prepared
publication and control-flow authority, but it also revealed a lot of prealloc
classification machinery. Before x86/RISC-V rebuilds, we should verify whether
prealloc is correctly translating BIR control semantics into prepared
target-facing plans, or whether it is reconstructing facts BIR should have
made explicit.

## Owned Files

- Audit/read:
  - `src/backend/bir/bir.hpp`
  - `src/backend/bir/lir_to_bir/cfg.cpp`
  - `src/backend/bir/lir_to_bir/scalar.cpp`
  - `src/backend/bir/lir_to_bir/module.cpp`
  - `src/backend/prealloc/out_of_ssa.cpp`
  - `src/backend/prealloc/control_flow.hpp`
  - `src/backend/prealloc/publication_plans.*`
  - `src/backend/prealloc/prepared_lookups.*`
  - `src/backend/prealloc/formal_publications.*`
  - `src/backend/prealloc/prepared_printer/*`
- No implementation files should be edited by this idea.

## In Scope

- Inventory BIR control-flow, phi, select, compare, and terminator facts.
- Inventory prealloc control/publication facts, including:
  - out-of-SSA join transfers
  - prepared edge publications
  - current-block entry publications
  - select-chain and compare-join classification
  - store/source publication plans
  - prepared lookup APIs used by arch consumers
  - prepared printer/dump contract coverage
- Classify each overlap as:
  - `bir-control-semantic-fact`
  - `prealloc-transfer-plan-authority`
  - `prealloc-rederives-bir-control-fact`
  - `bir-missing-target-neutral-fact`
  - `lookup-api-contract-gap`
  - `prepared-printer-contract-gap`
  - `needs-follow-up-idea`
- Produce follow-up ideas only for traced overlaps or contract-test gaps.

## Out Of Scope

- Implementing control-flow/publication cleanup.
- Moving register/storage placement or target emission details into BIR.
- Reopening AArch64 dispatch/publication cleanup routes.
- Creating arch-specific x86/RISC-V implementation work before the shared
  contract gaps are named.

## Proof Expectations

- This is analysis-only; no backend tests are required unless implementation
  files are accidentally changed.
- The close note must name any generated follow-up ideas, and must also list
  lookup or prepared-printer contract gaps that should become tests before
  x86/RISC-V rebuild work starts.

## Reviewer Reject Signals

- The audit treats out-of-SSA transfer planning as duplicated BIR CFG semantics
  without tracing the actual fact.
- It proposes broad `prepared_lookups.cpp` rewrites without identifying
  consumer-facing API gaps.
- It jumps directly to x86/RISC-V arch implementation.

