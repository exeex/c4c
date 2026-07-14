# Raw-BIR Function-Owned Label-Address GEP Base Runbook

Status: Active
Source Idea: ideas/open/774_raw_bir_gep_function_label_address_base.md
Supersedes: 773 Step 2 while its out-of-scope Raw-BIR schema/builder blocker runs

## Purpose

Make the Raw-BIR GEP boundary able to retain one verified current-function
direct label-address as structured base authority, without weakening its
global-object base contract or borrowing 773's printer/forwarding work.

## Goal

Represent, build, and lower the bounded function-owned label-address GEP base
alongside the existing global-array base.

## Core Rule

Raw-BIR must carry typed identity and explicit ownership. A label spelling,
rendered text, synthetic value, or global-object coercion is never authority.

## Read First

- `ideas/open/774_raw_bir_gep_function_label_address_base.md`
- `ideas/open/773_lir_gep_direct_label_address_constant_contract.md`
- `src/backend/bir/core/ir.hpp`, `builder.hpp`, and `builder.cpp`
- downstream Raw-BIR GEP lowering consumers and nearby Raw-BIR tests

## Non-Goals

- no generic Raw-BIR redesign or unrelated operand widening
- no raw-text/importer recovery, synthetic SSA/global, or label-spelling
  identity
- no LIR-to-BIR dispatch or LIR printer work, 773 Step 2 completion, 772
  forwarding repair, 734, or
  computed-goto work

## Execution Rules

1. Preserve the existing global-array GEP-base route and its validation.
2. Make the new base variant explicit and validate current-function ownership
   plus direct-label-address identity at the appropriate builder/lowering seam.
3. Add nearby positive and malformed tests; no named case, rendered text, or
   testcase-specific selector may establish authority.
4. Build fresh and run the focused Raw-BIR proof before handoff.
   Do not refresh or accept the rejected full-suite baseline candidate.
5. On acceptance, return only the representation contract to 773 Step 2; do
   not silently perform 773 printer/forwarding or 772 production work here.

## Ordered Steps

### Step 1 - Specify the structured Raw-BIR GEP-base variant

Goal: replace the `GlobalObjectId`-only GEP-base assumption with an explicit,
bounded representation that can carry either the existing global object or the
verified function-owned direct label address.

Primary targets:

- `src/backend/bir/core/ir.hpp`
- `src/backend/bir/core/builder.hpp`
- focused node/spec/builder contract tests

Actions:

- inspect all `GetElementPtrSpec` / `GetElementPtrNode` storage and consumers
- introduce the narrow structured variant and preserve an unambiguous legacy
  global-object alternative
- define builder-visible invariants for the function-owned label-address form
  without a generic value-ID admission path
- add focused structural and malformed coverage for both alternatives

Completion check:

- the schema and builder can distinguish supported alternatives, retain the
  global-object contract, and fail closed for invalid function-owned identity.

### Step 2 - Carry the variant through Raw-BIR GEP lowering consumers

Goal: consume the new structured base through Raw-BIR GEP lowering without
text recovery, global coercion, or synthetic values.

Primary targets:

- Raw-BIR GEP builder call sites and lowering consumers
- focused positive and malformed boundary tests

Actions:

- update all necessary consumers to use the variant's typed authority
- prove the existing global-array route still behaves identically
- add focused coverage that builds and reaches the new lowering path, plus
  malformed cases

Completion check:

- the structured function-owned form reaches Raw-BIR lowering as authority;
  unsupported forms fail closed and global-array behavior remains covered.

### Step 3 - Prove the bounded boundary and hand 773 back

Goal: provide accepted Raw-BIR evidence and the precise parent return contract.

Actions:

- run a fresh build and selected focused Raw-BIR tests
- inspect coverage for positive plus malformed boundaries and preserved global
  behavior
- report exact changed representation, proof, and the handoff for 773 to
  resume Step 2

Completion check:

- fresh build and narrow proof pass, acceptance evidence is ready for the
  supervisor, and 773's resumption point remains explicitly limited to printer
  receipt and LIR-to-BIR dispatch before its later 772 handoff.
