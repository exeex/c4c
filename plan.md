# LIR Typed Expression Result Carrier Decomposition Runbook

Status: Active
Source Idea: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Activated from: 775's repeated first-loss boundary without failure-family reduction.

## Purpose

Replace the stalled helper-result route with independently observable
frontend-LIR expression-result seams.

## Goal

Identify the smallest typed expression-result carrier contract that can later
support a bounded producer repair for one family at a time.

## Core Rule

Expression-result IDs are semantic authority. `%t` names, labels, printed
LLVM, instruction order, and testcase names are display or incidental data,
never recovery inputs.

## Read First

- `ideas/open/776_lir_typed_expression_result_carrier_decomposition.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- active `emit_rval_payload`, `emit_rval_id`, `coerce`, ternary, logical, and
  vaarg frontend-LIR producer paths

## Non-Goals

- no generic all-expression migration or implementation claim from this
  decomposition alone
- no `LirPhiOp` carrier/verifier, Raw-BIR/importer, backend-case, side-table,
  CFG, target-lowering, MIR, or emission work
- no parsing of value names, labels, printer text, or rendered LLVM

## Execution Rules

1. Establish the carrier boundary before proposing any production migration.
2. Use focused frontend-LIR probes, not backend case files, to isolate the
   ternary, logical, and vaarg first-loss seams independently.
3. End by publishing only a precise typed PHI-consumer handoff; no PHI repair
   occurs here.

## Ordered Steps

### Step 1 - Establish the typed expression-result carrier boundary

Goal: identify the smallest allocation/ownership boundary across
`emit_rval_payload`, `emit_rval_id`, and `coerce` that can be probed without
committing to a broad migration.

Primary targets:

- typed expression-result carrier API and result allocation ownership
- focused frontend-LIR probe harnesses for later one-family observations

Actions:

- record the exact pre-render allocation/ownership boundary and whether a
  result can traverse it as a typed operand without PHI changes
- add or identify one frontend-LIR probe shape that observes this carrier only
- classify the ternary, logical, and vaarg routes as separate follow-on steps

Completion check:

- a precise carrier-boundary record and one focused frontend-LIR probe contract
  exist; no broad migration, PHI change, or backend proof is claimed.

### Step 2 - Isolate the ternary/coerce result path

Goal: bind one focused frontend-LIR probe to the ternary/coerce first-loss
seam and record its exact typed result requirement.

Completion check:

- the ternary/coerce route has an independent probe and does not rely on
  logical or vaarg facts.

### Step 3 - Isolate the logical short-circuit result path

Goal: bind one focused frontend-LIR probe to the `fresh_tmp` logical result
seam and record its exact typed result requirement.

Completion check:

- the logical route has an independent probe and does not rely on ternary or
  vaarg facts.

### Step 4 - Isolate the vaarg helper result path

Goal: bind one focused frontend-LIR probe to the vaarg helper / `emit_lir_op`
first-loss seam and record its exact typed result requirement.

Completion check:

- the vaarg route has an independent probe and does not rely on ternary or
  logical facts.

### Step 5 - Publish the PHI-carrier consumer handoff

Goal: state exactly which typed result facts are proven for a later 751 PHI
carrier packet and which require separate successors.

Completion check:

- the handoff names each family, typed field, proof, and 751 return point;
  it does not authorize PHI text recovery or Raw-BIR work.
