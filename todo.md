Status: Active
Source Idea Path: ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Consumer and Authority Inventory

# Current Packet

## Just Finished

Step 1: `Consumer and Authority Inventory` is complete.

Consumer hook point: the narrow x86 hook is
`src/backend/mir/x86/prepared/dispatch.cpp::consume_edge_publication_move_intent(...)`
after it has found the unique prepared row with
`find_unique_indexed_prepared_edge_publication(...)` and before it marks the
intent `Available`. The module-level consumer is
`append_prepared_compare_join_parallel_copy(...)` in
`src/backend/mir/x86/module/module.cpp`, which already treats the intent status
as the decision point and keeps move emission, register spelling, and fallback
policy target-local.

Route 5 authority fields: `Route5CfgEdgePublicationRecord` carries the
predecessor/successor blocks and labels, destination phi/value identity, source
value identity, `source_producer_kind`, producer block label and instruction
index, plus `source_memory_identity_available` and the Route 3
`source_memory_access` record for dynamic `LoadLocal` sources. Lookup helpers
are `route5_build_edge_join_source_index(...)`,
`route5_find_cfg_edge_publication(...)`, and the MIR-facing
`find_bir_cfg_edge_publication_source_identity(...)` /
`BirCfgEdgePublicationSourceIdentity` adapter.

Prepared compatibility surface: preserve public
`PreparedEdgePublicationLookups::publications_by_edge_destination`,
`find_indexed_prepared_edge_publications(...)`,
`find_unique_indexed_prepared_edge_publication(...)`,
`prepare_edge_copy_source_facts(...)`, status names, source-home/source-memory
helpers, move facts, helper/oracle names, and existing x86 intent statuses.

Route 3 dependency: any agreeing dynamic `LoadLocal` publication must require
the existing Route 3 source-memory authority, not raw prepared facts alone.
Prepared rows expose `source_memory_access_status`,
`source_memory_access`, address/base/offset/size/alignment fields, and
producer instruction identity; Route 5 exposes the corresponding
`Route3MemoryAccessRecord`. Step 2 should route `LoadLocal` agreement through
the established x86 Route 3 source-memory agreement helpers instead of
duplicating id comparisons.

Fail-closed matrix for the bridge: missing shared lookups, missing prepared
publication, ambiguous prepared publication, unsupported prepared publication,
missing/unsupported source or destination homes, missing/mismatched
predecessor, successor, destination value/id/type, missing source value,
missing source producer, producer kind mismatch, producer block/instruction
mismatch, prepared-only row, Route 5-only row, duplicate Route 5/prepared rows,
non-agreeing fallback rows, and `LoadLocal` missing/incomplete/mismatched Route
3 source-memory authority must not report agreement.

Focused proof surfaces: `backend_prepared_lookup_helper` already covers the
prepared/BIR/Route 5 source identity oracle, including `LoadLocal` memory-source
records and wrong-edge/status rows; `backend_riscv_prepared_edge_publication`
covers prepared fallback and dynamic `LoadLocal` source-memory behavior outside
x86; `backend_x86_handoff_boundary_joined_branch` contains the x86 shared
edge-publication compare-join consumer plus selected-`LoadLocal` Route 3 proof
rows from ideas 258/261/262; `backend_x86_route_debug` should be checked for
nearby route-debug output preservation. Step 2 can design against these
surfaces without adding implementation code.

## Suggested Next

Step 2: design an x86-local Route 5/prepared agreement facade that accepts the
prepared publication row plus BIR context, builds/queries Route 5 for the same
edge and destination, compares source value and producer identity, and delegates
dynamic `LoadLocal` memory agreement to the existing Route 3 source-memory
agreement authority.

## Watchouts

- Keep this bridge x86-local and narrow.
- Do not rename, delete, privatize, or bypass public prepared
  `edge_publications` helpers.
- Do not move target policy, output spelling, carrier selection, or register
  choice into Route 5/BIR.
- Dynamic `LoadLocal` publication agreement must depend on the existing Route 3
  source-memory agreement authority, not raw id comparisons.
- Reject testcase-shaped shortcuts and expectation downgrades.
- Do not use the RISC-V Route 5 oracle as an x86 implementation template for
  target policy; use it only as compatibility/proof context.
- `consume_edge_publication_move_intent(...)` currently has no BIR function or
  block context parameter, so Step 2 must choose the smallest context shape
  needed to query Route 5 without widening public prepared lookup APIs.

## Proof

Read-only/design packet. No build was required by the delegated packet.
Validation: `git diff --check -- todo.md`.
