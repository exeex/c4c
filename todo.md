Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Map Route 5-Only Publication Rows

# Current Packet

## Just Finished

Step 4: `Map Route 5-Only Publication Rows` completed the Route 5-only
publication row map for the active `edge_publications` proof map.

Route 5-only publication row map:

| Row | Consumer boundary | Route 5/BIR authority | Retained compatibility mirror behavior | Observable failure/status behavior | Current x86/riscv evidence or non-applicability | Proof gap / disposition |
| --- | --- | --- | --- | --- | --- | --- |
| Route 5/BIR has an available same-edge publication record for a predecessor/successor/destination, but `PreparedFunctionLookups::edge_publications` has no matching prepared row for that key. | Shared prepared helper/oracle boundary: `find_unique_indexed_prepared_edge_publication(...)`, `PreparedEdgePublicationLookups::publications_by_edge_destination`, `prepare_edge_copy_source_facts(...)`, and target consumers that still enter through prepared `edge_publications`. | `bir::Route5CfgEdgePublicationRecord` from `route5_cfg_edge_publication_record(...)` or `route5_find_cfg_edge_publication(...)` is the real publication authority when status is `Available` or `MemorySource` and predecessor, successor, destination value name/type, source value, and source producer agree. | The retained public mirror is allowed to report absence. Missing prepared lookup/publication statuses remain compatibility output, not proof that Route 5 authority is absent. The mirror must not synthesize a prepared publication from Route 5 facts until a later implementation explicitly migrates that consumer. | Prepared-only consumers should fail closed as `MissingPublication`, `MissingSharedLookups`, `MissingPreparedLookups`, or `PublicationUnavailable` rather than silently accepting the Route 5-only record through the public prepared table. If a Route 5-aware diagnostic is attached, it may report available Route 5 authority while the prepared-backed output remains blocked by missing mirror data. | x86 prepared edge-publication tests prove missing shared lookups and missing prepared publications return explicit missing statuses and do not append target output; they do not pass a Route 5-only record through the prepared consumer. riscv tests prove missing prepared publications return `MissingPublication` even when Route 5 facts exist nearby; Route 5 remains diagnostic/fallback input for current riscv output. | No same-consumer proof currently demonstrates that a Route 5-only available row is consumed as BIR authority while preserving prepared missing-publication compatibility. This is authority-present but mirror-missing, so it is not demotion-ready. |
| Route 5/BIR has a `MemorySource` edge-publication record for selected dynamic `LoadLocal`, but the matching prepared edge-publication row is missing from `edge_publications`. | Selected x86 dynamic `LoadLocal` agreement boundary: `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`, `agreed_route5_edge_publication_source(...)`, Route 3 source-memory agreement, and `x86_prepared::consume_edge_publication_move_intent(...)`. | Route 5 owns predecessor/successor/destination/source identity and Route 3 owns load-local source-memory identity. Agreement requires the Route 5 `MemorySource` record and Route 3 memory fields to match the prepared publication. | The prepared mirror remains a required compatibility input for the current x86 prepared move path. If the prepared publication is absent, the current prepared consumer reports `MissingPublication` and emits nothing; the selected Route 5/Route 3 authority is not enough by itself to retire public lookup behavior. | Expected fail-closed behavior is explicit missing prepared publication at the prepared consumer boundary, or non-agreement if a future bridge attempts to compare without a mirror. It must not emit a selected `LoadLocal` move solely from Route 5/Route 3 facts through the old public prepared API. | Closed idea 259 proves x86 selected dynamic `LoadLocal` accepts only Route 5/prepared plus Route 3 agreement and rejects reachable Route 5/Route 3 disagreement rows. `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` proves missing prepared lookup/publication statuses and unchanged output. riscv has diagnostic-only Route 5/Route 3 fields and preserves prepared-backed `lw` output for agreeing and non-agreeing diagnostics; when the prepared mirror is missing it fails closed as `MissingPublication`. | There is no direct x86 fixture that hand-builds a Route 5 `MemorySource` row with no prepared edge-publication mirror and proves the selected consumer refuses Route 5-only output. Existing positive agreement proof depends on the prepared mirror, so this row remains blocked. |
| Route 5/BIR has an available non-memory source record (`Binary`, `Cast`, `SelectMaterialization`, `Immediate`, or other non-`LoadLocal`) without a matching prepared publication. | Non-selected/non-`LoadLocal` target compatibility boundary: x86 prepared move intent, riscv prepared move intent, helper/source-facts oracles, and exact target-output surfaces. | Route 5 records can identify same-edge destination/source/producers for non-memory publications. They do not own target move selection, source/destination homes, instruction spelling, scratch policy, wrapper formatting, or exact emitted output. | The prepared mirror remains the compatibility source for target homes and emitted output. Missing mirror data should remain observable as missing prepared publication or unavailable source facts; available Route 5 identity alone should not authorize target text. | Target consumers should fail closed at the prepared lookup boundary if the mirror is missing. If Route 5 diagnostics are present, non-agreement or missing mirror must remain observable and not be converted into accepted prepared-backed output. | Helper tests prove Route 5/BIR can expose named non-memory producer identity and missing-destination/missing-source statuses. x86 tests prove prepared-backed register/immediate/stack moves and missing prepared publication fail-closed behavior, but non-`LoadLocal` non-agreeing x86 rows deliberately remain on prepared compatibility. riscv tests prove scalar Route 5 diagnostics can agree or disagree while output stays prepared-backed; missing prepared publication still fails closed. | No implementation or proof consumes a Route 5-only non-memory publication as BIR authority at the same target boundary while preserving missing prepared mirror behavior. Current target output is still prepared-backed, so this row is blocked and not demotion evidence. |
| Route 5 index exposes diagnostic statuses for a same-edge destination while the prepared mirror is absent or incomplete: `MissingPublication`, `MissingDestination`, `MissingSourceProducer`, `NoMatch`, or `NoSource` after same-successor search. | Route 5 indexed oracle and prepared compatibility status boundary: `route5_find_cfg_edge_publication(...)`, `mir::find_bir_cfg_edge_publication_source_identity(...)`, prepared helper statuses, and optional riscv diagnostic fields. | Route 5/BIR authority is the indexed status and record metadata. `NoMatch`, `NoSource`, and missing statuses are fail-closed route facts, not prepared publications. | Public prepared lookup/status strings and helper/oracle behavior remain compatibility mirrors. A missing or incomplete prepared mirror may preserve prepared statuses, while Route 5 status may be reported as diagnostic metadata only. | Observable behavior should be explicit route status plus prepared missing/unavailable status. A diagnostic status must not be used to create a synthetic prepared publication or to weaken helper/oracle failures. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves Route 5 indexed statuses for missing source, wrong destination type, missing destination, wrong predecessor, wrong successor, and later matching predecessor. riscv tests expose `NoMatch`/`NoSource` diagnostics while preserving prepared output for present mirrors and `MissingPublication` for absent mirrors. x86 current prepared consumer has no diagnostic-only Route 5-only output path. | This is useful fail-closed route evidence, but not target same-consumer proof. It maps the Route 5-only diagnostic boundary and blocks public lookup retirement until x86/riscv consumers either consume Route 5 directly with mirror preservation or explicitly remain prepared-backed. |

Boundaries kept out of Step 4:

- Prepared-only rows remain the completed Step 3 boundary; those rows have
  prepared compatibility data with no selected Route 5 authority.
- Wrong-edge rows belong to Step 5. Step 4 uses wrong-predecessor or
  wrong-successor statuses only as boundaries for Route 5-only diagnostics, not
  as wrong-edge row dispositions.
- Duplicate same-edge Route 5 rows remain the completed Step 2 boundary.
- Selected x86 Route 5/prepared agreement is positive supported-path evidence
  only. It is not public `edge_publications` lookup retirement proof because it
  still relies on the prepared publication mirror and Route 3 agreement.

Disposition:

- Route 5-only publication rows are mapped as real Route 5/BIR authority plus
  missing prepared compatibility mirrors.
- Current evidence proves Route 5 indexed authority/status behavior, prepared
  missing-publication fail-closed behavior, x86 selected agreement when the
  prepared mirror exists, and riscv diagnostic/fallback behavior.
- No Route 5-only row is demotion-ready. The missing proof is same-consumer
  x86/riscv handling for available Route 5 rows without matching prepared
  publications, with public prepared lookup/status/output compatibility
  preserved.

## Suggested Next

Execute Step 5 by mapping wrong-edge publication rows. Keep Route 5-only rows
as a closed Step 4 boundary, and treat wrong-edge cases as natural-edge
identity mismatches rather than missing prepared mirror rows.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::edge_publications`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat selected supported-path x86 Route 5/prepared agreement as public
  lookup retirement proof.
- Step 4 did not prove demotion readiness: Route 5-only rows still lack
  x86/riscv same-consumer proof for available Route 5 authority with a missing
  prepared mirror.
- Do not collapse Step 5 wrong-edge rows into Route 5-only rows. Wrong-edge
  rows should be judged by natural-edge identity mismatch, not by mere absence
  of prepared compatibility data.
- Keep target policy out of Route 5/BIR authority: move selection,
  register/home/storage policy, scratch registers, instruction spelling,
  wrapper formatting, branch/layout behavior, and exact target output remain
  target/prepared compatibility surfaces.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
