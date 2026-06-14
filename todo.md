Status: Active
Source Idea Path: ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Agreement Facade Design

# Current Packet

## Just Finished

Step 2: `Agreement Facade Design` is complete.

Selected contract: keep the agreement bridge x86-local and split at the
existing ownership boundary. `append_prepared_compare_join_parallel_copy(...)`
should own the BIR context because it already has the prepared module
function, control-flow edge, prepared value-location function, predecessor
label, successor label, and parallel-copy destination. It should build/query
Route 5 for the current function and pass an agreement decision into the
existing prepared-dispatch consumer. Do not widen public prepared lookup APIs.

Smallest context shape: add a narrow module-local facade, e.g.
`find_agreed_route5_edge_publication_source(...)`, that accepts the prepared
module, consumed plans, BIR function, predecessor label id, successor label id,
destination prepared value id, and the selected
`PreparedEdgePublication` row. The facade may resolve predecessor/successor BIR
blocks and destination value identity locally, then query
`route5_build_edge_join_source_index(...)` /
`route5_find_cfg_edge_publication(...)` or the MIR
`find_bir_cfg_edge_publication_source_identity(...)` adapter. The prepared
dispatch helper should continue to find the unique prepared row and render the
move intent/status; the new agreement result is only a gate before intent
`Available`.

Required equality checks: agreement requires the same predecessor label/id,
same successor label/id, same destination value id/name/type, same source value
id/name/type, same source producer kind, same producer block label/id, and same
producer instruction index. Immediate, param, same-module global, pointer-backed
global, trailing-immediate, and selected `LoadLocal` rows should only agree
when Route 5/BIR and the prepared publication describe the same semantic source
for the same edge and destination. The facade must not compare unrelated raw id
domains as authority when a resolved prepared/BIR identity is available.

Dynamic `LoadLocal` branch: when Route 5 reports
`Route5PublicationSourceKind::LoadLocal`, the facade must require Route 5
source-memory identity to be available and delegate memory agreement to the
existing Route 3 source-memory authority used by the x86 Route 3 bridge. The
accepted row must tie the Route 5 producer block/instruction/source value to
the selected prepared publication and then require Route 3/prepared memory
agreement for base slot, address space, volatility, size, alignment, and
supported base-plus-offset addressing. Prepared source-memory facts alone are
not sufficient, and Route 5 producer identity without Route 3 source-memory
agreement is not sufficient.

Fail-closed matrix: missing shared lookups, absent prepared publication,
duplicate/ambiguous prepared publication, absent Route 5 record, duplicate or
ambiguous Route 5 record, wrong predecessor, wrong successor, wrong destination
value/id/type, missing source value, source value mismatch, missing source
producer, unsupported producer kind, producer kind mismatch, producer
block/instruction mismatch, prepared-only row, Route 5-only row, unsupported
publication, unsupported source/destination home, existing fallback rows,
non-agreeing fallback rows, dynamic `LoadLocal` missing source-memory
identity, incomplete prepared source-memory publication, and Route 3 memory
disagreement all return non-agreement and must not produce an x86 agreement
claim.

Compatibility retained: public
`PreparedEdgePublicationLookups::publications_by_edge_destination`,
`find_indexed_prepared_edge_publications(...)`,
`find_unique_indexed_prepared_edge_publication(...)`,
`prepare_edge_copy_source_facts(...)`, prepared lookup/status names,
helper/oracle names, x86 intent status spelling, fallback behavior, register
choice, carrier choice, output instruction text, module output formatting,
route-debug text, and RISC-V diagnostics remain compatibility authority. The
Route 5 bridge may only gate agreement in the x86 compare-join edge-publication
consumer; it must not move formatting or target policy into Route 5/BIR.

## Suggested Next

Step 3: implement the narrow x86 agreement bridge by adding the module-local
facade described above, passing its decision into the prepared
edge-publication move intent path, and keeping non-agreeing rows on the
existing compatibility fallback path.

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
- Keep BIR function/block and Route 5 query context in
  `append_prepared_compare_join_parallel_copy(...)` or a nearby x86 module
  helper; avoid widening public prepared lookup APIs.
- Do not make `consume_edge_publication_move_intent(...)` responsible for BIR
  block discovery or target policy beyond its current prepared row/status and
  operand rendering role.
- The `LoadLocal` branch must reuse or closely wrap the existing x86 Route 3
  source-memory agreement authority; do not recreate the rejected raw-id
  comparison path from idea 258.

## Proof

Read-only/design packet. No build was required by the delegated packet.
Validation: `git diff --check -- todo.md`.
