Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map Duplicate Same-Edge Route 5 Records

# Current Packet

## Just Finished

Step 2: `Map Duplicate Same-Edge Route 5 Records` completed the duplicate
same-edge Route 5 row map for the active `edge_publications` proof map.

Duplicate same-edge row map:

| Row | Consumer boundary | Semantic authority | Retained compatibility surface | Expected fail-closed result | Current evidence | Proof gap / disposition |
| --- | --- | --- | --- | --- | --- | --- |
| Route 5 index contains two `Route5CfgEdgePublicationRecord` entries for the same predecessor label/id, successor label/id, and destination value identity. | BIR/MIR Route 5 helper boundary: `bir::route5_build_edge_join_source_index(...)`, `bir::route5_find_cfg_edge_publication(...)`, and `mir::find_bir_cfg_edge_publication_source_identity(...)`. | Route 5/BIR publication identity for the natural edge: predecessor, successor, destination value, source value, source producer kind/instruction/index, and optional Route 3 source-memory identity for `LoadLocal`. | Public prepared lookup remains the compatibility mirror: `PreparedFunctionLookups::edge_publications`, `PreparedEdgePublicationLookups::publications_by_edge_destination`, and `find_unique_indexed_prepared_edge_publication(...)`. | Duplicate Route 5 authority must not be silently accepted as a selected semantic agreement. It should stay diagnostic/fail-closed at the Route 5/BIR index boundary until a target consumer proves the duplicate cannot drive emitted output. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` manually duplicates `route5_edge_index.edge_records.front()` and verifies the index exposes the extra duplicate record. The same test proves adjacent non-duplicate Route 5 lookup statuses for available memory-source, missing source producer, destination type mismatch (`no_match`), wrong predecessor (`no_source`), and wrong successor (`missing_successor`). | No target-consumer proof yet. The helper evidence exposes duplicate records but does not prove x86 or riscv rejects a duplicate same-edge Route 5 row at the emission boundary, so this row is not demotion-ready. |
| Duplicate same-edge Route 5 row is passed near riscv diagnostic/fallback consumption. | riscv edge-publication move adapter: `riscv::consume_edge_publication_move_intent(...)` with optional `Route5CfgEdgePublicationRecord*` diagnostics and prepared-backed output. | Same Route 5/BIR record authority as above, surfaced only through diagnostic fields `route5_edge_status`, `route5_edge_source_agrees`, and `route3_source_memory_agrees`. | Prepared riscv compatibility remains authoritative for move text and fail-closed status: unique prepared publication lookup, `EdgePublicationMoveIntentStatus`, and exact `mv`, `lw`, `li`, `sw`, or large-offset output. | Duplicate Route 5 rows must not become accepted output authority. The expected behavior is either an explicit duplicate diagnostic/fail-closed Route 5 status before consumption or continued prepared-backed output with non-agreeing Route 5 diagnostics clearly separated from authority. | `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` creates a duplicate Route 5 index and confirms duplicate exposure while separately proving `no_match` and `no_source` Route 5 diagnostics preserve prepared-backed riscv output. | Duplicate exposure is not wired to a concrete riscv consumer rejection. The current riscv test does not pass a duplicate-result record with a duplicate-specific status to the adapter, and Route 5 currently has no duplicate edge status returned by `route5_find_cfg_edge_publication(...)`. Not demotion-ready. |
| Duplicate same-edge Route 5 row at the x86 selected dynamic `LoadLocal` agreement boundary. | x86 prepared-module selected `LoadLocal` bridge from closed idea 259: selected publication emission requires Route 5/prepared agreement plus Route 3 source-memory agreement before preserving selected output. | Same Route 5/BIR record authority, with Route 3 memory authority required for selected `LoadLocal` source-memory agreement. | x86 retained compatibility surfaces are `x86::ConsumedPlans` shared function lookups, `x86_prepared::consume_edge_publication_move_intent(...)`, prepared-module handoff errors, and exact emitted `mov` output. | A duplicate same-edge Route 5 row should make selected Route 5 agreement unavailable or rejected before x86 treats prepared `LoadLocal` output as agreed. It must not fall through as an accepted selected output solely because one duplicate record happens to match. | Existing x86 proof rejects missing Route 5 source-memory evidence, Route 5/Route 3 source-memory mismatch, and Route 5 source-producer index mismatch for selected dynamic `LoadLocal`. It does not construct duplicate same-edge Route 5 records. | Missing x86 duplicate proof. The selected-path bridge is positive/mismatch evidence, not duplicate same-edge fail-closed evidence; no demotion readiness. |

Boundaries kept out of Step 2:

- Prepared-only publication rows are not duplicate same-edge Route 5 rows; they
  belong to Step 3 even when a prepared lookup or printer surface has duplicate
  or fallback wording.
- Route 5-only rows are not duplicate same-edge rows unless the duplicate
  Route 5 records share the same natural edge and destination; missing prepared
  mirror behavior belongs to Step 4.
- Wrong-edge rows are not duplicate same-edge rows; wrong predecessor and wrong
  successor evidence remains Step 5 boundary evidence.
- Route 4 duplicate printer fallback rows are compatibility/printer evidence,
  not Route 5 same-edge target-consumer proof.

Disposition:

- Duplicate same-edge Route 5 records are currently mapped as blocked
  diagnostic/fail-closed obligations.
- No row above is demotion-ready because the available evidence stops at
  Route 5/BIR index exposure plus adjacent riscv/x86 diagnostic or mismatch
  behavior, not full duplicate same-edge target-consumer fail-closed proof.

## Suggested Next

Execute Step 3 by mapping prepared-only publication rows. Keep duplicate
same-edge Route 5 records as a closed Step 2 boundary and do not reuse Route 4
duplicate printer fallback evidence as Route 5 target-consumer proof.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::edge_publications`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat selected supported-path x86 Route 5/prepared agreement as public
  lookup retirement proof.
- Step 2 did not prove demotion readiness: duplicate same-edge Route 5 rows
  still lack x86/riscv target-consumer fail-closed proof.
- Do not map Route 5-only or wrong-edge rows in Step 3 except as boundaries;
  prepared-only rows should be judged by missing Route 5 authority plus
  preserved public prepared compatibility behavior.
- Keep target policy out of Route 5/BIR authority: move selection,
  register/home/storage policy, scratch registers, instruction spelling,
  wrapper formatting, branch/layout behavior, and exact target output remain
  target/prepared compatibility surfaces.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
