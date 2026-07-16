# LIR Compact Scalar and ABI-Leaf Migration Runbook

Status: Active
Source Idea: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Activated From: closed 866 remaining-authority triage ordering

## Purpose

Execute the first ordered producer/schema successor from idea 866 by moving
compact scalar and ABI-leaf authority away from universal type/text handling.

## Goal

Introduce a compact scalar family and migrate scalar-only schemas so vector,
aggregate, and function refs are compile-time invalid there, while preserving
only evidenced pointer/void ABI-leaf compatibility.

## Core Rule

This is first-owner LIR producer/schema work. Do not implement 734 Raw-BIR
receiver packets, generic residual sweeps, or terminal deletion before the
named scalar/ABI-leaf gates are accepted.

## Read First

- `ideas/open/841_lir_compact_scalar_abi_leaf_migration.md`
- `docs/lir_remaining_authority_owner_triage/current_evidence.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- Existing scalar lowering, verifier, printer, and receiver tests discovered
  during Step 1

## Current Targets And Scope

- Compact scalar store/ref and scalar operation schemas.
- Separately evidenced pointer/void ABI-leaf compatibility boundaries.
- Removal of scalar text classification/comparison and scalar `LirTypeRef`
  fields only after named consumers accept `LirScalarRef`.
- Opaque semantics only as an evidence-needed subroute.

## Non-Goals

- Do not assume opaque is scalar.
- Do not migrate aggregate, vector, or function families under this idea.
- Do not remove external compatibility before a native consumer exists.
- Do not reopen accepted 734 receiver rows, including `LirAbsOp`
  selected-global/i32 from `0c44e810ad`.
- Do not parse rendered text, printer output, or testcase names as scalar
  authority.

## Execution Rules

- Work in bounded packets with a fresh build plus focused proof for each code
  slice.
- Keep producer/schema, verifier/printer, and receiver compatibility evidence
  separated in the runbook and in tests.
- Retire each opaque factory or compatibility adapter only after the exact
  named consumer has migrated.
- Any eventual 734 return requires an accepted exact typed scalar or ABI-leaf
  handoff naming one bounded Raw-BIR receiver row.

## Steps

### Step 1 - Inventory scalar and ABI-leaf authority users

Goal: identify the concrete scalar-only schemas, helpers, verifier/printer
paths, and receiver compatibility points this idea may own.

Actions:

- Trace current scalar type/ref construction and consumption through lowering,
  operation schemas, verifier, printer, and Raw-BIR receiver compatibility.
- Separate true scalar rows from vector, aggregate, function, opaque, pointer,
  and void ABI-leaf cases.
- Record the first bounded migration target and the tests that prove current
  behavior before editing implementation files.

Completion check:

- `todo.md` records the selected bounded target, excluded families, proof
  command, and any missing evidence.
- No implementation change is required for this step.

### Step 2 - Add the compact scalar authority carrier

Goal: introduce the minimal native scalar carrier/store/ref needed by the
selected bounded target.

Actions:

- Add the compact scalar representation and checked access used by the selected
  scalar-only schemas.
- Preserve pointer/void ABI-leaf compatibility only where Step 1 found named
  evidenced consumers.
- Keep old universal fields available only as compatibility mirrors until all
  named consumers migrate.

Completion check:

- Fresh build passes.
- Focused scalar positive and wrong-family coverage proves vector, aggregate,
  and function refs cannot silently enter the scalar-only path.

### Step 3 - Migrate named scalar producers and consumers

Goal: move the selected scalar lowering, operation verifier/printer, and
receiver compatibility path to the compact scalar authority.

Actions:

- Migrate one bounded producer/consumer group at a time.
- Update verifier/printer checks to use native scalar authority and treat
  rendering as parity only.
- Keep compatibility adapters for named unmigrated consumers.

Completion check:

- Fresh build and focused tests cover migrated scalar lowering, verifier,
  printer, receiver compatibility, pointer/void ABI compatibility where
  selected, and wrong-family rejection.

### Step 4 - Retire accepted scalar text escape hatches

Goal: remove scalar text classification/comparison and scalar `LirTypeRef`
fields only where every named consumer has migrated.

Actions:

- Delete only the scalar text helpers, factories, fields, or conversions whose
  named consumers are proven migrated.
- Retain opaque/external/inline-asm compatibility until a native consumer
  accepts its own route.
- Document any exact typed handoff that can later return to 734.

Completion check:

- Fresh build and focused regression prove no remaining named scalar consumer
  depends on text classification or the retired scalar `LirTypeRef` fields.
- Any 734 return condition names one exact typed scalar or ABI-leaf row and
  excludes all other families.
