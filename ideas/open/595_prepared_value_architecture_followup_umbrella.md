# Prepared Value Architecture Follow-Up Umbrella

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `none`
Handoff Directory: `docs/prepared_value_architecture_followup_umbrella/`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/open/591_prepared_mir_view_contract_research.md`
- `ideas/open/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/open/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`

## Goal

Use the six recently identified architecture improvement directions, plus the
closure notes from the freshness and branch-publication idea chain, to classify
remaining prepared-value architecture work and generate ordered follow-up ideas
under `ideas/open/`.

This umbrella must not implement any repair directly. Its job is to decide
which remaining problems are already covered by open ideas, which should be
absorbed into the Prepared MIR view research line, and which require new narrow
source ideas.

## Why This Exists

The recent closed idea chain moved the system from local reuse heuristics
toward explicit prepared value freshness authority:

- idea 587 introduced the first-class freshness authority MVP;
- idea 588 migrated and inventoried shared-prealloc source consumers;
- idea 589 settled direct edge-publication move freshness ownership;
- idea 590 settled the branch stack-load freshness contract;
- idea 592 is actively extending branch stack-source producer publication;
- ideas 593 and 594 are queued as RV64 consume-side follow-ups.

That progress changed the shape of the remaining architecture debt. The older
six-point assessment is still useful, but some items are now partially solved,
some are active or queued, and some should become research or implementation
ideas only after their first owning layer is clear.

Direct implementation from the six-point list would be premature because it
would risk mixing publication producers, target consumers, address-value
modeling, MIR interface design, and diagnostic policy in one route. This
umbrella exists to classify that evidence first and prevent route drift.

## Current Evidence

The umbrella must start from these six improvement directions:

1. Prepared publication model remains incomplete.
2. Move-bundle authority design had been insufficient.
3. Value-home, preservation, and rematerialization priority had been brittle.
4. Pointer/address arithmetic and local-memory contract boundaries remain
   blurry.
5. Call ABI is healthier, but post-call value publication can still expose
   lifetime and freshness issues.
6. Diagnostic narrowing can accidentally replace capability closure.

It must then re-evaluate those directions against the current lifecycle state:

- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- active or latest `plan.md` / `todo.md` state for
  `ideas/open/592_typed_aggregate_branch_stack_source_publication.md`
- queued `ideas/open/593_rv64_branch_stack_source_freshness_consumption.md`
- queued `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/open/591_prepared_mir_view_contract_research.md`

Fresh closure notes and active lifecycle files supersede older chat summaries
or stale open-idea references.

## In Scope

- Create or refresh handoff documents under
  `docs/prepared_value_architecture_followup_umbrella/`.
- Classify each of the six improvement directions as:
  - already substantially addressed;
  - active in the current 592/593/594 branch freshness queue;
  - belongs inside the 591 Prepared MIR view research line;
  - needs a new narrow implementation idea;
  - needs a new research idea before implementation;
  - intentionally deferred with a concrete reason.
- Distinguish first owning layer for each remaining problem:
  shared-prealloc producer publication, shared-prealloc consumer authority,
  target RV64 consumption, target-independent MIR view contract, pointer/address
  semantic model, call-boundary freshness, or diagnostics/reviewer policy.
- Generate ordered follow-up ideas under `ideas/open/` for any remaining work
  not already covered by 591 through 594.
- Record dependency rules so follow-up ideas do not start before their
  producer, consumer, or research prerequisites are complete.

## Out Of Scope

- Implementing source, test, backend, or diagnostic changes inside this
  umbrella.
- Switching or interrupting the active 592 plan unless the supervisor
  separately decides to change lifecycle state.
- Rewriting the 591 Prepared MIR view research idea unless this umbrella
  explicitly concludes that a small source-intent repair is required.
- Folding RV64 consume-side migration into shared-prealloc producer repair.
- Mixing pointer/address semantic-model work with target branch-stack
  freshness consumption in one follow-up.
- Expectation rewrites, unsupported-marker edits, allowlist edits, runtime
  behavior changes, or weaker tests as proof of progress.

## Priority Model

Order generated follow-up ideas by first owning layer and dependency pressure:

1. Preserve the active 592 -> 593 -> 594 branch stack-source queue unless fresh
   evidence proves it is blocked or mis-scoped.
2. Prefer shared producer/publication authority before target consumer
   migration.
3. Prefer target-independent prepared/MIR view research before broad MIR
   consumer rewrites.
4. Split pointer/address semantic-model questions from branch freshness unless
   closure evidence proves the same owner and proof surface.
5. Treat call-boundary and post-call publication issues as freshness authority
   follow-ups only when concrete evidence shows a remaining stale-home or
   rematerialization gap not already handled by 587.
6. Treat diagnostic-only work as insufficient unless it creates a reviewer,
   verifier, or dump boundary that prevents diagnostic facts from becoming
   semantic authority.

## Required Handoff Documents

The umbrella must produce these documents:

- `docs/prepared_value_architecture_followup_umbrella/index.md`
- `docs/prepared_value_architecture_followup_umbrella/01_six_point_reassessment.md`
- `docs/prepared_value_architecture_followup_umbrella/02_current_open_queue_mapping.md`
- `docs/prepared_value_architecture_followup_umbrella/03_followup_idea_backlog.md`
- `docs/prepared_value_architecture_followup_umbrella/04_dependency_and_priority_order.md`

`01_six_point_reassessment.md` must evaluate all six improvement directions
against the current closed/open idea evidence.

`02_current_open_queue_mapping.md` must state what 591, 592, 593, and 594
already cover and what they deliberately do not cover.

`03_followup_idea_backlog.md` must list every generated or intentionally
deferred follow-up, with owning layer and reason.

`04_dependency_and_priority_order.md` must state the recommended execution
order and prerequisites.

## Required Follow-Up Idea Families

Unless fresh evidence proves a better split, the umbrella must decide whether
to generate or explicitly decline these follow-up families:

- Prepared publication remaining-family backlog after 592/593/594.
- Pointer/address value semantic model research or implementation split.
- Call-boundary post-call publication and rematerialization gap follow-up.
- Diagnostic/proof artifact boundary follow-up, especially where prepared
  dumps, route proofs, or diagnostics could be mistaken for codegen authority.
- Prepared MIR view integration amendments, if 591 needs explicit references
  to freshness, pointer/address, or diagnostic-boundary findings.

Each generated follow-up idea must name its owning layer and must not mix
producer repair with target consumer migration unless the handoff documents
justify that the first owner is truly shared.

## Acceptance Criteria

- The handoff directory contains all four required numbered documents plus
  `index.md`.
- The six improvement directions are each classified with current evidence,
  current status, owning layer, and recommended disposition.
- The documents explicitly account for open ideas 591, 592, 593, and 594 so
  the generated backlog does not duplicate already scheduled work.
- Follow-up ideas are generated under `ideas/open/` for remaining work that is
  ready for durable source intent.
- Each generated follow-up idea names its owning layer, prerequisites,
  acceptance criteria, and reviewer reject signals.
- The priority document states which follow-up should run immediately after the
  current active queue, and which ones must wait for 592, 593, 594, or 591.
- The umbrella makes no implementation, test expectation, unsupported marker,
  allowlist, runtime behavior, or default harness changes.

## Closure Note Requirements

The closure note must state:

1. Which closed ideas and active/open lifecycle files were used as evidence.
2. How each of the six improvement directions was classified.
3. Which existing open ideas already cover parts of the six-point list.
4. Which new follow-up ideas were created, in dependency order.
5. Which candidate follow-ups were explicitly declined or deferred, and why.
6. Whether 591 needs amendment, should remain unchanged, or should wait for the
   branch freshness queue to close.
7. Which follow-up is recommended as the next lifecycle activation after the
   active 592/593/594 queue reaches a stable handoff point.

## Reviewer Reject Signals

- Reject direct implementation inside this umbrella idea.
- Reject output that merely repeats the six-point list without rechecking
  current closure notes and open lifecycle state.
- Reject follow-up ideas that duplicate 592, 593, or 594 instead of referencing
  them as prerequisites.
- Reject broad umbrella follow-ups that mix shared-prealloc producer repair,
  RV64 target consumption, Prepared MIR view design, and pointer/address model
  work in one implementation idea.
- Reject any follow-up that claims capability progress through diagnostics,
  expectation rewrites, unsupported-marker changes, allowlist filtering, or
  weaker runtime checks.
- Reject testcase-shaped or named-case-only shortcuts in generated ideas.
- Reject classifying diagnostic or proof artifacts as semantic codegen
  authority without a fail-closed verifier or reviewer boundary.
- Reject retaining the same stale-home, stack-home-only, or target-local
  inference failure mode behind a new abstraction name.
