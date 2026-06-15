Status: Active
Source Idea Path: ideas/open/281_phase_f5_edge_publication_source_producers_x86_consumer_boundary_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect x86 Source-Producer Consumers

# Current Packet

## Just Finished

Completed Step 1 inspection for the x86
`PreparedFunctionLookups::edge_publication_source_producers` boundary gate.

The x86 surface examined was `src/backend/mir/x86/module/module.cpp`, especially
the prepared-module `edge_publications` consumer and its Route 5 / Route 3
agreement helpers:
`route5_edge_source_agrees_with_prepared_publication`,
`agreed_route5_edge_publication_source`,
`edge_publication_move_allowed_by_route5_agreement_or_compatibility`,
`find_agreed_route3_load_local_source_memory_access`, and
`render_agreed_route3_load_local_statement_memory_operand`. I also inspected
the adjacent selected-`LoadLocal` fixture in
`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp` and the
source-producer lookup/helper construction under `src/backend/prealloc/`.

Finding: no real x86 backend consumer boundary reads
`PreparedFunctionLookups::edge_publication_source_producers` directly. The x86
prepared-module path consumes `lookups->edge_publications.publications` and the
source-producer facts embedded in each `PreparedEdgePublication`; the public
`edge_publication_source_producers` lookup is used by prealloc/helper planning
and classification paths such as select-chain, call/store planning, and
prepared lookup helper tests.

Adjacent support is `LoadLocal`-specific: x86 requires Route 5 agreement for
`LoadLocal` edge-publication sources and checks Route 3 source-memory agreement.
Non-`LoadLocal` source-producer kinds still fall through the compatibility
allowance in `edge_publication_move_allowed_by_route5_agreement_or_compatibility`,
so Step 1 found no direct non-`LoadLocal` public-field consumer proof surface.

## Suggested Next

Execute Step 2 as bounded non-applicability: record that x86 currently has no
real consumer boundary for
`PreparedFunctionLookups::edge_publication_source_producers`, and name the
smallest prerequisite as adding or exposing a true x86 consumer of that public
lookup before any future parity proof. Do not add a focused proof against the
existing `edge_publications` consumer, because that would prove adjacent
embedded-publication facts rather than the public source-producer lookup.

## Watchouts

- Do not open implementation work until a real x86 consumer boundary is named.
- Do not claim helper-only, classification-only, or adjacent
  `edge_publications` coverage as public-field parity for
  `edge_publication_source_producers`.
- Preserve the current `LoadLocal`-only Route 5 agreement support and
  non-`LoadLocal` fallback limitation; the inspected x86 path still permits
  non-`LoadLocal` compatibility when Route 5 agreement is absent.
- Do not weaken status/debug, route-debug, prepared-printer, helper/oracle,
  wrapper, exact-output, fallback, or baseline behavior.

## Proof

Inspection-only packet. No build or tests run, and `test_after.log` was not
touched.
