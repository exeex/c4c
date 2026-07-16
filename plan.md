# LIR Restricted First-Class Value Boundary Unions Runbook

Status: Active
Source Idea: ideas/open/842_lir_restricted_first_class_value_unions.md
Activated From: 866 remaining-authority ordering after bounded 841 closure

## Purpose

Execute the next ordered producer/schema successor by replacing universal
first-class value boundary type carriers with exact checked alternatives.

## Goal

Define boundary-local tagged value unions for call argument/result, PHI, select,
and return so admitted scalar, vector, aggregate, and evidenced pointer
alternatives are explicit and wrong families are rejected.

## Core Rule

This is producer/schema boundary-union work. Do not implement generic 734
Raw-BIR receiver packets, universal variants/IDs, or signature/body-use
ownership outside the named boundaries.

## Read First

- `ideas/open/842_lir_restricted_first_class_value_unions.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- The accepted 841 commits if scalar compact authority evidence is needed:
  `da9fdd291`, `8198c5f7b`, `e89fa5ffe`, `0645ae3c7`
- Existing call, PHI, select, return, verifier, printer, coercion, and receiver
  tests discovered during Step 1

## Current Targets And Scope

- Call argument/result boundary value alternatives.
- PHI, select, and return boundary value alternatives.
- Exhaustive verifier, printer, coercion, and BIR receipt handling for the
  named boundaries.
- Delete boundary `LirTypeRef` fields only after all named boundaries use
  exhaustive checked alternatives.

## Non-Goals

- Do not add a universal value bag, generic ID, RTTI/vtable abstraction, or
  implicit cross-family adapter.
- Do not migrate aggregate/vector/scalar producer families beyond what the
  named boundaries need.
- Do not take ownership of signature/body-use authority outside call
  argument/result, PHI, select, and return.
- Do not reopen accepted receiver rows or implement unbounded 734 work.

## Execution Rules

- Work in bounded packets with a fresh build plus focused proof for each code
  slice.
- Keep producer/schema changes separate from verifier/printer/coercion and
  receiver compatibility evidence.
- Require exhaustive valid, malformed, and wrong-kind coverage before deleting
  any old universal boundary field.
- Any eventual 734 return requires one exact accepted typed handoff naming a
  call, PHI, select, or return receiver row.

## Steps

### Step 1 - Inventory first-class boundary carriers

Goal: identify the exact current carriers, helper APIs, verifier/printer paths,
coercion paths, and receiver receipt points for call argument/result, PHI,
select, and return.

Actions:

- Trace current `LirTypeRef` or universal value use through each named boundary.
- Separate admitted scalar, vector, aggregate, and evidenced pointer
  alternatives from rejected function, void, opaque, metadata, or unbounded
  payload cases.
- Select the first bounded boundary target and record the proof command and
  wrong-kind matrix before implementation.

Completion check:

- `todo.md` records the selected boundary target, admitted alternatives,
  rejected families, proof command, and any missing evidence.
- No implementation change is required for this step.

### Step 2 - Add the restricted boundary union carrier

Goal: introduce the minimal boundary-local tagged union and checked accessors
needed by the selected boundary.

Actions:

- Add explicit enum alternatives and C++20 traits for only the admitted
  families required by the selected boundary.
- Provide checked construction/access APIs and wrong-kind diagnostics.
- Keep existing universal fields only as compatibility mirrors until named
  consumers migrate.

Completion check:

- Fresh build passes.
- Focused positive and wrong-kind coverage proves the selected boundary cannot
  silently accept unadmitted families.

### Step 3 - Migrate named boundary consumers

Goal: move the selected boundary verifier, printer, coercion, and receipt path
to the restricted union.

Actions:

- Migrate one boundary at a time.
- Update verifier/printer/coercion logic to use exhaustive checked
  alternatives.
- Preserve compatibility adapters only for named unmigrated consumers.

Completion check:

- Fresh build and focused tests cover valid alternatives, malformed cases,
  wrong-kind rejection, coercion behavior, and receiver compatibility for the
  migrated boundary.

### Step 4 - Retire accepted universal boundary escape hatches

Goal: remove old boundary `LirTypeRef` or universal carrier fields only where
every named consumer has migrated.

Actions:

- Delete only fields, helpers, comparisons, or conversions whose named boundary
  consumers are proven migrated.
- Document any exact typed handoff that can later return to 734.
- Leave unmigrated boundaries and unrelated authority families untouched.

Completion check:

- Fresh build and focused regression prove no remaining named consumer depends
  on the retired universal boundary field.
- Any 734 return condition names one exact call, PHI, select, or return row and
  excludes all other families.
