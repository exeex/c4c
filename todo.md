Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Producer And Select-Chain Users

# Current Packet

## Just Finished

Step 1: Map Current Producer And Select-Chain Users completed as an audit-only
slice. AST-backed queries over `dispatch_producers.cpp` and consumer translation
units identified the current helper-to-consumer map before extraction:

- `find_same_block_named_producer(context, value_name, before)` is the main
  target-neutral same-block lookup. It scans the retained BIR block backward and
  reports producers whose result names match across `BinaryInst`, `CastInst`,
  `SelectInst`, `LoadLocalInst`, and `LoadGlobalInst`.
- `producer_instruction_index(context, producer)` is the companion
  target-neutral pointer-to-index lookup used when recursive consumers need a
  narrower `before_instruction_index` for nested dependencies.
- `find_same_block_select_producer(context, value, before)` is a
  select-specific same-block lookup returning `SameBlockSelectProducer{select,
  instruction_index}`; it currently feeds select-chain materialization.
- `find_same_block_binary_producer(context, value)` plus
  `evaluate_same_block_integer_constant(context, value)` implement same-block
  binary producer discovery and bounded integer constant folding for target
  instruction choices.
- `select_chain_contains_direct_global_load(context, value, before)` traverses
  same-block `SelectInst`, `CastInst`, and `BinaryInst` dependencies to detect
  whether a chain reaches a direct `LoadGlobalInst`.

Consumer map:

- Call consumers: `dispatch_calls.cpp` uses same-block named producer and index
  lookup in `resolve_indirect_callee_local_load_store` and
  `emit_indirect_callee_value_to_register_with_csel`; it also uses
  `select_chain_contains_direct_global_load` in
  `materialize_indirect_call_callee_to_prepared_register`.
- Call/edge-copy select-chain consumer: `dispatch_edge_copies.cpp` uses
  `find_same_block_select_producer` in `emit_select_chain_value_to_register`
  and uses `select_chain_contains_direct_global_load` in
  `materialize_direct_global_select_chain_call_argument`.
- Branch-fusion consumers: `dispatch_branch_fusion.cpp` uses hook-injected
  `find_same_block_named_producer`/`producer_instruction_index` in materialized
  compare, stack-home, and selected-operand checks. It also has local duplicate
  `branch_fusion_find_same_block_binary_producer` and
  `branch_fusion_evaluate_same_block_integer_constant` helpers for constant RHS
  fusion and support-instruction filtering.
- Support-instruction filtering: `is_fused_compare_branch_support_instruction`
  classifies same-block `CastInst` and `BinaryInst` producers that can be
  consumed by fused branch lowering; the producer discovery and constant fact
  are target-neutral, while the fused branch legality and emission remain
  AArch64 policy.
- Publication/value materialization consumers:
  `dispatch_value_materialization.cpp` uses same-block named/index lookup,
  integer constant evaluation, global target lookup, and join-copy source
  filtering in `emit_value_publication_to_register` and
  `emit_fp_value_to_register`. `dispatch_publication.cpp` uses named/index
  lookup in `value_publication_may_read_register_index`.
- Store consumers: `dispatch_store_sources.cpp` uses same-block named/index
  lookup in store value classification helpers, `lower_store_local_value_publication`,
  and pointer base-plus-offset materialization; it uses
  `select_chain_contains_direct_global_load` when deciding whether a store value
  needs extra publication.
- Dispatch glue: `dispatch.cpp` wires `find_same_block_named_producer` and
  `producer_instruction_index` through `DispatchBranchFusionHooks`, uses
  same-block lookup for missing conditional branch condition publication, and
  still owns block dispatch and final instruction insertion.

Boundary recorded for Step 2:

- Target-neutral facts to extract: same-block result-name producer lookup,
  producer instruction index lookup, select producer lookup with index, bounded
  same-block integer constant evaluation, dependency traversal over select/cast/
  binary chains, and records that identify producer kind plus instruction index.
- Retained AArch64 policy: register view choice, scratch register selection,
  branch condition suffixes, cmp/csel/fcsel/branch text emission, ADRP/ADD/GOT
  global materialization, frame-slot addressing, ABI/call-boundary choices,
  current block entry publication policy, store writeback shape, and final
  machine instruction construction.
- Keep local for now: `find_load_global_target`, `load_global_symbol_label`,
  `find_bir_block`, and `is_current_block_join_parallel_copy_source`; they mix
  prepared-module naming, block mapping, or publication/home policy with the
  AArch64 lowering path and should not be moved in the first query API slice.

## Suggested Next

Execute Step 2: Introduce Shared MIR Query Records. Add a small shared MIR query
surface that can answer same-block producer lookup, producer index, select
producer with index, bounded integer constant facts, and select-chain dependency
reachability without mentioning AArch64 registers, mnemonics, ABI policy, or
machine instruction construction. Start by moving only the target-neutral
helpers or wrapping them behind a shared record API, then rewire one narrow
AArch64 include/use site as compile proof preparation for Step 3.

## Watchouts

- Keep AArch64 final emission, ABI policy, condition mapping, memory operands,
  and machine instruction construction target-local.
- Do not combine this route with publication planning, store-source planning,
  or call-boundary ordering migrations.
- Cross-target proof must compile or exercise the shared query surface; an
  unused include or comment is not enough.
- `dispatch_branch_fusion.cpp` contains duplicate binary-producer and constant
  folding helpers; Step 2 should avoid extracting only the
  `dispatch_producers.cpp` copy if the branch-fusion duplicate would leave the
  generic responsibility owned by AArch64.
- `select_chain_contains_direct_global_load` currently encodes a
  direct-global-load predicate. The traversal is target-neutral, but the
  question may be better expressed as a generic dependency record plus an
  AArch64-side predicate in Step 2.

## Proof

Audit-only packet. Used `c4c-clang-tool-ccdb list-symbols`,
`function-signatures`, and focused `function-callers` queries for
`dispatch_producers.cpp`, `dispatch_calls.cpp`, `dispatch_edge_copies.cpp`,
`dispatch_branch_fusion.cpp`, `dispatch_value_materialization.cpp`,
`dispatch_publication.cpp`, `dispatch_store_sources.cpp`, and `dispatch.cpp`.
No build or test proof was required, and no `test_after.log` was created.
