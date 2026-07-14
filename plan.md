# Automatic Local Label-Address Table Initializer Authority Runbook

Status: Active
Source Idea: ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md
Activated from: 768 Step 5 automatic local initializer blocker

## Purpose

Repair the distinct automatic local initializer emission route that loses an
already-structured native label-address direct constant before 768 retries its
direct-rvalue producer packet.

## Goal

Retain native direct-constant identity through nested automatic label-address
table initialization without widening into table decay, carrier, or backend
work.

## Core Rule

Follow the structured operand from `LabelAddrExpr` through the generic
initializer-list/coordinator consumer. Do not manufacture identity or recover
it from text.

## Read First

- `ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md`
- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `ideas/closed/770_lir_to_bir_native_label_address_constant_contract.md`
- existing frontend-LIR label-address probes and automatic initializer
  coordinator/emission surfaces

## Non-Goals

- no automatic-table `DeclRef` decay-to-GEP, broad rvalue/table redesign, or
  local-object authority work
- no carrier, verifier relaxation, Raw-BIR/importer, backend, 764, 767, or 769
  work
- no synthetic bridge, raw-text recovery, testcase routing, or weaker contract

## Execution Rules

1. Keep the current uncommitted 768 Step 5 packet unaccepted; use its broader
   regression only as boundary evidence.
2. Bind every edit to the generic automatic initializer consumer, not to
   `frontend_lir_call_type_ref` or a named external case.
3. Add positive and nearby malformed structured-authority coverage before
   accepting a route.
4. For code, require `cmake --build --preset default`, the dedicated focused
   test, and `ctest --test-dir build -j --output-on-failure -R
   '^frontend_lir_'`.

## Ordered Steps

### Step 1 - Map the automatic local initializer authority handoff

Goal: establish the exact generic coordinator/initializer-list consumer where
the nested direct constant loses identity.

Actions:

- inspect the automatic `void *table[] = { &&first, &&second };` frontend-LIR
  lowering path and compare its direct structured operand input/output;
- identify the smallest owned consumer boundary and write focused positive and
  malformed test expectations before implementation;
- keep direct `LabelAddrExpr` production, table `DeclRef` decay, and 768's
  uncommitted Step 5 acceptance outside this packet.

Completion check:

- the planned seam is a generic automatic initializer consumer with a concrete
  structured-authority contract and direct focused proof target.

### Step 2 - Preserve structured direct-constant authority through emission

Goal: implement only the selected automatic initializer consumer passage.

Actions:

- retain the native direct constant's function, target-label, pointer-type,
  and produced-value identity through nested local table element emission;
- change a generic direct consumer only if it is required by the same route;
- add focused positive and nearby malformed frontend-LIR coverage.

Completion check:

- the table initializer emits a valid structured operand without a synthetic
  bridge, raw-text recovery, carrier change, or decay implementation.

### Step 3 - Prove the bounded initializer route and hand back to 768

Goal: accept only evidence that the local initializer authority route is
stable, then preserve 768's return point.

Actions:

- run a fresh build, the dedicated focused proof, and the exact
  `^frontend_lir_` guard;
- reject scope drift or regressions; record the accepted seam and proof;
- hand 768 back to Step 5, with no claim that its uncommitted direct-rvalue
  packet is accepted.

Completion check:

- all selected proof passes and the lifecycle handoff names 768 Step 5 as the
  sole return point.
