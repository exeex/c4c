Status: Active
Source Idea Path: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit Prepared Lookup APIs And Prepared Printer Coverage

# Current Packet

## Just Finished

Step 4 - Audit Prepared Lookup APIs And Prepared Printer Coverage is complete.
I used `c4c-clang-tool-ccdb function-callers` on
`src/backend/prealloc/publication_plans.cpp`,
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
`src/backend/mir/aarch64/codegen/memory.cpp`, and
`src/backend/mir/x86/module/module.cpp`; `c4c-clang-tool function-signatures`
on `src/backend/prealloc/control_flow.hpp`; and targeted `rg`/source reads for
arch consumers plus prepared-printer output.

### Prepared Lookup / Printer Contract Audit

| Audited item | Classification | Disposition | Concrete evidence | Boundary decision |
| --- | --- | --- | --- | --- |
| `find_select_materialization_join_transfer` | retained/no-overlap | retained | `control_flow.hpp` exposes a named lookup that filters `PreparedJoinTransferKind::PhiEdge` plus effective carrier `SelectMaterialization`, optional true/false predecessor labels, and rejects ambiguous matches. `rg` found no arch consumer currently calling this exact lookup; nearby consumers mostly inspect published transfer/source facts directly. `prepared_printer/control_flow.cpp` prints `join_transfer ... carrier=select_materialization`, `source_branch`, `source_incomings`, and `source_transfer_indexes`. | The API and dump name the prepared select-materialization carrier sufficiently for current consumers. Keep as retained context, but no Step 5 follow-up unless future x86/RISC-V work starts inferring this association from raw BIR shape instead of this lookup/dump contract. |
| `find_prepared_compare_join_continuation_targets` | retained/no-overlap | no-overlap | Header overloads prefer `published_prepared_compare_join_continuation_targets` on an authoritative join transfer, then short-circuit context lookup. AArch64 comparison uses the published helper after `find_materialized_compare_join_context`; x86 module code calls the named lookup for ownership/target validation. `prepared_printer/control_flow.cpp` prints `continuation_targets=(true, false)` for published join-transfer targets. | Consumer-facing contract is explicit and dump-visible. No lookup or printer follow-up needed. |
| `find_prepared_scalar_select_chain_materialization` | `prepared-printer-contract-gap` and possible `lookup-api-contract-gap` | needs Step 5 follow-up | AST shows the implementation itself has no local caller, while AArch64 consumers call it from `dispatch_value_materialization.cpp` and `alu.cpp`. The lookup returns `available`, `root_value_name`, `root_is_select`, `root_instruction_index`, and nested direct-global dependency by recursively walking source-producer chains. `rg` found no prepared-printer output naming `PreparedScalarSelectChainMaterialization`, `root_is_select`, `root_instruction_index`, or scalar select-chain availability. | The lookup is named, but its multi-hop recognition result is target-significant and not dump-visible. Step 5 should create a contract-test/dump-coverage follow-up before more arch rebuild work depends on this fact. |
| Source-producer lookup fields | retained/no-overlap with printer gap risk | retained | `make_edge_publication_source_producers` indexes load-local, load-global, cast, binary, and select-materialization producers by value name; duplicates collapse to unknown. `apply_source_producer_fact` copies `source_producer_kind`, block label, instruction index, and producer pointers onto edge publications. Consumers in AArch64 edge copies, calls, memory, and dispatch producers use `find_indexed_prepared_edge_publication_source_producer` or the copied fields; RISC-V checks at least load-local source-producer publication. | Source-producer provenance is a legitimate prepared fact and not duplicate BIR authority. It remains retained because prepared-printer output does not expose these copied source-producer fields for edge/store publication facts, so Step 5 should decide whether to pair this with the scalar/direct-global printer follow-up or leave it as no-action. |
| Direct-global select-chain dependency facts | `prepared-printer-contract-gap` | needs Step 5 follow-up | `find_prepared_direct_global_select_chain_dependency` publishes `contains_direct_global_load`, `root_is_select`, and root instruction index; call planning, AArch64 call argument materialization, indirect-callee handling, and store-source publication use these facts. Store-source plans persist `direct_global_select_chain_source`, `direct_global_select_chain_root_is_select`, and `direct_global_select_chain_root_instruction_index`; AArch64 memory lowering uses them to avoid duplicate emission and decide covered instructions. `rg` found no prepared-printer output for `direct_global_select_chain`, `root_is_select`, or root instruction index. | This is the clearest dump contract gap: target-significant facts are used by consumers but not visible in prepared dumps. Step 5 should create a focused prepared-printer/contract-test follow-up. |
| Prepared-printer coverage for retained Step 3 compare/join items | retained/no-overlap | no-overlap | `prepared_printer/control_flow.cpp` prints branch conditions, join-transfer carrier, ownership, source branch, source incoming labels, source transfer indexes, edge transfers, and published continuation targets. | Compare/join continuation and select-materialization join-transfer summaries have adequate control-flow dump coverage for this audit. |
| Prepared-printer coverage for scalar select-chain/source-producer/direct-global items | `prepared-printer-contract-gap` | needs Step 5 follow-up | Targeted `rg` across `src/backend/prealloc/prepared_printer` only found control-flow join-transfer/continuation output. It did not find dump text for scalar select-chain materialization, direct-global select-chain dependencies, source-producer kind/block/index on edge/store publication facts, or root instruction indexes. | Step 5 should convert this into a durable follow-up idea. The follow-up should require prepared dump/test visibility for the consumer-facing scalar select-chain/direct-global/source-producer facts without moving target emission policy into BIR. |

### Step 4 Dispositions

- Follow-up-worthy now: prepared-printer/contract-test coverage for
  scalar select-chain materialization and direct-global select-chain dependency
  facts; likely include source-producer kind/block/index visibility for the
  same dump contract if Step 5 can keep the scope focused.
- Retained/no-overlap: `find_select_materialization_join_transfer` has a
  named lookup and current control-flow dump coverage; source-producer indexing
  is legitimate prepared provenance.
- No-overlap: compare-join continuation targets have named lookup use in x86,
  published-helper use in AArch64, and prepared-printer output.

## Suggested Next

Begin Step 5 by creating a focused follow-up idea for prepared-printer and
contract-test coverage of scalar select-chain/direct-global dependency facts,
including source-producer kind/block/index visibility only where it directly
supports that same consumer-facing contract. Then summarize no-action findings
for compare-join continuation and select-materialization join-transfer lookup
coverage.

## Watchouts

- This is analysis-only; do not edit implementation files.
- The Step 5 follow-up should be about dump/contract visibility, not a rewrite
  of lookup APIs or target lowering policy.
- Do not create separate ideas for compare-join continuation or
  `find_select_materialization_join_transfer`; this packet found adequate named
  lookup/dump coverage for those retained Step 3 items.
- Source-producer fields are widely consumed, but the strongest traced gap is
  printer visibility for target-significant scalar select-chain/direct-global
  facts.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 4 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains:
`analysis-only Step 4 proof: no implementation changes under src/backend/bir or src/backend/prealloc`.
