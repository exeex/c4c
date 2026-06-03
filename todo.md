Status: Active
Source Idea Path: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inventory Select And Compare Join Classification

# Current Packet

## Just Finished

Step 3 - Inventory Select And Compare Join Classification is complete. I used
`c4c-clang-tool-ccdb function-signatures` and `function-callees` on
`src/backend/prealloc/out_of_ssa.cpp` and
`src/backend/prealloc/publication_plans.cpp`, plus
`c4c-clang-tool function-signatures` on
`src/backend/prealloc/control_flow.hpp` and
`src/backend/prealloc/prepared_lookups.hpp`, before reading targeted source
ranges.

### Select / Compare / Join Boundary Classification

| Area | Classification | Disposition | Concrete evidence | Boundary decision |
| --- | --- | --- | --- | --- |
| `materialize_phi` / `materialize_funnel_subtree` in `src/backend/prealloc/out_of_ssa.cpp` | `prealloc-transfer-plan-authority` with retained `bir-missing-target-neutral-fact` lead | retained | `materialize_phi` first proves the PHI incoming labels exactly match funnel leaves collected from BIR terminators, then requires `find_compare_for_condition` on the entry branch condition. `materialize_funnel_subtree` recursively follows BIR branch targets and emits a `bir::SelectInst` carrying the compare predicate/type/lhs/rhs and true/false values. | Legitimate out-of-SSA publication: the emitted `SelectInst` is the target-facing carrier replacing a reducible PHI funnel. BIR owns the component branch/compare/PHI facts, but does not currently publish a stable "this select materializes this branch/phi funnel" fact. Keep this retained for Step 4 lookup/printer scrutiny, not follow-up-worthy yet. |
| `collect_select_materialized_join_transfers` in `src/backend/prealloc/out_of_ssa.cpp` | `prealloc-transfer-plan-authority` with retained `lookup-api-contract-gap` lead | retained | Scans first-in-block `bir::SelectInst`, matches it against `PreparedBranchCondition` predicate/type/lhs/rhs, derives true/false predecessor labels with `find_prepared_linear_join_predecessor`, and publishes a `PreparedJoinTransfer` with carrier `SelectMaterialization`, edge transfers, source branch label, and true/false transfer indices. | This is a valid prepared join-transfer publication after PHI removal. The semantic association is still reconstructed from select shape plus branch-condition shape, so Step 4 should check whether consumers get a named lookup/dump contract instead of inferring from carrier kind and indices. |
| `annotate_branch_owned_join_transfers` in `src/backend/prealloc/out_of_ssa.cpp` | `prealloc-transfer-plan-authority` | no-overlap | For two-edge join transfers, it walks prepared branch conditions, finds linear true/false predecessors into the join, maps transfer edge indices, rejects ambiguity, and writes source branch label plus true/false incoming labels and indices onto `PreparedJoinTransfer`. | This is target-facing publication of branch-owned transfer routing. BIR owns branch targets and PHI incoming labels, but the published source/edge-index mapping is an out-of-SSA consumer contract. No BIR authority overlap found. |
| `publish_branch_owned_join_transfer_continuation_labels` plus `find_prepared_compare_join_continuation_targets` in `src/backend/prealloc/out_of_ssa.cpp` / `control_flow.hpp` | `prealloc-transfer-plan-authority` with retained fallback-shape concern | retained | Publication first looks for an already-published continuation pair. If absent, `find_prepared_compare_join_continuation_targets` checks the join block, requires a supported carrier, recognizes `join_result != 0` or `join_result == 0`, and maps true/false labels from the join branch condition. The result is stored as `continuation_true_label` / `continuation_false_label` and exposed by `published_prepared_compare_join_continuation_targets`. | Published continuation labels are legitimate prepared authority. The fallback shape recognizer is acceptable as a producer, but Step 4 should verify consumers prefer `published_prepared_compare_join_continuation_targets` and do not independently re-derive continuation semantics. |
| `publish_short_circuit_continuation_branch_conditions` in `src/backend/prealloc/out_of_ssa.cpp` | `prealloc-transfer-plan-authority` with retained `lookup-api-contract-gap` lead | retained | Uses `find_prepared_compare_branch_target_labels`, `find_prepared_short_circuit_join_context`, and `find_prepared_short_circuit_branch_plan`; publishes continuation labels on the join transfer and appends fused-compare `PreparedBranchCondition` rows for continuation blocks by reading each continuation block's trailing `bir::BinaryInst`. | This publishes branch conditions that targets need after short-circuit compare-join rewriting. It is not a competing BIR semantic source, but it creates consumer-visible synthetic branch-condition rows; Step 4 should check lookup names and printer output make these rows distinguishable enough. |
| `find_prepared_scalar_select_chain_materialization` and select-chain helpers in `src/backend/prealloc/publication_plans.cpp` | `lookup-api-contract-gap` candidate | retained | `prepared_select_chain_source_producer` resolves an indexed source producer by value name, requires same block and instruction-before relation, and verifies the BIR instruction still matches. `prepared_select_chain_contains_direct_global_load` recursively walks load/cast/binary/select producers. `find_prepared_scalar_select_chain_materialization` publishes availability, root value name, `root_is_select`, root instruction index, and direct-global dependency. | This is not BIR authority duplication; it is a prepared lookup over source-producer facts. The retained concern is contract surface: the stable fact is "source producer kind/index, possibly select materialization", while consumers asking for scalar select-chain materialization may depend on a multi-hop recognition rule. Step 4 should decide whether this needs a named lookup/dump contract or tests. |
| `src/backend/prealloc/prepared_lookups.*` source-producer indexing | `prealloc-transfer-plan-authority` | no-overlap | `make_edge_publication_source_producers` indexes BIR load-local, load-global, cast, binary, and select instructions by result value; duplicate producers collapse to unknown. `apply_source_producer_fact` copies the source kind, block label, instruction index, and source instruction pointer onto edge publications. `find_indexed_prepared_edge_publication_source_producer` exposes the indexed producer by value name. | The lookup owns prepared producer provenance, not BIR select/compare semantics. No Step 3 follow-up by itself. It remains evidence for Step 4 contract coverage. |
| Prepared printer coverage for compare/join | `prepared-printer-contract-gap` rejected for compare/join | no-overlap | `src/backend/prealloc/prepared_printer/control_flow.cpp` prints branch-owned join-transfer source indices and prints `continuation_targets=(true, false)` when `published_prepared_compare_join_continuation_targets` is available. | Compare/join continuation publication has dump coverage. Do not create a printer follow-up for this item unless Step 4 finds a consumer-visible omission elsewhere. |
| Prepared printer coverage for scalar select-chain materialization | `prepared-printer-contract-gap` candidate | retained | `rg` found prepared-printer output for compare/join continuation targets but no printer row naming `PreparedScalarSelectChainMaterialization`, `direct_global_select_chain`, `root_is_select`, or scalar select-chain root instruction. | Step 4 should decide whether this lack of dump coverage is a real contract-test gap. Not follow-up-worthy until lookup/printer audit confirms consumers need to inspect this fact. |

### Step 3 Dispositions

- Follow-up-worthy now: none. Every Step 2 re-derived control row is currently
  explainable as prepared publication or lookup over prepared producer facts.
- Retained for Step 4: select materialization lacks a target-neutral BIR
  association fact; scalar select-chain materialization is a multi-hop lookup
  over source producers; short-circuit synthetic branch-condition rows and
  compare-join fallback recognition should be checked against consumer lookup
  usage and dump coverage.
- No-overlap: branch-owned join-transfer source/edge-index publication,
  source-producer indexing, and compare/join continuation printer coverage.

## Suggested Next

Begin Step 4 by auditing prepared lookup API consumers and prepared-printer
coverage for the retained Step 3 items. Focus on
`find_select_materialization_join_transfer`,
`find_prepared_compare_join_continuation_targets`,
`find_prepared_scalar_select_chain_materialization`, source-producer lookup
fields, and printer coverage for scalar select-chain/direct-global dependency
facts.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Do not convert retained Step 3 items into ideas until Step 4 proves a
  concrete consumer-facing lookup or dump contract gap.
- The Step 3 retained items are contract-surface concerns, not current evidence
  that prealloc owns duplicate BIR semantic authority.
- `publish_branch_owned_join_transfer_continuation_labels` stores continuation
  labels, but `control_flow.hpp` still has fallback recognition paths; Step 4
  should distinguish producer fallback from consumer re-derivation.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 3 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains:
`analysis-only Step 3 proof: no implementation changes under src/backend/bir or src/backend/prealloc`.
