# 248 Phase F2 x86/riscv prepared boundary completion criteria audit

## Goal

Define the concrete completion criteria for saying the BIR/prepared boundary is
clean enough to support x86/riscv portability work, then produce follow-up
ideas for the exact blocker families that still fail those criteria.

This is analysis-only. It must not implement adapters, delete prepared fields,
open draft 155, or claim prepared aggregate retirement readiness. Its job is to
turn the Phase F1 blocker map into precise "done enough to connect x86/riscv"
criteria and smaller follow-up ideas.

## Why This Exists

Ideas 243 through 247 already completed one portability and demotion-readiness
pass:

- `ideas/closed/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`
- `ideas/closed/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
- `ideas/closed/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
- `ideas/closed/246_phase_f1_prepared_publication_status_compatibility_retention.md`
- `ideas/closed/247_phase_f1_final_prepared_field_group_demotion_gate.md`

That pass proved selected x86/riscv route-native diagnostics, adapters,
compatibility-retention rows, and wrapper-output baselines. It also concluded
that no `PreparedBirModule` or `PreparedFunctionLookups` field group is ready
for final deletion, demotion, or privatization.

The remaining problem is not simply "more cleanup." The remaining problem is
that the BIR/prepared ownership boundary is not yet specified tightly enough to
decide when x86 and riscv can share the same semantic facts without prepared
still acting as a second semantic IR.

## Completion Target

The boundary is clean enough to connect x86/riscv only when each relevant
field group or fact family has one explicit classification:

- BIR-owned semantic fact;
- target-owned policy product;
- private prepared pass context;
- retained public compatibility adapter;
- blocked public prepared authority;
- deletion or demotion candidate.

For every fact family that x86 and riscv both need, the audit must define what
"done enough" means:

- the positive semantic fact is route/BIR-owned;
- x86 and riscv consume the same fact family, or the target-specific gap is
  explicitly named;
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive cases fail closed;
- wrapper output, route-debug output, helper/oracle statuses, expected strings,
  and baselines remain stable;
- ABI, layout, register, stack, storage, addressing, carrier/runtime helper,
  formatting, emission, and wrapper policy remain outside BIR ownership;
- prepared public APIs are either retained as named compatibility adapters or
  demoted behind private pass context with no public consumer left.

## In Scope

- Re-reading closure notes from 243 through 247.
- Inspecting the current ownership boundary for:
  - `call_plans`;
  - `memory_accesses`;
  - `edge_publications`;
  - `edge_publication_source_producers`;
  - `module`;
  - `names`;
  - `control_flow`;
  - `store_source_publications`;
  - prepared helper/oracle/status/fallback/route-debug/wrapper-output
    compatibility surfaces.
- Defining separate completion criteria for:
  - route/BIR semantic ownership;
  - x86 consumption;
  - riscv consumption;
  - public prepared compatibility retention;
  - target policy retention;
  - private pass-context demotion;
  - field-group deletion/demotion readiness.
- Producing follow-up ideas that are narrow enough for an agent to execute
  without broad route drift.

## Out Of Scope

- Direct implementation of any adapter or demotion.
- Broad deletion of `PreparedBirModule` or `PreparedFunctionLookups`.
- Opening draft 155 as a broad retirement plan.
- Claiming field-group readiness from one-reader, x86-only, riscv-only,
  diagnostic-only, compatibility-only, or baseline-only proof.
- Moving target policy into BIR.
- Renaming, weakening, hiding, or deleting prepared compatibility surfaces as a
  substitute for ownership transfer.

## Required Analysis

### 1. Boundary Classification Matrix

Build a matrix for the remaining blocker groups named by idea 247. Each row
must classify the group as BIR semantic, target policy, private pass context,
retained compatibility, blocked public authority, deletion candidate, or mixed.

Mixed groups must be split into smaller candidate rows. Do not leave a row
with only "mixed" as the conclusion.

### 2. x86/riscv Portability Readiness Criteria

For each candidate fact family, state the minimum evidence needed before it can
be called portable across x86/riscv:

- which BIR or route fact is the semantic authority;
- which x86 consumer must read it;
- which riscv consumer must read it;
- which prepared answer must remain as agreement/fallback;
- which diagnostics and strings must remain stable;
- which target-policy surfaces are adjacent and must not move.

### 3. Prepared Public API Exit Criteria

For each public prepared API or lookup group, state whether it can eventually:

- remain public as compatibility;
- become private pass context;
- split into target-policy product plus BIR-owned semantic index;
- be deleted after all public consumers move;
- stay blocked.

The audit must name the observable condition that makes each transition safe.

### 4. Follow-Up Idea Generation

Create follow-up ideas only when the criteria are concrete enough for bounded
execution. Each generated idea must say whether it is:

- analysis-only;
- one-adapter implementation;
- compatibility-retention proof;
- x86/riscv parity proof;
- private-pass-context demotion;
- deletion/demotion gate.

Prefer one fact family or one field-group sub-slice per idea. Do not create a
large "retire prepared" implementation idea.

## Expected Output

The closure note must include:

- a boundary classification matrix for the blocker groups from idea 247;
- a definition of "clean enough to connect x86/riscv" in operational terms;
- a list of x86/riscv fact families that are portable, partially portable, or
  blocked;
- a list of prepared public APIs that remain compatibility authority;
- a list of target-policy surfaces that must not move to BIR;
- a list of candidate private pass-context demotions, if any;
- follow-up idea files for every next executable packet;
- a disposition for draft 155: keep blocked, rewrite, supersede, or retire
  obsolete.

If no implementation packet is safe, close with a blocker map and analysis-only
follow-up ideas instead of forcing demotion work.

## Closure Notes

Status: complete as an analysis-only Phase F2 audit. The audit produced a
concrete ownership matrix, operational x86/riscv readiness criteria, prepared
public API exit criteria, bounded Phase F3 follow-up ideas, and an explicit
draft 155 disposition. It did not implement adapters, demote fields, delete
prepared APIs, rewrite baselines, or claim prepared aggregate retirement.

### Boundary Classification Matrix Summary

| Boundary family | Final classification | Closure result |
| --- | --- | --- |
| `call_plans`, `PreparedBirModule::call_plans`, and `find_prepared_call_plans` | blocked public prepared authority with adjacent Route 6 BIR semantic candidate | Retain public prepared authority until x86 direct-call/wrapper evidence and riscv consumer or fail-closed non-applicability are proven. |
| Route 6 scalar call-use/source identity | partially portable BIR-owned semantic fact | Accepted x86 scalar `i32` evidence is useful but not broad wrapper, direct-call, or riscv parity evidence. |
| `ConsumedPlans`, direct-call wrapper output, and x86 route-debug rows | retained public compatibility adapter | Keep strings, fallback rows, wrapper output, and status/debug names stable while Route 6 facts move underneath them. |
| `memory_accesses` and indexed memory lookup helpers | blocked public prepared authority with adjacent Route 3 BIR semantic candidate | Retain prepared lookup/status authority until x86 evidence and riscv route-driven agreement/fail-closed rows are proven for the same fact. |
| Route 3 memory/source identity | partially portable BIR-owned semantic fact | riscv evidence exists, but x86 evidence is missing or indirect and prepared fallback/status authority remains. |
| `edge_publications` and edge-publication lookup helpers | blocked public prepared authority with adjacent Route 4/5 BIR semantic candidate | Retain public lookup/status authority until x86 and riscv validate the same Route 4/5 fact through adapters. |
| Route 4/5 edge-publication identity | partially portable BIR-owned semantic fact | riscv agreement diagnostics exist, but x86 parity and full prepared agreement/fail-closed coverage remain incomplete. |
| x86/riscv edge-publication dispatch/status surfaces | retained public compatibility adapter | Preserve `EdgePublicationMoveIntentStatus`, status strings, fallback rows, module output, and exact target output. |
| `edge_publication_source_producers` and source-producer lookup helpers | blocked public prepared authority | No direct x86/riscv source-producer consumers or explicit non-applicability were proven; AArch64/prepared helper use keeps this public. |
| `PreparedBirModule::module` | retained compatibility wrapper around BIR module plus blocked public prepared aggregate authority | Underlying BIR structure is semantic authority, but targets/tests still access it through the prepared aggregate. |
| `PreparedBirModule::names` | split BIR semantic name index candidate plus retained debug/formatting compatibility; blocked today | Semantic lookup, route-debug text, formatting, printer output, and fallback rendering must be separated before exit. |
| `PreparedBirModule::control_flow` | split BIR CFG/dominance/block-index candidate plus target branch/layout policy; blocked today | Prepared CFG helpers remain public authority until route/BIR CFG facts and consumer migration are proven. |
| `PreparedBirModule::store_source_publications` | blocked public prepared authority with BIR store-source semantic candidate | Current x86/riscv evidence is indirect; source-memory/status compatibility remains authoritative. |
| Prepared helper/oracle statuses, fallback, unsupported behavior, route-debug, wrapper, printer, and target-output rows | retained public compatibility adapters | These rows are public oracle contracts and cannot be renamed, hidden, or weakened as proof of ownership transfer. |
| ABI, layout, registers, stack, storage, addressing, carriers/helpers, formatting, emission, instruction spelling, and wrapper policy | target-owned policy products | BIR may publish semantic facts consumed by target policy, but these policy decisions must not move to BIR. |
| `route`, `invariants`, `completed_phases`, and `notes` | private prepared pass-context demotion candidates | Candidate only after printer/status/debug compatibility and absence of target-facing public consumers are proven. |
| `liveness` | blocked prepared planning authority | Keep separate from simple metadata because prealloc/regalloc/helper-planning consumers and target-policy implications remain unmapped. |

### Operational Completion Criteria

The BIR/prepared boundary is clean enough to connect x86/riscv only when each
candidate fact family satisfies all of these conditions:

- one route/BIR semantic authority is named for the positive fact;
- x86 consumes that fact directly or through a bounded adapter, or has an
  explicit fail-closed non-applicability proof;
- riscv consumes the same fact directly or through a bounded adapter, or has
  an explicit fail-closed non-applicability proof;
- public prepared APIs that remain visible are compatibility mirrors, not the
  semantic source of truth;
- prepared agreement, fallback, helper/oracle status, route-debug, wrapper,
  printer, and target-output rows stay stable;
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive cases fail closed;
- ABI, layout, register, stack, storage, addressing, carrier/helper,
  formatting, emission, instruction spelling, and wrapper decisions remain
  target-owned policy.

### x86/riscv Fact-Family Readiness

Portable fact families: none. No current family has complete x86 evidence,
riscv evidence or explicit non-applicability, stable prepared compatibility,
and full fail-closed coverage.

Partially portable fact families:

- Route 6 scalar call-use/source identity: accepted x86 scalar evidence exists,
  but wrapper/direct-call breadth and riscv parity or explicit non-applicability
  are still missing.
- Route 3 memory/source identity: riscv Route 3/status evidence exists, but x86
  evidence is missing or indirect and prepared memory/status authority remains.
- Route 4/5 edge-publication identity: riscv agreement diagnostics exist, but
  x86 route/BIR agreement and full prepared status/emission agreement are still
  incomplete.
- Embedded BIR module/function/block structure: the underlying BIR is semantic
  authority, but the public prepared aggregate remains the observable handoff.
- Direct-call wrapper, route-debug, edge-publication status, fallback,
  unsupported, and helper/oracle rows are portable only as compatibility
  requirements, not as proof that semantic ownership transferred.
- `route`, `invariants`, `completed_phases`, and `notes` are partial demotion
  candidates, not portable semantic facts.

Blocked fact families:

- public `call_plans`, `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` lookup authority;
- source-producer identity;
- `PreparedBirModule::module`, `names`, `control_flow`, and
  `store_source_publications` public aggregate/field authority;
- semantic name index and CFG/dominance/block-index helper ownership until
  public prepared helpers are separated;
- `liveness`, because it remains active prepared planning authority.

### Prepared Public APIs That Remain Compatibility Authority

The following surfaces must remain public compatibility authority until a
future bounded idea proves a narrower exit path:

- `PreparedFunctionLookups::call_plans`,
  `PreparedBirModule::call_plans`, and `find_prepared_call_plans`;
- `PreparedFunctionLookups::memory_accesses` and public indexed memory lookup
  helpers;
- `PreparedFunctionLookups::edge_publications` and public edge-publication
  lookup helpers;
- `PreparedFunctionLookups::edge_publication_source_producers` and
  source-producer lookup helpers;
- `PreparedBirModule::module`, `names`, `control_flow`, and
  `store_source_publications`;
- prepared lookup helper/oracle status names, fallback names, unsupported
  fail-closed rows, source-memory statuses, publication statuses,
  `EdgePublicationMoveIntentStatus`, `ConsumedPlans`, route-debug output,
  wrapper output, prepared printer/debug text, module output, and exact target
  output rows.

### Target-Policy Surfaces That Must Not Move To BIR

Future packets must keep these policy decisions in target lowering/emission or
stable compatibility baselines:

- ABI call sequence and argument/result layout;
- register, stack, storage, home, scratch-register, and frame policy;
- addressing modes, load/store selection, and address materialization;
- carrier/runtime-helper selection;
- branch/layout/label policy;
- move/publication emission, instruction spelling, formatting, exact assembly
  text, wrapper behavior, and target-specific debug/output rendering.

### Private Pass-Context Demotion Candidates

Only `PreparedBirModule::route`, `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes` are
near-term private pass-context demotion candidates. They are not deletion-ready:
any demotion must first preserve printer/status/debug text and prove no
x86/riscv target-facing public consumer depends on direct field access.

`PreparedBirModule::liveness` is not part of that candidate set. It remains
blocked prepared planning authority until prealloc, regalloc, helper-planning,
target relevance, fallback, and policy-sensitive behavior are mapped.

### Generated Follow-Up Ideas

- `ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md`:
  x86/riscv parity proof for Route 6 call-use/source identity and public
  call-plan compatibility.
- `ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md`:
  x86/riscv parity proof for Route 3 memory/source identity and public
  memory/status compatibility.
- `ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md`:
  x86/riscv parity proof for Route 4/5 edge-publication identity and public
  edge-publication compatibility.
- `ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md`:
  analysis-only blocker map for source-producer lookup authority.
- `ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md`:
  analysis-only blocker map for `module`, `names`, `control_flow`, and
  `store_source_publications`.
- `ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md`:
  compatibility-retention proof matrix for statuses, fallback, debug, wrapper,
  printer, and target-output rows.
- `ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md`:
  private-pass-context demotion gate for `route`, `invariants`,
  `completed_phases`, and `notes`.
- `ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md`:
  analysis-only blocker map for `liveness` planning authority.

### Draft 155 Disposition

Draft 155 disposition: keep blocked.

Broad prepared aggregate retirement remains unsafe. No public lookup group,
`PreparedBirModule` field, or whole prepared aggregate is deletion-ready or
privatization-ready. Draft 155 may only be rewritten or superseded later by
bounded one-field-group, one-adapter, compatibility-retention, parity-proof, or
demotion-gate ideas that preserve prepared fallback, diagnostics/oracles,
wrapper compatibility, target policy boundaries, and fail-closed proof
matrices.

## Reviewer Reject Signals

- Treating this as another vague audit without concrete completion criteria.
- Claiming "BIR owns semantics" while prepared public APIs still provide the
  observable semantic answer.
- Treating x86 evidence as riscv readiness, or riscv evidence as x86 readiness.
- Treating retained compatibility strings as proof that ownership transferred.
- Creating broad prepared retirement work instead of bounded fact-family or
  field-group follow-ups.
- Hiding public prepared authority behind private wrapper names.
- Moving ABI, register, stack, storage, addressing, formatting, emission, or
  wrapper policy into BIR.
