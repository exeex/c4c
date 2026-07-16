# LIR Direct HIR Family Construction and Array Composition Runbook

Status: Active
Source Idea: ideas/open/843_lir_direct_hir_family_construction_array_composition.md
Activated From: 866 remaining-authority ordering after bounded 842 closure

## Purpose

Execute the next ordered producer/schema successor by replacing HIR-to-LIR
semantic construction from `llvm_ty()` or rendered text with direct family facts
from the HIR producer side.

## Goal

Migrate bounded HIR-to-LIR producer groups, `emit_rval_*`, and `coerce` toward
producer-specific construction from `TypeSpec`, canonical owners/layout, vector
facts, and signature facts while retaining arrays as recursive typed
composition.

## Core Rule

This is producer construction and recursive array composition work. Do not
parse printer output, migrate global/extern mirrors, run collector scans, or
delete compatibility adapters whose named producer is not migrated.

## Read First

- `ideas/open/843_lir_direct_hir_family_construction_array_composition.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- Accepted 842 commits for explicit value-boundary prerequisites:
  `464fa3b92`, `e54821000`, `7d9895dfe`, `577f8ea25`
- Existing HIR-to-LIR lowering, `emit_rval_*`, `coerce`, array/aggregate,
  vector, signature, and GEP verification tests

## Current Targets And Scope

- Bounded HIR-to-LIR producer groups where semantic type construction still
  flows from `llvm_ty()` or rendered strings.
- `emit_rval_*` and `coerce` paths that can be migrated to producer-owned
  family facts without broad rewrites.
- Recursive array composition: array length and typed element refs as facts,
  including nested arrays and aggregate children.
- GEP verification evidence that consumes typed element facts instead of
  rendered-text recovery.

## Non-Goals

- Do not parse printer output or use rendered text as semantic authority.
- Do not migrate global or extern initializer mirrors; idea 844 owns that
  scope.
- Do not perform collector/import-preparation scans; idea 845 owns that scope.
- Do not delete runtime-text/rendered aggregate or array factories until the
  exact named producer emits a family ref and named-consumer parity is proven.
- Do not return to 734 without one exact accepted family construction handoff.

## Execution Rules

- Work in bounded producer groups; do not rewrite every lowering path at once.
- For each code slice, run a fresh build plus focused scalar/vector/array,
  nested aggregate/signature, or GEP proof matching the touched path.
- Keep semantic producer construction separate from terminal compatibility
  deletion.
- Preserve parser/extern/asm adapters by named consumer until their own owner
  accepts migration.

## Steps

### Step 1 - Inventory HIR construction authority users

Goal: identify exact HIR-to-LIR producer paths that still construct semantic
family facts from `llvm_ty()` or rendered text, and select one bounded first
producer group.

Actions:

- Trace `emit_rval_*`, `coerce`, array/aggregate construction, vector
  construction, signature construction, and GEP element verification consumers.
- Separate direct `TypeSpec`/owner/layout/vector/signature facts from
  `llvm_ty()` and rendered-text recovery.
- Select one bounded Step 2 producer target, preferably the smallest route that
  proves recursive typed array or family construction without broad lowering
  churn.
- Record the valid scalar/vector/array/nested aggregate/signature cases,
  malformed cases, rejected rendered-text authority, and proof command.

Completion check:

- `todo.md` records the selected producer group, current authority users,
  accepted and rejected construction inputs, proof command, and any missing
  evidence.
- No implementation change is required for this step.

### Step 2 - Add direct family construction for the selected producer

Goal: make the selected producer emit semantic family refs from producer-owned
facts rather than `llvm_ty()` or rendered output.

Actions:

- Add or reuse exact family construction helpers for the selected producer's
  scalar, vector, aggregate, signature, or array facts.
- Preserve compatibility mirrors only as parity text for unmigrated named
  consumers.
- Add focused positive, malformed, and wrong-authority coverage.

Completion check:

- Fresh build passes.
- Focused tests prove the selected producer constructs family refs directly
  and rejects rendered-text or `llvm_ty()` authority for the covered path.

### Step 3 - Prove recursive array composition and GEP consumption

Goal: ensure arrays remain typed recursive composition and GEP consumes typed
element facts.

Actions:

- Add or migrate recursive array element/length facts for the selected route.
- Cover nested arrays and aggregate children where the selected producer owns
  those facts.
- Update GEP verification or receipt evidence only for the named migrated path.

Completion check:

- Fresh build and focused tests prove recursive array facts and GEP typed
  element consumption for the selected route.

### Step 4 - Retire selected runtime-text construction escape hatches

Goal: delete only the old runtime-text/rendered factory or conversion surface
whose exact named producer and consumers have migrated.

Actions:

- Delete helpers, comparisons, or conversions only where the selected named
  producer emits direct family refs and all named consumers have parity.
- Document any exact typed aggregate/vector/array row that can later return to
  734.
- Leave global/extern, collector, parser, asm, and unrelated producer families
  untouched.

Completion check:

- Fresh build and focused regression prove no remaining selected named
  consumer depends on the retired rendered-text construction surface.
- Any 734 return condition names one exact typed aggregate/vector/array row and
  excludes all other families.
