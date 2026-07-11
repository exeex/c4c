# BIR Route Index Retirement Research

Status: Open
Type: Research and architecture documentation
Parent: `none`
Related:
- `ideas/closed/678_lir_to_bir_adapter_boundary_umbrella.md`
- `ideas/closed/683_prepared_mir_view_contract_research.md`
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
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
Owning Layer: canonical BIR route/index boundary between LIR import and prepared/prealloc

## Goal

Produce concrete research documents under
`docs/bir_route_index_retirement_research/` that explain how to retire the
numbered `bir_route1..8` public surface and replace it with named,
ownership-based BIR views.

The research must decide which route facts are still required now that the
compiler no longer depends on exhaustive intermediate prepared/BIR dump
comparison for correctness. It should distinguish real semantic producer
inputs from historical debug/proof or route-observation artifacts.

## Why This Exists

The `src/backend/bir/lir_to_bir/` adapter layer has been cleaned up, and the
MIR side now has a prepared view boundary. The remaining messy middle is the
older BIR route/index family in `src/backend/bir/*.cpp/.hpp`: route-numbered
producer lookup, select-chain records, memory observations, publication
records, call publication, comparison provenance, return-chain facts, and route
facade validation.

Those route names mostly encode historical discovery order, not durable
ownership. Before rewriting BIR-to-prealloc or revisiting residual stack
destination authority ideas, the project needs to know which route facts should
survive as named semantic views, which should become private implementation
details, and which can be removed because frontend/backend correctness is now
proved through the connected MIR/runtime/object surfaces.

## Research Questions And Required Answer Files

There are seven research questions. The delivery must contain exactly seven
question-answer Markdown files, one for each question, plus one `index.md`.
Each answer file must answer only its assigned question and may link to the
other answer files for supporting context.

1. `01_current_route_inventory.md`

   Question: What does each current `bir_route*.cpp`, route-index header, and
   facade entry point own today?

   Required answer shape:
   - table every route file and route-index public record family
   - classify each as producer lookup, semantic view, prealloc input,
     diagnostic/proof artifact, or compatibility residue
   - identify direct consumers in `src/backend/prealloc/`, prepared printer,
     and MIR/prepared view code

2. `02_required_bir_to_prealloc_inputs.md`

   Question: Which route facts are still required by the BIR-to-prealloc handoff
   when intermediate dump comparison is no longer the primary proof contract?

   Required answer shape:
   - list the minimum required BIR facts for prealloc behavior
   - separate codegen inputs from verifier/debug dump inputs
   - identify facts that should be recomputed locally instead of stored as
     route-index state

3. `03_named_view_replacement_shape.md`

   Question: What named BIR views should replace numbered route APIs?

   Required answer shape:
   - propose first-cut names and C++ ownership boundaries, such as producer,
     memory access, publication, control-flow value, call boundary, and return
     chain views
   - state which current route records belong to each view
   - identify which view should be public, private, or compatibility-only

4. `04_publication_and_authority_boundaries.md`

   Question: How should publication, freshness, destination authority, and
   route-proof records be separated so diagnostic artifacts do not become
   semantic authority?

   Required answer shape:
   - classify route4/route5/route7 interactions with prepared publication,
     freshness view, and residual stack destination authority ideas
   - identify which records may influence codegen and which must remain
     observational
   - state reviewer rules for rejecting authority claims sourced only from
     route-numbered debug records

5. `05_retirement_sequence.md`

   Question: What is the safest behavior-preserving sequence for retiring
   numbered route APIs?

   Required answer shape:
   - propose phased commits that introduce named views before moving consumers
   - name the first low-risk consumer migration
   - identify rollback points and focused proof commands
   - state which routes must remain untouched until later BIR/prealloc rebuild

6. `06_test_and_dump_policy_after_route_retirement.md`

   Question: Which tests or dump contracts should remain after route APIs become
   named views, and which intermediate fact checks should stay removed from the
   default baseline?

   Required answer shape:
   - map current route/prepared fact tests to keep, gate, rewrite, or delete
     decisions
   - explain how MIR/object/runtime proof replaces intermediate route dump
     comparison
   - define when a new route-view test is justified

7. `07_followup_idea_recommendations.md`

   Question: Which follow-up ideas should be opened after the research, and in
   what dependency order?

   Required answer shape:
   - recommend ordered implementation or umbrella ideas
   - separate route facade cleanup, named view extraction, publication
     boundary cleanup, and stack destination authority follow-up
   - state which recommendations are prerequisites for revisiting ideas 647 and
     655

## Required Documentation Output

Create the research documents in:

```text
docs/bir_route_index_retirement_research/
```

Required files:

- `docs/bir_route_index_retirement_research/index.md`
- `docs/bir_route_index_retirement_research/01_current_route_inventory.md`
- `docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`
- `docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`
- `docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`
- `docs/bir_route_index_retirement_research/05_retirement_sequence.md`
- `docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`
- `docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`

`index.md` must link to all seven answer files and summarize the recommended
route-retirement strategy. It must not replace any required answer file.

## In Scope

- Inventory route-numbered BIR APIs and consumers.
- Classify route facts by durable owner and proof role.
- Design named BIR view replacements for the BIR-to-prealloc handoff.
- Define how route dump/proof artifacts should be gated, deleted, or replaced
  now that MIR and runtime/object proof are connected.
- Recommend follow-up ideas and dependency ordering.

## Out Of Scope

- Implementation changes to BIR, prealloc, MIR, tests, or build files.
- Rewriting BIR-to-prealloc directly.
- Reopening residual stack destination authority ideas 647 or 655.
- Changing runtime behavior, target ABI behavior, unsupported markers,
  allowlists, expectations, timeout policy, or baseline acceptance policy.
- Treating numbered route APIs as permanent public architecture.

## Acceptance Criteria

- `docs/bir_route_index_retirement_research/` contains one `index.md` plus
  exactly one `.md` answer file for each numbered question.
- Each answer file answers its assigned question directly and follows its
  required answer shape.
- The documents cite concrete code surfaces and consumers rather than relying
  on generic architecture claims.
- The research clearly states which route facts are required codegen inputs and
  which are diagnostic/proof artifacts.
- The research recommends an ordered follow-up queue and names prerequisites
  for revisiting ideas 647 and 655.
- No implementation files, tests, expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject answer-file count mismatches or collapsed/split numbered questions.
- Reject research that preserves `route1..8` as permanent public names without
  a concrete reason.
- Reject claims that a route fact is semantic authority without tracing its
  producer and consumer.
- Reject treating diagnostic, proof, or dump-only artifacts as codegen inputs.
- Reject implementation edits, expectation rewrites, unsupported-marker
  changes, allowlist changes, runtime-output changes, or baseline-policy
  changes under this research idea.
- Reject testcase-shaped recommendations that focus on residual GCC torture
  filenames instead of BIR route ownership.
