# LIR Extern Parameter Type Facts Repair Runbook

Status: Active
Source Idea: ideas/open/844_lir_global_extern_initializer_family_facts.md
Repaired From: exhausted extern aggregate return route after commits 63c080ea1, 553a24dd7, 3323d25e2, 17c5aaf88, and 091e1e5c3

## Purpose

Continue idea 844 after the selected global aggregate and extern aggregate
return slices completed. The remaining in-scope target is extern fixed
parameter type facts, currently represented through nominal function-signature
store entries rather than direct `LirExternDecl` parameter fields.

## Goal

Move the next exact extern parameter type-fact path toward structured family
refs while preserving extern declaration output compatibility and avoiding
initializer-text, collector-only, Raw-BIR receiver, or non-type string routing
work.

## Core Rule

This repaired route is still idea 844 only: global/extern type facts. It must
not redesign initializer text, delete the initializer scanner, absorb 812/813
string routing, treat final declaration text as semantic type authority, or
invent direct extern parameter fields unless inventory proves that is the
minimal producer-owned carrier.

## Accepted Prior Routes

- The selected declared aggregate global slice now requires matching
  structured refs and renders aggregate globals from structured
  `StructNameId` authority when present.
- The selected global legacy mirror was retained because `LirGlobal.llvm_type`
  still serves compatibility/output text for unselected families and
  receiver/backend paths.
- The selected extern aggregate return slice now verifies and renders aggregate
  returns from structured `return_type` authority when present.
- The selected extern return legacy mirror was retained because
  `return_type_str` still serves compatibility/output text and fallback for
  nonaggregate runtime-text declarations and unselected parameter/signature
  store work.

## Current Targets And Scope

- Extern fixed parameter type facts carried by nominal module
  function-signature entries.
- The producer path that records extern declaration fixed parameters into the
  signature store.
- Verifier/printer behavior for extern declarations whose parameter facts can
  be checked or rendered from structured family refs.
- Family-ref collection/import-preparation seams only as local observations
  needed to identify the selected producer carrier.

## Non-Goals

- Do not migrate initializer payload text or global policy identity.
- Do not delete final extern declaration output text before structured printer
  parity is proven for the exact selected declaration form.
- Do not add direct `LirExternDecl` parameter fields unless Step 1 proves the
  signature store cannot be the selected producer-owned carrier.
- Do not absorb collector-only receiver migration, Raw-BIR import work,
  nonaggregate runtime-text declaration handling, varargs policy work, or
  812/813 non-type string routing.

## Execution Rules

- Work in one bounded extern parameter packet at a time.
- Keep fixed parameter facts separate from extern symbol identity, return type
  facts, final declaration text, varargs metadata, and initializer payload
  text.
- For code slices, run a fresh build plus focused extern declaration,
  signature-store, printer/verifier, and global initializer compatibility
  tests.
- Preserve final declaration rendering until selected structured printer parity
  is proven.
- Record any remaining compatibility mirrors as compatibility/output mirrors,
  not semantic authority, unless all selected consumers have migrated.

## Steps

### Step 1 - Inventory extern fixed parameter type fact carriers

Goal: select the first bounded extern fixed parameter target and identify its
producer-owned structured carrier.

Actions:

- Trace fixed parameter facts through `record_extern_decl`, `ExternDeclInfo`,
  nominal function-signature store entries, verifier, printer, and call
  signature checks.
- Identify whether the selected producer carrier should stay in the signature
  store or requires a minimal `LirExternDecl` API/field.
- Separate fixed parameter type facts from return facts, symbol identity,
  varargs metadata, final declaration text, initializer payload text, and
  collector-only receiver work.
- Record positive, malformed, stale-text, missing-carrier, and wrong-family
  proof needs for the selected target.

Completion check:

- `todo.md` records the selected extern fixed parameter target, current
  carriers, accepted and rejected authority inputs, proof command, and missing
  evidence.
- No implementation change is required for this step.

### Step 2 - Add or require the selected parameter family-ref carrier

Goal: make the selected extern fixed parameter path publish or require the
minimal structured family ref from producer-owned facts.

Actions:

- Add or complete the selected carrier/API in the signature-store path, or add
  the minimal direct declaration carrier only if Step 1 proves that is required.
- Preserve old declaration text only as compatibility/output text for named
  consumers.
- Add positive and wrong-authority coverage for stale text, missing refs, wrong
  family inputs, and nonaggregate parameters that must remain supported.

Completion check:

- Fresh build passes.
- Focused tests prove the selected extern parameter target publishes or uses
  structured family refs and does not recover semantic type authority from final
  declaration text.

### Step 3 - Migrate selected parameter verifier/printer consumers

Goal: move selected extern parameter consumers to the selected structured
carrier while preserving output compatibility.

Actions:

- Migrate verifier checks for the selected extern parameter target.
- Migrate printer or final declaration rendering for the selected declaration
  form to consume the structured carrier as semantic input while preserving
  normal output.
- Record collection/import-preparation implications without implementing
  collector-only receiver work unless the selected producer carrier demands a
  local observation.

Completion check:

- Fresh build and focused tests cover valid facts, malformed refs, stale text,
  final declaration compatibility, and no initializer scanner regression.

### Step 4 - Retire selected parameter legacy mirrors only with parity

Goal: delete or demote selected extern parameter runtime-text mirrors only when
every selected named consumer has migrated.

Actions:

- Delete or demote text mirrors only for the exact selected parameter target
  with proven verifier/printer parity.
- Retain final output text unless structured rendering fully owns the selected
  declaration form.
- Document any exact typed extern handoff that can later return to 734.

Completion check:

- Fresh build and focused regression prove no selected named consumer depends
  on the retired legacy mirror.
- Any 734 return condition names one exact typed extern row and excludes
  initializer text, global policy semantics, collector-only work, Raw-BIR
  receiver work, and non-type string routing.
