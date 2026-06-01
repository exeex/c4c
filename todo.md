Status: Active
Source Idea Path: ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Scalar And Control-Flow Authority Duplication

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for AArch64 scalar/control-flow authority
duplication. The current split is:

- Shared/BIR authority already available or intended to be consumed:
  `bir::BinaryOpcode`, `bir::CastOpcode`, `bir::TypeKind`, prepared value
  homes/storage/regalloc, `PreparedSameBlockScalarProducer`,
  `PreparedComputedValue`, `PreparedSupportedImmediateBinary`,
  `PreparedBranchCondition`, `PreparedShortCircuitBranchPlan`,
  `PreparedMaterializedCompareJoin*`, and
  `PreparedFusedCompareOperandProducer`.
- AArch64-local policy that must stay local: opcode/mnemonic selection,
  immediate encodability checks, fallback materialization, scratch/register
  view spelling, condition-code spelling, compare operand materialization, and
  final scalar/cast/compare/branch record construction.
- Concrete duplication sites to unwind first: ALU and cast helpers already
  read BIR opcodes/types, but they still encode semantic allowlists and
  operation classification locally around
  `make_prepared_scalar_alu_record`,
  `is_scalar_alu_publication_opcode`,
  `scalar_alu_publication_operation`,
  `make_prepared_scalar_cast_record`,
  `is_simple_integer_cast_opcode`, and
  `is_supported_scalar_conversion_cast_opcode`.
- Later control-flow duplication sites: branch lowering consumes
  `PreparedBranchCondition`, but local helpers still repeat condition suffix,
  label, fusable-compare, materialized-condition, selected-operand, and
  constant/fused operand policy around `branch_condition_suffix`,
  `lower_prepared_conditional_branch_terminator`,
  `lower_materialized_compare_condition_branch`,
  `lower_current_block_entry_fused_compare_branch`,
  `lower_constant_rhs_fused_compare_branch`,
  `lower_fused_compare_branch_from_emitted_cast`, and
  `lower_stack_home_fused_compare_branch`.

## Suggested Next

First implementation packet: `plan.md` Step 2, scoped to BIR scalar/cast
semantic fact consumption in `src/backend/mir/aarch64/codegen/alu.cpp` and
`src/backend/mir/aarch64/codegen/cast_ops.cpp`.

Owned files for that packet:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `todo.md`

Packet goal: make the ALU/cast record builders take BIR opcode/type facts as
the semantic source before local AArch64 record selection, while leaving
immediate admissibility, materialization, register spelling, and machine-record
construction local. Start by replacing local scalar/cast semantic allowlist
duplication with one narrow helper boundary that classifies from
`bir::BinaryOpcode`, `bir::CastOpcode`, and `bir::TypeKind`; do not touch
branch/fused-compare helpers in this packet.

Recommended proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Watchouts

Do not fold local register-view helpers under this route; that belongs to
`ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md`. Keep
AArch64 immediate admissibility, materialization, condition-code spelling, and
machine-record construction target-local.

Do not turn Step 2 into shared-prealloc redesign. The first packet should only
consume existing BIR opcode/type facts more cleanly in AArch64 ALU/cast code.
`PreparedSupportedImmediateBinary`, computed values, branch conditions,
short-circuit plans, materialized compare joins, and fused operand producers
belong to later packets unless the first implementation exposes a direct
compile blocker.

## Proof

Lifecycle/mapping-only packet; no build or CTest proof required and no
`test_after.log` was produced. The recommended first implementation proof is:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
