# AArch64 Comparison Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/53_aarch64_comparison_prepared_authority_repair.md

## Purpose

Repair duplicate comparison authority in AArch64 lowering so
`comparison.cpp` consumes shared prepared control-flow, branch-condition,
value-home, scalar-publication, block-entry publication, source-producer, and
memory facts instead of reconstructing semantic branch and fused-compare
state locally.

## Goal

Make `src/backend/mir/aarch64/codegen/comparison.cpp` rely on prepared
authority for conditional branch records, fused-compare operand recovery,
materialized compare condition branches, current block-entry fused compares,
constant RHS fused compares, and stack-home/select/load producer gates, adding
narrow shared lookups only where existing prepared facts cannot answer the
comparison consumer query.

## Core Rule

Comparison lowering may own AArch64 compare, branch, `cset`, `csel`, register,
immediate, and print-operand spelling, but semantic authority must come from
prepared facts rather than raw terminator reconstruction, same-block
cast/load/constant producer scans, named materialized-condition lookups, raw
block-entry move recovery, or local select/load producer-kind gates.

## Read First

- `ideas/open/53_aarch64_comparison_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- Prepared control-flow, branch-condition, value-home, scalar-publication,
  block-entry publication, source-producer, and memory-access lookup helpers.
- Recent AArch64 prepared-authority repairs when shared lookup shape is
  unclear, especially the memory, store-source, and ALU authority plans.

## Current Targets

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- Shared prepared lookup surfaces only when existing prepared contracts cannot
  represent the comparison consumer's semantic query.

## Non-Goals

- Do not change condition-code spelling, `cmp`, `cmn`, `fcmp`, `cset`, `csel`,
  branch emission, typed register/immediate print operands, scratch selection,
  fused compare-branch instruction shape, or `cbnz` fallback except to consume
  shared facts.
- Do not classify emitted-scalar reuse or typed register conversion as
  duplicate semantic authority by itself.
- Do not fold ALU select-chain repair, dispatch publication repair, call
  materialization, or unrelated AArch64 backend cleanup into this plan.
- Do not weaken supported-path expectations or downgrade failing tests to
  unsupported without explicit user approval.

## Working Model

- `comparison.cpp` is the AArch64 comparison lowering owner and should remain
  the place that converts prepared comparison facts into target compare,
  branch, and condition-materialization instructions.
- `PreparedControlFlowBlock`, `PreparedBranchCondition`,
  `find_prepared_branch_condition`, and `PreparedValueHome` should provide
  branch target, predicate, compare type, and operand-home meaning before
  lowering reaches target-specific emission.
- Prepared scalar-publication, edge source-producer, memory-access, and
  block-entry publication facts should answer source recovery questions now
  reconstructed from same-block producers or raw moves.
- If the current prepared lookup is missing a key needed by comparison
  lowering, add the smallest shared query that exposes the already-prepared
  fact.
- Delete or narrow obsolete local recovery once the prepared fact is consumed;
  do not keep the old authority under a renamed helper.

## Execution Rules

- Keep each code-changing step behavior-preserving apart from authority-source
  repair.
- Use `todo.md` to record audit findings and proof results; do not rewrite the
  source idea for routine discoveries.
- Prefer one bounded repair family per executor packet.
- Reject testcase-shaped matching, named-case shortcuts, expectation rewrites,
  unsupported downgrades, or helper-only renames as progress.
- For each implementation step, run at least a fresh build or compile proof and
  the supervisor-selected narrow comparison/backend tests.
- Escalate to broader backend validation before closure because comparison
  lowering affects branch emission, fused compare-branch lowering,
  materialized condition values, and publication paths.

## Step 1: Audit Comparison Recovery Sites

Goal: map each duplicate authority path in `comparison.cpp` to an existing or
missing prepared fact before making implementation edits.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Inspect `make_prepared_conditional_branch_record` and its callers to confirm
  which branch target, predicate, compare type, and operand facts already come
  from prepared control-flow and branch-condition state.
- Inspect `lower_fused_compare_branch_from_emitted_cast` for same-block
  cast/load producer scans and local constant evaluation used as fused-compare
  authority.
- Inspect `lower_materialized_compare_condition_branch` for
  `hooks.find_same_block_named_producer` and
  `hooks.emit_value_publication_to_register` semantic source discovery.
- Inspect `lower_current_block_entry_fused_compare_branch` for block-entry
  publication recovery that should consume prepared block-entry state.
- Inspect `lower_constant_rhs_fused_compare_branch` for binary producer,
  constant-evaluation, and stack-home publication gates.
- Inspect `lower_stack_home_fused_compare_branch` and
  `fused_compare_operand_has_select_producer` for local select/load
  producer-kind gates.
- Record in `todo.md` the exact replacement authority or missing lookup needed
  for each family.

Completion check:

- The duplicate recovery sites are grouped into bounded repair packets with
  named prepared facts or explicit missing shared queries.

## Step 2: Repair Conditional Branch Record Authority

Goal: make conditional branch record construction consume prepared branch and
control-flow facts without reconstructing predicate or target labels from raw
terminators.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Route `make_prepared_conditional_branch_record` through
  `PreparedControlFlowBlock`, `PreparedBranchCondition`, and
  `find_prepared_branch_condition` for branch targets, predicates, compare
  type, and operand references.
- Keep `make_condition_register_operand`,
  `make_fused_compare_print_operand`, and
  `install_fused_compare_print_operands` as target-local print operand paths
  after prepared semantic facts are known.
- Remove or narrow any raw terminator parsing or branch-target reconstruction
  made redundant by prepared facts.
- Preserve AArch64 branch and compare instruction spelling.

Completion check:

- Conditional branch records are keyed by prepared branch/control-flow facts,
  with focused proof covering affected conditional branch lowering.

## Step 3: Repair Fused-Compare Operand Producer Authority

Goal: replace same-block cast/load/constant recovery for fused compare
branches with shared prepared branch-condition, source-producer, scalar
publication, or memory authority.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Replace `find_same_block_cast_producer`,
  `find_same_block_load_producer`, and
  `evaluate_same_block_integer_constant` recovery in
  `lower_fused_compare_branch_from_emitted_cast` with existing prepared
  operand, producer, scalar-publication, or memory lookups where available.
- Add a narrow prepared fused-compare operand producer query only if existing
  prepared facts have the information but no comparison-suitable consumer
  lookup.
- Add or consume a prepared load-local source query when fused compare lowering
  needs load-local producer recovery.
- Remove obsolete local producer scans once the prepared lookup owns the
  semantic decision.

Completion check:

- Fused-compare cast, load, and constant operand recovery no longer depends on
  comparison-local same-block producer or constant scans.

## Step 4: Repair Materialized Condition Branch Authority

Goal: make materialized compare condition branch lowering consume prepared
producer or scalar-publication authority instead of named same-block hooks.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Replace semantic source discovery through
  `hooks.find_same_block_named_producer` with a prepared branch-condition
  producer instruction index, scalar-publication lookup, or another shared
  producer query keyed by the materialized condition value.
- Keep target-local condition-register operand construction and publication to
  registers only after the prepared source is known.
- Add the smallest shared lookup if materialized condition producer facts are
  already prepared but not exposed to this consumer.
- Remove or narrow obsolete same-block named-producer hooks from the comparison
  path after prepared authority is consumed.

Completion check:

- Materialized compare condition branch lowering no longer depends on local
  same-block producer hooks for semantic source discovery.

## Step 5: Repair Block-Entry And Constant RHS Fused-Compare Authority

Goal: route current block-entry and constant RHS fused-compare paths through
prepared publication and branch-condition operand facts.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Route `lower_current_block_entry_fused_compare_branch` through
  `PreparedBlockEntryPublication` and
  `prepared_block_entry_publication_available` instead of rebuilding
  block-entry state from raw moves or predecessor labels.
- Replace `lower_constant_rhs_fused_compare_branch` binary producer and
  constant-evaluation recovery with a prepared constant or materialized
  fused-compare operand query keyed by branch-condition operand.
- Preserve stack/global home materialization and target compare emission
  behavior once prepared operand facts are known.
- Delete or narrow obsolete local recovery helpers after the prepared facts are
  consumed.

Completion check:

- Current block-entry and constant RHS fused-compare lowering consume prepared
  publication and operand authority rather than local producer reconstruction.

## Step 6: Repair Stack-Home And Producer-Kind Gates

Goal: replace stack-home, select, load-local, and load-global producer-kind
gates with shared fused-compare operand producer-kind authority.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Route `lower_stack_home_fused_compare_branch` through prepared value-home,
  scalar-publication, or fused-compare operand producer-kind facts instead of
  local stack-home publication gates.
- Replace `fused_compare_operand_has_select_producer` local select/load checks
  with a shared prepared fused-compare operand producer-kind query.
- Ensure the shared query can distinguish at least select, load-local, and
  load-global source kinds where comparison lowering needs that distinction.
- Remove local producer-kind gates once the shared query owns the semantic
  decision.

Completion check:

- Stack-home/select/load producer gates are represented by shared prepared
  fused-compare operand producer-kind authority.

## Step 7: Final Validation And Closure Check

Goal: prove the source idea's acceptance criteria are satisfied without
testcase overfit or expectation downgrades.

Actions:

- Run the supervisor-selected narrow comparison/backend proof for each repaired
  family.
- Run broader backend validation sufficient to catch conditional branch,
  fused-compare, materialized condition, and publication regressions.
- Review the diff for raw terminator target reconstruction, same-block
  cast/load scans, local constant-fold producer scans, named compare-condition
  matches, raw block-entry move recovery, new select/load-global named-case
  checks, unsupported-test rewrites, or helper-only renames.

Completion check:

- All source acceptance criteria are satisfied, regression proof is current,
  and any remaining work is outside this source idea's scope.
