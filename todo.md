Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Map Wrong-Edge Publication Rows

# Current Packet

## Just Finished

Step 5: `Map Wrong-Edge Publication Rows` completed the wrong-edge publication
row map for the active `edge_publications` proof map.

Wrong-edge publication row map:

| Row | Natural-edge / Route 5 surface | Prepared compatibility surface | Semantic authority | Expected fail-closed result | Current evidence | Proof gap / blocker status |
| --- | --- | --- | --- | --- | --- | --- |
| Wrong predecessor for a single natural edge: request uses the expected successor and destination, but the predecessor label/block is not the predecessor that owns the source value. | Natural edge is `(predecessor_label, successor_label, destination)`; Route 5 record lookup through `route5_cfg_edge_publication_record(...)` or `route5_find_cfg_edge_publication(...)` rejects the alternate predecessor as `NoSource` with `explicit_no_source`. | `PreparedFunctionLookups::edge_publications` and `find_unique_indexed_prepared_edge_publication(...)` remain keyed by predecessor, successor, and destination id. Prepared helper/oracle comparison must reject the mismatched predecessor instead of treating a same-successor row as transferable. | Route 5/BIR owns source identity only for the exact predecessor/successor/destination natural edge. Prepared rows are a public mirror only when they name that same key. | Fail closed as Route 5 `NoSource` / MIR `MissingSourceValue`, or prepared `MissingPublication`/non-match at the helper boundary. It must not emit target output or report agreement by borrowing a publication from the requested successor on the wrong predecessor. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves wrong-predecessor Route 5 direct lookup returns `NoSource`, MIR Route 5 lookup returns `MissingSourceValue`, and `prepared_and_bir_cfg_edge_publication_source_identity_match(...)` rejects mismatched predecessor keys like the prepared oracle. RISC-V diagnostic tests expose wrong-predecessor absence as Route 5 `NoSource` while preserving prepared-backed output for present mirrors. | Blocked for demotion: evidence is helper/oracle and riscv diagnostic/fallback proof, not x86/riscv same-consumer proof that every target edge-publication consumer rejects a wrong-predecessor publication while preserving public prepared lookup/status/output behavior. |
| Wrong successor for a single predecessor/destination: request uses the expected predecessor and destination, but the successor label/block is not the successor whose PHI/edge publication owns the destination. | Natural edge is `(predecessor_label, successor_label, destination)`; Route 5 indexed lookup reports `MissingSuccessor` for an alternate successor. For block-entry Route 4/BIR publication, wrong successor reports `MissingSuccessorLabel`. | Prepared `edge_publications` lookup and current-block/block-entry publication queries keep successor label as part of the public compatibility key. The mirror may report missing publication for the wrong successor even when the natural successor has an available publication. | Route 5/BIR natural-edge identity owns edge publication only for the real successor block and destination. Route 4/BIR owns block-entry publication identity only for the real successor label. | Fail closed as Route 5 `MissingSuccessor`, MIR block-entry `MissingSuccessorLabel`, or prepared `MissingPublication`. It must not treat same predecessor/destination publication on another successor as a match or append target moves/output. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves wrong-successor Route 5 indexed lookup returns `MissingSuccessor`, `prepared_and_bir_cfg_edge_publication_source_identity_match(...)` rejects mismatched successor keys, and BIR block-entry publication identity fails closed for wrong successor. | Blocked for demotion: current proof covers shared helper/oracle and block-entry identity categories, but not the full x86/riscv target consumer surface for wrong-successor edge-publication rows. |
| Multi-predecessor wrong edge: successor has multiple valid predecessor publications, but the requested predecessor is neither one of the natural incoming predecessors. | Route 5 edge index may continue past a first non-matching predecessor and find a later matching predecessor for a valid natural edge; for a predecessor outside the incoming set, `route5_find_cfg_edge_publication(...)` returns `NoSource` only after all predecessors miss. | Prepared compatibility still treats predecessor identity as part of the key and must not fall back to any predecessor with the right successor/destination. | Route 5/BIR authority is the natural incoming predecessor that matches the PHI incoming/source producer. Existence of another predecessor's publication is not authority for the requested edge. | Fail closed as explicit Route 5 `NoSource` / prepared missing/non-match for the outsider predecessor, while still allowing a later valid predecessor row to be found when requested by its own natural key. | `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves Route 5 continues past the first predecessor to find a later matching predecessor, then reports `NoSource` for a predecessor label not present in the incoming set. | Blocked for demotion: this proves route-index discrimination, but not target same-consumer preservation across x86/riscv when multiple incoming edge-publication rows exist and one requested predecessor is wrong. |
| Wrong-edge selected dynamic `LoadLocal` publication: Route 5/Route 3 memory-source facts are present, but predecessor/successor identity does not match the prepared publication mirror used by the selected x86 or riscv edge move path. | Route 5 `MemorySource` authority is valid only when predecessor, successor, destination, source value, source producer, and Route 3 load-local memory identity agree with the prepared publication for that natural edge. Wrong-edge Route 5 facts are non-agreement, not portable authority. | x86 selected `LoadLocal` still enters through prepared `edge_publications`, then requires Route 5/prepared plus Route 3 agreement. riscv currently exposes Route 5/Route 3 as diagnostics while output remains prepared-backed. | Combined Route 5/Route 3/BIR authority can validate a selected memory-source edge only for the same natural edge as the prepared mirror. The prepared mirror remains a compatibility surface, not independent semantic authority. | Expected result is non-agreement plus no new target output from the wrong-edge row. x86 should reject selected Route 5/prepared agreement for wrong-edge identity; riscv may preserve prepared-backed output for the actual prepared row while diagnostic `route5_edge_source_agrees` is false for the wrong-edge Route 5 fact. | Closed idea 259 proves x86 selected dynamic `LoadLocal` now requires Route 5/prepared agreement with Route 3 gating and rejects reachable disagreement rows. RISC-V tests prove non-agreeing Route 5/Route 3 diagnostics keep prepared-backed output stable and mark agreement false. | Blocked for demotion: no direct same-consumer fixture isolates a wrong predecessor or wrong successor `MemorySource` row at the selected x86 path and proves no emission from the wrong-edge authority while preserving the prepared compatibility statuses. |
| Wrong-edge non-memory publication (`Binary`, `Cast`, `SelectMaterialization`, immediate/scalar source): source producer exists on another edge or predecessor, but the requested natural edge key does not own it. | Route 5 can expose non-memory source-producer identity only when predecessor/successor/destination/source producer match the natural edge. Wrong predecessor/successor rows are `NoSource`, `MissingSuccessor`, `NoMatch`, or missing publication facts. | Existing x86/riscv target move output for non-`LoadLocal` rows remains prepared-backed; helper/source-facts oracles and public lookup/status strings are compatibility surfaces. | Route 5/BIR owns source identity for the exact natural edge, but target move selection, homes, instruction spelling, wrapper formatting, and exact output remain target/prepared compatibility policy. | Fail closed at the route/helper boundary or preserve prepared-backed output for the actual prepared row with diagnostic non-agreement. It must not use a source producer from another edge to synthesize a prepared publication or target move. | Helper tests prove BIR CFG edge source identity exposes named non-memory producer identity only on matching keys and fails closed for missing destination/source and wrong-edge predecessor/successor. RISC-V scalar diagnostics show `NoMatch`/`NoSource` with `route5_edge_source_agrees == false` while prepared output remains stable. X86 non-`LoadLocal` rows remain prepared compatibility output with missing lookup/publication fail-closed tests. | Blocked for demotion: there is no x86/riscv same-consumer proof that non-memory wrong-edge Route 5 identity is rejected at every target edge-publication consumer while public prepared compatibility surfaces remain unchanged. |

Boundaries kept out of Step 5:

- Route 5-only rows remain the completed Step 4 boundary. Wrong-edge rows are
  not merely missing prepared mirrors; they are predecessor/successor natural
  edge identity mismatches.
- Prepared-only rows remain the completed Step 3 boundary; those rows have
  prepared compatibility data without selected Route 5 authority.
- Duplicate same-edge Route 5 rows remain the completed Step 2 boundary.
- Block-entry Route 4 wrong-successor evidence is adjacent identity evidence,
  not a replacement for Route 5 CFG-edge target-consumer proof.

Disposition:

- Wrong-edge publication rows are mapped as natural-edge identity mismatches:
  predecessor, successor, destination, source value, source producer, and Route
  3 memory-source agreement must belong to the same edge before Route 5/BIR can
  be semantic authority.
- Current evidence proves route/helper/oracle fail-closed behavior for wrong
  predecessor and wrong successor, multi-predecessor Route 5 no-source
  behavior, RISC-V diagnostic non-agreement preservation, and adjacent x86
  selected `LoadLocal` agreement gating.
- No wrong-edge row is demotion-ready. The unresolved blocker is same-consumer
  x86/riscv proof for wrong-edge rows across target output/status surfaces
  while preserving public prepared `edge_publications` compatibility behavior.

## Suggested Next

Execute Step 6 by consolidating the duplicate same-edge, prepared-only, Route
5-only, and wrong-edge maps into the final bounded disposition for idea 266.
Do not request closure unless the consolidated map keeps all demotion and
implementation follow-up blocked or explicitly scoped to later proof work.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::edge_publications`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat selected supported-path x86 Route 5/prepared agreement as public
  lookup retirement proof.
- Step 5 did not prove demotion readiness: wrong-edge rows still lack
  x86/riscv same-consumer proof for target output/status surfaces.
- Do not collapse wrong-edge rows into Route 5-only or prepared-only rows.
  Wrong-edge rows are natural-edge identity mismatches, not mirror absence.
- Keep target policy out of Route 5/BIR authority: move selection,
  register/home/storage policy, scratch registers, instruction spelling,
  wrapper formatting, branch/layout behavior, and exact target output remain
  target/prepared compatibility surfaces.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
