Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Map Stale-Publication Rows

# Current Packet

## Just Finished

Step 3: `Map Stale-Publication Rows` completed as an analysis-only row map for
old prepared source-memory/publication facts that can survive after Route 3 or
Route 5 authority changes.

Stale-publication definition for this step:

- In scope means a prepared `source_memory_access` publication, or copied
  source-memory field sourced from `PreparedFunctionLookups::memory_accesses`,
  remains available while current Route 3/Route 5 authority for the same
  consumer boundary has changed, disappeared, become incomplete, or points at a
  different producer identity.
- Byte-offset-only disagreement is deferred to Step 4 unless it is part of a
  broader stale Route 3/Route 5 authority row. Cross-publication disagreement
  across public surfaces is deferred to Step 5 unless needed to name the stale
  publication boundary.
- Retained prepared compatibility surfaces remain public lookup helpers,
  `PreparedEdgePublication::source_memory_access_status`, copied source-memory
  fields, x86 fallback/error/output behavior, riscv
  `EdgePublicationMoveIntent` diagnostics, helper/oracle status names, route
  debug/printer text, wrapper output, and exact target output.

Stale-publication row map:

| Row | Consumer boundary | Semantic authority | Retained prepared compatibility surface | Expected fail-closed behavior | Current x86/riscv evidence | Proof gap |
| --- | --- | --- | --- | --- | --- | --- |
| SP-1: stale prepared `LoadLocal` source-memory publication remains available after the same x86 statement `LoadLocal` Route 3 record changes or no longer matches result/type/base/slot/address metadata | x86 statement-memory handoff through `find_agreed_route3_load_local_source_memory_access(...)` and `render_agreed_route3_load_local_statement_memory_operand(...)` | Current same-block Route 3 `LoadLocal` record from `route3_build_memory_access_index(...)` / `route3_find_memory_access_record(...)`, plus the exact prepared publication candidate for the same producer block/instruction/result | Prepared `memory_accesses` row, `PreparedEdgePublication::source_memory_access`, copied source-memory fields, local-slot fallback/error strings, and exact x86 output | If a candidate stale publication exists and does not match current Route 3 authority, x86 must return no agreement and avoid silently rendering from stale prepared data. When no Route 3 record or no candidate exists, preserved no-candidate fallback remains compatibility/target policy, not demotion proof. | x86 source shows Route 3 result identity and `route3_load_local_source_memory_matches_publication(...)` are checked before agreement, and a rendered prepared access that differs from the agreed access throws `"local-slot load drifted from agreed Route 3 source-memory publication"`. Joined-branch tests cover positive agreement, missing source-memory, carrier-only drift, incomplete prepared source memory, missing Route 5 source-memory, Route 5/Route 3 source-memory mismatch, and source-producer index mismatch. | No synthetic stale publication test directly mutates an otherwise available prepared `source_memory_access` while current Route 3 authority changes at the same x86 statement boundary and proves stable fallback/error/output behavior. Not demotion-ready. |
| SP-2: stale Route 5 dynamic `LoadLocal` publication source still points at an old Route 3 source-memory identity after the Route 5 edge or producer changes | x86 edge-publication move boundary and selected dynamic `LoadLocal` agreement path via `route5_edge_source_agrees_with_prepared_publication(...)` and the statement-memory Route 3 join | Current Route 5 edge record for predecessor/successor/destination/source/producer, with `MemorySource` status and embedded Route 3 `LoadLocal` source-memory identity | Prepared edge-publication row, source-producer fields, `source_memory_access_status`, exact x86 edge move/render output, and preserved non-`LoadLocal` compatibility path | Dynamic `LoadLocal` Route 5 agreement must be false unless Route 5 and embedded Route 3 memory-source identity agree with the prepared publication. Stale Route 5 or producer-index rows must fail closed instead of authorizing the prepared memory row as shared authority. | x86 closure evidence and tests prove selected dynamic `LoadLocal` Route 5/prepared agreement, missing Route 5 source-memory rejection, Route 5/Route 3 source-memory mismatch rejection, and source-producer index mismatch rejection. Source requires `Route5PublicationStatus::MemorySource`, matching producer labels/instruction index, and `route3_load_local_source_memory_matches_publication(...)` before agreeing for `LoadLocal`. | Current proof is selected-path and route-reachable; it does not enumerate stale prepared rows where the old publication remains available but the current Route 5 edge points to a different natural edge/source value or an otherwise obsolete producer. Wrong-edge/duplicate stale publication shapes remain for later proof and are not demotion evidence. |
| SP-3: riscv dynamic stack-source publication preserves old prepared source-memory fields while Route 5/Route 3 diagnostics report non-agreement | riscv publication consumer boundary: `consume_edge_publication_move_intent(...)`, `route5_edge_source_agrees_with_prepared_publication(...)`, and `route3_source_memory_agrees_with_prepared_publication(...)` | Current Route 5 `MemorySource` record and embedded Route 3 `LoadLocal` memory-source record, passed as diagnostics to the prepared-backed consumer | `EdgePublicationMoveIntent` status, `route5_edge_status`, `route5_edge_source_agrees`, `route3_source_memory_agrees`, prepared dynamic stack-source instruction text, and exact riscv output | The riscv consumer may preserve prepared-backed output and target-local instruction text while marking Route 5/Route 3 agreement false. That is fail-closed for semantic transfer: stale prepared output remains compatibility, not authority. | riscv tests prove agreeing dynamic memory-source output has both agreement booleans true, then mutate the Route 3 memory edge byte offset and incomplete identity so output remains `"lw a1, 12(s2)"` while both agreement booleans become false. A no-Route-5 diagnostic call keeps output stable without claiming agreement. | Current evidence proves diagnostic non-agreement for byte-offset/incomplete Route 3 mutations, but not every stale publication axis: stale producer block/instruction, stale source value id/name, stale base kind, and stale Route 5 wrong-edge rows are not all covered for both output and diagnostic fields. Not demotion-ready. |
| SP-4: prepared publication helper/oracle retains copied source-memory fields after the underlying prepared `memory_accesses` row no longer matches current semantic authority | Helper/oracle boundary around `apply_source_memory_access_fact(...)`, `prepared_edge_publication_source_memory_matches_access(...)`, `copy_source_memory_access_fact(...)`, and public lookup readers | Current Route 3/Route 5 source-memory authority for the producer, when the consumer needs semantic agreement; helper-level self-consistency is not enough | Public `PreparedFunctionLookups::memory_accesses`, `PreparedMemoryAccessLookups`, prepared source-memory status spelling, copied base/slot/pointer/offset/size/alignment/address-space/volatility flags, and duplicate/invalid lookup behavior | Helper/oracle rows can preserve stale prepared facts for compatibility, but any semantic consumer must separately join to current Route 3/Route 5 authority and fail closed on disagreement. A self-consistent stale prepared row must not be counted as agreement. | Prepared lookup helper tests prove source-memory fields are copied, matching prepared access is accepted, offset mismatch is rejected, unnamed source memory is rejected, and duplicate/invalid lookup behavior stays observable. x86/riscv agreement paths perform separate authority joins before treating the row as agreement. | Missing end-to-end stale helper proof where a prepared publication remains internally self-consistent after the current Route 3/Route 5 fact changes, then both x86 and riscv consumers preserve compatibility surfaces while refusing semantic agreement. |
| SP-5: same-block prepared source-producer fallback carries an old `source_access` pointer after the producer/source-memory authority changes | Prepared same-block source-producer helper boundary: `find_prepared_same_block_load_local_source_producer(...)` and callers using `PreparedSameBlockLoadLocalStoredValueSource::source_access` | Current Route 3 `LoadLocal` source identity when the result is consumed as shared memory-source semantics; prepared helper result alone is fallback/compatibility | Prepared frame-slot access matching, source-producer fallback data, store-overlap checks, helper/oracle status behavior, and downstream x86/riscv compatibility output | The helper may continue to expose a prepared source-access pointer, but stale helper state must not satisfy Route 3/Route 5 agreement unless the downstream consumer performs the current-authority join. | Source and Step 1 evidence show the helper reads `memory_accesses` by result-name/position and validates prepared result/frame-slot/overlap conditions. x86/riscv selected agreement paths are separate from this helper and require Route 3/Route 5 joins. | No target-specific stale proof maps this fallback helper to an x86/riscv semantic consumer boundary and shows a stale `source_access` pointer cannot silently become shared source-memory authority. |

Disposition:

- No stale-publication row is demotion-ready.
- Existing x86 evidence is strongest for selected dynamic `LoadLocal` rows that
  naturally reach the Route 3/Route 5 agreement bridges, but it does not cover
  all stale prepared publication axes without synthetic state.
- Existing riscv evidence proves diagnostic false-agreement behavior while
  preserving prepared-backed output for some stale Route 3 mutations; that is
  compatibility preservation, not semantic transfer or demotion evidence.
- Helper/oracle evidence preserves public prepared surfaces and rejects some
  local mismatches, but it does not replace same-consumer Route 3/Route 5
  authority checks.
- Byte-offset drift as a standalone row family remains out of scope for Step 4,
  and cross-publication mismatch remains out of scope for Step 5.

## Suggested Next

Execute Step 4 by mapping byte-offset drift rows: isolate prepared
source-memory offset disagreement from broader stale-publication and
cross-publication mismatch rows, then name each consumer boundary, authority,
expected fail-closed behavior, existing x86/riscv evidence, and proof gap
without claiming `memory_accesses` demotion readiness.

## Watchouts

- This active plan is analysis/proof work only.
- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat x86 no-candidate prepared local-slot fallback as Route 3
  agreement. It is retained compatibility/target policy and keeps stale
  publication rows blocked for demotion.
- Do not reuse SP-3's riscv mutated byte-offset diagnostic as full Step 4
  coverage. It proves one diagnostic stale/non-agreement row while preserving
  output, but standalone offset drift still needs its own map.
- Do not collapse stale Route 5 wrong-edge/source-producer rows into the
  Step 5 cross-publication mismatch family unless the next packet explicitly
  maps disagreement across publication surfaces.
- A supported-path x86/riscv agreement proof is not a public lookup demotion
  proof; stale synthetic rows still block demotion.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
