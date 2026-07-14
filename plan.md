# Unresolved External Direct Scalar-Call Signature and Result Authority Runbook

Status: Active
Source Idea: ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md
Supersedes: blocked idea-744 Plan Step 7.32 while this prerequisite is active

## Purpose

Publish only the missing source-level authority for the plain-`DeclRef`
unresolved-external direct, fixed-empty scalar-call route, then hand the exact
contract back to idea 744.

## Goal

Retain the native `FnPtrSig` for the authoritative unresolved-external
declaration, use it for the structured fixed-empty signature, and allocate the
actual scalar result with the owning function's `fresh_value(ctx)` path before
rendering.

## Core Rule

Source-level native authority precedes verification and presentation. Do not
recover signatures, declaration identities, or result IDs from names, rendered
operands, `%t*` spelling, type text, printer/LLVM output, declaration order, or
testcase identity.

## Read First

- `ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`
- `ideas/open/744_lir_remaining_ordinary_value_identity_publication.md`
- `src/codegen/lir/hir_to_lir/call/target.cpp`
- ordinary call emission/result construction and `LirFunction::fresh_value`
- `src/codegen/lir/ir.hpp` and `src/codegen/lir/verify.cpp`
- existing unresolved-external declaration/link-ID construction and focused
  frontend LIR call tests

## Scope

- plain-`DeclRef` unresolved external direct scalar calls
- fixed, nonvariadic, zero-argument declarations only when the source
  declaration supplies those facts
- native declaration link identity, `FnPtrSig`, structured fixed-empty
  signature, scalar result `LirValueId`, and reachable malformed rejection
- one focused positive probe plus nearby malformed-neighbor coverage
- an exact handoff sufficient to retry only idea-744 Step 7.32

## Non-Goals

- no verifier-only acceptance of a text-only route
- no HIR mutation, local-prototype relocation, parallel symbol/value table, or
  text/name/type/result recovery
- no call arguments, indirect/variadic/ABI-expanded calls, conversions,
  floating-operation expansion, ABI lowering, new-BIR, or idea-734 receiver
  work
- no closure or supersession of idea 744

## Execution Rules

1. Keep every implementation packet bounded to one generic producer seam and
   its exact malformed proof.
2. Preserve the existing external declaration/link-ID route; augment it only
   where the source declaration authoritatively carries the required facts.
3. Allocate the result before presentation and propagate the same `LirValueId`
   through the ordinary call result path.
4. Keep unsupported external shapes fail-closed; do not generalize from this
   focused fixed-empty route.
5. Require fresh build, focused positive proof, malformed-neighbor proof, and
   the supervisor-selected regression checkpoint for each code packet.

## Ordered Steps

### Step 1 - Trace and bind the authoritative producer seam

Goal: identify the exact plain-`DeclRef` unresolved-external production path
and bind a focused probe before changing carriers or verification.

Actions:

- trace declaration, link-ID, and call-target construction from source `DeclRef`
  through `extern_decl_link_name_map` and `extern_decls`
- identify where the source declaration can retain or construct native
  `FnPtrSig`, fixed-empty signature facts, return type, and result allocation
- create or bind one minimal unresolved-external fixed-empty scalar-call probe
  to the producer and exact verifier rejection obligations
- document the first bad fact and keep every unsupported external shape outside
  the packet

Completion check:

- one exact source-level seam and probe show how native signature and result
  authority must be published without text recovery or HIR fabrication

### Step 2 - Publish signature and scalar-result authority

Goal: make the bounded route emit its native fixed-empty signature and owned
result identity before rendering.

Actions:

- retain or construct `FnPtrSig` at the authoritative producer seam only when
  supplied by the source declaration
- publish the structured fixed-empty signature from that native fact
- allocate the scalar result with `fresh_value(ctx)` and propagate the exact
  `LirValueId` through the ordinary call result path
- preserve existing link-ID authority and unsupported-shape fail-closed behavior

Completion check:

- the focused plain-`DeclRef` route has native link, signature, return, and
  owned result authority with no display-derived fallback

### Step 3 - Verify malformed authority and hand off to idea 744

Goal: prove the new producer contract is reachable and narrowly sufficient to
unblock a retry of idea-744 Step 7.32.

Actions:

- cover absent, invalid, and mismatched declaration link identity; missing or
  conflicting signature/return facts; and missing, invalid, duplicate,
  cross-owner, or type-conflicting result identity
- run fresh build, focused positive proof, malformed-neighbor proof, and the
  supervisor-selected regression checkpoint
- record the exact source fields, carrier guarantees, verifier obligations,
  and proof that idea 744 may consume; leave all other external shapes
  explicitly fail-closed

Completion check:

- accepted evidence proves only the bounded native-authority contract and
  supplies a precise handoff for a later idea-744 Step 7.32 retry

## Runbook Completion And Handoff

Completing this prerequisite does not complete idea 744 or idea 734. After
accepted supervisor proof, plan-owner must re-evaluate and switch back to idea
744 before Step 7.32 is retried.
