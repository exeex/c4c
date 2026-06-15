Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Map Cross-Publication Mismatch Rows

# Current Packet

## Just Finished

Step 5: `Map Cross-Publication Mismatch Rows` completed as an analysis-only map
for disagreement across public prepared, Route 3, Route 5, riscv, and x86
publication surfaces.

Cross-publication mismatch definition for this step:

- In scope means two or more public surfaces name the same apparent
  memory-source publication boundary but disagree about producer identity,
  destination identity, edge ownership, Route 3 source-memory identity,
  availability, or duplicate uniqueness.
- Out of scope means target-owned storage layout, instruction spelling,
  fallback text, stack/register policy, large-offset legality, wrapper output,
  and exact target output. Those remain compatibility/output behavior, not BIR
  ownership.
- Offset-only disagreement stays in Step 4 unless the offset drift also proves
  that different publication surfaces are naming different owners for the same
  fact.
- No row below is demotion-ready; every row keeps
  `PreparedFunctionLookups::memory_accesses` as retained public compatibility
  until same-consumer fail-closed proof exists.

Cross-publication mismatch row map:

| Row | Consumer boundary | Semantic authority | Retained prepared compatibility surface | Expected fail-closed behavior | Current x86/riscv evidence | Proof gap |
| --- | --- | --- | --- | --- | --- | --- |
| CP-1: prepared `memory_accesses`/edge-publication row names a `LoadLocal` source, but Route 3/Route 5 identifies a different producer instruction, producer kind, source value, or destination edge for the same consumer | x86 selected `LoadLocal` statement-memory handoff through `find_agreed_route3_load_local_source_memory_access(...)`, `route5_edge_source_agrees_with_prepared_publication(...)`, and `render_agreed_route3_load_local_statement_memory_operand(...)`; riscv edge move intent through `consume_edge_publication_move_intent(...)` with an optional Route 5 record | Current same-consumer Route 3 `LoadLocal` memory record and Route 5 CFG edge record are semantic authority. Prepared rows are mirrors only after the same producer block, instruction index, source value, destination value, predecessor, successor, and memory-source identity agree. | Public prepared `memory_accesses`, `edge_publications`, copied `source_memory_access` fields, helper/oracle statuses, x86 local-slot fallback/error messages, riscv `EdgePublicationMoveIntent` fields, route-debug/printer text, wrapper output, exact target output, and baseline behavior | x86 must reject or avoid selected output when the prepared row and Route 3/Route 5 authority disagree. riscv may preserve prepared-backed instruction text, but `route5_edge_source_agrees` and, when applicable, `route3_source_memory_agrees` must be false. Helper/oracle APIs may expose the mismatch, but they must not convert it into semantic agreement. | x86 compares producer labels, instruction indices, destination/source values, Route 5 source kind, Route 3 memory identity, and source-memory fields before agreement; selected-path tests cover missing Route 5 source-memory evidence, Route 5/Route 3 source-memory mismatch, and source-producer index mismatch without accepting the old selected output. riscv tests cover scalar Route 5 `NoMatch` while preserving `"mv a1, a0"`, and dynamic memory-source mismatch while preserving `"lw a1, 12(s2)"` with both agreement booleans false. | Proof is still selected-path and diagnostic-path only. It does not exhaust synthetic prepared rows where every public prepared field is internally consistent but points at the wrong current Route 3/Route 5 owner. Not demotion-ready. |
| CP-2: public prepared source-memory helper row and MIR/Route 5 edge lookup disagree about whether the publication has memory-source identity | Helper/oracle boundary around `prepared_edge_publication_source_memory_matches_access(...)`, `prepared_and_bir_cfg_edge_publication_source_identity_match(...)`, `find_bir_cfg_edge_publication_source_identity(...)`, `route5_cfg_edge_publication_record(...)`, and `route5_find_cfg_edge_publication(...)` | MIR/Route 5 derived source identity is authority for BIR publication ownership; the prepared `memory_accesses` row is a compatibility mirror unless it matches the current source-memory access and the Route 5 record exposes `MemorySource` with source-memory identity available | Prepared memory access lookup tables, copied publication source-memory fields, source-memory status names, `BirCfgEdgePublicationSourceStatus`, `Route5PublicationStatus`, missing-source/missing-destination/no-source/no-match statuses, and prepared-printer/debug output | Helper-level mismatch must fail closed as a local mirror mismatch, and MIR/Route 5 lookup must fail closed with explicit missing/no-source/no-match/duplicate statuses. A prepared-only row can remain visible, but it must not be counted as Route 5 memory-source authority. | Helper tests require `prepared_edge_publication_source_memory_matches_access(...)` to reject mismatched source-memory offsets and unnamed source-memory accesses. BIR/Route 5 helper tests require matching load-local memory-source identity, non-memory producers without memory-source identity, missing-source and missing-destination statuses, wrong-predecessor/wrong-successor no-source behavior, and duplicate Route 5 edge diagnostics. | Evidence is mostly helper/oracle level. It does not prove every x86/riscv consumer observes each helper status without silently accepting a prepared-only memory-access row. Blocked for demotion. |
| CP-3: Route 5 index contains duplicate or conflicting publication records for a natural edge while prepared lookup exposes a single indexed publication | Route 5 index lookup and target consumer agreement gates: `route5_build_edge_join_source_index(...)`, `route5_find_cfg_edge_publication(...)`, x86 `route5_edge_source_agrees_with_prepared_publication(...)`, and riscv `route5_edge_source_agrees_with_prepared_publication(...)` | Unique current Route 5 CFG edge source record is authority. Duplicate or conflicting Route 5 rows invalidate agreement even if the prepared index still finds one public publication row. | Prepared indexed-publication lookup, Route 5 duplicate diagnostics, riscv `route5_edge_status`/agreement flags, x86 no-agreement fallback/error behavior, and all emitted target text as compatibility/output behavior | Duplicate or conflicting Route 5 records must block agreement. The preserved prepared publication may stay observable, but target consumers must not treat a single prepared row as enough to authorize memory-source ownership when Route 5 is ambiguous. | Helper tests explicitly create duplicate Route 5 edge records and require diagnostics before riscv authority changes. x86 agreement code rejects multiple matching Route 5 candidates by returning false; riscv scalar tests expose duplicate/mismatch/absence diagnostics and false Route 5 agreement on non-agreeing facts. | No dedicated memory-source target proof covers duplicate Route 5 memory-source records across both x86 and riscv with wrapper/exact-output preservation. Blocked. |
| CP-4: prepared publication edge labels or destination identity disagree with the current Route 5/MIR edge while source-memory facts otherwise look plausible | Consumer boundaries that key by predecessor label, successor label, destination value id/name/type, and source value name: prepared indexed lookup, MIR Route 5 lookup, x86 Route 5 agreement, and riscv intent attachment | Current Route 5/MIR edge key is authority for ownership. Prepared labels and destination ids are compatibility data unless they match the same current edge and destination type. | Public prepared publication index, destination/source value ids and names, missing-publication/no-source/no-match/missing-successor statuses, route-debug/printer text, wrapper output, and target exact output | Wrong edge, wrong destination, or wrong type must fail closed with explicit lookup/status behavior. Target output may preserve prepared fallback where the target already treats Route 5 diagnostics as non-authoritative, but agreement must be false. | Helper tests cover missing destination, missing source, wrong predecessor, wrong successor, and destination type mismatch for Route 5/MIR lookups. riscv tests cover scalar `NoMatch` preserving output with false agreement. x86 requires predecessor/successor/destination/source key agreement before Route 5 source agreement. | Missing combined proof for memory-source-specific wrong-edge/wrong-destination rows on x86 and riscv. Current evidence is enough to block silent agreement, but not enough for `memory_accesses` demotion. |
| CP-5: x86 and riscv expose different consumer behavior for the same non-agreeing memory-source publication: x86 rejects selected output while riscv preserves prepared-backed output with false agreement diagnostics | Cross-target publication consumer boundary: x86 prepared-module emitter for selected `LoadLocal` statement memory versus riscv `EdgePublicationMoveIntent` publication move emission | Shared semantic authority remains Route 3/Route 5 agreement. Target output behavior is target-owned policy: x86 may fail closed by rejecting selected output, while riscv may preserve prepared output if diagnostics clearly report non-agreement. | x86 handoff error strings/no-candidate fallback, riscv instruction text plus `route5_edge_status`, `route5_edge_source_agrees`, and `route3_source_memory_agrees`, wrapper/exact target output, and baseline behavior | Cross-target behavior mismatch is acceptable only as compatibility policy. It must never be used to move target-owned output policy into BIR or to claim the public prepared memory lookup can be hidden. | x86 selected-path tests reject missing/incomplete/mismatched Route 5/Route 3 source-memory evidence; riscv dynamic memory-source tests preserve `"lw a1, 12(s2)"` while setting agreement booleans false for mismatched/incomplete Route 3 evidence. | There is no single cross-target proof matrix proving equivalent status/fallback contracts for all mismatch classes. This row remains a blocker against demotion or private-accessor work. |

Disposition:

- No cross-publication mismatch row is demotion-ready.
- Existing x86 evidence blocks silent selected-output acceptance for several
  reachable Route 5/Route 3 disagreement rows, but it is not a complete public
  prepared lookup retirement proof.
- Existing riscv evidence proves preserved output plus false agreement
  diagnostics for scalar Route 5 mismatch and dynamic memory-source Route 3
  mismatch/incomplete rows. That is compatibility preservation, not semantic
  transfer.
- Helper and MIR/Route 5 evidence names explicit fail-closed statuses for
  missing, wrong-edge, wrong-destination, duplicate, and local source-memory
  mismatches, but helper status proof alone is not a same-consumer x86/riscv
  proof.
- Target-owned policy and output behavior must stay out of BIR ownership.
  Differences between x86 rejection and riscv preserved output are acceptable
  compatibility contracts only when agreement diagnostics fail closed.

## Suggested Next

Execute Step 6 by consolidating the prepared-only, stale-publication,
byte-offset drift, and cross-publication mismatch maps into a final bounded
disposition. The next packet should name which rows have selected-path or
helper-level proof, which remain blocked, whether any follow-up implementation
idea is safe to create, and whether closure/deactivation is appropriate without
claiming `memory_accesses` demotion readiness.

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
- Do not treat CP-1 through CP-5 as demotion proof. They show blockers and
  fail-closed expectations, not readiness to hide public `memory_accesses`.
- Keep target-owned policy and output behavior out of BIR ownership. x86
  rejection and riscv preserved output can both be valid compatibility
  behavior when Route 3/Route 5 agreement is false.
- Do not collapse helper/oracle proof into target proof; a helper status is not
  sufficient unless the same x86/riscv consumer boundary is exercised.
- Do not move stack offset, frame-slot offset, base-plus-offset legality,
  large-offset syntax, or target operand policy into BIR. Those remain
  prepared/target-owned compatibility surfaces.
- A supported-path x86/riscv agreement proof is not a public lookup demotion
  proof; stale synthetic rows still block demotion.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
