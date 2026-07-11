# Prepared MIR View Contract Research Index

This index completes the Prepared MIR view contract research set. It links the
seven required answer files, summarizes the recommended design at handoff
level, and audits the source-idea acceptance criteria. The numbered answer
files remain the authority for evidence, code-surface citations, and detailed
design tradeoffs.

## Answer Files

| File | Role |
| --- | --- |
| [01_current_mir_dependencies_on_prepared_bir.md](01_current_mir_dependencies_on_prepared_bir.md) | Evidence baseline for current MIR dependencies on `PreparedBirModule`, including the live x86 entry path, cross-target dependency families, and the smallest first-view dependency set. |
| [02_prepared_mir_core_view_shape.md](02_prepared_mir_core_view_shape.md) | Proposed first-cut `PreparedMirCoreView` and `PreparedMirFunctionView` shape, required accessors, excluded prepared fields, traversal strategy, and invariants. |
| [03_feature_views_and_optional_contracts.md](03_feature_views_and_optional_contracts.md) | Optional feature-view contracts for calls, variadic entry, special carriers, atomics, intrinsics, inline asm, object data, and fail-closed presence rules. |
| [04_debug_proof_and_diagnostic_boundaries.md](04_debug_proof_and_diagnostic_boundaries.md) | Required, optional, verifier, proof-certificate, diagnostic, and compatibility fact boundaries, plus reviewer reject rules for diagnostic authority drift. |
| [05_old_bir_new_bir_equivalence_strategy.md](05_old_bir_new_bir_equivalence_strategy.md) | Old/new producer equivalence definition at the `PreparedMirView` boundary, with canonical dumps, structural comparison, verifier comparison, and fallback rules. |
| [06_incremental_migration_plan_for_mir_consumers.md](06_incremental_migration_plan_for_mir_consumers.md) | Adapter-first migration sequence for x86, RV64, and AArch64, including rollback points and proof categories. |
| [07_open_questions_and_followup_implementation_ideas.md](07_open_questions_and_followup_implementation_ideas.md) | Recommended defaults, discussion-required architecture decisions, narrow implementation ideas, interactions with ideas 589 and 590, and follow-up source-idea recommendations. |

## Recommended Design Summary

The recommended path is to introduce a read-only, adapter-backed
`PreparedMirView` family before changing BIR production or deleting prepared
facts. The first implementation should wrap the existing
`prepare::PreparedBirModule` by reference, own only view-scoped derived lookup
caches, and keep public backend behavior stable while MIR target internals
move behind the view.

The required core should be narrow and x86-first:

- module and target identity through `target_profile()` and `target_triple()`;
- BIR names, prepared names, defined-function traversal, globals, and string
  constants through explicit read-only accessors rather than raw module
  authority;
- per-function `PreparedMirFunctionView` access to the BIR function, prepared
  control flow, value locations, stack layout, addressing facts, and
  view-owned prepared lookups;
- instruction cursor bindings that connect BIR instructions to stable
  prepared function, block, and instruction identities.

Feature-specific state should stay out of the core view. Calls, dynamic stack,
frame, storage/regalloc, variadic entry, i128/f128 carriers, atomics,
intrinsics, inline asm, object data, runtime helpers, publication, and source
freshness should be exposed through named feature or source-dependency views
only when the target needs them. Absence or incomplete facts must fail closed
or fall back before MIR lowering observes a partial view.

Diagnostics and proofs should be observational. Verifier reports, proof
certificates, route summaries, completed phases, notes, focus filters, and
rendered debug text may reject or explain a lowering decision, but they must
not supply emitted operands, labels, homes, call ABI resources, provenance, or
feature admission unless a design explicitly promotes the underlying typed
fact into a required or optional feature view.

Old BIR and a future new BIR should be compared at the `PreparedMirView`
boundary, not by cloning the entire current `PreparedBirModule`. Equivalence
requires matching target identity, durable ids or hidden deterministic id
mapping, traversal bindings, core facts, admitted feature facts, publication
and freshness authority, and stable verifier outcomes. Canonical dumps are
supporting evidence; the main proof should be structural comparison plus
verifier comparison, with old-route fallback before MIR lowering for
unsupported or blocking differences.

The lowest-risk migration is adapter-first: add the core adapter, migrate the
x86 internal module-emission path before public APIs, move x86 per-function
consumption and lookup ownership behind the view, add feature-view admission
gates, then add raw dependency gates. RV64 follows after x86 core and
per-function proof is stable. AArch64 comes last because its current lowering
context carries the broadest raw prepared surface.

## Acceptance Audit

| Acceptance item | Result |
| --- | --- |
| Directory contains exactly one `index.md` plus seven numbered answer files. | Satisfied by this directory audit. |
| The seven numbered filenames match the source idea exactly. | Satisfied: files `01` through `07` match the required names linked above. |
| Each numbered file answers only its assigned question. | Satisfied by the answer-file split and summarized roles above. |
| Documents cite concrete backend, prealloc, and MIR code surfaces. | Satisfied in the numbered answer files, especially docs `01`, `04`, `05`, and `06`. |
| Live x86 dependencies are distinguished from markdown-only legacy artifacts. | Satisfied in doc `01`. |
| The core view shape and invariants are concrete. | Satisfied in doc `02`. |
| Optional feature views and fail-closed absence rules are defined. | Satisfied in doc `03`. |
| Diagnostic and proof facts are prevented from becoming codegen authority. | Satisfied in doc `04`. |
| Old/new producer equivalence avoids freezing the full prepared module ABI. | Satisfied in doc `05`. |
| Ordered low-risk migration sequence and proof categories are documented. | Satisfied in doc `06`. |
| Follow-ups are separated into documentation, narrow implementation ideas, and discussion-required architecture work. | Satisfied in doc `07` and the final recommendation table below. |
| Research did not change implementation files, tests, expectations, unsupported markers, allowlists, runtime harness policy, build files, active lifecycle source ideas, or plan history. | Satisfied for this index packet; Step 5 edits are limited to this `index.md` and canonical `todo.md`. |

## Final Recommendation Table

| Follow-up | Classification | Recommended disposition |
| --- | --- | --- |
| Accept this research set as the documentation handoff for the Prepared MIR view contract. | Documentation | Supervisor lifecycle review can use this index plus the seven answer files as the complete acceptance artifact. |
| `PreparedMirView Core Adapter And X86 Entry Migration` | Narrow implementation idea | Open or activate as the first implementation follow-up. It should add the reference-only adapter, keep the public x86 wrapper stable, and migrate the x86 internal module-emission path. |
| `PreparedMirView Equivalence Dump And Comparator MVP` | Narrow implementation idea | Open after or alongside the first adapter slice when a view object exists to dump and compare. Start with old-vs-old structural equality before any new producer route. |
| `PreparedMir Source Dependency And Freshness View Contract` | Discussion-required architecture work | Discuss and scope before implementation because it defines selected freshness, publication, branch source, verifier, and proof-certificate boundaries across ideas 589 and 590. |
| View schema versioning and canonical dump format | Discussion-required architecture work | Settle before golden dump tests or old/new producer comparison become acceptance gates. |
| Stable id policy for a future new producer | Discussion-required architecture work | Settle before a new producer allocates ids that may differ from the old prepared route. |
| Raw `PreparedBirModule` dependency gate for migrated MIR surfaces | Narrow implementation idea | Add after the first migrated x86 surface proves the view can carry ordinary lowering input. Begin with review-time grep or allowlist checks, then move to compile-time checks where practical. |
| RV64 text emitter migration to the core view | Narrow implementation idea | Defer until x86 core and per-function migration proof is stable, then migrate text emission and prepared lookups behind the same view shape. |
| AArch64 view-backed context sibling | Narrow implementation idea | Defer until core and feature-view APIs are stable enough to replace selected raw context fields without deleting the legacy context in one broad patch. |
| Additional evidence or design notes discovered during implementation | Documentation | Record in the implementation idea's `todo.md` or a new focused research idea if the finding changes source intent; do not expand this completed research set silently. |
