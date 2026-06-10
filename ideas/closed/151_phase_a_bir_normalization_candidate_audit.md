# 151 Phase A BIR normalization candidate audit

## Goal

Audit `Prepared*` facts that are really BIR normalization relationships and
produce follow-up ideas for moving those relationships forward into BIR-owned
semantics.

This is Phase A of the BIR/prealloc thinning program. It is analysis-only and
must create smaller follow-up ideas instead of directly rewriting BIR,
prealloc, or MIR consumers.

## Shared Artifact Contract

This phase wrote its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`

The artifact is the required input for Phase B and an input dependency for all
later BIR/prealloc thinning phases.

## Direction

The long-term direction is to make prealloc as thin as possible. BIR should own
normalizable semantic relationships before prealloc runs. Prealloc should not
be the place where target-neutral producer, dependency, publication, call, or
edge relationships first become understandable.

## Questions

- Which `Prepared*` facts are actually BIR semantic relationships?
- Which relationships are currently computed in prealloc only because AArch64
  needed a stable lowering fact first?
- Which facts should become BIR node, block, function, or edge relationships
  before any allocation/layout pass?
- Which facts must stay out of BIR because they encode layout, ABI placement,
  target routing, or final emission?

## Closure Note

Status: Closed.

Closed after the analysis-only runbook completed all six steps and produced
the durable handoff artifact:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`

The artifact contains the required handoff sections:

- `## BIR-Normalization Candidates`: the accepted candidate table.
- `## Facts Rejected From BIR Normalization`: the reject table and non-BIR
  owner boundaries.
- `## Dependency Order`: ordered normalization route groups, dependencies,
  consumer switch timing, and BIR schema boundaries.
- `## Follow-Up Idea Payloads`: seven route-sized follow-up idea payloads.
- `## Proof-Route Recommendations`: route-specific proof guidance with
  query-equivalence expectations.

Candidate table summary:

| Candidate group | BIR-owned semantic relationship to pursue |
| --- | --- |
| Same-block producer/materialization | Same-block producer node, immediate constant, value/name/type identity, instruction index, and materialization availability. |
| Select-chain dependency/materialization | Select-root, direct global-load dependency, root producer, root instruction index, and scalar materialization eligibility. |
| Current-block entry publication consumption | Block-entry/current-block value availability mapped to source producer, produced value/name, producer instruction, and source kind. |
| Edge-publication and parallel-copy source identity | CFG edge value transfer identity keyed by predecessor, successor, destination value/name, source value/name/kind, producer, and optional memory source. |
| Call-boundary source/dependency identity | Call argument/result source identity, same-block materializable producer, direct global select-chain dependency, memory source, and publication-source route identity. |
| Memory/access source identity | Semantic memory operation/access identity, address space/volatile bits, base/source kind, same-block global-load access, and load-local stored-value source links. |
| Comparison/materialized-condition producer identity | Branch or condition value producer identity, operand producers/constants, comparison-producing instruction, and producer instruction index. |

Reject table summary:

| Rejected fact family | Reason it must stay outside BIR normalization |
| --- | --- |
| ABI placement, stack slots, frame offsets, and layout | They encode ABI/layout decisions rather than target-neutral BIR dataflow. |
| Register spelling, banks, views, occupied registers | They are target allocation and operand-spelling facts. |
| Scratch policy and aggregate transport lanes | They are target transport and ABI lowering policy. |
| Target addressing modes, TLS relocations, and base-plus-offset legality | They are target encoding, relocation, and address-materialization decisions. |
| Parallel-copy execution and publication routing policy | They are out-of-SSA copy scheduling and target/prealloc routing decisions. |
| Publication hooks, storage encoding, and typed stack-source emission payloads | They choose target publication/materialization mechanics after semantic source identity is known. |
| Final instruction routing, ordering, hazards, and record errors | They are MIR/codegen behavior, not BIR normalization data. |
| Runtime helper, special-carrier, variadic, and dynamic-stack resource policy | They describe ABI/helper protocols and target resources. |
| Whole mixed `Prepared*Plan`, `Prepared*Publication`, `PreparedAddress`, or `PreparedValueHome` shapes | They combine semantic identity with non-BIR placement, routing, and target payloads. |

Dependency order:

1. Producer/source identity foundation.
2. Select-chain and direct-global dependency identity.
3. Memory/access semantic identity.
4. Block-entry and current-block publication identity.
5. CFG edge publication and join-source identity.
6. Call-boundary semantic source facts.
7. Comparison and materialized-condition producer identity.

Consumer switch rule: no MIR consumer should switch until the corresponding
BIR query can answer the same semantic subset as the prepared query, with the
old prepared surface retained as a comparison oracle for before/after proof.

Follow-up ideas filed from the artifact:

- `ideas/open/152_bir_producer_source_identity_foundation.md`
- `ideas/open/153_bir_select_chain_direct_global_identity.md`
- `ideas/open/154_bir_memory_access_identity.md`
- `ideas/open/155_bir_block_entry_publication_identity.md`
- `ideas/open/156_bir_cfg_edge_publication_identity.md`
- `ideas/open/157_bir_call_boundary_source_facts.md`
- `ideas/open/158_bir_comparison_condition_producer_identity.md`

Proof-route guidance: each follow-up should use matching before/after backend
or route-specific subsets, retain `test_before.log` and `test_after.log` as
canonical regression artifacts, and include query-equivalence evidence that the
new BIR query matches the relevant prepared query before any broad MIR consumer
switch.

## Reject Signals

- Treating every `Prepared*` fact as BIR normalization.
- Moving ABI placement, stack offsets, target register spelling, scratch
  policy, or final instruction routing into BIR.
- Creating broad ideas that say only "move prepared into BIR".
- Deleting prepared facts before MIR consumers have an equivalent BIR-owned
  relationship.
