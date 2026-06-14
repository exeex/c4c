# 264 Phase F4 post-F3 prepared boundary reassessment gate

## Goal

Reassess the BIR/prepared boundary after the completed Phase F3 blocker maps,
agreement bridges, structural one-reader candidates, and baseline repair, then
produce the next bounded follow-up ideas.

This is analysis-only. It must decide whether any prepared field group,
subfield, public lookup group, compatibility surface, or private metadata field
is now ready for a narrow demotion/exit packet. It must not directly implement
that packet, delete prepared aggregates, open draft 155, or claim broad
`PreparedBirModule` retirement.

## Why This Exists

The previous gates repeatedly found that broad prepared retirement was unsafe,
but Phase F3 changed the evidence:

- Route 6 call identity parity was mapped and the x86 scalar call argument
  source identity adapter was confirmed.
- Route 3 memory/source parity was mapped, x86 `LoadLocal` agreement bridge
  support landed, and the needed fixture/compare-join support closed.
- Route 4/5 edge-publication parity was mapped and x86 Route 5/prepared edge
  publication agreement bridge landed.
- Prepared compatibility fail-closed proof requirements were documented.
- `PreparedBirModule::route` was demoted behind private storage while
  `invariants`, `completed_phases`, and `notes` stayed retained.
- `PreparedBirModule::liveness` remained blocked planning authority.
- The structural one-reader queue for `module`, `names`, `control_flow`, and
  `store_source_publications` completed all named candidates.
- The full-suite baseline timeout candidate from idea 263 was diagnosed as
  non-reproducible in isolation, the fresh full-suite proof restored
  3428/3428, and the accepted baseline is green again.

That means the next question is no longer just "what is blocked?" The question
is whether any part of prepared is now thin enough to move from public
prepared authority into one of these stable categories:

- BIR-owned semantic fact;
- target-owned policy product;
- private pass context;
- retained explicit compatibility adapter;
- deletion/demotion candidate;
- still-blocked public prepared authority.

## Required Inputs

Read the closure notes for:

- `ideas/closed/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md`
- `ideas/closed/249_phase_f3_route6_call_identity_parity_blocker_map.md`
- `ideas/closed/250_phase_f3_route3_memory_source_parity_blocker_map.md`
- `ideas/closed/251_phase_f3_route45_edge_publication_parity_blocker_map.md`
- `ideas/closed/252_phase_f3_edge_publication_source_producer_blocker_map.md`
- `ideas/closed/253_phase_f3_prepared_module_structural_exit_blocker_map.md`
- `ideas/closed/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md`
- `ideas/closed/255_phase_f3_prepared_private_pass_context_metadata_gate.md`
- `ideas/closed/256_phase_f3_prepared_liveness_authority_blocker_map.md`
- `ideas/closed/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md`
- `ideas/closed/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
- `ideas/closed/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md`
- `ideas/closed/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `ideas/closed/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md`
- `ideas/closed/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md`
- `ideas/closed/263_phase_f3_full_suite_timeout_baseline_regression_diagnosis.md`

Also verify the current accepted baseline is still 3428/3428 and that no
`test_baseline.new.log` regression candidate is pending.

## In Scope

Reassess the current exit status of:

- `PreparedFunctionLookups::call_plans`;
- `PreparedFunctionLookups::memory_accesses`;
- `PreparedFunctionLookups::edge_publications`;
- `PreparedFunctionLookups::edge_publication_source_producers`;
- `PreparedBirModule::module`;
- `PreparedBirModule::names`;
- `PreparedBirModule::control_flow`;
- `PreparedBirModule::store_source_publications`;
- `PreparedBirModule::route`;
- `PreparedBirModule::invariants`;
- `PreparedBirModule::completed_phases`;
- `PreparedBirModule::notes`;
- `PreparedBirModule::liveness`;
- prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output
  compatibility surfaces.

For each row, decide whether it is:

- ready for one concrete demotion/exit implementation idea;
- ready only for another analysis blocker map;
- retained compatibility authority;
- target-policy-owned and not a BIR/prepared-retirement candidate;
- private pass context already done;
- still blocked by missing x86/riscv parity or missing fail-closed proof.

## Completion Standard

The boundary is "thin enough to connect x86/riscv" for a fact family only if
all of these are true:

- a single route/BIR semantic authority is named;
- x86 consumes it through an agreement-gated path, or has explicit
  fail-closed non-applicability;
- riscv consumes it through an agreement-gated path, or has explicit
  fail-closed non-applicability;
- public prepared surfaces that remain are compatibility mirrors, not the
  semantic source of truth;
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  route-only, fallback, and policy-sensitive rows fail closed;
- helper/oracle statuses, fallback names, route-debug output, prepared printer
  output, wrapper output, exact target output, and baseline behavior stay
  stable;
- ABI, layout, register, stack, storage, addressing, carrier/helper,
  formatting, emission, instruction spelling, and wrapper policy remain
  target-owned.

If any row fails this standard, do not mark it demotion-ready. Name the exact
missing proof or consumer boundary instead.

## Required Output

The closure note must include:

- an updated post-F3 boundary matrix for every in-scope row;
- a list of rows that are now thinner than they were after idea 247;
- a list of rows still blocked and the exact missing x86/riscv or
  compatibility proof;
- a list of rows that should remain target policy and never move to BIR;
- a list of rows that are already private pass context or ready for a private
  pass-context packet;
- follow-up idea files for each safe next packet;
- an explicit draft 155 disposition: keep blocked, rewrite, supersede, or
  retire obsolete.

Follow-up ideas must be bounded. Prefer one fact family, one lookup group,
one field sub-slice, or one private metadata field per idea. If no
implementation packet is safe, close with the blocker matrix and create only
analysis follow-ups.

## Out Of Scope

- Directly editing implementation code.
- Deleting, privatizing, or wrapping a prepared field in this gate.
- Opening draft 155 as broad retirement work.
- Claiming whole `PreparedBirModule` or whole `PreparedFunctionLookups`
  retirement from one-reader or one-target evidence.
- Moving target policy into BIR.
- Weakening expected strings, helper/oracle statuses, unsupported behavior,
  fallback behavior, wrapper output, prepared printer output, target output,
  or baseline tests.

## Reviewer Reject Signals

- The gate ignores any closure note from 248 through 263.
- The result repeats old blocker language without incorporating the completed
  F3 bridges and structural one-reader candidates.
- A row is marked demotion-ready without both x86/riscv evidence or explicit
  non-applicability, retained compatibility proof, and fail-closed behavior.
- Compatibility strings or prepared helper/oracle statuses are treated as proof
  of semantic ownership transfer.
- Public prepared authority is hidden behind a private accessor name instead
  of removed, retained explicitly, or proven as a compatibility mirror.
- The gate creates broad prepared retirement work instead of bounded next
  packets.

## Closure Note

Close accepted. The Phase F4 post-F3 prepared boundary reassessment gate is
complete as an analysis-only gate. No demotion or exit implementation packet is
safe from this gate; the safe output is the bounded follow-up proof set listed
below.

### Boundary Matrix

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

### Rows Thinner Than After Idea 247

- `PreparedFunctionLookups::call_plans`: selected Route 6 scalar call identity
  now has x86 agreement-gated consumption and riscv non-applicability, while
  public surfaces remain compatibility.
- `PreparedFunctionLookups::memory_accesses`: selected Route 3 `LoadLocal`
  source-memory identity now has x86 agreement-gated consumption and riscv
  diagnostics, with unsupported stale/synthetic fail-closed rows still blocking
  public lookup demotion.
- `PreparedFunctionLookups::edge_publications`: selected Route 5 dynamic
  `LoadLocal` publication moves now have an x86 agreement bridge plus riscv
  diagnostics, with unsupported duplicate/prepared-only/Route 5-only/wrong-edge
  rows still blocking public lookup demotion.
- `PreparedBirModule::module`, `names`, `control_flow`, and
  `store_source_publications`: all named structural one-reader candidates
  completed local agreement/fail-closed packets, but public aggregate fields
  remain retained compatibility authority.
- `PreparedBirModule::route`: private pass-context demotion is already
  complete.

### Blocked Rows And Missing Proofs

- `memory_accesses`: supported fail-closed proof for prepared-only,
  stale-publication, byte-offset drift, and cross-publication mismatch rows.
- `edge_publications`: supported fail-closed proof for duplicate Route 5
  records on one natural edge, prepared-only rows, Route 5-only publication
  rows, and wrong-edge publication rows.
- `edge_publication_source_producers`: a concrete x86 consumer boundary that
  joins prepared source-producer rows to the same Route 5/BIR source-producer
  identity and rejects disagreement, plus fail-closed proof for duplicate,
  conflict, mismatch, missing, prepared-only, fallback, `LoadLocal`
  memory-source, immediate-producer, and policy-sensitive rows.
- `liveness`: one exact identity-only reader, one semantic fact, the retained
  compatibility surface, and full fail-closed proof for absent/skipped, stale,
  mismatch, duplicate/conflict, unsupported, fallback, and derived
  printer/target behavior.
- `invariants`, `completed_phases`, and `notes`: printer/status/debug
  preservation and fail-closed proof for invalid, mismatched, missing,
  direct-payload-read, and absent metadata behavior before any future private
  pass-context demotion.
- `module`, `names`, `control_flow`, and `store_source_publications`:
  whole-field public aggregate exit proof is still missing for public aggregate
  exposure, lookup surfaces, printer/debug/route-debug output, fallback
  behavior, target output, and target-policy-sensitive rows.

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

### Follow-Up Ideas

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

### Close-Time Regression Evidence

Close-time regression guard used matching full-suite canonical logs:

```text
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
before: 3428/3428
after: 3428/3428
result: PASS
```
