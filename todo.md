Status: Active
Source Idea Path: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Close or Reassess the Gate

# Current Packet

## Just Finished

Step 6, `Close or Reassess the Gate`, was reassessed rather than closed.

The source-output evidence is complete enough for a closure note, and the
closure-ready matrix is recorded below. The active lifecycle state remains open
because the close-time regression guard could not be accepted from the current
canonical logs: `test_before.log` covers one test, while `test_after.log` covers
the full 3428-test suite. The delegated packet also marked
`test_baseline.log`, `test_before.log`, and `test_after.log` as do-not-touch, so
this packet did not regenerate or overwrite the required matching logs.

Close result: close rejected.

### Closure-Ready Boundary Matrix

| Row | Classification | Boundary decision |
| --- | --- | --- |
| `PreparedFunctionLookups::call_plans` | retained compatibility authority | Route 6 scalar `i32` call argument source identity is proven for the selected x86 adapter, and riscv is explicitly non-applicable for that fact family. Public call-plan APIs, `ConsumedPlans`, helper/oracle statuses, fallback names, route-debug names, wrapper output, unsupported rows, and target ABI/register/stack/result policy remain observable compatibility or target-policy contracts. |
| `PreparedFunctionLookups::memory_accesses` | still blocked by missing x86/riscv parity or missing fail-closed proof | Route 3 selected `LoadLocal` source-memory identity has x86 agreement-gated support and riscv diagnostic evidence, but public lookup demotion is blocked by missing fail-closed proof for prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows. |
| `PreparedFunctionLookups::edge_publications` | still blocked by missing x86/riscv parity or missing fail-closed proof | Route 5 selected dynamic `LoadLocal` edge-publication agreement exists for x86, with Route 3 agreement gating and riscv diagnostics, but public lookup demotion is blocked by missing fail-closed proof for duplicate Route 5 records on one natural edge, prepared-only rows, Route 5-only publication rows, and wrong-edge rows. |
| `PreparedFunctionLookups::edge_publication_source_producers` | still blocked by missing x86/riscv parity or missing fail-closed proof | Same-edge publication source-producer identity lacks a concrete x86 consumer boundary that joins prepared source-producer rows to the same Route 5/BIR identity and rejects disagreement. Duplicate, conflict, mismatch, missing, prepared-only, fallback, `LoadLocal` memory-source, immediate-producer, and policy-sensitive rows still need fail-closed proof. |
| `PreparedBirModule::module` | retained compatibility authority | Completed structural packets proved local BIR/prepared module-text and function/block-structure agreement, but public aggregate field compatibility, prepared lookup maps, prepared-printer section order, headers, note/phase/invariant output, spacing, global/string compatibility, and raw-label compatibility remain observable. |
| `PreparedBirModule::names` | retained compatibility authority | Completed packets cover value-name, function-name, block-label, resolver, and lookup sub-slices with fail-closed rows, but direct names-table lookup compatibility, route-debug/formatting/printer behavior, raw spellings/ids, and public surfaces remain observable. |
| `PreparedBirModule::control_flow` | retained compatibility authority | Completed packets cover prior preserved-value lookup, branch-target identity, and block-index label bridge with fallbacks and invalid-label behavior preserved. They do not prove all control-flow helpers or the public field can exit. |
| `PreparedBirModule::store_source_publications` | retained compatibility authority | Completed packets cover recovered-source identity, byval pointer-source classification, direct-global select-chain dependency, and source-value/source-producer metadata. Prepared publication planning, call-plan policy, status, intent, destination access, source home, storage encoding, recovered/direct-global flags, pointer-base homes, pending policy, and duplicate policy remain retained. |
| `PreparedBirModule::route` | private pass context already done | Idea 255 already demoted route metadata behind private storage and `prepared_route(const PreparedBirModule&)` while preserving focused prepared-printer/CLI compatibility. |
| `PreparedBirModule::invariants` | retained compatibility authority | Public compatibility remains until a separate proof initiative preserves printer/status/debug rows and fail-closes invalid, mismatched, missing, direct-payload-read, and absent metadata behavior. |
| `PreparedBirModule::completed_phases` | retained compatibility authority | Public compatibility remains until a separate proof initiative preserves phase printer/status/debug behavior and fail-closes invalid, mismatched, missing, direct-payload-read, and absent metadata behavior. |
| `PreparedBirModule::notes` | retained compatibility authority | Public compatibility remains until a separate proof initiative preserves note output, absent-note omission behavior, direct payload reads, and fail-closed handling for invalid or mismatched note metadata. |
| `PreparedBirModule::liveness` | still blocked by missing x86/riscv parity or missing fail-closed proof | The F3 liveness map found no safe one-reader implementation candidate. The row still needs one exact identity-only reader, one semantic fact, a retained prepared compatibility surface, and fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict, unsupported, fallback, and derived printer/target behavior. |
| prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output compatibility surfaces | retained compatibility authority | These public strings, statuses, fallback names, debug/printer text, wrapper output, target output, and baseline behavior are compatibility contracts, not proof of semantic ownership transfer. Future packets must preserve them explicitly. |

### Thinner Rows

- `PreparedFunctionLookups::call_plans`: selected Route 6 scalar call identity now has x86 agreement-gated consumption and riscv non-applicability, while public surfaces remain compatibility.
- `PreparedFunctionLookups::memory_accesses`: selected Route 3 `LoadLocal` source-memory identity now has x86 agreement-gated consumption and riscv diagnostics, with unsupported stale/synthetic fail-closed rows still blocking public lookup demotion.
- `PreparedFunctionLookups::edge_publications`: selected Route 5 dynamic `LoadLocal` publication moves now have an x86 agreement bridge plus riscv diagnostics, with unsupported duplicate/prepared-only/Route 5-only/wrong-edge rows still blocking public lookup demotion.
- `PreparedBirModule::module`, `names`, `control_flow`, and `store_source_publications`: all named structural one-reader candidates completed local agreement/fail-closed packets, but public aggregate fields remain retained compatibility authority.
- `PreparedBirModule::route`: private pass-context demotion is already complete.

### Blocked Rows and Missing Proofs

- `memory_accesses`: supported fail-closed proof for prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows.
- `edge_publications`: supported fail-closed proof for duplicate Route 5 records on one natural edge, prepared-only rows, Route 5-only publication rows, and wrong-edge publication rows.
- `edge_publication_source_producers`: a concrete x86 consumer boundary that joins prepared source-producer rows to the same Route 5/BIR source-producer identity and rejects disagreement, plus fail-closed proof for duplicate/conflict/mismatch/missing, prepared-only, fallback, `LoadLocal` memory-source, immediate-producer, and policy-sensitive rows.
- `liveness`: one exact identity-only reader, one semantic fact, the retained compatibility surface, and full fail-closed proof for absent/skipped, stale, mismatch, duplicate/conflict, unsupported, fallback, and derived printer/target behavior.
- `invariants`, `completed_phases`, and `notes`: printer/status/debug preservation and fail-closed proof for invalid, mismatched, missing, direct-payload-read, and absent metadata behavior before any future private-pass-context demotion.
- `module`, `names`, `control_flow`, and `store_source_publications`: whole-field public aggregate exit proof is still missing for public aggregate exposure, lookup surfaces, printer/debug/route-debug output, fallback behavior, target output, and target-policy-sensitive rows.

### Target-Policy Rows

ABI call sequence, argument/result layout, registers, stack, storage,
addressing modes, source homes, carrier/helper choice, runtime helper
selection, frame/layout policy, move scheduling, branch/layout policy,
instruction spelling, formatting, emission, wrapper behavior, route-debug
rendering, exact target output, and target-specific publication/output policy
remain target-owned and must not move to BIR.

### Private Pass-Context Rows

- Already private pass context: `PreparedBirModule::route`.
- Proof-gate-only before any future private pass-context packet:
  `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`, and
  `PreparedBirModule::notes`.
- No other row is ready for a private pass-context packet.

### Follow-Up Ideas Created

- `ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
- `ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md`
- `ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md`
- `ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md`
- `ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md`

### Draft 155 Disposition

Keep blocked. Do not rewrite draft 155 as a broad `PreparedBirModule`,
`PreparedFunctionLookups`, or prepared aggregate retirement plan. Do not retire
draft 155 obsolete yet, because successor analysis/proof ideas have not closed
all useful public lookup, compatibility, metadata, liveness, and fail-closed
proof scope. Do not supersede draft 155 with implementation demotion ideas from
this gate.

## Suggested Next

Supervisor can either provide matching close-scope `test_before.log` and
`test_after.log` artifacts, or authorize the plan owner/supervisor workflow to
regenerate matching canonical close logs. After a valid close-time regression
guard is available, this active idea can be closed with the matrix above unless
new lifecycle evidence changes the source-output decision.

## Watchouts

Do not treat the five follow-up ideas as demotion-ready evidence. They are
bounded analysis/proof contracts only. Draft 155 remains blocked until
successor proof ideas close enough useful public lookup, compatibility,
metadata, liveness, and fail-closed scope for a later lifecycle gate to
reassess it.

## Proof

No implementation files were touched. The existing canonical regression logs
were inspected but not modified. The close-time regression guard was not
accepted because `test_before.log` and `test_after.log` do not cover the same
test command or scope.
