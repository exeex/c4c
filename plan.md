# BIR MIR Interface Cleanup From Prepared MIR View Docs

Status: Active
Source Idea: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md

## Purpose

Introduce the first behavior-preserving MIR-facing prepared view described by
the completed Prepared MIR view research docs.

## Goal

Add a narrow, reference-only `PreparedMirView` core adapter and migrate the
first x86 internal module-emission surface without changing public backend
behavior.

## Core Rule

MIR code must gain typed read-only access to required prepared facts through
the new view. Do not rename or re-expose the whole `PreparedBirModule` as the
new interface, and do not let diagnostics, route text, notes, phases, or proof
details become semantic lowering authority.

## Read First

- `docs/prepared_mir_view_contract_research/index.md`
- `docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`
- `docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`

## Required Starting Evidence

- Core shape: implement the first-cut `PreparedMirCoreView` and
  `PreparedMirFunctionView` from
  `02_prepared_mir_core_view_shape.md`, backed by references into the current
  `prepare::PreparedBirModule` and view-owned derived lookup caches.
- Dependency narrowed first: target identity, BIR/prepared names, defined
  function traversal, globals, string constants, per-function control flow,
  value locations, stack layout, addressing, and prepared lookups from
  `01_current_mir_dependencies_on_prepared_bir.md`.
- Migration order: follow `06_incremental_migration_plan_for_mir_consumers.md`
  phases 1 and 2 before attempting RV64, AArch64, feature-view admission, raw
  dependency gates, or old/new producer comparison.
- Diagnostic boundary: keep verifier reports, proof certificates, route
  summaries, focus filters, notes, completed phases, and rendered debug text
  observational as required by
  `04_debug_proof_and_diagnostic_boundaries.md`.

## Current Targets

- First implementation surface: MIR-facing prepared view declarations and a
  small old-route adapter over `prepare::PreparedBirModule`.
- First consumer migration: x86 internal module emission, keeping
  `x86::api::emit_prepared_module(const PreparedBirModule&)` as the public
  compatibility wrapper.
- Proof surface: fresh build plus focused x86 backend coverage chosen by the
  supervisor, with no expected output, unsupported marker, allowlist, runtime,
  or baseline-policy downgrade.

## Non-Goals

- Do not rewrite BIR, replace the full BIR-to-prealloc pipeline, or implement
  phoenix rebuild work.
- Do not migrate every target in this plan.
- Do not delete broad `PreparedBirModule` fields before migrated consumers no
  longer depend on them.
- Do not add optional feature views except where required to keep the first
  x86 entry migration behavior-preserving.
- Do not change emitted MIR, assembly, object output, runtime behavior,
  diagnostics, expectations, unsupported markers, allowlists, timeouts, or
  baseline acceptance policy as proof of progress.
- Do not use diagnostic/proof text or route names to synthesize operands,
  labels, homes, call ABI resources, provenance, or feature admission.

## Working Model

- The first view is an adapter over today's old prepared route, not a new BIR
  producer ABI.
- Public x86 API compatibility remains stable while internal x86 code learns a
  view-taking surface.
- Core view accessors expose required shared MIR input. Optional feature facts
  stay out of the core until a named feature-view packet needs them.
- Missing required facts fail closed; migrated code must not recover authority
  from raw module access, diagnostic text, or testcase shape.

## Execution Rules

- Keep packets small enough to prove with compile/build plus targeted backend
  coverage.
- Prefer new MIR-facing headers that ordinary target lowering can include
  without pulling in broad prealloc ownership when practical.
- Keep any adapter implementation bridge visibly isolated as the place allowed
  to include `src/backend/prealloc/module.hpp`.
- Preserve old direct `PreparedBirModule` paths for unmigrated consumers until
  their migration is explicit and proved.
- Add focused adapter or structural-equivalence tests when a packet introduces
  separately testable view-owned lookup behavior.
- Record research-to-code mapping notes in
  `docs/prepared_mir_view_contract_research/implementation_notes.md` only when
  they clarify a durable implementation decision.

## Ordered Steps

### Step 1. Add Reference-Only Core Adapter

Goal: introduce the MIR-facing core view and old-route adapter without changing
target behavior.

Primary targets:

- `src/backend/mir/`
- `src/backend/prealloc/module.hpp`

Actions:

- Add `PreparedMirCoreView` and `PreparedMirFunctionView` declarations in a
  MIR-facing namespace and header.
- Back the first adapter with references or pointers into
  `prepare::PreparedBirModule`; own only view-scoped derived lookup caches.
- Expose the required core accessors named in the research docs: target
  identity, names, defined-function traversal, globals, string constants,
  per-function BIR/control-flow/value-location/stack-layout/addressing facts,
  and prepared lookups.
- Keep diagnostics, verifier details, proof certificates, notes, completed
  phases, and route names out of the core view.
- Add the smallest focused compile or unit coverage that proves lifetime,
  lookup agreement, or adapter construction if the repo has an existing
  suitable test surface.

Completion check:

- Fresh build succeeds.
- Any adapter-specific test added for this step passes.
- Existing MIR target behavior remains routed through the old public entry
  points unless the packet also explicitly starts Step 2.
- No new target lowering helper uses the view as a raw full-module escape
  hatch.

### Step 2. Migrate X86 Internal Module Emission Entry

Goal: keep the public x86 API wrapper stable while routing the internal module
emitter through the core view.

Primary targets:

- `src/backend/mir/x86/api/api.cpp`
- `src/backend/mir/x86/module/module.cpp`
- `src/backend/mir/x86/`

Actions:

- Keep `x86::api::emit_prepared_module(const PreparedBirModule&)` callable as
  the compatibility wrapper.
- Construct the old-route view adapter in or below the wrapper and call a
  view-taking internal `x86::module::emit(...)` surface.
- Migrate only target identity, target triple, module data inputs, defined
  function iteration, function view lookup, and prepared-name reads needed by
  the internal module-emission entry.
- Leave broad per-function plan consumption to Step 3 unless a tiny local
  handoff change is required to compile.

Completion check:

- Fresh build succeeds.
- Focused x86 backend subset passes.
- A representative x86 assembly or backend-output comparison shows no behavior
  delta for existing supported cases when practical.
- Public API callers still use the compatibility wrapper.

### Step 3. Move X86 Per-Function Consumption Behind The View

Goal: replace the first broad x86 per-function raw-module dependency with
view/function-view inputs.

Primary targets:

- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/prepared/`
- `src/backend/mir/x86/module/module.cpp`

Actions:

- Replace the first selected `consume_plans(const PreparedBirModule&, ...)`
  path with a `PreparedMirFunctionView` or narrow view-backed input.
- Move prepared lookup ownership for that path behind the view instead of
  target-local recomputation.
- Keep Route 6, call, publication, storage, regalloc, and other feature-like
  authority either unchanged behind legacy compatibility or explicitly carried
  by a named view-backed input.
- Do not migrate unrelated route helpers in the same packet.

Completion check:

- Fresh build succeeds.
- x86 call, branch, memory, data, and contract-first stub subsets relevant to
  the touched path pass.
- Old consumed-plan behavior and view-derived consumed-plan behavior are
  structurally equivalent for the selected path, or any mismatch is fail-closed
  before lowering.

### Step 4. Add First Raw Dependency Gate For Migrated X86 Surfaces

Goal: make new direct `PreparedBirModule` dependencies visible after at least
one x86 surface consumes the view.

Primary targets:

- `src/backend/mir/x86/`
- repo scripts or test helpers, if an existing gate location is suitable

Actions:

- Add a review-time grep or lightweight script that lists raw
  `PreparedBirModule`, `prealloc/module.hpp`, or broad prealloc include hits
  under migrated x86 MIR surfaces.
- Classify existing hits as adapter bridge, compatibility wrapper, tracked
  legacy surface, or diagnostic-only rendering.
- Reject new raw dependencies in migrated surfaces unless they carry an owner,
  deletion condition, and proof that the fact cannot yet be expressed through
  the view.

Completion check:

- Gate command runs and reports the expected legacy hits without hiding them.
- Fresh build succeeds.
- The gate does not require broad RV64 or AArch64 migration.

### Step 5. Audit And Handoff Follow-Up Ideas

Goal: record whether the first implementation idea is complete and what should
follow.

Actions:

- Audit the migrated x86 surfaces against the source idea acceptance criteria
  and reviewer reject signals.
- Record only durable research-to-code mapping notes in
  `docs/prepared_mir_view_contract_research/implementation_notes.md` if they
  clarify future migrations.
- If the core adapter plus first x86 migration is stable, leave separate
  follow-up ideas for comparator MVP, source/freshness view contract, RV64
  migration, or AArch64 context work rather than expanding this plan.

Completion check:

- The source idea's acceptance criteria are either satisfied or the remaining
  work is explicitly separated into new open ideas.
- Final proof includes a fresh build and focused backend subset covering the
  migrated x86 path.
- No testcase-shaped shortcut, expectation rewrite, unsupported downgrade, or
  diagnostic-authority drift is used as progress.
