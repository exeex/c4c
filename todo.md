Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Map Byte-Offset Drift Rows

# Current Packet

## Just Finished

Step 4: `Map Byte-Offset Drift Rows` completed as an analysis-only row map for
prepared source-memory byte-offset disagreement.

Byte-offset drift definition for this step:

- In scope means the prepared `memory_accesses` row, copied
  `PreparedEdgePublication::source_memory_byte_offset`, or target consumer
  operand offset disagrees with the current same-consumer Route 3/Route 5
  memory-source offset while the row is otherwise recognizable as the same
  `LoadLocal` source-memory fact.
- This step isolates offset-only disagreement from broader stale-publication
  rows where producer/source identity changed and from Step 5
  cross-publication rows where public surfaces disagree about which publication
  owns the fact.
- Retained prepared compatibility surfaces remain public lookup helpers,
  copied source-memory offset fields, helper/oracle status names,
  x86 fallback/error/output behavior, riscv `EdgePublicationMoveIntent`
  diagnostics and instruction text, route-debug/printer text, wrapper output,
  exact target output, and baseline behavior.

Byte-offset drift row map:

| Row | Consumer boundary | Semantic authority | Retained prepared compatibility surface | Expected fail-closed behavior | Current x86/riscv evidence | Proof gap |
| --- | --- | --- | --- | --- | --- | --- |
| BO-1: x86 selected `LoadLocal` statement-memory publication has the same producer/result but a prepared `source_memory_byte_offset` that differs from the Route 3 effective byte offset | x86 statement-memory handoff through `find_agreed_route3_load_local_source_memory_access(...)` and `render_agreed_route3_load_local_statement_memory_operand(...)` | Current same-block Route 3 `LoadLocal` record from `route3_build_memory_access_index(...)` / `route3_find_memory_access_record(...)`; x86 computes the effective source offset as `load.byte_offset + route3_access.byte_offset` | Prepared `memory_accesses` row, `PreparedEdgePublication::source_memory_access`, copied source-memory offset/size/align fields, local-slot fallback/error strings, and exact x86 output | If a prepared publication candidate exists but its copied source-memory offset does not equal the current Route 3 effective offset, x86 must return no agreement and must not silently render through the mismatched prepared row. If rendering later uses a different prepared access than the agreed access, it must fail closed with the existing handoff error rather than accepting drift. | x86 source checks `publication.source_memory_byte_offset == route3_effective_byte_offset` inside `route3_load_local_source_memory_matches_publication(...)`. The selected `LoadLocal` Route 5 drift test mutates the selected load offset and expects the prepared-module consumer not to accept the old selected output. Source also throws `"local-slot load drifted from agreed Route 3 source-memory publication"` if the rendered prepared access is not the agreed access. | Current x86 proof exercises a reachable selected-path offset mutation through Route 5 agreement drift, but not a synthetic row where only `PreparedEdgePublication::source_memory_byte_offset` is stale while Route 3 producer/result/base/size/align stay fixed. Not demotion-ready. |
| BO-2: riscv dynamic stack-source publication has the same Route 5 memory-source edge but the embedded Route 3 source-memory byte offset differs from the prepared publication offset | riscv publication consumer boundary: `consume_edge_publication_move_intent(...)`, `route5_edge_source_agrees_with_prepared_publication(...)`, and `route3_source_memory_agrees_with_prepared_publication(...)` | Current Route 5 `MemorySource` edge and embedded Route 3 `LoadLocal` memory-source record; riscv agreement compares `route3_access.byte_offset` to `publication.source_memory_byte_offset` | `EdgePublicationMoveIntent` status, `route5_edge_status`, `route5_edge_source_agrees`, `route3_source_memory_agrees`, prepared dynamic stack-source instruction text, source-memory offset diagnostics, and exact riscv output | The riscv consumer may preserve prepared-backed output and target-local instruction text, but both Route 5 and Route 3 agreement booleans must become false when the offset drifts. Preserved output is compatibility only and must not be counted as shared source-memory authority. | riscv source compares byte offset directly in `route3_source_memory_agrees_with_prepared_publication(...)`. The dynamic memory-source test starts with `"lw a1, 12(s2)"` and both agreement booleans true, mutates `source_memory_access.byte_offset` to `16`, then requires output to stay `"lw a1, 12(s2)"` while `route5_edge_source_agrees` and `route3_source_memory_agrees` are false. | Current proof covers the riscv diagnostic false-agreement row for a Route 3 edge offset mutation, but it does not enumerate stale prepared publication offsets, large-offset target syntax, pointer-base offsets, or every status/output surface. Not demotion-ready. |
| BO-3: helper/oracle copied source-memory offset disagrees with the underlying prepared `memory_accesses` row used as the local mirror | Helper/oracle boundary around `apply_source_memory_access_fact(...)`, `copy_source_memory_access_fact(...)`, and `prepared_edge_publication_source_memory_matches_access(...)` | The current prepared memory-access row is the helper-local mirror source; semantic transfer still requires a separate Route 3/Route 5 authority join at the target consumer | `PreparedFunctionLookups::memory_accesses`, `PreparedMemoryAccessLookups`, `source_memory_byte_offset`, source-memory status spelling, copied base/slot/pointer/size/align/address-space/volatility fields, duplicate/invalid lookup behavior, and prepared-printer/debug text | Helper-level mismatch must fail closed by reporting that the publication does not match the access. The copied offset can remain observable for compatibility, but a self-consistent prepared helper row is not enough to authorize semantic agreement. | Helper source copies `access.address.byte_offset` into `publication.source_memory_byte_offset` and `prepared_edge_publication_source_memory_matches_access(...)` requires offset equality. Helper tests mutate `source_memory_byte_offset` from `4` to `8` and expect `prepared_edge_publication_source_memory_matches_access(...)` to reject the mismatch. | This proves local helper mismatch rejection only. It does not prove x86/riscv consumers preserve their fallback/status/output contracts when a copied helper offset drifts from current Route 3/Route 5 authority. |
| BO-4: prepared same-block load-local stored-value/source helper has a frame-slot byte offset that disagrees with BIR/Route 3 local memory identity | Prepared same-block source helper boundary: `find_prepared_same_block_load_local_stored_value_source(...)`, `find_prepared_recovered_narrow_store_source_for_wide_local_load(...)`, and `prepared_and_bir_same_block_load_local_stored_source_match(...)` | Current BIR/Route 3 same-block store/load source identity and byte offsets for the local memory source | Prepared frame-slot access lookup, recovered/narrow-store source helper output, source-producer fallback data, store-overlap checks, helper/oracle status behavior, and downstream target compatibility output | Offset disagreement between prepared addressing and BIR/Route 3 identity must reject the helper match. The prepared helper may remain as compatibility/fallback data, but it must not silently become shared Route 3 memory-source authority. | Prepared helper tests mutate BIR store/load address byte offsets to `9` and expect recovered narrow-store source lookup to fail closed. They also mutate the prepared addressing offset to `16` and require both the prepared stored-value source and prepared/BIR same-block match to reject the mismatch. | Existing evidence is helper-level and source-identity oriented; it is not tied to a concrete x86/riscv semantic consumer boundary with stable target output and diagnostics. |
| BO-5: target operand rendering would use a prepared local-slot or stack-source offset that differs from the already agreed source-memory access | x86 local-slot operand render boundary and riscv dynamic stack-source render boundary after agreement diagnostics are attached | The agreed Route 3/Route 5 source-memory fact for semantic agreement; target-owned addressing, stack slots, base-plus-offset legality, and instruction spelling remain prepared/target policy | x86 local-slot memory operand text/errors, riscv `"lw ... offset(base)"` text, wrapper output, exact target output, and fallback behavior for no-candidate/no-agreement rows | When agreement exists, the rendered prepared access must be the agreed access. If target rendering would use a different prepared offset, x86 must throw the existing drift error; riscv must expose false agreement diagnostics when Route 3/Route 5 offset authority disagrees while keeping prepared output as compatibility. | x86 source compares the rendered access pointer against the agreed access and throws on drift. riscv tests preserve `"lw a1, 12(s2)"` while marking offset-drift agreement false. Both targets therefore have partial guardrails against silent offset acceptance. | No combined proof mutates target-rendered prepared offsets after a positive agreement while preserving all wrapper/exact-output contracts. Large-offset and target addressing legality remain target-owned and are not demotion evidence. |

Disposition:

- No byte-offset drift row is demotion-ready.
- Existing x86 evidence prevents silent acceptance on the selected
  `LoadLocal` consumer path, but the proof is still selected-path and does not
  cover every synthetic stale prepared-offset row or all target operand
  surfaces.
- Existing riscv evidence proves diagnostic false agreement for one Route 3
  offset mutation while preserving prepared output. That is compatibility
  preservation, not semantic transfer or public lookup demotion evidence.
- Helper/oracle evidence rejects copied publication/access offset mismatches
  and prepared/BIR same-block offset mismatches, but helper self-consistency is
  not enough to replace same-consumer Route 3/Route 5 authority checks.
- Stale producer/source identity remains the Step 3 family, and disagreement
  across publication surfaces remains the Step 5 family.

## Suggested Next

Execute Step 5 by mapping cross-publication mismatch rows: identify
disagreement across public prepared/Route 3/Route 5/riscv/x86 publication
surfaces, then name the consumer boundary, semantic authority, compatibility
mirror behavior, expected fail-closed result, and proof gap without claiming
`memory_accesses` demotion readiness.

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
- Do not treat BO-1 or BO-2 as broad demotion proof. They are selected-path
  guardrails against silent offset acceptance, not complete public lookup
  retirement evidence.
- Do not collapse offset drift into Step 5 unless the mismatch is specifically
  across publication surfaces rather than a same-consumer byte-offset
  disagreement.
- Do not move stack offset, frame-slot offset, base-plus-offset legality,
  large-offset syntax, or target operand policy into BIR. Those remain
  prepared/target-owned compatibility surfaces.
- A supported-path x86/riscv agreement proof is not a public lookup demotion
  proof; stale synthetic rows still block demotion.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
