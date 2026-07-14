# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted idea-744 ordinary value-identity handoff (`69d91e613`)

## Purpose

Resume bounded target-independent Raw-BIR receipt from the producer-authority
handoff, preserving prior module/signature and direct-void-call work. Take one
checked receiver row at a time and leave every other form fail-closed.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

Every admitted row maps existing typed LIR authority directly to a typed
Raw-BIR container, importer path, verifier rule, and transactional proof.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_remaining_ordinary_value_identity/handoff_to_734.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- Raw-BIR function/value/node builders, views, verifier, and LIR importer

## Landed Progress

- Steps 1-3: coverage foundation plus typed module/type/value, global, string,
  extern, symbol, initializer, specialization, intrinsic, and direct scalar
  return-signature receipt.
- Steps 4.1-4.4: selected-global integer Store, Load, array-decay GEP, and
  scalar integer Return receipt from closed idea 741.
- Steps 4.5 and its bounded follow-ons: authorized parameter signatures,
  linkage/elision metadata, and direct zero-argument void Call receipt.
- The previous Step 5.1 direct-void Call is complete (`49ed1b386`). Do not
  repeat it; the first new ordinary receiver work begins at Step 5.2.

## Non-Goals

- no LIR redesign or presentation-text recovery
- no target interpretation, ABI placement, canonicalization, allocation, MIR,
  emission, assembler work, or legacy-BIR revival
- no receipt of producerless, raw, aggregate/object, CFG/local/body-parameter,
  indirect, variadic, ABI-expanded, or otherwise fail-closed handoff rows

## Execution Rules

1. Implement exactly one handoff row or explicitly shared typed seam per packet.
2. Add container, importer, reachable Raw-BIR verification, and transactional
   positive/negative proof together.
3. Keep source IDs, types, callee identity, and source order native; names and
   rendering are diagnostics only after structured authority exists.
4. Preserve full-module rollback for every malformed or unsupported form.
5. Record a separate producer initiative for any required authority gap.

## Ordered Steps

### Step 5.2 - Receive resolved direct integer-result calls

Goal: receive the first newly authorized idea-744 handoff row without
generalizing to unproven call forms.

Primary target:

- typed Raw-BIR direct-call payload/result registry, importer dispatch, and
  reachable verification

Actions:

- define a typed direct-call receipt for only a resolved direct `LinkNameId`
  callee, owning `LirValueId` result, matching structured return/parameter
  types, and fixed-void immediate or SSA argument subrows
- resolve callee identity and source value IDs directly from the established
  module/function registries; materialize native integer immediates directly
- verify result uniqueness/ownership, callee/signature agreement, argument
  order/type/extension, and atomic rejection of malformed authority
- prove positive direct integer-result receipt and neighboring missing,
  cross-owner, signature/type/count, alternative, and rollback failures
- keep indirect, variadic, ABI-expanded, aggregate, unresolved, nonmatching
  coercion, and floating forms unsupported

Completion check:

- a fresh build and focused receipt proof show structured result/callee/
  signature/argument edges imported transactionally with no text recovery.

### Step 5.3 - Take subsequent checked ordinary rows one at a time

Goal: receive only the next source row explicitly selected from the idea-744
handoff after Step 5.2 is accepted.

Actions:

- select the next exact handoff row and record its typed source/destination,
  verifier, and focused proof contract before implementation
- retain all unselected and fail-closed rows as unsupported

Completion check:

- execution has one bounded next receiver contract, not a broad ordinary
  instruction claim.

### Step 6 - Complete terminators and structured inline-assembly transport

Goal: receive only structured-authority terminator and inline-assembly rows.

Completion check:

- admitted rows have typed containers and transactional neighboring proof;
  target interpretation remains absent.

### Step 7 - Integrate the dispatcher, verifier and build boundary

Goal: prove landed families form one production importer with no partial state.

Completion check:

- broader proof passes and every accepted row has typed authority and receipt.

### Step 8 - Prove lossless completeness and transactional publication

Goal: close only after exhaustive matrix coverage and accepted full proof.

Completion check:

- every source acceptance criterion is callable and evidenced without
  unsupported rows being claimed as complete.
