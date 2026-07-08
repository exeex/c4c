# Prepared MIR View Contract Research

Status: Open
Type: Research and architecture documentation
Parent: `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
Related:
- `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
- `ideas/open/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/open/590_branch_stack_load_freshness_contract.md`
Owning Layer: PreparedBir-to-MIR interface contract, MIR input views, old/new
BIR compatibility boundary, and backend consumption dependencies

## Goal

Produce concrete research documents under
`docs/prepared_mir_view_contract_research/` that explain how to design a
single `PreparedMirView` contract that can be produced by both the current BIR
pipeline and a future rewritten BIR pipeline.

The research must answer how MIR targets should consume prepared backend facts
through a narrow, explicit view instead of reaching into the full
`PreparedBirModule`. It should also define what must be stable for old BIR and
new BIR to converge on the same MIR-facing contract.

## Why This Exists

The current BIR/prealloc architecture carries substantial semantic,
publication, proof, route, freshness, and debug state. That contract-first
approach has protected the backend from testcase-shaped fixes, but it has also
made the shared middle layer heavy. The next architecture improvement should
not begin by deleting prepared facts or rewriting BIR in place.

The safer first step is to make the `PreparedBir -> MIR` boundary explicit:

- identify which prepared facts target MIR emission actually requires;
- separate required codegen inputs from optional feature plans and diagnostic
  proof artifacts;
- prevent target MIR code from depending on the entire `PreparedBirModule`;
- give a future BIR rewrite a stable output target;
- allow old BIR and new BIR to produce the same MIR-facing contract during a
  parallel migration.

The key design claim to research is:

```text
Old BIR -> current prepare ------\
                                  -> PreparedMirView -> MIR
New BIR -> new/compat prepare ---/
```

The research should determine what `PreparedMirView` must contain, what it
must deliberately exclude, and how to verify that old and new producers are
contract-equivalent without freezing the current oversized
`PreparedBirModule` as a permanent ABI.

## Research Questions And Required Answer Files

There are seven research questions. The delivery must contain exactly seven
question-answer Markdown files, one for each question, plus one `index.md`.
Each answer file must answer only its assigned question and may link to the
other answer files for supporting context.

1. `01_current_mir_dependencies_on_prepared_bir.md`

   Question: Which fields, helper APIs, lookup paths, and implicit assumptions
   do current MIR targets consume from `PreparedBirModule`?

   Required answer shape:
   - trace the current x86 MIR entry path from backend entry to prepared-module
     consumption
   - identify every direct `PreparedBirModule` field family touched by live MIR
     code
   - distinguish live implementation dependencies from markdown-only legacy
     artifacts
   - mark each dependency as required for codegen, feature-specific,
     diagnostic-only, or accidental
   - state the smallest current dependency set that a first
     `PreparedMirView` must expose

2. `02_prepared_mir_core_view_shape.md`

   Question: What should the required `PreparedMirCoreView` contain, and which
   facts should be excluded from the core view?

   Required answer shape:
   - propose the core view fields and accessors in C++ interface terms
   - explain why each field is required by MIR rather than target-local
     convenience
   - identify which current `PreparedBirModule` fields are not core
   - state whether the core view should expose BIR traversal directly, an
     instruction cursor abstraction, or both
   - list invariants the core view must guarantee before any MIR target runs

3. `03_feature_views_and_optional_contracts.md`

   Question: How should feature-specific contracts such as calls, variadic
   entry, i128/f128 carriers, atomics, intrinsics, inline asm, and object data
   be exposed without making them part of the core view?

   Required answer shape:
   - table each feature contract and the current prepared field family that
     backs it
   - classify each feature as always required, target-dependent, instruction
     dependent, or diagnostic-only
   - propose optional view names and required presence checks
   - state how MIR should fail closed when a required feature view is absent or
     incomplete
   - identify feature contracts that should probably move target-local instead
     of remaining shared-prealloc truth

4. `04_debug_proof_and_diagnostic_boundaries.md`

   Question: Which prepared facts are codegen inputs, verifier facts, route
   proofs, or diagnostic/debug artifacts, and how should `PreparedMirView`
   prevent diagnostic artifacts from becoming semantic authority?

   Required answer shape:
   - define `RequiredFact`, `VerifierFact`, `DiagnosticFact`, and any needed
     intermediate categories
   - classify publication/freshness/provenance/local-array/select-chain route
     families using those categories
   - identify any current target-consumer paths where diagnostic or proof
     artifacts risk influencing emission
   - propose naming or type-system boundaries that make diagnostic-only facts
     observational
   - state how reviewer tooling should reject codegen dependence on
     diagnostic-only facts

5. `05_old_bir_new_bir_equivalence_strategy.md`

   Question: How can old BIR and a future new BIR both produce the same
   `PreparedMirView` contract during a parallel rewrite?

   Required answer shape:
   - describe the old-producer and new-producer architecture without requiring
     MIR changes
   - define what equivalence means at the `PreparedMirView` boundary
   - propose golden dumps, structural comparison, or verifier checks that can
     compare old and new producers
   - identify which current compatibility fields should not leak into the new
     contract
   - state how fallback to the old BIR route should work when the new route is
     incomplete

6. `06_incremental_migration_plan_for_mir_consumers.md`

   Question: What is the least risky migration sequence for moving MIR targets
   from direct `PreparedBirModule` access to `PreparedMirView` access?

   Required answer shape:
   - propose a phased migration that starts with reference-only adapter views
   - identify which x86 entry or lowering surface should be migrated first
   - define compile-time or review-time checks that prevent new direct
     `PreparedBirModule` dependencies from entering MIR
   - state how RV64 and AArch64 should be brought in after x86
   - identify rollback points and proof commands for each phase

7. `07_open_questions_and_followup_implementation_ideas.md`

   Question: Which design decisions must be settled before implementation, and
   which follow-up ideas should be opened after the research?

   Required answer shape:
   - list unresolved design choices with recommended defaults
   - separate discussion-required architecture decisions from narrow
     implementation ideas
   - identify interactions with ideas 589 and 590, especially freshness and
     branch/edge publication authority
   - state which parts of `PreparedBirModule` can be slimmed only after the
     view contract lands
   - recommend the next one to three source ideas, if any

## Required Documentation Output

Create the research documents in:

```text
docs/prepared_mir_view_contract_research/
```

Required files:

- `docs/prepared_mir_view_contract_research/index.md`
- `docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`
- `docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`

`index.md` must link to all seven answer files and summarize the overall
recommended `PreparedMirView` design. It must not replace any required answer
file.

## In Scope

- Read and cite current backend entry, prepared/prealloc, and MIR target
  consumption surfaces.
- Inventory live MIR dependencies on `PreparedBirModule`, especially x86 live
  code versus markdown-only legacy artifacts.
- Design a MIR-facing view contract that can be backed by the current prepared
  module without changing behavior initially.
- Separate core codegen inputs, optional feature views, verifier facts, and
  diagnostic/debug facts.
- Define how old BIR and new BIR producers can be compared at the same
  `PreparedMirView` boundary.
- Recommend a phased migration path and follow-up implementation ideas.

## Out Of Scope

- Implementation changes to BIR, prealloc, MIR, tests, or build files.
- Activating the idea into `plan.md` unless explicitly requested later.
- Rewriting `PreparedBirModule` fields before the research establishes the
  view boundary.
- Rewriting old BIR or building the new BIR architecture directly.
- Changing target ABI classification, freshness authority behavior, branch or
  edge-publication semantics, runtime behavior, expectations, unsupported
  markers, or allowlists.
- Treating the current full `PreparedBirModule` as the permanent new
  `PreparedMirView` ABI.

## Acceptance Criteria

- `docs/prepared_mir_view_contract_research/` contains one `index.md` plus
  exactly one `.md` answer file for each numbered question in
  `## Research Questions And Required Answer Files`.
- The seven answer filenames match the seven required filenames exactly.
- Each answer file answers its assigned question directly and follows its
  `Required answer shape`.
- The number of answer files equals the number of numbered research questions;
  do not merge two questions into one file and do not split one question into
  multiple primary answer files.
- The documents cite concrete code surfaces rather than relying on generic
  architecture claims.
- `01_current_mir_dependencies_on_prepared_bir.md` distinguishes live x86 MIR
  dependencies from markdown-only legacy artifacts.
- `02_prepared_mir_core_view_shape.md` proposes a concrete first-cut
  `PreparedMirCoreView` shape and states its invariants.
- `03_feature_views_and_optional_contracts.md` identifies which feature views
  are optional and how MIR should fail closed when a required view is absent.
- `04_debug_proof_and_diagnostic_boundaries.md` states how diagnostic facts
  are prevented from becoming codegen authority.
- `05_old_bir_new_bir_equivalence_strategy.md` explains how old and new BIR
  producers can be compared at the same view boundary without freezing the
  full current `PreparedBirModule` as permanent ABI.
- `06_incremental_migration_plan_for_mir_consumers.md` provides an ordered,
  low-risk migration sequence and names proof commands or proof categories.
- `index.md` includes a final recommendation table classifying each
  recommended follow-up as documentation, narrow implementation idea, or
  discussion-required architecture work.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject research that treats "same PreparedMirView contract" as "expose the
  entire current `PreparedBirModule` under a new name."
- Reject answer files that do not trace actual MIR consumption code before
  proposing a view.
- Reject a design that lets target MIR code keep unrestricted access to
  `PreparedBirModule` while claiming the interface was narrowed.
- Reject a design that makes diagnostic, proof, route, or local-array
  provenance artifacts semantic authority for codegen without an explicit
  required-fact promotion.
- Reject old/new BIR equivalence claims that rely only on textual dumps while
  ignoring structural identity, value homes, frame layout, moves, and feature
  contracts.
- Reject recommendations that require rewriting BIR before the MIR-facing view
  boundary is established.
- Reject broad implementation work, expectation rewrites, unsupported-marker
  edits, allowlist edits, or runtime-output changes under this research idea.
- Reject testcase-shaped analysis that proves the view on one hand-picked
  function or target while ignoring the intended x86-first, RV64/AArch64-later
  migration path.
- Reject follow-up recommendations that do not separate discussion-required
  architecture decisions from narrow implementation ideas.
