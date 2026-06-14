Status: Active
Source Idea Path: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build the Boundary Matrix

# Current Packet

## Just Finished

Step 2, `Build the Boundary Matrix`, completed the post-F3 boundary matrix for
every in-scope prepared row. Each row below is represented exactly once and uses
one allowed plan classification.

| Row | Current semantic authority | Consumers / compatibility boundary | Classification | Blocking or retention reason |
| --- | --- | --- | --- | --- |
| `PreparedFunctionLookups::call_plans` | Route 6 scalar `i32` call argument source identity is the semantic fact for the proven sub-slice. | X86 consumes the selected fact through `find_consumed_scalar_i32_call_argument_source_authority(...)` with Route 6/prepared `source_value_id` agreement; riscv is explicitly non-applicable for this fact family; public call-plan APIs, `ConsumedPlans`, wrapper output, route-debug names, fallback names, helper/oracle statuses, unsupported rows, and target ABI/register/stack/result policy stay observable. | retained compatibility authority | Post-F3 evidence proves the selected x86 adapter and riscv non-applicability, but closure notes still retain public call-plan lookup/status surfaces as compatibility authority rather than approving public lookup demotion or broad call-plan retirement. |
| `PreparedFunctionLookups::memory_accesses` | Route 3 `LoadLocal` result/source-memory identity is the semantic fact for the proven selected-`LoadLocal` sub-slice. | Riscv has diagnostic agreement evidence. X86 now requires matching Route 3 `LoadLocal` memory authority plus `PreparedEdgePublication::source_memory_access` before treating prepared data as the semantic mirror. Prepared lookup/status behavior, helper/oracle names, fallback names, x86 output spelling, and riscv prepared edge-publication behavior stay stable. | still blocked by missing x86/riscv parity or missing fail-closed proof | Public lookup demotion is blocked because prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows remain outside the supported proof surface; they still lack fail-closed proof without synthetic or stale prepared state fixtures. |
| `PreparedFunctionLookups::edge_publications` | Route 5 CFG-edge publication source identity, with Route 3 source-memory agreement for selected dynamic `LoadLocal` publication moves. | Riscv exposes diagnostic Route 5 agreement fields. X86 now requires Route 5/prepared agreement for selected dynamic `LoadLocal` publication moves and requires Route 3 source-memory agreement before accepting those moves. Non-`LoadLocal` non-agreeing rows remain on preserved prepared compatibility paths. | still blocked by missing x86/riscv parity or missing fail-closed proof | Public edge-publication lookup demotion is blocked by missing supported fail-closed proof for duplicate Route 5 records on one natural edge, prepared-only rows, Route 5-only publication rows, and wrong-edge publication rows. |
| `PreparedFunctionLookups::edge_publication_source_producers` | Same-edge CFG publication source-producer identity, with `LoadLocal` requiring Route 5 `memory_source` plus Route 3 source-memory identity agreement. | Riscv has diagnostic agreement evidence only. X86 has no direct or indirect consumer that joins prepared source-producer rows to the same Route 5/BIR identity and rejects disagreement fail-closed. Public source-producer lookup authority, helper/oracle names, compatibility strings, fallback names, prepared statuses, target policy rows, and output boundaries remain compatibility-owned. | still blocked by missing x86/riscv parity or missing fail-closed proof | Missing x86 consumer boundary and full fail-closed proof for duplicate, conflict, mismatch, missing/absent, prepared-only, fallback, `LoadLocal` memory-source, immediate-producer, and policy-sensitive rows. |
| `PreparedBirModule::module` | Embedded BIR module/function/block structure is the semantic structure for completed lookup/printer sub-slices. | Completed one-reader packets added BIR function/block structure agreement helpers and complete-module-text agreement boundaries while preserving public prepared aggregate fields, prepared lookup maps, prepared-printer section order, headers, note/phase/invariant output, spacing, global/string compatibility, and raw-label compatibility. | retained compatibility authority | The row is thinner after completed structural packets, but public aggregate field compatibility remains observable and no closure approved broad `PreparedBirModule` deletion, privatization, wrapping, or aggregate retirement. |
| `PreparedBirModule::names` | Prepared/BIR value-name, function-name, and block-label agreement is the semantic authority for completed resolver and lookup sub-slices. | Completed one-reader packets cover same-block value-name lookup, value-home lookup, and semantic resolver APIs with fail-closed rows for unnamed/empty/missing/stale/conflicting/drifted names and retained direct `names.*.find(...)` compatibility. | retained compatibility authority | Public names-table compatibility, route-debug/formatting/printer behavior, raw spellings/ids, and direct lookup surfaces remain observable; no public field demotion or aggregate retirement proof exists. |
| `PreparedBirModule::control_flow` | Route/control-flow dominance, BIR conditional target identity, and BIR block-id/prepared block-label agreement are semantic facts for completed sub-slices. | Completed one-reader packets cover prior preserved-value lookup, branch-target identity, and block-index label bridge with prepared fallbacks and invalid-label behavior preserved. | retained compatibility authority | The proven helpers are local agreement boundaries, not approval to migrate all control-flow helpers or retire the public prepared field; remaining public compatibility and target branch/layout policy boundaries must stay stable. |
| `PreparedBirModule::store_source_publications` | BIR load/store identity, LoadLocal/prepared memory agreement, prepared planner agreement, and source-producer/value identity are semantic facts for completed sub-slices. | Completed one-reader packets cover recovered-source identity, byval pointer-source classification, direct-global select-chain dependency, and source-value/source-producer metadata while preserving prepared publication planning, call-plan policy, status, intent, destination access, source home, storage encoding, recovered/direct-global flags, pointer-base homes, pending policy, and duplicate policy. | retained compatibility authority | All named structural candidates are completed, but prepared publication planning remains the policy owner and no proof approves public field demotion, aggregate retirement, or migration of unrelated backend, printer, debug, fallback, target-output, or policy rows. |
| `PreparedBirModule::route` | Private pass context route metadata behind `prepared_route(const PreparedBirModule&)`. | Idea 255 already demoted the field behind private storage and preserved focused prepared-printer/CLI compatibility. | private pass context already done | No further Step 2 blocker for this row; it is the only completed private pass-context metadata demotion from the F3 metadata gate. |
| `PreparedBirModule::invariants` | Prepared metadata remains the observable source for invariant printer/status/debug compatibility. | Direct payload reads, invalid or mismatched metadata fences, and printer/status rows were not fully proved in the F3 metadata gate. | retained compatibility authority | Retained as a public compatibility field; not demotion-approved until a separate proof initiative preserves printer/status/debug rows and rejects invalid/mismatched metadata fail-closed. |
| `PreparedBirModule::completed_phases` | Prepared metadata remains the observable source for phase printer/status/debug compatibility. | Phase output/header behavior and direct payload reads remain compatibility-sensitive. | retained compatibility authority | Retained as a public compatibility field; not demotion-approved until a separate proof initiative proves phase printer/status/debug stability and missing/mismatched metadata fail-closed behavior. |
| `PreparedBirModule::notes` | Prepared metadata remains the observable source for note printer/status/debug compatibility. | Note output, absent-note omission behavior, and direct payload reads remain compatibility-sensitive. | retained compatibility authority | Retained as a public compatibility field; not demotion-approved until a separate proof initiative preserves absent-note fallback/omission behavior and fail-closed handling for invalid or mismatched note metadata. |
| `PreparedBirModule::liveness` | Prepared liveness remains public prepared planning authority. | The F3 liveness map identified producer, direct consumer, compatibility, and derived target buckets but found no safe one-reader implementation candidate. | still blocked by missing x86/riscv parity or missing fail-closed proof | Missing one exact reader, one semantic fact, retained prepared compatibility surface, and fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict, unsupported, fallback, and derived printer/target behavior. |
| prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output compatibility surfaces | Route/BIR facts may move underneath selected rows, but these public strings, statuses, fallback names, debug/printer text, wrapper output, target output, and baseline behavior are compatibility contracts rather than semantic ownership proof. | Idea 254 produced a reusable compatibility-retention proof matrix as citation/test obligation only. Existing F3 packets preserved helper/oracle statuses, fallback behavior, prepared printer output, route-debug output, wrapper output, exact target output, and baseline behavior for their focused surfaces. | retained compatibility authority | These surfaces must not be weakened, renamed, hidden, or treated as proof of semantic transfer; future packets must preserve them explicitly and keep ABI, layout, register, stack, storage, addressing, carrier/helper, formatting, emission, instruction spelling, and wrapper policy target-owned. |

Rows thinner than after idea 247:

- `PreparedFunctionLookups::call_plans`: selected Route 6 scalar call identity now has x86 agreement-gated consumption and riscv non-applicability, while public surfaces remain compatibility.
- `PreparedFunctionLookups::memory_accesses`: selected Route 3 `LoadLocal` source-memory identity now has x86 agreement-gated consumption and riscv diagnostics, with unsupported stale/synthetic fail-closed rows still blocking public lookup demotion.
- `PreparedFunctionLookups::edge_publications`: selected Route 5 dynamic `LoadLocal` publication moves now have an x86 agreement bridge plus riscv diagnostics, with unsupported duplicate/prepared-only/Route 5-only/wrong-edge rows still blocking public lookup demotion.
- `PreparedBirModule::module`, `names`, `control_flow`, and `store_source_publications`: all named one-reader structural candidates completed local agreement/fail-closed packets, but public aggregate fields remain retained compatibility authority.
- `PreparedBirModule::route`: private pass-context demotion is already complete.

Rows still blocked by exact missing proof:

- `memory_accesses`: fail-closed proof for prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows.
- `edge_publications`: fail-closed proof for duplicate Route 5 records on one natural edge, prepared-only rows, Route 5-only publication rows, and wrong-edge publication rows.
- `edge_publication_source_producers`: x86 consumer boundary plus duplicate/conflict/mismatch/missing/prepared-only/fallback/`LoadLocal` memory-source/immediate-producer/policy-sensitive fail-closed proof.
- `liveness`: one exact reader, one semantic fact, retained prepared compatibility surface, and fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict, unsupported, fallback, and derived printer/target behavior.

Rows that should remain target policy and not move to BIR:

- Target ABI, layout, register, stack, storage, addressing, carrier/helper, formatting, emission, instruction spelling, wrapper, route-debug rendering, exact target-output, and target-specific branch/layout policy portions are not BIR/prepared-retirement candidates. They are referenced by the compatibility row and must remain target-owned.

Rows already private pass context or plausible private-pass-context follow-up:

- Already done: `PreparedBirModule::route`.
- Plausible only after separate proof, not currently ready: `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

## Suggested Next

Proceed to Step 3, `Apply the Thin-Enough Gate`, by rechecking the rows that
look thinner after F3 against the full thin-enough standard. The highest-value
checks are `call_plans`, `memory_accesses`, `edge_publications`,
`PreparedBirModule::module`, `names`, `control_flow`,
`store_source_publications`, and the metadata rows, because Step 3 must
separate true implementation candidates from retained compatibility authority
and still-blocked public prepared authority.

## Watchouts

Do not treat local one-reader agreement helpers as public field retirement
proof. Do not treat helper/oracle statuses, fallback names, route-debug output,
prepared printer output, wrapper output, exact target output, or the restored
3428/3428 baseline as evidence that compatibility contracts may change.

No row in this Step 2 matrix is marked `ready for one concrete demotion/exit
implementation idea`; Step 3 may still narrow a follow-up shape, but it should
reject any implementation packet that hides old public prepared authority behind
a renamed accessor, weakens compatibility behavior, or relies on unsupported
synthetic/stale rows without fail-closed proof.

## Proof

No build/test proof required for this analysis-only packet. No implementation
files were touched. No `test_after.log` was generated or modified for this
packet.
