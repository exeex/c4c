# Pass-Ready BIR Schema And Legacy Quarantine Research

Status: Open
Type: Research and architecture documentation
After: `ideas/open/714_backend_test_source_reachability_cleanup.md`
Parent: none
Related:
- `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
- `ideas/open/704_bir_semantic_handoff_views.md`
- `ideas/open/713_persistent_backend_c_testsuite_and_bir_internal_test_retirement.md`
- `ideas/open/714_backend_test_source_reachability_cleanup.md`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/`
- `ref/claudes-c-compiler/src/backend/`
Owning Layer: LIR-to-BIR output schema, BIR core IR, BIR analyses, compatibility quarantine, and BIR-to-MIR stage boundaries

## Goal

Produce concrete research documents under `docs/backend/pass_ready_bir/` that
decide whether the BIR emitted by `src/backend/bir/lir_to_bir/` can safely be
the mutable IR for modular IR-to-IR passes, identify exactly what it lacks,
and define a staged schema/API refactor before the post-BIR pipeline is
rewritten as passes.

The research must also define one named, observational
`LegacyBirCompatibilityCapsule` boundary that gathers state we do not want in
future core BIR so later migration can delete that state without another
whole-backend ownership search.

## Why This Exists

LIR is LLVM-like and deliberately higher-level than the reference compiler's
backend IR, so c4c needs an additional lowering and canonicalization stage.
The existing `Module -> Function -> Block -> Inst/Terminator` shape may be a
sound starting point, but its real pass-readiness depends on current identity,
ownership, side-table, pointer, index, name, ABI, provenance, publication, and
mutation assumptions—not on the container hierarchy alone.

Ideas 713 and 714 are intended to remove representation-shaped tests and dead
test sources that currently constrain BIR restructuring.  Before using that
freedom, this research must establish the core IR boundary, quarantine legacy
state, and provide concrete migration packets.  Otherwise the old route and
prepared systems could merely be wrapped in pass-shaped APIs while retaining
the same authority duplication.

## Research Questions And Required Answer Files

There are six research questions.  Delivery must contain exactly six numbered
question-answer Markdown files plus one `index.md`.  Each answer file answers
only its assigned question and may link to the others for context.

1. `01_current_lir_to_bir_schema.md`

   Question: What exact data model and ownership contract does
   `lir_to_bir` emit today, and which facts are authoritative?

   Required answer shape:
   - AST/source-backed inventory of `Module`, `Function`, `Block`, `Inst`, and
     `Terminator`, including construction paths and all owned or referenced
     side tables;
   - inventory of function/block/instruction/value identity, names, raw
     pointers, vector/index coordinates, ABI data, provenance/publication
     records, route records, printer/debug payloads, and lookup indirection;
   - producer/consumer dependency map identifying who mutates each family and
     what append-only, pointer-stability, ordering, or freeze assumptions exist;
   - concrete mutation hazards for instruction insertion/removal/reorder,
     replace-all-uses, block split, edge redirect, function movement, and
     module growth.

2. `02_field_classification_and_quarantine.md`

   Question: Which current fields belong in future core BIR, and how should
   everything else be separated or quarantined?

   Required answer shape:
   - an exhaustive field/family classification table with exactly these
     destinations: future core IR, recomputable analysis, lowering-only input,
     prepared/MIR output, or legacy compatibility/debug state;
   - explicit authority and lifetime rules for each destination;
   - a concrete `LegacyBirCompatibilityCapsule` schema and access boundary
     gathering route-numbered records, agreement/status payloads,
     display-name authority fallbacks, raw pointers, unstable block/inst
     indices, printer-only state, duplicate publication/lookups, and any other
     artifact excluded from future core BIR;
   - a consumer allowlist and measurable deletion guard: no new writers or
     authoritative consumers, monotonically decreasing fields/readers, core
     passes unable to include/access the capsule, and a final zero-field,
     zero-reader checkpoint.

3. `03_pass_ready_bir_contract.md`

   Question: What invariants and APIs must BIR provide before it can safely
   host composable IR-to-IR transformations?

   Required answer shape:
   - required stable `FunctionId`, `BlockId`, `InstId`, and `ValueId`
     semantics, including storage/handle lifetime and iteration-order rules;
   - core node semantic requirements plus builder/mutator API sketches for
     insertion, replacement, erasure, operand/successor traversal,
     replace-all-uses, block split, and edge redirect;
   - CFG source-of-truth rule derived from blocks and terminators, with CFG,
     def-use, dominator, loop, provenance, and liveness treated as
     recomputable results rather than persistent authorities;
   - verifier invariants, analysis invalidation/preservation rules, and safe
     per-function mutation/parallelism while `Module` retains ownership of
     symbols, types, globals, functions, and cross-function facts.

4. `04_reference_backend_comparison.md`

   Question: What should c4c adopt from
   `ref/claudes-c-compiler/src/backend`, and what should it deliberately avoid?

   Required answer shape:
   - source-cited comparison of c4c BIR with reference `IrModule`,
     `IrFunction`, `BasicBlock`, `BlockId`, `Value`, `FlatAdj`, `CfgAnalysis`,
     short-lived analysis contexts, pass execution, and phi elimination;
   - explanation of how the reference keeps module ownership while using
     function/block-index analysis granularity before and after CFG analysis;
   - adopted principles: terminator-derived CFG, persistent semantic IDs,
     dense short-lived analysis indices, disposable contexts, and direct
     function transformations;
   - rejected weaknesses: no stable `InstId`, manual/implicit invalidation,
     position identity leaking beyond one analysis, and target-local mutable
     state becoming an architectural substitute for explicit stage contracts.

5. `05_target_schema_and_api_blueprint.md`

   Question: What concrete file, type, storage, and API layout should replace
   the current mixed BIR interface?

   Required answer shape:
   - proposed source-tree layout separating core schema/storage, builders and
     mutation, verification, analyses, canonical passes, compatibility
     capsule, prepared planning, and MIR lowering;
   - concrete type/API sketches for ID arenas or equivalent stable storage,
     module/function/block/instruction access, traversal, mutation, verifier,
     and analysis result/cache boundaries;
   - an explicit boundary such as
     `lower_lir_to_raw_bir -> canonical BIR pass pipeline -> verified
     CanonicalBir -> preparation/BIR-to-MIR`, without requiring duplicated
     full raw/canonical structs when a verified wrapper/state token suffices;
   - precise stage ownership showing that ABI/frame/register/call-move
     decisions and target instruction selection are prepared/MIR outputs, not
     ordinary canonicalization facts written back into core BIR.

6. `06_staged_migration_and_followups.md`

   Question: How can the target design be reached in reviewable packets while
   retaining behavior and deleting compatibility state?

   Required answer shape:
   - ordered migration packets beginning with schema isolation, stable IDs,
     builder/mutation APIs, verifier, analysis boundary, and
     `lir_to_bir` construction through the new interface;
   - subsequent rewrite order for legalization, CFG/SSA normalization,
     memory/address canonicalization, aggregate/intrinsic lowering,
     out-of-SSA, and eventual allocation/ABI/frame planning into BIR-to-MIR;
   - compatibility adapters and deletion checkpoints for each packet,
     including when capsule fields/readers disappear and when old route or
     prepared ownership becomes unreachable;
   - ordered follow-up implementation-idea proposals with proof boundaries,
     rollback points, dependency ordering after 715, and an explicit
     no-semantic-change first phase rather than a big-bang rewrite.

## Required Documentation Output

Create exactly:

```text
docs/backend/pass_ready_bir/
  index.md
  01_current_lir_to_bir_schema.md
  02_field_classification_and_quarantine.md
  03_pass_ready_bir_contract.md
  04_reference_backend_comparison.md
  05_target_schema_and_api_blueprint.md
  06_staged_migration_and_followups.md
```

`index.md` must link all six numbered files, summarize the decision, list the
proposed invariants and migration packet order, and call out unresolved choices
requiring human approval.  It must not replace any numbered answer.

## Research Method And Evidence

- Use AST/source inventory where practical rather than inferring ownership
  from filenames or tests alone.
- Trace construction from `lir_to_bir` through every direct consumer and
  mutation site; record file/symbol references for all schema classifications.
- Trace identities and coordinates end-to-end, including every conversion
  among IDs, pointers, names, vector positions, route indices, and prepared
  lookup keys.
- Demonstrate mutation hazards with current-code examples and show how the
  proposed API/invariants prevent each hazard.
- Compare the proposed target against both the current c4c pipeline and the
  reference implementation using concrete symbols and data-flow traces.
- Separate confirmed facts, inferences, design choices, and unresolved
  questions.  Do not present a preferred design as current behavior.
- Use the accepted 703--714 results when available; if a predecessor is not
  complete, identify the provisional dependency instead of assuming its
  intended end state already exists.

## In Scope

- Research and architecture documentation for current BIR schema, ownership,
  identities, mutation readiness, analyses, legacy compatibility state, and
  stage boundaries.
- A field-by-field classification, consumer map, invariants, file/API
  blueprint, and reviewable migration/deletion plan.
- Follow-up idea proposals derived from evidence; creating those separate
  source ideas requires a later lifecycle action.

## Out Of Scope

- Implementing IDs, storage, builders, mutators, passes, analyses, verifiers,
  compatibility capsules, or BIR-to-MIR changes.
- Deleting or changing tests, expectations, unsupported markers, allowlists,
  implementation files, runtime behavior, or supported semantics.
- Activating 715, modifying the current `plan.md`/`todo.md`, or changing any
  existing idea.
- Calling regalloc, frame layout, ABI planning, or instruction selection a
  canonical BIR-to-BIR pass without first defining a distinct output/stage.

## Acceptance Criteria

- `docs/backend/pass_ready_bir/` contains one `index.md` plus exactly the six
  numbered answer files above, with every required question answered directly.
- The inventory accounts for every current `lir_to_bir` output field/family,
  construction path, consumer, mutation assumption, and authority claim using
  concrete source evidence.
- Every field/family has exactly one target classification, and no legacy,
  analysis, prepared, MIR, or printer/debug state remains implicitly part of
  the proposed core schema.
- The proposed core provides stable IDs, safe mutation/traversal APIs,
  terminator-derived CFG, recomputable analyses, verification, invalidation,
  raw/canonical boundaries, and safe function-granularity processing under
  module ownership.
- `LegacyBirCompatibilityCapsule` is observational and non-authoritative, is
  inaccessible to new core passes, has an explicit consumer allowlist, and
  has machine-checkable monotonically decreasing and final-zero deletion
  checkpoints.
- The blueprint names target files/types/APIs and the migration plan provides
  reviewable packets, compatibility exits, proof boundaries, and rollback
  points rather than a big-bang rewrite.
- Reference comparison adopts its useful short-lived-analysis model without
  importing unstable instruction identity or manual invalidation as target
  design.
- Research makes no semantic, implementation, test, baseline, or active
  lifecycle changes.

## Reviewer Reject Signals

- Reject a generic pass-manager essay that lacks an exact current-field,
  constructor, consumer, authority, and mutation-hazard inventory.
- Reject wrapping route-numbered records, prepared lookups, agreement/status
  payloads, or existing side-table authority in a class named “pass” while
  preserving the old ownership and failure mode.
- Reject any target core schema that still treats display names, raw pointers,
  block/inst vector positions, printer payloads, or compatibility records as
  stable semantic identity or authority.
- Reject a capsule that remains writable or authoritative, is accessible to
  new core passes, admits new consumers, or lacks measurable field/reader
  deletion gates.
- Reject persistent duplicate CFG, def-use, dominator, loop, provenance, or
  liveness authority instead of terminator-derived and invalidatable analysis
  results.
- Reject a big-bang rewrite without staged adapters, proof boundaries,
  rollback points, and compatibility deletion checkpoints.
- Reject treating regalloc, frame/stack layout, ABI/call planning, or target
  instruction selection as ordinary canonicalization without explicit IR
  stage and output ownership.
- Reject implementation, test deletion, expectation/unsupported/allowlist
  changes, active-plan mutation, broad unrelated redesign, testcase-shaped
  shortcuts, or named-case-only proposals under this research idea.
- Reject helper renames, classification-only changes, or documentation wording
  that claims progress while leaving the exact legacy authority duplication
  behind a new abstraction name.
