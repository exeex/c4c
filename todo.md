Status: Active
Source Idea Path: ideas/open/53_aarch64_comparison_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Comparison Recovery Sites

# Current Packet

## Just Finished

Step 1: Audit Comparison Recovery Sites completed for
`src/backend/mir/aarch64/codegen/comparison.cpp`.

Bounded future repair packets:

- Conditional branch record authority: `lower_prepared_conditional_branch_terminator`
  already requires `PreparedControlFlowBlock` plus
  `find_prepared_branch_condition`, and
  `make_prepared_conditional_branch_record` already derives branch targets,
  condition value, fused predicate, compare type, and lhs/rhs identities from
  `PreparedBranchCondition`, with operand identity filled through
  `PreparedValueHome`. Remaining duplicate authority is the raw BIR
  terminator cross-check and canonical target rewrite passed into the record
  builder; Step 2 can narrow that to validation after prepared block/condition
  authority has chosen the semantic branch.
- Fused compare cast/load/constant operand authority:
  `lower_fused_compare_branch_from_emitted_cast` uses
  `find_same_block_cast_producer`, `find_same_block_load_producer`, and
  `mir::evaluate_same_block_integer_constant`. Existing replacement authority
  is `PreparedBranchCondition` for compare operands plus
  `PreparedEdgePublicationSourceProducerLookups` /
  `find_prepared_same_block_scalar_producer` for cast/load producer identity,
  `PreparedMemoryAccess` for frame-slot load addresses, and
  `PreparedValueHomeKind::RematerializableImmediate` for constants where
  present. Missing shared query: a comparison-suitable fused-compare operand
  producer lookup that returns producer kind, instruction index, cast/load
  payload, and optional encodable immediate without rescanning the BIR block.
- Materialized condition branch authority:
  `lower_materialized_compare_condition_branch` finds the condition producer
  through `hooks.find_same_block_named_producer` and then uses
  `hooks.emit_value_publication_to_register` for lhs/rhs recovery. Existing
  replacement authority is `PreparedBranchCondition` for condition value,
  target labels, and fused compare facts when available, plus
  `find_prepared_same_block_scalar_producer` for the materialized binary
  producer and existing scalar-publication helpers for register emission.
  Missing shared query: materialized-condition producer lookup keyed by the
  prepared condition value, returning the binary producer and instruction index.
- Block-entry fused compare authority:
  `lower_current_block_entry_fused_compare_branch` already routes through
  `hooks.current_block_entry_publication_register`, which consumes
  `collect_prepared_block_entry_publications` and
  `prepared_block_entry_publication_available`. This packet can replace the
  hook-shaped recovery with direct prepared block-entry publication lookup only
  if comparison needs a narrower consumer API; no new semantic fact appears
  required.
- Constant RHS fused compare authority:
  `lower_constant_rhs_fused_compare_branch` uses
  `mir::find_same_block_binary_producer`, stack-home gating,
  `value_has_current_block_entry_publication`, and
  `mir::evaluate_same_block_integer_constant`. Existing authority covers
  branch operands via `PreparedBranchCondition`, homes via
  `PreparedValueHome`, and block-entry publication via
  `PreparedBlockEntryPublication`. Missing shared query: prepared constant RHS
  or materialized fused-compare operand lookup that can confirm producer kind,
  absence/presence of block-entry publication, and encodable immediate value.
- Stack-home and producer-kind gates:
  `lower_stack_home_fused_compare_branch` locally allows only same-block
  `LoadLocalInst` / `LoadGlobalInst` producers before consuming
  `PreparedValueHome`, and `fused_compare_operand_has_select_producer` scans
  for a same-block `SelectInst`. Existing authority is
  `PreparedValueHome`, `PreparedEdgePublicationSourceProducerKind`
  (`LoadLocal`, `LoadGlobal`, `SelectMaterialization`, etc.),
  `find_prepared_same_block_load_local_source_producer`, and
  prepared memory-access lookups. Missing shared query: fused-compare operand
  producer-kind lookup that distinguishes at least select, load-local,
  load-global, binary, cast, immediate, and unknown for a branch-condition
  operand.

## Suggested Next

Implement Step 2: narrow `make_prepared_conditional_branch_record` and its
caller so prepared control-flow and branch-condition facts are the semantic
authority, with raw terminator use reduced to consistency validation only.

## Watchouts

Do not replace local scans with another comparison-local wrapper over BIR
iteration. The next code packet should either consume existing prepared
lookups directly or add the smallest shared prepared query needed by the
comparison consumer.

## Proof

Read-only audit only. No build/test was required or run; no `test_after.log`
was produced for this audit-only packet.
