# Structured LIR Operand And Terminator Identity Runbook

Status: Active
Source Idea: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Activated from: blocked Step 4 of ideas/open/734_lir_to_new_bir_container_completeness.md

## Purpose

Decompose the producer-side LIR identity gap that blocks idea 734 into checked
authority seams, focused probes, and the smallest generic typed carrier work.

## Goal

Make producer-emitted instruction operands and non-void terminator values carry
stable, lossless, verifiable typed identities, then hand those rows back to the
open new-BIR receiving initiative.

## Core Rule

Bind every schema change to one checked authority row, focused probe, producer,
and verifier contract before implementation. Never recover identity by parsing
or matching rendered text, names, printer output, or testcase identity.

## Read First

- `ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md`
- blocked consumer `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/operands.hpp`, and
  `src/codegen/lir/verify.cpp`
- actual HIR-to-LIR producers under `src/codegen/lir/hir_to_lir/`
- focused cases named by the source idea under `tests/backend/case/`

## Current Targets

- every `LirInst` and `LirTerminator` operand/result/symbol authority row
- global-symbol identity for load/store
- global aggregate/array address identity for `LirGepOp`
- non-void return-value identity for `LirRet`
- existing stable `LirValueId`, `LinkNameId`, and typed-immediate seams as the
  minimality reference

## Non-Goals

- no new-BIR instruction, terminator, container, builder, verifier, or importer
  implementation
- no text parsing, testcase matchers, allowlists, or weakened expectations
- no parallel value/symbol model or broad LIR redesign
- no downstream lowering, canonicalization, ABI placement, allocation, MIR,
  emission, or assembler work
- do not close or supersede idea 734

## Execution Rules

1. Record exact evidence in the authority matrix; use no catch-all disposition.
2. Keep one primary identity contract per focused probe and retain the original
   larger cases only as integration evidence.
3. Do not edit schema or producers until a probe is bound to a generic carrier,
   producer, and verifier contract.
4. Extend existing stable identity conventions when sufficient; do not invent
   a parallel value or symbol subsystem.
5. Keep display text presentation-only after structured authority exists.
6. For code packets, use a fresh build, the exact focused proof, neighboring
   malformed coverage, and the supervisor-selected backend checkpoint.
7. Update execution churn in `todo.md`; update this runbook or source idea only
   for a real route, scope, or proof correction.

## Ordered Steps

### Step 1 - Establish the blocked failure-family baseline

Goal: make the route collision and first unsupported identity boundaries
reproducible without changing code.

Actions:

- reproduce the current outcomes for `global_store.c`,
  `defined_pointer_global_pointer.c`, `defined_global_array.c`, and
  `riscv64_zero_aggregate_global_storage.c`
- record the exact first unsupported instruction or terminator row and the
  available structured versus textual identity facts
- preserve the fresh backend checkpoint and verify failures remain
  module-transactional

Completion check:

- each represented identity seam has a reproducible baseline and no result is
  described as a generic testcase failure

### Step 2 - Enumerate identity-authority seams

Goal: produce the exhaustive checked authority matrix before schema work.

Actions:

- enumerate every `LirInst` and `LirTerminator` operand, result, immediate, and
  symbol field
- classify each row as `LirValueId`, `LinkNameId`, typed immediate,
  structured-but-text-only `LirOperand`, or raw text
- name its producer, verifier, focused probe, blocked 734 consumer row, and
  keep/change disposition
- contrast stable value-ID and direct-call link-ID paths explicitly

Completion check:

- the matrix has no catch-all or omitted current variant and identifies the
  smallest candidate carrier family for each blocked row

### Step 3 - Extract or confirm focused probes

Goal: give each owned identity seam one focused observable contract.

Actions:

- confirm whether the existing global load/store cases isolate one contract;
  extract a smaller case only when they conflate identities
- confirm `defined_global_array.c` as the focused `LirGepOp` address seam
- extract a minimal scalar non-void return probe when the RISC-V aggregate case
  cannot isolate `LirRet` value identity
- retain the original cases as integration probes

Completion check:

- every owned seam has one focused probe with a single primary contract and a
  recorded baseline outcome

### Step 4 - Bind probes to owned carrier contracts

Goal: decide the narrow generic schema shape from evidence before editing it.

Actions:

- bind each focused probe to one typed carrier, producer-population rule, and
  reachable verifier obligation
- define malformed, missing, conflicting, and cross-owner rejection cases
- prove the proposal reuses ordinary `LirValueId`, `LinkNameId`, or typed
  immediate conventions where applicable
- reject bindings that depend on text normalization or testcase knowledge

Completion check:

- every proposed edit is justified by a checked row and probe, neighboring
  coverage is specified, and no parallel identity model is required

### Step 5 - Implement the narrowest generic carrier

Goal: land the smallest coherent typed identity-carrier families in dependency
order.

Actions:

- implement bounded carrier, producer, verifier, and focused-test packets
- preserve existing display strings only as non-authoritative presentation or
  parity fields
- prove neighboring producer rows and malformed states, not only one target case
- run `build -> focused proof -> backend checkpoint` for each coherent packet

Completion check:

- the owned global-symbol, aggregate/array address, and non-void return rows
  carry stable typed identity and malformed state rejects in a fresh build

### Step 6 - Prove and hand back to idea 734

Goal: close the decomposition initiative only after the blocked consumer has an
exact typed-authority handoff.

Actions:

- re-audit the authority matrix against current variants, producers, and
  verifier paths
- run the supervisor-selected focused, backend, and broader regression proof
- name every 734 instruction/terminator receipt row now unblocked
- record any genuinely separate remaining identity initiative without
  expanding this source idea
- request lifecycle return to open idea 734; do not implement its new-BIR rows
  here

Completion check:

- the matrix is exhaustive, all owned probes and malformed cases pass, no
  rendered-text authority remains in the owned rows, and 734 has a precise
  resumable handoff

## Runbook Completion And Handoff

Completing Step 6 makes idea 741 eligible for closure and idea 734 eligible for
reactivation. It does not itself complete or supersede idea 734.
