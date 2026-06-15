Status: Active
Source Idea Path: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Consolidate Disposition

# Current Packet

## Just Finished

Step 6: `Consolidate Disposition` completed as an analysis-only final proof
map for `PreparedFunctionLookups::memory_accesses`.

Final bounded proof map:

| Family | Rows with selected-path or same-consumer target proof | Rows with helper/oracle or route-level proof only | Rows that remain blocked for demotion/private-accessor work | Explicitly non-applicable rows |
| --- | --- | --- | --- | --- |
| Prepared-only | PO-3 has partial x86 selected-path guardrails for missing/incomplete Route 5 or Route 3 source-memory evidence when a selected `LoadLocal` agreement candidate is present. PO-4 has partial riscv consumer proof for missing shared lookups, missing publication, missing prepared memory access, incomplete source memory, and non-`LoadLocal` source-memory unavailability. | PO-1 has public lookup helper proof for construction, unique lookup, duplicate-name/id ambiguity, duplicate-vector preservation, and invalid-name rejection. PO-2 has helper proof for copied source-memory fields, accepted matching prepared access, offset mismatch rejection, unnamed source-memory rejection, and supported Route 5/Route 3 identity exposure. PO-5 is identified as a prepared fallback/helper reader, not semantic authority. | All PO rows remain blocked because no proof shows every still-public `memory_accesses` consumer rejects a self-consistent prepared-only row at the same Route 3/Route 5 authority boundary while preserving helper/status/fallback/debug/printer/wrapper/output behavior. x86 no-candidate local-slot fallback remains compatibility policy, not agreement. | Target-owned local-slot, stack, register, addressing, wrapper, formatting, and exact-output policy are not BIR authority and are not demotion criteria. |
| Stale-publication | SP-1 and SP-2 have selected x86 proof for reachable Route 5/Route 3 disagreement, missing source-memory, source-producer index mismatch, and incomplete publication rows. SP-3 has riscv consumer proof for dynamic memory-source byte-offset and incomplete Route 3 mutations preserving output while setting agreement booleans false. | SP-4 has helper/oracle proof for copied source-memory fields, offset mismatch rejection, unnamed source-memory rejection, and duplicate/invalid lookup preservation. SP-5 is mapped as fallback/helper state that must be joined to current Route 3/Route 5 authority before semantic use. | All SP rows remain blocked because no synthetic stale row matrix proves old prepared source-memory/publication facts fail closed for producer block, instruction index, source value, base kind, wrong edge, duplicate, and obsolete Route 5 owner across both x86 and riscv same-consumer surfaces. | Preserved riscv instruction text on false agreement and x86 fallback/error behavior are compatibility contracts only, not evidence that stale prepared data can become BIR authority. |
| Byte-offset drift | BO-1 has selected x86 guardrail proof for a reachable selected `LoadLocal` offset mutation and source code checks for effective Route 3 offset equality before agreement. BO-2 has riscv consumer proof that an embedded Route 3 offset mutation preserves prepared output while Route 5 and Route 3 agreement booleans become false. BO-5 has partial x86/riscv guardrail evidence after agreement diagnostics are attached. | BO-3 has helper proof that copied publication/access offsets must match. BO-4 has prepared/BIR same-block helper proof that store/load and prepared addressing offset mismatches reject helper matches. | All BO rows remain blocked because current proof is either selected-path or helper-level. It does not cover every synthetic prepared offset drift row, every public lookup consumer, every target operand surface, or all wrapper/exact-output contracts. | Large-offset syntax, base-plus-offset legality, frame-slot layout, stack offset choice, and target operand spelling remain target/prepared policy, not BIR ownership. |
| Cross-publication mismatch | CP-1 has partial x86 selected-path proof for Route 5/Route 3 source-memory mismatch and producer-index mismatch, plus riscv diagnostic-path proof for scalar Route 5 no-match and dynamic memory-source mismatch/incomplete rows. CP-5 has partial cross-target evidence that x86 rejection and riscv preserved output can both be valid when agreement diagnostics fail closed. | CP-2, CP-3, and CP-4 have helper/MIR/Route 5 proof for missing source/destination, wrong predecessor/successor, destination type mismatch, duplicate Route 5 diagnostics, non-memory producer handling, local source-memory mismatch, and explicit no-source/no-match statuses. | All CP rows remain blocked because no combined memory-source target proof exhausts internally consistent but wrong prepared owners, duplicate/conflicting Route 5 memory-source rows, wrong-edge/wrong-destination memory-source rows, and cross-target status/fallback/output matrices. | Differences between x86 selected-output rejection and riscv preserved prepared-backed output are target compatibility policy, not a reason to move target output behavior into BIR or hide public prepared lookups. |

Disposition by row class:

- Proof-supported guardrails exist for selected x86 `LoadLocal` Route 3/Route 5
  agreement, selected x86 disagreement rejection, riscv dynamic memory-source
  false-agreement diagnostics with preserved output, and helper/oracle local
  mismatch/status behavior.
- Blocked rows are still the controlling result: every family contains rows
  that lack same-consumer proof across the still-public prepared lookup,
  helper/oracle/status/fallback/route-debug/printer, wrapper-output,
  target-output, and baseline surfaces named by the source idea.
- `PreparedFunctionLookups::memory_accesses` is not demotion-ready. The map
  proves why the table must remain retained public compatibility until future
  work supplies full same-consumer fail-closed proof.
- No later implementation idea is safe to create from this map yet. A future
  implementation proposal would need to pick one blocked row, name the exact
  real consumer path, and prove stable helper/oracle/status/fallback/debug,
  wrapper-output, target-output, and baseline behavior without weakening
  expectations or hiding prepared authority behind a rename.
- No separate follow-up idea creation is requested by this packet. The current
  source idea asked for the bounded proof map itself, and that map is now
  complete.
- Closure should be requested from the plan owner, not deactivation. The active
  runbook is exhausted and the source-idea completion criteria are satisfied
  as a proof-map result, while explicitly preserving the no-demotion
  conclusion.

## Suggested Next

Ask the plan owner to close the active source idea as a completed analysis-only
proof map. The close packet should preserve the final disposition that
`memory_accesses` remains public compatibility and that no implementation or
demotion follow-up is authorized by this runbook.

## Watchouts

- Do not demote, delete, privatize, wrap, or migrate
  `PreparedFunctionLookups::memory_accesses`.
- Do not create an implementation idea from this map without a narrower source
  idea that names one blocked row and its exact same-consumer proof surface.
- Do not weaken unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- Do not collapse helper/oracle proof into target proof. Same-consumer x86 or
  riscv proof is required before any future demotion/private-accessor proposal.
- Do not treat supported selected-path x86/riscv agreement as public lookup
  retirement proof.

## Proof

No build/test proof was required for this analysis-only packet. No
`test_after.log` was generated or modified.
