# BIR Route Index Retirement Umbrella Runbook

Status: Active
Source Idea: ideas/open/694_bir_route_index_retirement_umbrella.md

## Purpose

Turn the completed BIR route-index retirement research package into a durable
handoff directory and dependency-ordered follow-up idea queue.

Goal: create `docs/bir_route_index_retirement/` with a research digest,
ownership classification, and ordered follow-up plan, then generate focused
`ideas/open/` follow-up ideas without changing implementation, tests, or
baseline policy.

## Core Rule

This is an umbrella triage plan only. Do not edit implementation files, tests,
expectations, unsupported markers, allowlists, runtime behavior, default
harness contracts, or baseline policy.

## Read First

- `ideas/open/694_bir_route_index_retirement_umbrella.md`
- `docs/bir_route_index_retirement_research/index.md`
- `docs/bir_route_index_retirement_research/01_current_route_inventory.md`
- `docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`
- `docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`
- `docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`
- `docs/bir_route_index_retirement_research/05_retirement_sequence.md`
- `docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`
- `docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`
- `docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`

## Current Targets

Create or update:

- `docs/bir_route_index_retirement/`
- ordered follow-up ideas under `ideas/open/`
- this active lifecycle state through `todo.md` only during execution

## Non-Goals

- Do not implement BIR named views, route facade contraction, prealloc
  migrations, prepared/MIR stack view changes, or test policy changes inside
  this umbrella.
- Do not reactivate ideas 647 or 655 unless the handoff names a concrete
  prerequisite from route-retirement evidence.
- Do not create mixed-owner follow-up ideas that combine BIR view extraction,
  prealloc consumer migration, test policy, and stack destination authority
  repair.
- Do not make route-numbered APIs durable public architecture unless the
  handoff records them as private compatibility during migration.

## Working Model

- Treat the idea 693 research documents as the current evidence source.
- Classify follow-ups by first owning layer before choosing ordering.
- Keep route-numbered BIR APIs as historical or compatibility surfaces while
  follow-up ideas define named ownership boundaries.
- Keep prepared/prealloc destination authority explicit; MIR consumers must not
  rediscover stack destination authority from raw route records.
- Treat parked ideas 647 and 655 as blocked until BIR publication or prepared
  authority evidence exposes a positive producer seam outside the closed idea
  637 contract.

## Execution Rules

- Keep documentation and idea generation separate from implementation.
- Prefer small follow-up ideas with one owning layer, one first consumer or
  producer boundary, and one proof surface.
- Every generated follow-up idea must include concrete reviewer reject signals.
- Preserve source intent in `ideas/open/694_bir_route_index_retirement_umbrella.md`;
  record routine execution state in `todo.md`.
- When closure is requested, the closure note must identify consumed research
  docs, handoff docs written, follow-up ideas generated, ordering rationale,
  route-numbered private compatibility, and the prerequisite for revisiting
  ideas 647 and 655.

## Ordered Steps

### Step 1: Digest Research Inputs

Goal: establish the exact research input set and extract the umbrella decisions
that drive handoff and follow-up ordering.

Primary target:
`docs/bir_route_index_retirement/`

Actions:

- Read all nine research files listed in the source idea.
- Summarize the route inventory, required BIR-to-prealloc inputs, named view
  replacement shape, publication boundaries, retirement sequence, test policy,
  follow-up recommendations, and stack view handoff.
- Identify which research facts supersede stale route-numbered API assumptions.

Completion check:
the handoff has a research digest that names the consumed input files and
records the evidence source for later follow-up ideas.

### Step 2: Classify Ownership And Dependencies

Goal: classify follow-up work by first owning layer and dependency order before
creating new source ideas.

Primary target:
`docs/bir_route_index_retirement/`

Actions:

- Classify candidate work into BIR named view extraction, route facade
  contraction, BIR publication boundary cleanup, prealloc consumer migration,
  prepared/MIR stack view contract, test/dump contract cleanup, and residual
  stack authority prerequisite buckets.
- Record which route-numbered APIs may remain private compatibility during
  migration.
- State why ideas 647 and 655 remain parked unless a positive producer seam is
  exposed by the route/publication boundary work.

Completion check:
the handoff includes an ownership classification and dependency model that
prevents mixed-owner implementation packets.

### Step 3: Write Ordered Follow-Up Plan

Goal: convert the classified evidence into a dependency-ordered queue.

Primary target:
`docs/bir_route_index_retirement/`

Actions:

- Order follow-up families by dependency and blast-radius reduction:
  route facade contraction and named aliases, consumer migration, publication
  boundary cleanup, stack/frame/value-home handoff cleanup, test/dump contract
  cleanup, and residual stack authority revisit only after prerequisite
  evidence exists.
- Name the first proof surface and rollback point for each follow-up family.
- Identify intentionally deferred or unassigned work.

Completion check:
the handoff contains an ordered follow-up plan that matches the priority model
from the source idea.

### Step 4: Generate Focused Open Ideas

Goal: create focused `ideas/open/` follow-up ideas from the handoff queue.

Primary target:
`ideas/open/`

Actions:

- Generate at least the required follow-up families named in the source idea
  unless the handoff proves a better split.
- For each follow-up, name owned files, first owning layer, first consumer or
  producer migration, proof surface, and numbered route APIs that must not
  become new public architecture.
- Include concrete reviewer reject signals in every generated idea.

Completion check:
each generated follow-up idea is narrow, ordered, traceable to the handoff, and
does not mix unrelated owning layers.

### Step 5: Prepare Umbrella Closure

Goal: make the umbrella result auditable and ready for plan-owner close review.

Primary target:
`todo.md`

Actions:

- Record which research docs were consumed.
- Record which handoff documents were written.
- Record which follow-up ideas were generated and how they are ordered.
- Record which route-numbered APIs remain private compatibility.
- Record what must happen before ideas 647 and 655 can be revisited.

Completion check:
`todo.md` contains the closure evidence needed for a later plan-owner close
decision; no implementation, tests, expectations, unsupported markers,
allowlists, runtime behavior, default harness contracts, or baseline policy
were changed by the umbrella.
