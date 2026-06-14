Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map Prepared-Only Rows

# Current Packet

## Just Finished

Step 2: `Map Prepared-Only Rows` completed as an analysis-only row map for
prepared-only `PreparedFunctionLookups::memory_accesses` behavior.

Prepared-only definition for this step:

- In scope means the prepared memory-access table, or a prepared publication
  field copied from it, contains a row that has no same-consumer Route 3
  `LoadLocal` semantic authority. Rows where Route 3 exists but disagrees are
  left for stale-publication, byte-offset drift, or cross-publication mismatch
  steps.
- The Route 3 semantic authority candidate remains
  `route3_build_memory_access_index(...)` plus
  `route3_find_memory_access_record(...)` for `LoadLocal`, and, when the
  publication consumer needs edge context, the Route 5 source-memory record
  that carries the same `Route3MemoryAccessRecord`.
- The retained prepared compatibility surfaces remain public
  `PreparedFunctionLookups::memory_accesses`, `PreparedMemoryAccessLookups`,
  result-name/result-id/position lookup helpers,
  `PreparedEdgePublication::source_memory_access_status`,
  `PreparedEdgePublication::source_memory_access`, source-memory copied fields,
  source-memory status names, route-debug/printer/helper text, wrapper output,
  target output, and fallback behavior.

Prepared-only row map:

| Row | Consumer boundary | Route 3 authority candidate | Retained public prepared compatibility surface | Expected fail-closed result | Current evidence | Proof gap |
| --- | --- | --- | --- | --- | --- | --- |
| PO-1: lookup-only memory-access row by position/result name/result id, with no matching Route 3 `LoadLocal` record at the same function/block/instruction/result | Public prepared lookup helpers and any consumer receiving `PreparedFunctionLookups::memory_accesses` directly | Route 3 `LoadLocal` record for the same block/instruction/result identity | `find_prepared_memory_access(...)`, `find_indexed_prepared_memory_access(...)`, `find_unique_indexed_prepared_memory_access_by_result_value_name(...)`, `find_unique_indexed_prepared_memory_access_by_result_value_id(...)`, and duplicate-preserving lookup vectors | The public lookup may remain observable for compatibility, but it must not become sufficient semantic authority for demotion. Unique prepared-only rows are compatibility data only; invalid and duplicate prepared rows must continue to fail closed through null unique lookup or preserved ambiguity. | `backend_prepared_lookup_helper_test.cpp` proves lookup construction, unique row lookup, duplicate-name/id ambiguity, duplicate-vector preservation, and invalid-name rejection. | No x86/riscv supported-path proof shows a prepared-only unique lookup rejected at a Route 3 agreement boundary for every consumer that can still read `memory_accesses`. This row is not demotion-ready. |
| PO-2: prepared publication has `source_memory_access_status == available` and copied source-memory fields sourced from `memory_accesses`, but the consumer has no Route 3 `LoadLocal` record for that producer | Publication helper/oracle boundary: `apply_source_memory_access_fact(...)`, `prepared_edge_publication_source_memory_matches_access(...)`, and BIR/prepared publication identity helpers | Route 3 `LoadLocal` source-memory identity for the same producer instruction/result and memory shape | `PreparedEdgePublication::source_memory_access`, source-memory status spelling, copied base/slot/pointer/offset/size/alignment/address-space/volatility flags | Preserve the prepared publication fields as compatibility, but semantic consumers must not treat them as agreed source memory without Route 3 authority. Helper-level acceptance of a self-consistent prepared publication is not demotion evidence. | Helper coverage proves prepared source-memory fields are copied, matching prepared access is accepted, offset mismatch is rejected, and unnamed source memory is rejected. Route 5/Route 3 identity exposure exists for supported rows. | Missing synthetic proof where prepared publication is internally available but Route 3 authority is absent, with x86 and riscv consumers both rejecting semantic agreement or recording non-agreement without weakening status/output strings. |
| PO-3: x86 selected `LoadLocal` consumer sees prepared local-slot memory access but no agreed Route 3 source-memory publication candidate | x86 handoff boundary: `find_agreed_route3_load_local_source_memory_access(...)` and `render_agreed_route3_load_local_statement_memory_operand(...)` | Same-block Route 3 `LoadLocal` record plus matching prepared source-memory publication candidate | `ConsumedPlans::shared_function_lookups()`, local-slot prepared memory operand fallback, handoff-boundary error strings, exact x86 output | If a publication candidate exists, x86 must not render selected `LoadLocal` through prepared data unless Route 3 and prepared publication agree. Existing no-candidate fallback behavior is retained compatibility/target policy, not proof that `memory_accesses` can be demoted. | x86 joined-branch tests prove positive agreement, missing source-memory rejection, carrier-only drift rejection, incomplete prepared source-memory rejection, missing Route 5 source-memory rejection, Route 5/Route 3 mismatch rejection, and source-producer index mismatch rejection. `module.cpp` explicitly returns the prepared local-slot fallback when no Route 3 record or no publication candidate exists. | No exact prepared-only candidate row proves "prepared access present, publication candidate present, Route 3 absent" fails closed with stable x86 fallback/error behavior. The retained no-candidate fallback also blocks demotion readiness. |
| PO-4: riscv dynamic stack-source consumer sees a prepared publication/source-memory row but no Route 3/Route 5 source-memory authority | riscv publication consumer boundary: `consume_edge_publication_move_intent(...)`, `route3_source_memory_agrees_with_prepared_publication(...)`, and `route5_edge_source_agrees_with_prepared_publication(...)` | Route 5 edge publication with source-memory identity, and the embedded Route 3 `LoadLocal` memory-source record | `EdgePublicationMoveIntent` status fields, dynamic stack-source instruction text, prepared publication source-memory status spelling, exact riscv prepared-module output | The consumer may preserve prepared publication diagnostics and target output, but agreement must remain false or unavailable without Route 3/Route 5 authority. A prepared-only row must not be accepted as shared memory-source semantics. | riscv tests prove dynamic stack-source consumption from shared source-memory authority plus fail-closed behavior for missing shared lookups, missing publication, missing prepared memory access, incomplete prepared source memory, and non-`LoadLocal` source-memory unavailability. | Missing exact prepared-only row where prepared source memory is internally available but Route 3/Route 5 source-memory authority is absent; current evidence does not prove all status fields, output, and diagnostics remain stable in that case. |
| PO-5: same-block scalar source-producer helper finds a prepared `LoadLocal` memory access without a Route 3 memory record | Prepared source-producer helper boundary: `find_prepared_same_block_load_local_source_producer(...)` and callers that use its `source_access` | Route 3 `LoadLocal` source identity for the same producer instruction/result, when used as semantic authority | Prepared source-producer fallback behavior, prepared frame-slot access matching, store-overlap checks, and helper/oracle status behavior | The helper can remain a prepared compatibility/fallback reader, but it cannot be counted as Route 3 memory-source proof. Consumers that need shared authority must join to Route 3/Route 5 separately and fail closed when that join is missing. | Source code shows the helper uses `memory_accesses` by result-name/position and validates prepared result/frame-slot/overlap conditions. Step 1 evidence covers nearby source-producer and memory-source helper tests. | No dedicated prepared-only proof maps this helper to an x86/riscv semantic consumer boundary and shows missing Route 3 authority cannot silently pass as shared memory-source agreement. |

Disposition:

- No prepared-only row is demotion-ready.
- PO-1 is retained public lookup compatibility with helper-level fail-closed
  evidence for invalid and duplicate rows, but not semantic transfer evidence.
- PO-2 through PO-5 are consumer-boundary rows that require additional
  synthetic prepared-only proof before any later implementation or demotion
  candidate can be considered.
- Stale-publication, byte-offset drift, and cross-publication mismatch remain
  explicitly out of scope for this step except where distinguished above.

## Suggested Next

Execute Step 3 by mapping stale-publication rows: identify rows where old
prepared source-memory/publication facts survive after Route 3/Route 5
authority changes, then name the consumer boundary, expected fail-closed
behavior, current x86/riscv evidence, and missing proof without weakening
helper/status/output contracts.

## Watchouts

- This active plan is analysis/proof work only.
- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat x86 no-candidate prepared local-slot fallback as Route 3
  agreement. It is retained compatibility/target policy and keeps PO-3
  blocked for demotion.
- Do not collapse PO-2 through PO-5 into byte-offset drift or
  cross-publication mismatch. Those later rows require disagreement between
  authorities; Step 2 rows are about missing same-consumer Route 3/Route 5
  authority.
- A supported-path x86/riscv agreement proof is not a public lookup demotion
  proof; prepared-only synthetic rows still block demotion.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
