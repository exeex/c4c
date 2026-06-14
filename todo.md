Status: Active
Source Idea Path: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Apply the Thin-Enough Gate

# Current Packet

## Just Finished

Step 3, `Apply the Thin-Enough Gate`, rechecked the Step 2 post-F3 boundary
matrix against the full thin-enough standard from `plan.md` and the source
idea. The gate result is that no row currently survives as a safe demotion/exit
implementation candidate. The only safe next-work shapes are bounded analysis
or proof initiatives; public prepared authority must remain explicit where the
evidence does not yet prove a compatibility mirror.

Gate decisions by candidate class:

| Candidate class | Rows | Gate decision | Reason |
| --- | --- | --- | --- |
| Demotion-ready rows | None | No safe demotion/exit implementation candidate accepted. | Every thinner row still fails at least one required gate: remaining public prepared surfaces are not fully proven compatibility mirrors, unsupported/stale/fallback/policy-sensitive rows do not all fail closed, or one target side lacks an agreement-gated consumer or explicit fail-closed non-applicability. |
| Retained compatibility authority | `PreparedFunctionLookups::call_plans`, `PreparedBirModule::module`, `PreparedBirModule::names`, `PreparedBirModule::control_flow`, `PreparedBirModule::store_source_publications`, `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`, `PreparedBirModule::notes`, and prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output compatibility surfaces | Retain as compatibility authority; do not demote or hide behind renamed accessors. | `call_plans` is the thinnest row because Route 6 scalar call identity has x86 agreement-gated use and riscv non-applicability, but public call-plan APIs, `ConsumedPlans`, helper/oracle statuses, fallback names, route-debug names, wrapper output, unsupported rows, and target ABI/register/stack/result policy remain observable contracts. The structural rows have completed local one-reader agreement/fail-closed packets, but public aggregates, direct lookup surfaces, printer/debug/route-debug output, fallback behavior, and target output are still compatibility or target-owned. Metadata fields other than `route` remain public compatibility because printer/status/debug and invalid/mismatched metadata proof is incomplete. |
| Analysis-only blockers | `PreparedFunctionLookups::memory_accesses`, `PreparedFunctionLookups::edge_publications`, `PreparedFunctionLookups::edge_publication_source_producers`, `PreparedBirModule::liveness`, plus the retained metadata proof gaps for `invariants`, `completed_phases`, and `notes` | Safe only as bounded analysis/proof follow-up candidates for Step 4 consideration. | `memory_accesses` and `edge_publications` now have selected x86 agreement bridges and riscv diagnostic evidence, but unsupported prepared-only, stale, duplicate, route-only, wrong-edge, byte-offset drift, and cross-publication mismatch rows remain outside supported proof. `edge_publication_source_producers` lacks an x86 consumer boundary and full fail-closed proof. `liveness` still lacks one exact reader, one semantic fact, retained compatibility surface, and full fail-closed proof. Metadata rows need a separate proof initiative before any private-pass-context packet. |
| Still-blocked public prepared authority | `PreparedFunctionLookups::memory_accesses`, `PreparedFunctionLookups::edge_publications`, `PreparedFunctionLookups::edge_publication_source_producers`, `PreparedBirModule::liveness` | Keep blocked; do not claim public lookup/field demotion. | These rows still depend on public prepared authority for at least one missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only, route-only, fallback, or policy-sensitive path. The old failure modes would remain if a packet merely renamed the access path or used classification-only notes. |
| Target policy | ABI, layout, register, stack, storage, addressing, carrier/helper, formatting, emission, instruction spelling, wrapper, route-debug rendering, exact target output, and target-specific branch/layout policy portions referenced by the matrix | Target-policy-owned and not BIR/prepared-retirement candidates. | The gate treats these as contracts to preserve. They must not move into BIR and must not be used as evidence that semantic ownership transferred. |
| Private pass context already done | `PreparedBirModule::route` | Already completed; no Step 3 follow-up needed. | Idea 255 demoted the route metadata behind private storage plus `prepared_route(const PreparedBirModule&)` while preserving focused prepared-printer/CLI compatibility. |

Rejected unsafe demotion/exit candidates:

- `PreparedFunctionLookups::call_plans`: rejected as a demotion candidate even
  though the selected Route 6 scalar call sub-slice is thin enough for x86 use;
  public call-plan lookup/status surfaces and compatibility output remain
  explicit authority.
- `PreparedFunctionLookups::memory_accesses`: rejected because prepared-only,
  stale-publication, byte-offset drift, and cross-publication mismatch rows
  still lack supported fail-closed proof.
- `PreparedFunctionLookups::edge_publications`: rejected because duplicate Route
  5 records on one natural edge, prepared-only rows, Route 5-only publication
  rows, and wrong-edge publication rows still lack supported fail-closed proof.
- `PreparedFunctionLookups::edge_publication_source_producers`: rejected because
  the x86 consumer boundary is missing and duplicate/conflict/mismatch/missing,
  prepared-only, fallback, `LoadLocal` memory-source, immediate-producer, and
  policy-sensitive rows are not fully fail-closed.
- `PreparedBirModule::module`, `names`, `control_flow`, and
  `store_source_publications`: rejected for demotion because completed
  one-reader packets prove local agreement boundaries only; public aggregate
  exposure, public lookup surfaces, printer/debug/route-debug behavior,
  fallback rows, target output, and policy rows remain observable.
- `PreparedBirModule::invariants`, `completed_phases`, and `notes`: rejected
  for private-pass-context demotion until a separate proof initiative preserves
  printer/status/debug rows and fail-closes invalid, mismatched, missing, and
  absent metadata behavior.
- `PreparedBirModule::liveness`: rejected because the F3 map found no safe
  one-reader implementation candidate and the field remains public prepared
  planning authority.

Bounded candidates that survive only for Step 4 shaping:

- Analysis/proof candidate: `PreparedFunctionLookups::memory_accesses`
  unsupported/stale fail-closed surface, limited to prepared-only,
  stale-publication, byte-offset drift, and cross-publication mismatch behavior.
- Analysis/proof candidate: `PreparedFunctionLookups::edge_publications`
  unsupported publication fail-closed surface, limited to duplicate Route 5
  records on one natural edge, prepared-only rows, Route 5-only rows, and
  wrong-edge rows.
- Analysis blocker candidate: `PreparedFunctionLookups::edge_publication_source_producers`
  x86 consumer boundary plus fail-closed proof map, limited to one
  producer/source relation.
- Analysis blocker candidate: `PreparedBirModule::liveness`, limited to finding
  one exact reader, one semantic fact, the retained compatibility surface, and
  the full fail-closed proof set.
- Metadata proof candidate: `PreparedBirModule::invariants`,
  `PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`, only as
  a proof gate before any future private-pass-context demotion. This is not yet
  an implementation candidate.

Preserved contracts:

- Helper/oracle statuses, fallback names, route-debug output, prepared printer
  output, wrapper output, exact target output, and baseline behavior are
  compatibility contracts to preserve, not proof of semantic transfer.
- The accepted baseline behavior remains evidence that must stay stable; it is
  not demotion proof.

## Suggested Next

Proceed to Step 4, `Decide Draft 155 and Follow-Up Shape`. Record draft 155 as
blocked or superseded by bounded follow-ups unless the supervisor chooses a
different lifecycle disposition, and shape follow-up work from the bounded
analysis/proof candidates above rather than creating a demotion/exit
implementation idea.

## Watchouts

Do not let Step 4 turn a surviving analysis/proof candidate into an
implementation demotion. The reviewer reject signals still apply: no
expectation weakening, no helper/oracle/status/fallback/route-debug/printer or
wrapper output weakening, no target-policy migration into BIR, no classification
only progress claims, and no retaining old public prepared authority behind a
new BIR, route, private-accessor, adapter, or wrapper name.

## Proof

No build/test proof required for this analysis-only packet. No implementation
files were touched. No `test_after.log` was generated or modified for this
packet.
