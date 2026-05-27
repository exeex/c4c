# AArch64 Comparison Prepared Authority Repair

## Goal

Repair duplicate comparison authority in
`src/backend/mir/aarch64/codegen/comparison.cpp` by consuming prepared control
flow, branch-condition, value-home, scalar-publication, block-entry
publication, source-producer, and memory facts, and by adding missing shared
queries for fused-compare and materialized-condition producer recovery.

## Why This Exists

The audit found that comparison lowering owns target-local compare and branch
spelling, but still carries same-block cast/load/constant producer scans,
materialized compare condition hooks, constant RHS producer scans, and
stack-home/select producer gates that should be shared prepared authority.

## Owned File

- `src/backend/mir/aarch64/codegen/comparison.cpp`

## Duplicated Helpers And Fallback Paths

- `make_prepared_conditional_branch_record` should remain a consumer of
  prepared conditional branch facts.
- `make_condition_register_operand`, `make_fused_compare_print_operand`, and
  `install_fused_compare_print_operands` are target-local print operand paths
  after prepared facts are known.
- `lower_fused_compare_branch_from_emitted_cast` uses
  `find_same_block_cast_producer`, `find_same_block_load_producer`, and
  `evaluate_same_block_integer_constant` to recover fused-compare operand
  producers and constants.
- `lower_materialized_compare_condition_branch` reaches through
  `hooks.find_same_block_named_producer` and
  `hooks.emit_value_publication_to_register` for materialized condition
  producers.
- `lower_current_block_entry_fused_compare_branch` should consume current
  block-entry publication facts.
- `lower_constant_rhs_fused_compare_branch` uses
  `mir::find_same_block_binary_producer`, constant evaluation, and a
  stack-home publication gate.
- `lower_stack_home_fused_compare_branch` and
  `fused_compare_operand_has_select_producer` gate branch lowering on local
  select/load producer-kind checks.

## Shared Facts To Consume Or Add

- Consume `PreparedControlFlowBlock`, `PreparedBranchCondition`,
  `find_prepared_branch_condition`, and `PreparedValueHome` for branch target,
  predicate, compare type, and operand home facts.
- Consume `PreparedScalarPublicationPlan`,
  `PreparedEdgePublicationSourceProducerLookups`, and `PreparedMemoryAccess`
  for materialized compare, fused-compare, and load-local source recovery.
- Consume `PreparedBlockEntryPublication` and
  `prepared_block_entry_publication_available` for current block-entry fused
  compare paths.
- Add or consume a prepared fused-compare operand producer index/kind and
  load-local source query.
- Add or consume a prepared branch-condition producer instruction index or
  scalar-publication hook for materialized compare conditions.
- Add or consume a prepared constant/materialized fused-compare operand query
  keyed by branch-condition operand.
- Add or consume a prepared fused-compare operand producer-kind query,
  especially for select, load-local, and load-global source kinds.

## Out Of Scope

- Do not change condition-code spelling, `cmp`/`cmn`/`fcmp`/`cset`/`csel` or
  branch emission, typed register/immediate print operands, scratch selection,
  fused compare-branch instruction shape, or `cbnz` fallback except to consume
  shared facts.
- Do not classify emitted-scalar reuse or typed register conversion as
  duplicate semantic authority by itself.
- Do not fold ALU select-chain or dispatch publication repair into this idea.

## Acceptance Criteria

- Conditional branch records consume prepared branch/control-flow facts without
  reconstructing predicate or target labels from raw terminators.
- Fused-compare cast/load/constant producer recovery is replaced by shared
  branch-condition, scalar-publication, source-producer, or memory authority.
- Materialized compare condition branch lowering no longer depends on local
  same-block producer hooks for semantic source discovery.
- Current block-entry fused compare lowering consumes prepared block-entry
  publication state.
- Stack-home/select/load producer gates are replaced by a shared
  fused-compare operand producer-kind query where needed.

## Reviewer Reject Signals

- Reject reconstructing fused-compare predicate or branch targets from raw
  terminator text when `PreparedBranchCondition` is available.
- Reject same-block cast/load scans, local constant-fold producer scans, or
  named compare-condition matches as permanent branch-fusion authority.
- Reject rebuilding block-entry publication state from raw moves or predecessor
  labels.
- Reject new select/load-global named-case checks in comparison lowering.
- Reject expectation downgrades, unsupported-test rewrites, broad rewrites
  outside this file, or helper renames claimed as capability progress.
