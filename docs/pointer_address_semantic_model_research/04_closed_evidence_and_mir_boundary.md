# 04. Closed Evidence And MIR Boundary

Status: Step 5 complete

## Purpose

This file positions the closed freshness evidence and the downstream
Prepared MIR view boundary. The goal is to prevent two route-drift mistakes:

- treating narrow closed branch stack-source work as global pointer/address
  semantic closure
- letting future `PreparedMirView` work invent semantic pointer/address
  authority inside a view abstraction

The Step 3 and Step 4 rules remain the controlling semantic model:
pointer/address authority is use-specific, support facts do not authorize by
themselves, and target-consume or diagnostic facts must not become semantic
authority.

## Decision Summary

Ideas 587 through 590 are the selected freshness authority baseline. They
establish the vocabulary and fail-closed ownership rule for selected value
freshness: value identity, use kind, program point, source kind, proof kind,
source reference, and rank must match the consuming use before a freshness
claim is accepted.

Ideas 592, 593, 594, and 596 are narrow branch pointer stack-source evidence.
They prove that selected branch stack-slot freshness can be published and
consumed for migrated RV64 fused pointer branch `Lhs` and `Rhs` stack-source
uses. They do not close pointer arithmetic, pointer-value indirect memory,
local-array/global GEP target consumption, relocation semantics, local memory
layout semantics, stack-home completeness, or target operand-shape semantics.

Idea 591 may consume this research as an input contract for
`PreparedMirView`. It may expose prepared pointer/address facts and selected
authority records through a narrower MIR-facing view. It must not synthesize
freshness, address validity, or semantic derivation from view shape, target
operand shape, complete stack homes, relocation records, range/layout facts,
or diagnostics.

## Selected Freshness Authority Baseline

| Evidence path | What it proves | What it does not prove | Impact on idea 591 |
| --- | --- | --- | --- |
| `ideas/closed/587_prepared_value_freshness_authority_mvp.md` | A first-class selected freshness model exists for representative prepared value uses. The model distinguishes value identity, use context, source kind, proof kind, source reference, and precedence/rank, and requires a shared query to fail closed when no selected source is proven. | It is an MVP, not a global dataflow lattice and not a pointer/address semantic model for every use. It does not authorize pointer arithmetic results, pointer-value memory uses, relocation materialization, local layout, stack homes, or target operands by itself. | `PreparedMirView` may expose selected freshness authority as a required codegen input for uses that already have an owning freshness route. It must not replace unresolved pointer/address semantics with a generic "fresh enough" view flag. |
| `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md` | Shared-prealloc consumers can be audited and migrated to the 587 freshness query one bounded family at a time. Missing, ambiguous, stale, wrong-value, wrong-use, and destination-only authority must fail closed before source acceptance. | The inventory deliberately left branch stack-load, direct edge-publication, typed/aggregate stack-source, select/alias, and target-specific consumers for separate ownership work. It does not prove all shared-prealloc or backend consumers are wired. | 591 may classify migrated shared-prealloc freshness queries as stable inputs only when the owning route is closed. It must preserve visible missing/unwired statuses instead of hiding them behind view completeness. |
| `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md` | Direct edge-publication move source freshness has its own ownership rule. Destination bundle legality, complete source homes, aliases, and local move shape are insufficient; accepted sources require explicit freshness for the exact edge-publication source and move resolution. | It does not solve branch stack-load freshness, typed/aggregate stack-source production, pointer arithmetic, memory address authority, or target operand validity. It also does not make every edge-publication or move-bundle path globally migrated. | 591 may consume direct edge-publication source freshness as a use-specific required fact where the closed route applies. It must not infer source freshness from edge rows, move bundles, aliases, or destination legality alone. |
| `ideas/closed/590_branch_stack_load_freshness_contract.md` | Branch stack-load source freshness is use-specific to the exact branch terminator point. A selected authority must match the same value/home, `BranchStackLoadSource` use, `BranchStackSlot` source/rank, `BranchTerminatorOrdering` proof, branch block, and terminator instruction point. Stack home, frame slot, branch payload, and clobber facts are support only. | The closure migrated a scalar condition route and left pointer `Lhs`/`Rhs` and typed/aggregate producer facts for later work. It does not prove all branch operands, all target emission paths, or all pointer/address families are solved. | 591 may rely on the branch-point freshness rule as the contract shape. It must represent missing branch freshness as unavailable, not as a target-local chance to consult stack homes or branch operand shape. |

The baseline contribution from 587-590 is architectural, not family-global:
freshness authority is selected, use-specific, and fail-closed. This baseline
is the model that later pointer/address authorities should follow when they
need freshness semantics.

## Narrow Branch Pointer Stack-Source Evidence

| Evidence path | What it proves | What it does not prove | Impact on idea 591 |
| --- | --- | --- | --- |
| `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md` | Shared prepared/prealloc producer paths can publish `BranchStackSlot` freshness for branch stack-load sources when the source value, use, proof, rank, stack-slot home, branch block, and terminator point match. It migrated pointer `Lhs` producer policy from inventory-only to selected freshness. | It did not migrate target consumption broadly, did not cover pointer `Rhs` producer policy, and did not authorize aggregate-adjacent consumers, select consumers, edge consumers, or non-branch pointer/address uses. | 591 may treat producer-published branch stack-source freshness as a required upstream input for the closed branch pointer route. It must not manufacture missing producer policy inside `PreparedMirView`. |
| `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md` | RV64 fused pointer branch emission for stack-slot `Lhs` consumes selected shared `BranchStackLoadSource` / `BranchStackSlot` authority for the exact branch block and terminator position. Stack-home-only, frame-slot-only, aggregate-lane-only, clobber-only, register-only, and operand-shape-only evidence are not freshness. | It left pointer `Rhs` blocked on missing producer policy and did not solve aggregate-adjacent branches, scalar-condition-register shapes, string assembly emission, AArch64, x86, or general target consumption. | 591 may cite the migrated RV64 `Lhs` route as evidence that MIR consumers should consume shared freshness, not target-local structural inference. It must keep the route narrow and not claim all RV64 pointer/address consumption is stable. |
| `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md` | The shared producer-side blocker for pointer `Rhs` branch stack-load policy was repaired. Valid `Rhs` rows can publish selected `BranchStackLoadSource` / `BranchStackSlot` authority for the same source value, exact branch block, exact terminator index, `BranchTerminatorOrdering` proof, and `BranchStackSlot` rank. | It did not migrate RV64 or any target consumer. It only cleared the producer policy gap needed by idea 594 and did not authorize target-local fallback from stack homes or operand shapes. | 591 may require the producer-side `Rhs` authority as an upstream fact where the closed route applies. It must not treat producer availability alone as proof that every target has consumed it correctly. |
| `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md` | RV64 fused pointer branch emission for stack-slot `Rhs` now consumes the selected shared authority published after 596. Together with 593, migrated RV64 pointer `Lhs` and `Rhs` fused-branch stack-source inputs are stable enough for 591 to treat selected shared branch stack-source freshness as the required backend input for those exact paths. | It explicitly leaves aggregate-adjacent branch stack-source consumers, scalar-condition-register branch shapes, string assembly emission, other target emission paths, and broader prepared-value families outside the narrow queue. It does not close global pointer/address semantics. | 591 may consume the closed RV64 pointer `Lhs`/`Rhs` branch stack-source queue as a concrete required-input example. It must still keep unresolved branch-adjacent and non-branch pointer/address families unavailable or deferred. |

The narrow branch queue proves one selected-freshness subset:

- use: `PreparedValueFreshnessUseKind::BranchStackLoadSource`
- source: `PreparedValueFreshnessSourceKind::BranchStackSlot`
- proof: `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`
- rank: `PreparedValueFreshnessSourceRank::BranchStackSlot`
- program point: exact branch block plus terminator instruction index
- migrated RV64 target use: fused pointer branch stack-slot `Lhs` and `Rhs`

This subset is semantic freshness authority for that exact branch use. It is
not authority for other pointer/address families.

## Evidence Versus Unresolved Semantics

The closed evidence above supports these semantic conclusions:

- Freshness authority must be selected for a specific value, use, source,
  proof, rank, and program point.
- Stack homes, frame slots, stack objects, aggregate lanes, clobber-safety
  facts, and target operand shape can support or consume a selected route but
  cannot authorize freshness alone.
- RV64 pointer fused-branch `Lhs`/`Rhs` stack-source emission has evidence
  that it consumes selected shared branch freshness instead of target-local
  structural inference.

The closed evidence does not resolve these semantics:

- `PreparedValueHomeKind::PointerBasePlusOffset` still needs a first owning
  pointer-arithmetic authority that names base freshness, result identity,
  delta, use, and program point.
- Pointer-value indirect memory access still needs a selected freshness owner
  for the named pointer value at the exact load/store use. Range/layout proof
  from `prepared_pointer_value_memory_has_proven_authority(...)` is address
  legality support, not pointer value freshness.
- Local-array and global static semantic GEP availability are semantic
  address-derivation authority, but later MIR/target work still must prove
  consumers read that authority instead of relocation, memory layout, or
  target shape.
- Relocation and address materialization remain target-consume facts for
  symbol, TLS, frame, or fixup materialization. They do not prove pointer
  freshness or address derivation.
- Global symbol memory access authority is exact symbol-backed memory
  address/range authority. It does not prove loaded-value freshness,
  store-source freshness, pointer-value freshness, or unrelated GEP validity.
- Target-local operand records, printed assembly, unsupported diagnostics,
  candidate counts, and dump rows remain target-consume or diagnostic facts.

Any later implementation idea should treat these unresolved entries as
deferred or split by first owner. They should not be silently absorbed into
the closed branch stack-source queue.

## Prepared MIR View Consumption Boundary

Idea 591 may consume the following from this research:

- the fact-class vocabulary from this research package: semantic authority,
  verifier/support fact, target-consume fact, route proof, and diagnostic-only
  artifact
- the global fail-closed rule that missing, ambiguous, stale, wrong-value,
  wrong-use, range-only, relocation-only, stack-home-only, local-layout-only,
  target-shape-only, or diagnostic-only evidence must not authorize a semantic
  pointer/address use
- selected freshness authority for already closed routes, including the 587
  through 590 baseline and the 592/593/594/596 RV64 pointer branch
  stack-source subset
- semantic GEP availability records as semantic address-derivation authority
  for local-array and global static GEP families, while keeping target
  consumption separate
- prepared memory access and address materialization facts as support or
  target-consume inputs only in the roles classified by Step 3
- unavailable/deferred statuses for families whose first owner or proof
  surface is not settled

Idea 591 must avoid inventing the following inside `PreparedMirView`:

- a new generic pointer/address validity bit that merges freshness, memory
  range legality, relocation materialization, local layout, and target shape
- freshness inferred from complete `PreparedValueHome` payloads, stack homes,
  frame slots, stack objects, clobber-safety, aggregate lanes, or register
  allocation facts
- address derivation inferred from relocation records, materialization
  records, final target operands, printed assembly, or diagnostics
- target emission permission inferred from the mere presence of a view field
  when the owning semantic authority is unavailable or deferred
- branch stack-source authority generalized beyond the closed exact
  branch-point `Lhs`/`Rhs` RV64 pointer queue
- old/new BIR equivalence claims that compare only dumps or target output
  while ignoring selected authority identity, use, source, proof, rank, and
  program point

The view boundary should therefore expose facts with their roles and
availability statuses intact. A `PreparedMirView` can narrow access to the
prepared module, but it cannot change which layer owns semantic authority.

## Responsibility Split

| Responsibility | Owner | Required behavior |
| --- | --- | --- |
| Define semantic pointer/address authority | This research route and later split implementation ideas by first owning layer | Classify each family before migration; defer unclear families instead of broadening closed evidence. |
| Publish selected freshness for a value/use | Shared prepared/prealloc freshness producer for that use | Publish exact value, use, source, proof, rank, and program-point evidence; fail closed when missing or ambiguous. |
| Consume selected freshness in MIR/target code | The specific MIR or target consumer route | Require selected authority before using freshness-sensitive sources; keep layout, stack homes, relocation, and operand shape as support or target inputs. |
| Expose MIR-facing inputs | Future `PreparedMirView` work under idea 591 | Present required semantic/support/target/diagnostic facts without synthesizing authority or hiding unavailable/deferred states. |
| Explain or debug rejection | Diagnostics and route-proof artifacts | Report why evidence is unavailable or rejected; never authorize codegen or semantics by observation alone. |

## Step 5 Conclusion

The closed freshness chain supplies a reusable selected-authority pattern, and
the branch pointer stack-source queue supplies a proven narrow instance of
that pattern. Neither evidence group is a global pointer/address semantic
model.

The Prepared MIR view line can consume this research only as a boundary
contract: expose the already-owned authorities and preserve unresolved
families as unavailable or deferred. It must not move semantic ownership into
`PreparedMirView` or let support, target, route-proof, or diagnostic facts
become authority by being placed behind a cleaner interface.
