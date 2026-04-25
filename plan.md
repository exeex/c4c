# LIR Backend Legacy Type Surface Readiness Audit Runbook

Status: Active
Source Idea: ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md

## Purpose

Classify remaining LIR and backend legacy type surfaces after the struct
declaration printer authority switch.

## Goal

Produce a review artifact that separates safe demotion candidates from
backend, layout-authority, type-ref-authority, proof-only, bridge-required,
printer-only, and planned-rebuild blockers.

## Core Rule

This is a report-only audit. Do not perform implementation cleanup, remove
legacy fields, rewrite tests, migrate MIR, or change compile targets while
executing this runbook.

## Read First

- `ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md`
- Closed parent ideas 107 through 111 when historical context is needed
- LIR module, verifier, HIR-to-LIR layout lookup, LLVM printer, and BIR layout
  code that mention legacy type text or `module.type_decls`

## Current Targets

- `LirModule::type_decls`
- `LirModule::struct_decls`
- `LirStructuredLayoutObservation`
- `StructuredLayoutLookup::legacy_decl`
- `LirGlobal::llvm_type` and `llvm_type_ref`
- `LirFunction::signature_text` and signature mirrors
- `LirExternDecl::return_type_str` and `return_type`
- call return and argument type refs plus formatted call text
- `LirTypeRef` equality, output, and raw string-only construction paths
- BIR layout consumers under `src/backend/bir/lir_to_bir/memory/`
- MIR/aarch64 legacy consumers only for `planned-rebuild` labeling
- backend helpers that parse type text or scan `module.type_decls`

## Non-Goals

- Do not modify implementation code.
- Do not remove or demote `module.type_decls`.
- Do not switch backend, HIR-to-LIR, printer, verifier, global, function,
  extern, call, or raw `LirTypeRef` authority.
- Do not migrate or preserve `src/backend/mir/` internals as part of this
  route.
- Do not downgrade tests, rewrite expectations, or add testcase-shaped
  shortcuts.
- Do not treat a planned MIR rebuild surface as a blocker for BIR/LIR cleanup.

## Working Model

The audit maps every remaining legacy type surface to an owner and blocker
class. Implementation follow-ups should use the report as the source of truth,
especially ideas 113, 114, and 115. MIR findings are classified as
`planned-rebuild`; if MIR `.cpp` files block future compilation, the intended
follow-up is compile-target exclusion, not MIR migration.

## Execution Rules

- Inspect code and tests only; keep this runbook report-only.
- Prefer AST-backed symbol queries when raw text search becomes too broad.
- Classify each surface using the source idea's exact category names.
- Separate active BIR/backend blockers from HIR-to-LIR layout blockers and raw
  type-ref blockers.
- Record proof gaps and recommended validation subsets for later execution
  ideas.
- Write the final audit under `review/112_lir_backend_legacy_type_surface_readiness_audit.md`.
- Preserve source-idea intent; routine findings belong in the review artifact,
  not in the source idea.

## Ordered Steps

### Step 1: Inventory Remaining Legacy Type Surfaces

Goal: Build the concrete list of remaining legacy APIs and rendered text
fields.

Primary targets:
- LIR declarations, globals, functions, externs, calls, and type refs
- HIR-to-LIR structured layout observation and lookup code
- BIR layout and backend helper code
- MIR/aarch64 legacy uses for classification only

Actions:
- Search for each target named in the source idea.
- Identify readers, writers, and verifier or printer-only uses.
- Record file/function ownership and whether the use is active authority,
  compatibility proof, final rendering, fallback, or planned-rebuild residue.
- Do not edit implementation files while collecting the inventory.

Completion check:
- The executor can produce a table of every inspected surface with its code
  location, consumer role, and candidate blocker class.

### Step 2: Classify Blocker Ownership

Goal: Assign each legacy surface to the precise source-idea category.

Primary targets:
- BIR aggregate layout paths
- HIR-to-LIR `StructuredLayoutLookup::legacy_decl` consumers
- raw `LirTypeRef` text identity paths
- printer-only and bridge-required text paths
- MIR/aarch64 planned-rebuild surfaces

Actions:
- Classify each surface as one of `safe-to-demote`, `legacy-proof-only`,
  `backend-blocked`, `planned-rebuild`, `layout-authority-blocked`,
  `type-ref-authority-blocked`, `printer-only`, `bridge-required`, or
  `needs-more-parity-proof`.
- Separate active backend blockers from planned MIR rebuild residue.
- Identify which legacy paths must not be removed yet.
- Flag any uncertain classification as a proof gap instead of guessing.

Completion check:
- The ownership map distinguishes BIR/backend, planned-MIR-rebuild,
  layout-authority, type-ref-authority, proof-only, bridge-required, and safe
  demotion surfaces.

### Step 3: Map Follow-Up Scope For Ideas 113, 114, And 115

Goal: Turn the audit into usable boundaries for the existing follow-up ideas.

Primary targets:
- Idea 113 backend structured layout dual path
- Idea 114 LIR type text authority demotion
- Idea 115 HIR-to-LIR layout `legacy_decl` demotion

Actions:
- List which audit findings belong to idea 113 and which must remain out of
  scope.
- List only safe or proof-only demotion candidates suitable for idea 114.
- List the selected HIR-to-LIR layout consumers suitable for idea 115, plus
  required fallbacks.
- State explicitly that MIR is not a current migration target and should not
  block these follow-ups.

Completion check:
- Each follow-up idea can start without re-deriving blocker ownership or MIR
  treatment.

### Step 4: Record Proof Gaps And Validation Recommendations

Goal: Name the evidence needed before any implementation demotion proceeds.

Primary targets:
- aggregate, HFA, sret, variadic, global-init, memory-addressing, GEP,
  initializer, `va_arg`, byval/byref, verifier, and printer proof areas

Actions:
- Identify focused test buckets or commands that should prove each follow-up
  route.
- Mark which proof is already sufficient and which needs broader parity.
- Recommend broader validation checkpoints when blast radius crosses backend,
  HIR-to-LIR, and LIR verifier boundaries.
- Do not run broad validation unless the supervisor explicitly delegates it.

Completion check:
- The audit report contains actionable proof gaps and recommended validation
  subsets for implementation follow-ups.

### Step 5: Write And Sanity Check The Review Artifact

Goal: Produce the required report-only artifact.

Primary target:
- `review/112_lir_backend_legacy_type_surface_readiness_audit.md`

Actions:
- Write the remaining legacy API/text field table.
- Include blocker owner for each blocked surface.
- Include recommended scope for ideas 113, 114, and 115.
- Include proof gaps and validation recommendations.
- Include explicit statements about paths that must not be removed yet.
- Include explicit MIR guidance: `src/backend/mir/` is not a migration target;
  compile failures there should become compile-target exclusion tasks.
- Sanity check that no implementation cleanup was performed.

Completion check:
- The review artifact satisfies the source idea's required output and
  acceptance criteria, and `todo.md` records the report-only proof status.
