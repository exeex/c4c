# LIR Global, Extern Declaration, and Initializer Family Facts Runbook

Status: Active
Source Idea: ideas/open/844_lir_global_extern_initializer_family_facts.md
Activated From: 866 remaining-authority ordering after bounded 843 closure

## Purpose

Execute the next ordered producer/schema successor by moving global and extern
type facts away from parallel `TypeSpec`, runtime text, and optional-ref mirrors
toward explicit family refs.

## Goal

Migrate global and extern declaration type facts to family refs while keeping
initializer-text semantics separately owned and preserving legacy initializer
compatibility until its named migration proves deletion.

## Core Rule

This is global/extern type-fact migration only. Do not redesign initializer
text semantics, delete legacy scanners prematurely, expand 812/813 non-type
string routing, or claim final rendering text as semantic type authority.

## Read First

- `ideas/open/844_lir_global_extern_initializer_family_facts.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- Accepted 843 commits for recursive array/GEP producer prerequisites:
  `0803da789`, `3e7691199`, `67e5e8a69`, `c7d7267ce`
- Existing global, extern declaration, initializer, verifier, printer, and
  family-ref collection tests

## Current Targets And Scope

- Global type facts currently held in `TypeSpec`, `llvm_type`, or optional
  type-ref mirrors.
- Extern declaration return/parameter type facts and their verifier/printer
  consumers.
- Family-ref collection for global and extern type facts.
- Legacy initializer compatibility proof, without redesigning initializer-text
  semantics.

## Non-Goals

- Do not migrate initializer-text semantics or non-type string policy.
- Do not delete the legacy initializer scanner until its exact producer and
  carrier migration are proven.
- Do not absorb 812/813 string-routing work.
- Do not return to 734 without one exact typed global or extern handoff.

## Execution Rules

- Work in bounded global or extern packets; do not rewrite both families at
  once unless Step 1 proves a shared helper is the minimal safe slice.
- For each code slice, run a fresh build plus focused global/extern lowering,
  verifier/printer, collection, and initializer compatibility proof matching
  the touched path.
- Preserve final output rendering and initializer scanner compatibility until
  named-consumer parity is proven.
- Keep type facts separate from global policy identity and initializer payload
  semantics.

## Steps

### Step 1 - Inventory global and extern type fact carriers

Goal: identify current global/extern type carriers, verifier/printer paths,
family-ref collection points, and initializer compatibility dependencies.

Actions:

- Trace global type facts through lowering, LIR schema, verifier, printer, and
  collector/import-preparation seams.
- Trace extern declaration return and parameter type facts through the same
  surfaces.
- Separate type facts from initializer payload text, global policy identity,
  and non-type string semantics.
- Select the first bounded Step 2 target and record valid/malformed/stale-text
  proof needs.

Completion check:

- `todo.md` records the selected global or extern target, current carriers,
  accepted and rejected authority inputs, proof command, and any missing
  evidence.
- No implementation change is required for this step.

### Step 2 - Add or complete the selected family-ref carrier

Goal: make the selected global or extern type path publish explicit family refs
from producer-owned type facts.

Actions:

- Add or complete the minimal carrier/API for the selected target.
- Preserve old text fields only as compatibility mirrors for named consumers.
- Add positive and wrong-authority coverage for stale text and missing family
  refs.

Completion check:

- Fresh build passes.
- Focused tests prove the selected target publishes family refs and does not
  recover semantic type authority from final rendering or initializer text.

### Step 3 - Migrate selected verifier, printer, and collection consumers

Goal: move selected consumers to the family-ref carrier while preserving final
output compatibility.

Actions:

- Migrate verifier checks for the selected target.
- Migrate printer or final rendering only to consume family refs as semantic
  input while preserving required compatibility output.
- Migrate family-ref collection for the selected target.

Completion check:

- Fresh build and focused tests cover valid facts, malformed refs, stale text,
  collection, and legacy initializer compatibility for the selected target.

### Step 4 - Retire selected legacy type mirrors only with parity

Goal: delete old type mirrors only where every selected named consumer has
migrated.

Actions:

- Delete `llvm_type`, extern runtime-text, helper comparisons, or conversions
  only for the exact selected target with proven parity.
- Retain final rendering and initializer scanner compatibility unless their
  own named migration is accepted.
- Document any exact typed global or extern handoff that can later return to
  734.

Completion check:

- Fresh build and focused regression prove no selected named consumer depends
  on the retired legacy type mirror.
- Any 734 return condition names one exact typed global or extern row and
  excludes initializer text and non-type global policy semantics.
