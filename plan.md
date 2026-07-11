# BIR Route Index Retirement Research Runbook

Status: Active
Source Idea: ideas/open/693_bir_route_index_retirement_research.md

## Purpose

Produce the research package that decides how to retire the numbered
`bir_route1..8` public surface in favor of named ownership-based BIR views.

Goal: write `docs/bir_route_index_retirement_research/` with one index and
exactly eight question-answer files that classify current route facts, required
BIR-to-prealloc inputs, named replacement views, publication authority
boundaries, retirement sequencing, test policy, follow-up ideas, and the stack
view handoff.

## Core Rule

This is a research and architecture-documentation plan only. Do not edit
implementation files, tests, expectations, unsupported markers, allowlists,
runtime behavior, baseline policy, active lifecycle history, or `ideas/closed/`.

## Read First

- `ideas/open/693_bir_route_index_retirement_research.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route1.cpp`
- `src/backend/bir/bir_route2.cpp`
- `src/backend/bir/bir_route3_memory.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir_route5_publication.cpp`
- `src/backend/bir/bir_route6_call_publication.cpp`
- `src/backend/bir/bir_route7_comparison.cpp`
- `src/backend/bir/bir_route8.cpp`
- `src/backend/bir/bir_route_facade.cpp`
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_index_prereqs.hpp`
- `src/backend/prealloc/`
- `src/backend/mir/prepared_view.hpp`

## Current Targets

Create these files:

- `docs/bir_route_index_retirement_research/index.md`
- `docs/bir_route_index_retirement_research/01_current_route_inventory.md`
- `docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`
- `docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`
- `docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`
- `docs/bir_route_index_retirement_research/05_retirement_sequence.md`
- `docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`
- `docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`
- `docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md`

## Non-Goals

- Do not implement BIR, prealloc, MIR, route facade, or test changes.
- Do not rewrite BIR-to-prealloc directly.
- Do not reopen residual stack destination authority ideas 647 or 655.
- Do not change dump tests, runtime output, unsupported markers, allowlists,
  expectations, timeout policy, or baseline acceptance policy.
- Do not preserve route-numbered APIs as permanent public architecture unless
  the research documents a concrete compatibility reason.

## Working Model

- Treat numbered route APIs as historical discovery-order surfaces until the
  research proves otherwise.
- Separate codegen inputs from diagnostic, proof, dump, and route-observation
  artifacts.
- Treat publication, freshness, and destination authority as explicit producer
  facts, not as authority inferred from route-numbered debug records.
- Keep MIR consumption fail-closed: MIR should consume explicit prepared stack
  facts and should not rediscover stack destination authority from raw route
  records.

## Execution Rules

- Each numbered answer file must answer only its assigned question.
- `index.md` must link all eight answer files and summarize the recommended
  route-retirement strategy without replacing any answer file.
- Cite concrete code surfaces and consumers for every ownership or authority
  claim.
- If a route fact is labeled semantic authority, trace its producer and
  consumer.
- If implementation work looks tempting, record it as a follow-up
  recommendation in `07_followup_idea_recommendations.md`; do not edit code.
- Proof for this plan is documentation-shape and source-reference inspection,
  not backend behavior changes.

## Ordered Steps

### Step 1: Inventory Current Route Ownership

Goal: document what each route file, route-index header, and facade entry
point owns today.

Primary target:
`docs/bir_route_index_retirement_research/01_current_route_inventory.md`

Actions:

- Inspect every `src/backend/bir/bir_route*.cpp` file, route-index header, and
  facade entry point named by the source idea.
- Table each route file and public route-index record family.
- Classify each item as producer lookup, semantic view, prealloc input,
  diagnostic/proof artifact, or compatibility residue.
- Identify direct consumers in `src/backend/prealloc/`, prepared printers, and
  MIR/prepared view code.

Completion check:
`01_current_route_inventory.md` contains the required table, classifications,
and concrete consumer references.

### Step 2: Separate Required BIR-To-Prealloc Inputs

Goal: decide which route facts are still required by the BIR-to-prealloc
handoff after intermediate dump comparison stops being the main proof
contract.

Primary target:
`docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`

Actions:

- List minimum required BIR facts for prealloc behavior.
- Separate codegen inputs from verifier/debug dump inputs.
- Identify facts that should be recomputed locally instead of stored as
  route-index state.

Completion check:
`02_required_bir_to_prealloc_inputs.md` clearly distinguishes codegen inputs,
debug/proof artifacts, and recomputable facts.

### Step 3: Propose Named BIR View Replacements

Goal: replace route-numbered public concepts with named ownership-based view
boundaries.

Primary target:
`docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`

Actions:

- Propose first-cut names and C++ ownership boundaries for producer, memory
  access, publication, control-flow value, call boundary, and return-chain
  views.
- Map current route records into those views.
- Mark each view as public, private, or compatibility-only.

Completion check:
`03_named_view_replacement_shape.md` gives named replacement views and does not
leave `route1..8` as the durable public model.

### Step 4: Classify Publication And Authority Boundaries

Goal: prevent diagnostic route records from becoming semantic authority.

Primary target:
`docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`

Actions:

- Classify route4, route5, and route7 interactions with prepared publication,
  freshness view, and residual stack destination authority ideas.
- Identify records that may influence codegen and records that must remain
  observational.
- Write reviewer rules for rejecting authority claims sourced only from
  route-numbered debug records.

Completion check:
`04_publication_and_authority_boundaries.md` separates publication,
freshness, destination authority, and route-proof records with explicit reject
rules.

### Step 5: Define The Retirement Sequence

Goal: produce a behavior-preserving order for retiring numbered route APIs.

Primary target:
`docs/bir_route_index_retirement_research/05_retirement_sequence.md`

Actions:

- Propose phased commits that introduce named views before moving consumers.
- Name the first low-risk consumer migration.
- Identify rollback points and focused proof commands.
- State which routes must remain untouched until later BIR/prealloc rebuild.

Completion check:
`05_retirement_sequence.md` contains a concrete sequence with rollback points,
proof commands, and deferred high-risk routes.

### Step 6: Define Test And Dump Policy

Goal: decide which route/prepared fact checks survive after route APIs become
named views.

Primary target:
`docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`

Actions:

- Map current route/prepared fact tests to keep, gate, rewrite, or delete
  decisions.
- Explain how MIR/object/runtime proof replaces intermediate route dump
  comparison.
- Define when a new route-view test is justified.

Completion check:
`06_test_and_dump_policy_after_route_retirement.md` states the post-retirement
test policy without reintroducing default intermediate dump comparison as the
main correctness proof.

### Step 7: Recommend Follow-Up Ideas

Goal: produce the dependency-ordered follow-up queue that should come after
this research.

Primary target:
`docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`

Actions:

- Recommend ordered implementation or umbrella ideas.
- Separate route facade cleanup, named view extraction, publication boundary
  cleanup, and stack destination authority follow-up.
- State prerequisites for revisiting ideas 647 and 655.

Completion check:
`07_followup_idea_recommendations.md` names ordered follow-up ideas and makes
the prerequisites for ideas 647 and 655 explicit.

### Step 8: Define Stack View And Destination Authority Handoff

Goal: decide where stack, frame, value-home, and destination-authority analysis
belongs.

Primary target:
`docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md`

Actions:

- State responsibilities for BIR semantic views, prealloc or prepared
  producers, and MIR consumers.
- Propose first-cut frame layout, value home, move bundle, and destination
  authority view shapes that MIR may consume.
- Explain how route publication facts may feed stack authority production
  without becoming direct MIR-side inference.
- Identify minimum positive producer evidence needed before ideas 647 or 655
  can resume.
- State fail-closed behavior when stack destination fan-in lacks explicit
  prepared authority.

Completion check:
`08_stack_view_and_destination_authority_handoff.md` defines the handoff and
keeps MIR from inferring destination authority from raw route records.

### Step 9: Assemble Index And Verify Research Package

Goal: finish the required documentation package and check it against the
source idea acceptance criteria.

Primary target:
`docs/bir_route_index_retirement_research/index.md`

Actions:

- Link all eight answer files from `index.md`.
- Summarize the recommended route-retirement strategy.
- Verify the directory contains exactly one `index.md` plus the eight numbered
  answer files required by the source idea.
- Check that each answer file answers only its assigned question and cites
  concrete code surfaces.

Completion check:
The research directory contains the exact required file set, the index links
all answer files, and the package satisfies the source idea acceptance
criteria without implementation changes.
