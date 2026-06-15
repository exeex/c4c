Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Gather Edge-Publication Evidence

# Current Packet

## Just Finished

Step 1: `Gather Edge-Publication Evidence` completed the inventory for the
active `edge_publications` proof map.

Evidence inventory:

- Source intent: idea 266 keeps this as analysis/proof work only. It explicitly
  blocks demotion, deletion, privatization, accessor wrapping, adapter
  migration, public-surface weakening, status/helper/output rewrites, and
  target-policy migration into BIR.
- Post-F3 gate evidence from closed idea 264: `edge_publications` is thinner
  than after idea 247 because x86 now has selected dynamic `LoadLocal`
  Route 5/prepared edge-publication agreement with Route 3 agreement gating.
  Public lookup demotion remains blocked by missing fail-closed proof for
  duplicate Route 5 records on one natural edge, prepared-only publication
  rows, Route 5-only publication rows, and wrong-edge rows.
- Phase F3 map evidence from closed idea 251: the selected semantic fact is
  Route 5 CFG-edge publication source identity checked against prepared
  `PreparedEdgePublication` for the same predecessor, successor, destination
  value, source value, and source producer. Route 4 remains publication
  availability/value-context evidence for this map, not the selected Route 5
  authority.
- Phase F3 x86 bridge evidence from closed idea 259: x86 now requires
  Route 5/prepared agreement for selected dynamic `LoadLocal` publication
  moves and requires Route 3 source-memory agreement before treating those
  moves as agreeing. Non-`LoadLocal` non-agreeing rows stay on the preserved
  prepared compatibility path.
- Compatibility matrix evidence from closed idea 254: helper/oracle statuses,
  fallback names, route-debug output, prepared-printer output, wrapper output,
  exact target output, and baseline behavior are compatibility contracts, not
  proof that semantic ownership transferred.

Authority candidates:

- Route 5/BIR semantic authority candidates are
  `bir::Route5CfgEdgePublicationRecord`, `bir::Route5EdgeJoinSourceIndex`,
  `bir::route5_cfg_edge_publication_record(...)`,
  `bir::route5_build_edge_join_source_index(...)`, and
  `bir::route5_find_cfg_edge_publication(...)`.
- Route 5 records carry predecessor/successor labels, destination value,
  source value, source producer kind/instruction/index, and for `LoadLocal`
  sources a Route 3 memory identity (`source_memory_identity_available` plus
  `Route3MemoryAccessRecord`).
- Route 5 status vocabulary currently includes `available`, `memory_source`,
  `no_source`, `missing_predecessor`, `missing_successor`,
  `missing_destination`, `missing_publication`, `missing_source_value`,
  `missing_source_producer`, `missing_source_memory_access`,
  `incomplete_source_memory_access`, and `no_match`.
- Prepared compatibility authority currently flows through
  `PreparedFunctionLookups::edge_publications`,
  `PreparedEdgePublicationLookups::publications`,
  `publications_by_edge_destination`,
  `PreparedEdgePublication`, `PreparedEdgePublicationLookupStatus`,
  `PreparedEdgePublicationSourceProducerKind`,
  `PreparedEdgePublicationSourceMemoryAccessStatus`,
  `make_prepared_edge_publication_lookups(...)`, and
  `find_unique_indexed_prepared_edge_publication(...)`.

Preserved compatibility surfaces:

- Public prepared lookup construction and indexing must stay stable:
  predecessor/successor/destination lookup keys, `PreparedEdgePublication`
  row contents, source/destination value ids, source/destination homes,
  parallel-copy bundle/step metadata, move identity, source producer facts,
  source-memory facts, aggregate stack-source authority, and status names.
- x86 compatibility surfaces include `x86::ConsumedPlans` shared function
  lookups, `x86_prepared::consume_edge_publication_move_intent(...)`,
  `append_edge_publication_move_instruction(...)`,
  `EdgePublicationMoveIntentStatus`, exact emitted `mov` text, and
  prepared-module handoff error strings.
- riscv compatibility surfaces include
  `riscv::consume_edge_publication_move_intent(...)`,
  `append_edge_publication_move_instruction(...)`,
  `EdgePublicationMoveIntentStatus`, exact emitted `mv`, `lw`, `li`, `sw`,
  and large-offset instruction text, plus diagnostic fields
  `route5_edge_status`, `route5_edge_source_agrees`, and
  `route3_source_memory_agrees`.
- Prepared-printer and helper/oracle surfaces remain preservation contracts:
  block-entry/current-block publication rows, fallback rows for missing or
  wrong Route 4 evidence, route/prepared agreement helper rows, and edge-copy
  source fact statuses must not be rewritten to claim progress.

Current proof coverage:

- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
  proves prepared `edge_publications` indexing for block-entry destinations
  and preservation of source/destination ids, source classification/home,
  parallel-copy ownership, execution site, and step metadata.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves Route 5
  and MIR/BIR CFG-edge source identity behavior for available memory-source
  rows, missing destination, missing source producer, destination type
  mismatch (`no_match`), wrong predecessor (`no_source`), wrong successor
  (`missing_successor`), multi-predecessor lookup, and prepared/BIR key
  mismatch rejection. It also exposes duplicate Route 5 records in the index
  as diagnostics but does not prove target consumer fail-closed behavior for
  duplicate same-edge rows.
- `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`
  proves x86 prepared lookup consumption for register, stack-slot, and
  rematerialized-immediate source homes; `MissingSharedLookups`,
  `MissingPublication`, unsupported source homes, unsupported destination
  homes, and exact `mov` output preservation.
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
  proves selected x86 dynamic `LoadLocal` agreement/rejection: accepted
  selected output when Route 5 and Route 3 agree, rejection for incomplete
  source-memory authority, missing Route 5 source-memory evidence,
  Route 5/Route 3 source-memory mismatch, and Route 5 source-producer index
  mismatch. It also proves a drifted prepared publication destination is
  rejected with the shared edge-publication authority contract.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` proves
  riscv prepared-backed output preservation for scalar and dynamic memory
  source rows; diagnostic Route 5 available/no-match/no-source status fields;
  Route 3 agreement booleans for dynamic `LoadLocal`; missing prepared
  publication fail-closed status; non-agreeing Route 5/Route 3 facts preserving
  prepared fallback/output; and a broad set of target-policy unsupported
  source/destination rows.
- `tests/backend/bir/backend_prepared_printer_test.cpp` proves public
  prepared-printer row preservation for block-entry publication rows when
  Route 4 evidence is prepared-only, agreeing, missing-PHI,
  wrong-destination, wrong-successor, wrong-instruction, or duplicate.

Missing evidence:

- Duplicate same-edge Route 5 rows currently have index/oracle diagnostics, but
  no full x86/riscv target consumer proof that duplicate same-edge semantic
  authority cannot be silently accepted while public prepared output remains
  stable.
- Prepared-only publication rows have compatibility evidence and Route 4
  printer fallback examples, but no complete Route 5 `edge_publications` row
  map proving expected fail-closed behavior at every public lookup, wrapper,
  helper/oracle, printer, and target-output surface.
- Route 5-only publication rows have Route 5/BIR authority candidates, but the
  missing prepared compatibility mirror behavior and target-consumer outcome
  are not yet mapped.
- Wrong-edge rows have Route 5/MIR helper statuses for wrong predecessor and
  wrong successor plus selected x86 drift rejection, but no complete same-row
  map across prepared lookup, wrapper output, route diagnostics, and
  target-output preservation.
- Existing x86 selected-path coverage is not demotion proof for the whole
  public lookup because it covers selected dynamic `LoadLocal` rows, not all
  duplicate/prepared-only/Route 5-only/wrong-edge unsupported rows.

x86/riscv applicability notes:

- x86 is applicable for selected dynamic `LoadLocal` Route 5/prepared agreement
  with Route 3 source-memory gating. Non-`LoadLocal` non-agreeing rows are
  explicitly preserved as prepared compatibility, so they are not demotion
  evidence.
- riscv is applicable as diagnostic/fallback evidence: it exposes Route 5 and
  Route 3 agreement fields while keeping prepared-backed output authoritative.
  Current riscv evidence does not make Route 5 diagnostics the consumer
  authority for unsupported rows.
- Neither target currently supplies complete fail-closed proof for duplicate
  same-edge, prepared-only, Route 5-only, or wrong-edge publication rows.

## Suggested Next

Execute Step 2 by mapping duplicate same-edge Route 5 records only. Use the
inventory above to separate Route 5/BIR index diagnostics from target-consumer
fail-closed proof, and do not claim demotion readiness from duplicate detection
alone.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::edge_publications`.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not treat selected supported-path x86 Route 5/prepared agreement as public
  lookup retirement proof.
- Do not map prepared-only, Route 5-only, or wrong-edge rows in Step 2 except
  as boundaries to avoid mixing them into duplicate same-edge rows.
- Keep target policy out of Route 5/BIR authority: move selection,
  register/home/storage policy, scratch registers, instruction spelling,
  wrapper formatting, branch/layout behavior, and exact target output remain
  target/prepared compatibility surfaces.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
