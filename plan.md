# LIR Extern Declaration Type Facts Repair Runbook

Status: Active
Source Idea: ideas/open/844_lir_global_extern_initializer_family_facts.md
Repaired From: exhausted selected global aggregate route after commits 0d81739ec, 17924cbbf, ab59b41d5, and 6149ed4a4

## Purpose

Continue idea 844 after the bounded global aggregate carrier route completed by
selecting the next in-scope global/extern type-fact target without reopening
initializer payload semantics, global policy identity, collector-only receiver
work, or Raw-BIR import work.

## Goal

Migrate the next exact extern declaration type-fact path toward family refs,
starting from the currently known extern return mirror and nominal signature
store evidence, while preserving final declaration output compatibility.

## Core Rule

This repaired route is still idea 844 only: global/extern type facts. It must
not redesign initializer text, delete the initializer scanner, absorb 812/813
string routing, or treat final declaration text as semantic type authority.

## Accepted Prior Route

The prior route completed the selected declared aggregate global slice:

- Step 1 inventory selected `LirGlobal.llvm_type_ref` as the first target.
- Step 2 required declared aggregate globals to carry matching structured type
  refs.
- Step 3 moved global printer rendering to structured `StructNameId` authority
  when present.
- Step 4 concluded no safe deletion of `LirGlobal.llvm_type` remains inside
  that selected slice because it is still compatibility/output text for
  unselected families and receiver/backend paths.

## Current Targets And Scope

- Extern declaration return type facts currently held in
  `LirExternDecl.return_type_str`, `LirExternDecl.return_type`, and the
  module function-signature store.
- Extern declaration fixed parameter type facts currently owned by nominal
  function-signature entries rather than direct `LirExternDecl` fields.
- Verifier/printer handling for extern declarations.
- Family-ref collection/import-preparation seams only as observations until an
  exact producer carrier exists.

## Non-Goals

- Do not migrate initializer payload text or global policy identity.
- Do not delete final extern declaration output text before printer parity is
  proven.
- Do not invent direct extern parameter fields unless Step 1 proves that is the
  minimal selected target.
- Do not absorb collector-only or Raw-BIR receiver migration.

## Execution Rules

- Work in one bounded extern return or extern parameter packet at a time.
- For code slices, run a fresh build plus focused extern declaration, global
  initializer compatibility, and selected printer/verifier tests.
- Preserve final declaration rendering until selected structured printer parity
  is proven.
- Keep extern return facts separate from function symbol identity and parameter
  signature-store facts unless a packet explicitly selects a store-backed
  target.

## Steps

### Step 1 - Inventory remaining extern declaration type fact carriers

Goal: select the next bounded extern return or parameter target after the
completed global aggregate route.

Actions:

- Trace extern return facts through `record_extern_decl`, `ExternDeclInfo`,
  `LirExternDecl`, verifier, printer, and call/signature-store seams.
- Trace extern fixed parameter facts through nominal signature-store entries
  and identify whether a direct `LirExternDecl` carrier is required or whether
  a store-backed target is sufficient.
- Separate return/parameter type facts from extern symbol identity,
  final declaration text, global initializer payload text, and collector-only
  receiver work.
- Select the first bounded repaired Step 2 target and record positive,
  malformed, stale-text, and missing-carrier proof needs.

Completion check:

- `todo.md` records the selected extern return or parameter target, current
  carriers, accepted and rejected authority inputs, proof command, and missing
  evidence.
- No implementation change is required for this step.

### Step 2 - Add or complete the selected extern family-ref carrier

Goal: make the selected extern declaration type path publish or require the
minimal structured family ref from producer-owned facts.

Actions:

- Add or complete the selected carrier/API.
- Preserve old return or declaration text only as compatibility/output mirror
  for named consumers.
- Add positive and wrong-authority coverage for stale text, missing refs, and
  wrong-family inputs.

Completion check:

- Fresh build passes.
- Focused tests prove the selected extern target publishes/uses structured
  family refs and does not recover semantic type authority from final
  declaration text.

### Step 3 - Migrate selected extern verifier/printer consumers

Goal: move selected extern consumers to the selected family-ref carrier while
preserving output compatibility.

Actions:

- Migrate verifier checks for the selected extern target.
- Migrate printer or final declaration rendering to consume the structured
  carrier as semantic input while preserving normal output.
- Record collector/import-preparation implications without implementing
  collector-only receiver work unless the selected producer carrier demands a
  local observation.

Completion check:

- Fresh build and focused tests cover valid facts, malformed refs, stale text,
  final declaration compatibility, and no initializer scanner regression.

### Step 4 - Retire selected extern legacy mirrors only with parity

Goal: delete selected extern runtime-text mirrors only when every selected named
consumer has migrated.

Actions:

- Delete or demote `return_type_str` or related text only for the exact
  selected extern target with proven verifier/printer parity.
- Retain final output text unless structured rendering fully owns the selected
  declaration form.
- Document any exact typed global or extern handoff that can later return to
  734.

Completion check:

- Fresh build and focused regression prove no selected named consumer depends
  on the retired legacy mirror.
- Any 734 return condition names one exact typed extern row and excludes
  initializer text, global policy semantics, and collector-only work.
