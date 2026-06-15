Status: Active
Source Idea Path: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Consolidate Disposition

# Current Packet

## Just Finished

Step 6: `Consolidate Disposition` completed the final bounded disposition for
the active `edge_publications` proof map.

Consolidated proof map:

| Family / row | Proof currently present | Blocked proof | Explicitly non-applicable rows | Final disposition |
| --- | --- | --- | --- | --- |
| Duplicate same-edge Route 5 records on one natural edge | Route-level/helper proof exists: `backend_prepared_lookup_helper_test.cpp` exposes duplicate `Route5CfgEdgePublicationRecord` entries in the Route 5 index and proves adjacent Route 5 statuses for available memory-source, missing source producer, destination type mismatch, wrong predecessor, and wrong successor. Diagnostic proof exists for riscv duplicate exposure plus nearby `no_match`/`no_source` diagnostics with prepared-backed output preservation. Selected-path x86 proof exists only for nearby dynamic `LoadLocal` agreement/rejection when Route 5/prepared and Route 3 facts agree or disagree. | No x86 or riscv same-consumer proof demonstrates that a duplicate same-edge Route 5 row is rejected, made unavailable, or kept diagnostic before it can drive emitted target output. There is also no duplicate-specific Route 5 consumer status returned by `route5_find_cfg_edge_publication(...)`. | Target move selection, register/home/storage policy, scratch registers, instruction spelling, wrapper formatting, branch/layout behavior, and exact target output are target-policy surfaces; they are not BIR authority and cannot be used as duplicate Route 5 semantic proof. | Mapped but blocked. Duplicate records are fail-closed/diagnostic obligations, not demotion evidence for public `PreparedFunctionLookups::edge_publications`. |
| Prepared-only publication rows | Helper/oracle proof exists: prepared lookup helpers preserve and fail closed through `MissingPreparedLookups`, `MissingPublication`, `PublicationUnavailable`, `AmbiguousPublication`, `MissingSourceHome`, source-home/source-memory mismatch, and current-block join fallback statuses. x86 selected-path proof exists for nearby dynamic `LoadLocal` Route 5/Route 3 disagreement rejection, while x86 prepared compatibility tests prove missing shared lookups/publications fail closed and preserve exact prepared-backed `mov` behavior where still supported. riscv diagnostic proof preserves prepared-backed output with false Route 5/Route 3 agreement flags and fails closed for missing prepared publication. | No direct same-consumer proof hand-builds a prepared-only edge-publication row with absent Route 5 authority and proves every x86/riscv target consumer rejects it when Route 5 authority is required while preserving public lookup/status/output behavior. | Non-`LoadLocal` x86 prepared-backed rows and current riscv prepared-backed output are compatibility output, not Route 5 authority. Prepared-printer, route-debug, wrapper, fallback names, exact target output, and baseline behavior are preservation contracts only. | Mapped but blocked. Prepared-only rows remain retained compatibility state plus missing Route 5 authority; no row is demotion-ready. |
| Route 5-only publication rows | Route-level proof exists for Route 5/BIR authority and statuses: available/memory-source records, missing destination, missing source producer, destination type mismatch, wrong predecessor, wrong successor, and later matching predecessor. Helper/status proof exists that missing prepared lookup/publication fails closed. Selected-path x86 proof shows selected dynamic `LoadLocal` accepts only Route 5/prepared plus Route 3 agreement when a prepared mirror exists. riscv diagnostic proof reports Route 5/Route 3 fields while preserving prepared-backed output or `MissingPublication`. | No same-consumer proof shows an available Route 5-only row being consumed as BIR authority while the missing prepared mirror remains observable and public prepared compatibility is preserved. No direct x86 selected fixture proves a Route 5 `MemorySource` row with no prepared edge-publication mirror is refused before output. No non-memory target proof consumes Route 5-only authority without prepared output policy. | Route 5/BIR source identity does not own target move selection, source/destination homes, instruction spelling, stack/register/storage policy, wrappers, or exact emitted output. Missing prepared mirror status is compatibility behavior, not proof that Route 5 authority is absent. | Mapped but blocked. Route 5-only rows are authority-present and mirror-missing; they block demotion until same-consumer x86/riscv behavior is proved or the consumers are explicitly migrated in a later approved idea. |
| Wrong-edge publication rows | Route/helper/oracle proof exists for wrong predecessor `NoSource`, wrong successor `MissingSuccessor`, MIR `MissingSourceValue`/`MissingSuccessorLabel`, prepared/BIR key mismatch rejection, multi-predecessor later-match behavior, and non-memory source identity fail-closed cases. Diagnostic proof exists for riscv wrong-predecessor/no-match/no-source preservation with `route5_edge_source_agrees == false`. Selected-path x86 proof exists for nearby selected dynamic `LoadLocal` Route 5/prepared plus Route 3 agreement gating and reachable disagreement rejection. | No direct same-consumer x86/riscv proof isolates wrong-predecessor or wrong-successor `MemorySource` and non-memory rows at the target edge-publication consumers, proves no emission comes from the wrong natural edge, and preserves public prepared lookup/status/output compatibility. | Block-entry Route 4 wrong-successor and duplicate-printer evidence is adjacent identity/printer evidence only; it is not replacement proof for Route 5 CFG-edge target consumers. Wrong-edge rows are not prepared-only or Route 5-only rows; they are natural-edge identity mismatches. | Mapped but blocked. Route 5/BIR semantic authority applies only to the exact predecessor, successor, destination, source value, source producer, and Route 3 memory-source identity for the same natural edge. |

Overall disposition:

- The source idea's bounded proof-map objective is satisfied: duplicate
  same-edge Route 5, prepared-only, Route 5-only, and wrong-edge publication
  families are mapped with semantic authority, consumer boundary,
  compatibility surface, expected fail-closed result, current proof, and
  unresolved proof gaps.
- No `edge_publications` demotion readiness is established. Every family still
  has a same-consumer x86/riscv blocker or is explicitly a retained
  compatibility/target-policy surface rather than semantic transfer evidence.
- No later implementation idea is safe to create from this map yet. Demotion,
  deletion, privatization, accessor wrapping, adapter migration, target-policy
  migration into BIR, and expectation/status/output rewrites remain blocked or
  out of scope.
- Follow-up implementation creation should not be requested. A later bounded
  proof-only idea could be considered by the supervisor if the project wants
  same-consumer target proof for one blocked family, but this packet does not
  require or authorize new idea creation.
- Closure should be requested from the plan owner, not deactivation, because
  the active runbook's analysis/proof-map criteria are complete and the
  durable disposition is "closed with no demotion/implementation follow-up."
  Close-time regression guard/log policy remains supervisor/plan-owner owned.

## Suggested Next

Ask the plan owner to close idea 266 with the consolidated disposition above.
Do not request an implementation follow-up, and do not treat the closure as
authorization to demote or wrap `PreparedFunctionLookups::edge_publications`.

## Watchouts

- `edge_publications` remains public retained compatibility. The map proves
  why demotion is still blocked; it does not transfer target output authority
  into Route 5/BIR.
- Selected x86 dynamic `LoadLocal` Route 5/prepared plus Route 3 agreement is
  positive supported-path evidence only. It is not public lookup retirement
  proof for duplicate, prepared-only, Route 5-only, or wrong-edge rows.
- riscv Route 5/Route 3 fields remain diagnostic/fallback evidence while
  prepared-backed output stays authoritative for current target text.
- Preserve helper/oracle statuses, fallback names, route-debug output,
  prepared-printer output, wrapper output, exact target output, and baseline
  behavior.

## Proof

No build/test proof required for this analysis-only packet. Validation was
source/history inspection plus `todo.md` readback, `git status --short`, and
`git diff --check`; no `test_after.log` was generated or modified.
