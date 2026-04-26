# LIR BIR Legacy Type Text Removal Runbook

Status: Active
Source Idea: ideas/open/118_lir_bir_legacy_type_text_removal.md

## Purpose

Remove legacy LIR/BIR type-text authority only where ideas 116 and 117 have
already provided structured layout and structured printer routes.

## Goal

Stop covered backend paths from using legacy declaration text, raw type strings,
or BIR printer fallback text as active semantic authority while preserving final
output spelling and dump compatibility.

## Core Rule

Remove one proven fallback family at a time. Missing structured coverage is a
blocker, not permission to weaken tests, rewrite expectations, or add
testcase-shaped shortcuts.

## Read First

- ideas/open/118_lir_bir_legacy_type_text_removal.md
- ideas/closed/116_bir_layout_dual_path_coverage_and_dump_guard.md
- ideas/closed/117_bir_printer_structured_render_context.md
- src/backend/lir.hpp
- src/backend/lir_to_bir.cpp
- src/backend/layout.cpp
- src/backend/bir/
- tests covering `--dump-bir`, prepared BIR, aggregate layout, and backend
  output

## Current Targets

- `LirModule::type_decls` and `TypeDeclMap` use as active BIR layout authority.
- Legacy declaration-body parsing where structured layout already covers the
  semantic family.
- BIR printer fallback string fields marked removable by idea 117.
- Proof-only parity shadows that no longer need to drive covered paths.
- A final inventory classifying any remaining legacy text as final spelling,
  type-ref authority work, planned-rebuild residue, or deferred compatibility.

## Non-Goals

- Do not redesign broad `LirTypeRef` equality or identity.
- Do not remove text needed only for final emitted LLVM spelling.
- Do not migrate or preserve `src/backend/mir/` internals.
- Do not remove raw BIR label strings; idea 119 owns structured block label
  identity.
- Do not downgrade tests, mark supported cases unsupported, or claim progress
  through expectation rewrites.

## Working Model

- Ideas 116 and 117 made selected structured layout and printer routes available.
- Legacy text can remain as final spelling or compatibility data, but it should
  not remain active authority for a covered structured path.
- Removal should proceed by semantic family so each deletion has a direct
  structured-only proof.
- If a path still depends on legacy text for semantics, classify the dependency
  and leave it in place until a separate runbook or later step proves readiness.

## Execution Rules

- Start every code-changing step by identifying the exact legacy authority being
  removed and the structured authority replacing it.
- Keep behavior byte-stable for `--dump-bir`, prepared BIR, and backend output
  unless the supervisor explicitly accepts an output contract change.
- Prefer structured layout/render APIs already introduced by ideas 116 and 117.
- Record deferred or blocked legacy surfaces in `todo.md`; edit the source idea
  only if durable intent changes.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to broader validation when a removal affects shared layout,
  lowering, or dump rendering behavior across multiple families.

## Steps

### Step 1: Classify Remaining Legacy Type Text Authority

Goal: Build the removal map before deleting any legacy field or fallback.

Primary target: `src/backend/lir.hpp`, `src/backend/lir_to_bir.cpp`,
`src/backend/layout.cpp`, and `src/backend/bir/`.

Actions:
- Inspect current uses of `LirModule::type_decls`, `TypeDeclMap`, raw
  `LirTypeRef` text, function signature strings, extern return strings, global
  LLVM type strings, and BIR printer fallback strings.
- Classify each use as active layout/lowering authority, active dump-render
  authority, final spelling/output data, proof-only shadow, blocked type-ref
  authority, or deferred compatibility.
- Cross-check idea 117's handoff inventory for BIR printer fallback surfaces.
- Pick the first semantic family whose structured-only replacement is already
  directly provable.
- Record the classification and selected first target in `todo.md`.

Completion check:
- `todo.md` contains a concise legacy-surface inventory and names the first
  code-changing target with its required structured proof.

### Step 2: Remove Covered Layout Authority From Legacy Declarations

Goal: Stop covered BIR layout paths from depending on legacy declaration-body
text or `TypeDeclMap` parsing where structured layout is complete.

Primary target: LIR-to-BIR layout handoff and backend layout helpers.

Actions:
- Identify one covered aggregate/layout family from Step 1.
- Route that family through structured layout metadata from idea 116.
- Remove or demote the corresponding legacy declaration parsing fallback only
  after structured-only coverage exists.
- Keep final emitted type spelling intact.
- Add or adjust focused tests so the covered path fails if structured layout is
  absent and does not silently fall back to legacy declaration text.

Completion check:
- The selected layout family no longer uses legacy declaration text as active
  authority, existing output is stable, and focused proof is recorded in
  `test_after.log`.

### Step 3: Remove Covered BIR Printer Fallback Authority

Goal: Stop covered BIR dump paths from using legacy type strings as active
render authority where idea 117 supplied structured context.

Primary target: `src/backend/bir/bir_printer.cpp` and BIR call/type metadata.

Actions:
- Select a printer fallback family from idea 117's inventory that has
  structured render coverage.
- Render the family through structured context and keep raw strings only as
  compatibility data when still needed.
- Remove fallback branches only when a structured-only fixture proves the path.
- Preserve byte-stable `--dump-bir` and prepared-BIR text.
- Classify inline asm and other raw payload strings as final output data or
  deferred work before attempting removal.

Completion check:
- The selected dump-render family uses structured context as authority, focused
  dump tests pass, and remaining printer fallbacks are explicitly inventoried.

### Step 4: Retire Proof-Only Shadows For Proven Families

Goal: Remove parity scaffolding that no longer protects an active dual path.

Primary target: tests and backend plumbing that only compare legacy text
against structured data for already-migrated families.

Actions:
- Locate parity shadows or dual-path guards that Step 2 or Step 3 made
  redundant.
- Remove only scaffolding whose semantic replacement is already covered by
  structured-only tests.
- Keep guards that still protect unresolved or compatibility paths.
- Avoid replacing semantic proof with weaker snapshot expectations.

Completion check:
- Redundant proof-only legacy shadows are gone for the selected families, while
  remaining guards still fail on real structured coverage regressions.

### Step 5: Preserve Output Contracts Across Prepared BIR And Backend Runs

Goal: Prove that legacy authority removal did not change user-visible output
contracts.

Primary target: prepared BIR tests, `--dump-bir` tests, and backend output
tests selected by the supervisor.

Actions:
- Run the delegated build and focused test subset after each removal family.
- Include prepared-BIR and dump-filter coverage when printer behavior changed.
- Include aggregate/backend output coverage when layout or lowering changed.
- Escalate to broader backend or full validation after multiple shared-path
  removals.
- Record proof commands and results in `todo.md`.

Completion check:
- Fresh proof logs show stable behavior for the touched families, with no
  testcase-overfit expectation downgrade.

### Step 6: Final Legacy Text Classification And Handoff

Goal: Leave the repo with a clear authority boundary for any text that remains.

Primary target: `todo.md`, active backend fallback sites, and source idea
handoff notes if durable intent changes.

Actions:
- Inventory remaining legacy text fields and fallback paths.
- Classify each as final spelling/output data, type-ref-authority work,
  planned-rebuild residue, or explicitly deferred compatibility.
- If a separate structured identity or type-ref redesign is required, ask the
  supervisor to route it as a separate idea rather than expanding this runbook.
- Run the supervisor-selected acceptance proof for the completed removal
  family or milestone.

Completion check:
- Remaining legacy text authority is explicitly classified, all accepted
  removals have structured-only proof, and the supervisor has enough state to
  decide whether idea 118 should close or continue with another family.
