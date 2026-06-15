Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Map Prepared-Only Publication Rows

# Current Packet

## Just Finished

Step 3: `Map Prepared-Only Publication Rows` completed the prepared-only
publication row map for the active `edge_publications` proof map.

Prepared-only publication row map:

| Row | Consumer boundary | Semantic authority | Retained compatibility surface | Expected fail-closed result | Current evidence | Proof gap / disposition |
| --- | --- | --- | --- | --- | --- | --- |
| A prepared edge-publication row exists for a predecessor/successor/destination, but no matching Route 5/BIR `Route5CfgEdgePublicationRecord` exists for the same natural edge and destination. | Shared prepared helper/oracle boundary: `PreparedFunctionLookups::edge_publications`, `PreparedEdgePublicationLookups::publications_by_edge_destination`, `find_unique_indexed_prepared_edge_publication(...)`, and helper rows such as `prepare_edge_copy_source_facts(...)`. | Route 5/BIR publication identity is the semantic authority when a row is being used as Route 5 agreement; a prepared-only row has no selected Route 5 authority. | Public prepared lookup/status remains retained compatibility: unique prepared publication lookup, source/destination homes, move identity, source-producer metadata, source-memory fields, and prepared helper statuses such as `MissingPreparedLookups`, `MissingPublication`, `PublicationUnavailable`, `AmbiguousPublication`, and `MissingSourceHome`. | Prepared-only data must remain observable as compatibility state but must not be silently upgraded into Route 5 agreement. If a consumer requires Route 5 authority, the expected result is fail-closed or non-agreeing diagnostics; if the consumer is still explicitly prepared-backed, output may be preserved only as compatibility output. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves prepared lookup helper fail-closed statuses for missing lookups/publications, unavailable publications, ambiguous publications, and missing source homes. It also proves prepared source-home/source-memory/source-producer helper checks reject mismatched source home kind, missing source home, mismatched memory offset, and memory accesses without result value while preserving exposed prepared metadata. | No same-consumer Route 5 rejection proof for a prepared-only edge-publication row. Helper evidence proves the compatibility surface and local helper fail-closed behavior, but not that x86/riscv target consumers reject prepared-only rows when Route 5 authority is required. Not demotion-ready. |
| Prepared-only row reaches the current-block join helper/oracle surface without Route 5 edge/join source evidence. | Prepared current-block join helper boundary: `prepare_current_block_join_parallel_copy_source_facts(...)`, `mir::find_bir_current_block_join_source_identity(...)`, and `bir::route5_current_block_join_source_records(...)`. | Route 5 current-block join-source records are semantic authority for Route 5 agreement. Prepared facts remain compatibility source facts when no Route 5 record agrees. | Prepared edge-publication and current-block join compatibility facts remain visible: source value id, source/destination homes, incoming-expression flags, unsupported-move status, and route5 fallback status fields. | Prepared-only join rows should not claim Route 5 evidence. Missing or no-source Route 5 data should leave prepared facts intact with `route5_join_source_agrees == false`, `route5_join_source == nullptr`, and explicit Route 5 status such as `Unavailable`, `MissingSuccessor`, or `NoSource`. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` checks that a prepared-only current-block join row does not claim Route 5 evidence, that empty Route 5 indexes produce `MissingSuccessor`, that no-source Route 5 records preserve prepared facts, and that BIR join identity fails closed for missing block, missing PHIs, or missing publication. | This is helper/oracle compatibility proof, not target edge-publication output proof. It maps the prepared-only boundary and fallback statuses but does not prove x86/riscv emitted output rejects prepared-only publication rows. Not demotion-ready. |
| Prepared-only row reaches x86 prepared edge-publication consumption with no Route 5/BIR authority. | x86 prepared compatibility boundary: `x86::ConsumedPlans::shared_function_lookups()`, `x86_prepared::consume_edge_publication_move_intent(...)`, and `x86_prepared::append_edge_publication_move_instruction(...)`. | For selected dynamic `LoadLocal`, Route 5/prepared agreement plus Route 3 source-memory agreement is semantic authority. For non-`LoadLocal` prepared compatibility paths, prepared output remains retained target compatibility and is not Route 5 demotion evidence. | x86 compatibility surfaces remain shared prepared lookups, `EdgePublicationMoveIntentStatus`, preserved `PreparedEdgePublication*`, source/destination value ids, source/destination operands, and exact emitted `mov` text. | A prepared-only selected `LoadLocal` row should not be accepted as Route 5 agreement. Non-selected/non-`LoadLocal` prepared-backed rows may continue to emit as compatibility output, but that output must not be cited as Route 5 authority. Missing shared lookup/publication must fail closed without appending output. | `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` proves x86 reads shared prepared edge publications and emits prepared-backed moves, while missing shared lookups and missing publications fail closed without inventing publications or changing output. It also proves unsupported source/destination homes return unsupported statuses while preserving the shared publication pointer. Closed idea 259 proves selected dynamic `LoadLocal` x86 rejects missing Route 5 source-memory evidence, Route 5/Route 3 mismatch, and producer-index mismatch. | Existing x86 evidence is split: compatibility output is prepared-backed, and selected `LoadLocal` rejects specific Route 5/Route 3 disagreement rows. There is no direct fixture that hand-builds a prepared-only selected publication row with absent Route 5 edge authority and proves target-consumer rejection. Not demotion-ready. |
| Prepared-only row reaches riscv prepared edge-publication consumption with no Route 5/BIR authority. | riscv prepared compatibility boundary: `riscv::consume_edge_publication_move_intent(...)` and optional diagnostic fields `route5_edge_status`, `route5_edge_source_agrees`, and `route3_source_memory_agrees`. | Route 5/BIR record authority is diagnostic only for the current riscv adapter; prepared lookup/status and target policy remain output authority. | riscv compatibility surfaces remain `EdgePublicationMoveIntentStatus`, exact instruction text (`mv`, `lw`, `li`, `sw`, large-offset sequences), prepared source/destination homes, and optional Route 5/Route 3 diagnostic fields. | Prepared-only riscv rows may preserve prepared-backed output when Route 5 diagnostics are absent or non-authoritative, but they must not be treated as demotion proof. Missing shared lookups/publications still fail closed through prepared statuses. | `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` proves missing shared lookups/publications fail closed, non-agreeing Route 5 `no_match`/`no_source` diagnostics preserve prepared-backed output with `route5_edge_source_agrees == false`, and dynamic memory-source Route 3 mismatch/incomplete diagnostics preserve prepared-backed `lw` output with agreement flags false. | riscv has diagnostic/fallback evidence only. It does not reject prepared-only rows at a Route 5 authority boundary, and prepared output remains intentionally authoritative for target text. Not demotion-ready. |
| Prepared-only publication appears only through printer, wrapper, route-debug, fallback, or exact-output surfaces. | Compatibility output boundary: prepared printer/debug output, wrapper output, helper/fallback names, exact target-output tests, and baseline behavior. | No Route 5 semantic authority is established by these surfaces alone; they are output contracts around retained public prepared state. | Status strings, fallback names, debug/printer text, wrapper output, exact x86/riscv target text, and baseline behavior remain preserved compatibility. | These observations should be preserved and may document fallback behavior, but they must not be reused as Route 5 target-consumer proof or as evidence that `edge_publications` can be demoted. | Closed idea 264 records prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output compatibility surfaces as retained compatibility authority. Step 2 already kept Route 4 duplicate printer fallback out of Route 5 target-consumer proof. | No demotion evidence. Printer/wrapper/debug/fallback observations are boundaries and preservation contracts only. |

Boundaries kept out of Step 3:

- Duplicate same-edge Route 5 rows remain the completed Step 2 boundary.
- Route 5-only rows are out of scope for this packet; they belong to Step 4
  because Route 5 authority exists without a prepared compatibility mirror.
- Wrong-edge rows are out of scope for this packet; they belong to Step 5
  because the natural-edge identity disagrees.
- Route 4 duplicate printer fallback remains compatibility/printer evidence,
  not Route 5 target-consumer proof.

Disposition:

- Prepared-only publication rows are mapped as retained compatibility state
  plus blocked fail-closed proof obligations.
- No prepared-only row is demotion-ready. Current evidence proves helper/status
  preservation, x86 selected-path disagreement gating for nearby rows, and
  riscv diagnostic/fallback behavior, but not full x86/riscv same-consumer
  rejection of prepared-only rows at the Route 5 authority boundary.

## Suggested Next

Execute Step 4 by mapping Route 5-only publication rows. Keep prepared-only
rows as a closed Step 3 boundary, and distinguish real Route 5/BIR authority
from missing prepared compatibility mirror behavior.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::edge_publications`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat selected supported-path x86 Route 5/prepared agreement as public
  lookup retirement proof.
- Step 3 did not prove demotion readiness: prepared-only rows still lack
  x86/riscv target-consumer fail-closed proof at the Route 5 authority
  boundary.
- Do not map wrong-edge rows in Step 4 except as boundaries; Route 5-only rows
  should be judged by present Route 5/BIR authority plus missing prepared
  compatibility mirror behavior.
- Keep target policy out of Route 5/BIR authority: move selection,
  register/home/storage policy, scratch registers, instruction spelling,
  wrapper formatting, branch/layout behavior, and exact target output remain
  target/prepared compatibility surfaces.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
