Status: Active
Source Idea Path: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide Draft 155 and Follow-Up Shape

# Current Packet

## Just Finished

Step 4, `Decide Draft 155 and Follow-Up Shape`, converted the Step 3
thin-enough gate into a disposition decision and a bounded Step 5 follow-up
shape. No row is safe for demotion/exit implementation, so Step 5 should create
only bounded analysis/proof ideas.

Rows thinner than after idea 247:

| Row | Why thinner | Step 4 disposition |
| --- | --- | --- |
| `PreparedFunctionLookups::call_plans` | Phase F3 proved the selected x86 scalar named-`i32` Route 6 call argument source identity adapter, with riscv explicitly non-applicable for that fact family. | Retain as public compatibility authority; do not create a follow-up. Public call-plan APIs, `ConsumedPlans`, helper/oracle statuses, fallback names, route-debug names, wrapper output, unsupported rows, and target ABI/register/stack/result policy remain observable contracts. |
| `PreparedFunctionLookups::memory_accesses` | Phase F3 added x86 Route 3 `LoadLocal` source-memory agreement bridge support and supporting fixture/compare-join evidence, alongside existing riscv diagnostics. | Still blocked; create only a bounded fail-closed proof follow-up for unsupported/stale surfaces. |
| `PreparedFunctionLookups::edge_publications` | Phase F3 added the x86 Route 5/prepared selected dynamic `LoadLocal` edge-publication agreement bridge with Route 3 agreement gating, alongside existing riscv diagnostics. | Still blocked; create only a bounded fail-closed proof follow-up for unsupported publication surfaces. |
| `PreparedBirModule::module`, `names`, `control_flow`, `store_source_publications` | Phase F3 completed all named structural one-reader candidates and proved local agreement/fail-closed packets. | Retain public aggregate compatibility; no demotion follow-up. Completed one-reader packets do not prove whole-field exit. |
| `PreparedBirModule::route` | Phase F3 demoted route metadata behind private storage plus `prepared_route(const PreparedBirModule&)`. | Already private pass context; no Step 5 follow-up. |

Rows still blocked and exact missing proof:

| Row | Missing proof |
| --- | --- |
| `PreparedFunctionLookups::memory_accesses` | Supported fail-closed proof for prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows. Current x86 proof covers the positive selected `LoadLocal` path plus naturally reachable missing/incomplete/drift rows only; unsupported/stale prepared state still lacks a safe proof route. |
| `PreparedFunctionLookups::edge_publications` | Supported fail-closed proof for duplicate Route 5 records on one natural edge, prepared-only rows, Route 5-only publication rows, and wrong-edge publication rows. Current x86 bridge covers selected dynamic `LoadLocal` agreement and reachable Route 5/Route 3 disagreement only. |
| `PreparedFunctionLookups::edge_publication_source_producers` | A concrete x86 consumer boundary that joins prepared source-producer rows to the same Route 5/BIR source-producer identity and rejects disagreement, plus fail-closed proof for duplicate/conflict/mismatch/missing, prepared-only, fallback, `LoadLocal` memory-source, immediate-producer, and policy-sensitive rows. |
| `PreparedBirModule::liveness` | One exact identity-only reader, one semantic fact, the retained compatibility surface, and full fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict, unsupported, fallback, and derived printer/target behavior. |
| `PreparedBirModule::invariants`, `completed_phases`, `notes` | Printer/status/debug preservation and fail-closed proof for invalid, mismatched, missing, direct-payload-read, and absent metadata behavior before any future private-pass-context demotion. |
| `PreparedBirModule::module`, `names`, `control_flow`, `store_source_publications` | Whole-field public aggregate exit proof remains missing: public aggregate exposure, lookup surfaces, printer/debug/route-debug output, fallback behavior, target output, and target-policy-sensitive rows are still observable compatibility or policy contracts. |

Rows that remain target policy and never move to BIR:

- ABI call sequence, argument/result layout, registers, stack, storage,
  addressing modes, source homes, carrier/helper choice, runtime helper
  selection, frame/layout policy, move scheduling, branch/layout policy,
  instruction spelling, formatting, emission, wrapper behavior, route-debug
  rendering, exact target output, and target-specific publication/output policy.

Rows already private pass context or proof-gate-only:

- Already private pass context: `PreparedBirModule::route`.
- Proof-gate-only before any future private pass-context packet:
  `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`, and
  `PreparedBirModule::notes`.
- No other row is ready for a private pass-context packet.

Draft 155 disposition:

- Keep blocked.
- Do not rewrite draft 155 as a broad `PreparedBirModule`,
  `PreparedFunctionLookups`, or prepared aggregate retirement plan.
- Do not retire draft 155 obsolete yet, because successor analysis/proof ideas
  have not closed all useful public lookup, compatibility, metadata, liveness,
  and fail-closed proof scope.
- Do not supersede draft 155 with implementation demotion ideas from this gate.
  The useful near-term replacement shape is a small set of analysis/proof
  follow-ups; a later lifecycle gate may decide whether their closed evidence
  justifies rewriting, superseding, or retiring draft 155.

Intended bounded follow-up set for Step 5:

1. `PreparedFunctionLookups::memory_accesses` unsupported/stale fail-closed
   proof map, limited to prepared-only, stale-publication, byte-offset drift,
   and cross-publication mismatch behavior. This is analysis/proof only, not
   memory lookup demotion.
2. `PreparedFunctionLookups::edge_publications` unsupported publication
   fail-closed proof map, limited to duplicate Route 5 records on one natural
   edge, prepared-only rows, Route 5-only rows, and wrong-edge rows. This is
   analysis/proof only, not edge-publication lookup demotion.
3. `PreparedFunctionLookups::edge_publication_source_producers` x86 consumer
   boundary and fail-closed proof map, limited to one producer/source relation.
   This is analysis/blocker mapping only unless it names a real x86 consumer and
   supported fail-closed route.
4. `PreparedBirModule::liveness` authority blocker follow-up, limited to
   finding one exact identity-only reader, one semantic fact, the retained
   compatibility surface, and the full fail-closed proof set. This is not a
   demotion or private-accessor packet.
5. `PreparedBirModule::invariants`, `completed_phases`, and `notes` metadata
   proof gate, limited to printer/status/debug preservation plus invalid,
   mismatched, missing, direct-payload-read, and absent metadata fail-closed
   behavior. This is a proof gate only before any later private-pass-context
   proposal.

## Suggested Next

Proceed to Step 5, `Create Bounded Follow-Up Ideas`. Create only the bounded
analysis/proof ideas listed above. Do not create implementation demotion/exit
ideas from this gate.

## Watchouts

Step 5 idea files should include reject signals against expectation weakening,
helper/oracle/status/fallback/route-debug/printer/wrapper output weakening,
target-policy migration into BIR, classification-only progress claims, broad
prepared retirement, and old public prepared authority hidden behind a new BIR,
route, private-accessor, adapter, or wrapper name.

## Proof

No build/test proof required for this analysis-only packet. No implementation
files were touched. No `test_after.log` was generated or modified for this
packet.
