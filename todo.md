Status: Active
Source Idea Path: ideas/open/208_aarch64_branch_compare_target_mir_records.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Record Core And Branch Inputs

# Current Packet

## Just Finished

Completed Step 1 inspection for `plan.md`: confirmed the accepted AArch64
target-record owner, the current branch/compare record gap, and the prepared
control-flow/BIR inputs for the first implementation packet.

Concrete owner files:
- Target record vocabulary owner: `src/backend/mir/aarch64/codegen/records.hpp`
  and `src/backend/mir/aarch64/codegen/records.cpp`.
- Local contract/guardrail owner when documentation changes are needed:
  `src/backend/mir/aarch64/codegen/records.md`.
- Prepared/BIR snapshot owner remains `src/backend/mir/aarch64/module/`;
  do not place new Step 2 target branch/compare records there.
- Shared prepared/BIR input definitions live in
  `src/backend/prealloc/prealloc.hpp` and `src/backend/bir/bir.hpp`.

Current branch/compare target-record gap:
- `BranchTargetOperand` carries only one `BlockLabelId`, `FunctionNameId`, and
  optional condition `PreparedValueId`.
- `BranchInstructionRecord` carries one target, optional generic condition
  operand, and `conditional`; it cannot represent true/false target pairs,
  distinguish materialized-bool branches from fused compare branches, or retain
  compare predicate/type/lhs/rhs authority.
- There is no AArch64 compare predicate record or compare operand-pair record;
  Step 2 should add record-only vocabulary before conversion helpers consume
  prepared control-flow facts.

Prepared control-flow structures to consume later:
- `PreparedBirModule::control_flow.functions[]` gives
  `PreparedControlFlowFunction`.
- `PreparedControlFlowFunction::blocks` gives `PreparedControlFlowBlock`
  entries with `block_label`, `terminator_kind`, `branch_target_label`,
  `true_label`, and `false_label`.
- `PreparedControlFlowFunction::branch_conditions` gives
  `PreparedBranchCondition` entries with `function_name`, `block_label`,
  `kind`, `condition_value`, `predicate`, `compare_type`, `lhs`, `rhs`,
  `can_fuse_with_branch`, `true_label`, and `false_label`.
- `PreparedBranchConditionKind` already distinguishes `MaterializedBool` from
  `FusedCompare`.
- `PreparedBranchTargetLabels` carries authoritative `true_label` and
  `false_label` pairs. Helpers such as
  `find_prepared_control_flow_branch_target_labels(...)`,
  `find_prepared_compare_branch_target_labels(...)`, and
  `resolve_prepared_compare_branch_target_labels(...)` can validate prepared
  target pairs against BIR terminators.

BIR terminator/predicate/value inputs:
- `bir::TerminatorKind` includes `Return`, `Branch`, and `CondBranch`.
- `bir::Terminator` carries `target_label_id` for unconditional branch,
  `condition`, `true_label_id`, and `false_label_id` for conditional branch;
  label spellings are compatibility/display fields only.
- `bir::Value` carries `kind`, `type`, immediate payload, display `name`, and
  optional semantic `pointer_symbol_link_name_id`.
- `bir::BinaryInst` carries `opcode`, `result`, `operand_type`, `lhs`, and
  `rhs`; compare predicates are represented by `bir::BinaryOpcode` values
  `Eq`, `Ne`, `Slt`, `Sle`, `Sgt`, `Sge`, `Ult`, `Ule`, `Ugt`, and `Uge`.
- `bir::TypeKind` supplies compare/value type facts; `PreparedValueId` /
  `ValueNameId` should remain the target-record identity where available.

## Suggested Next

Execute Step 2 from `plan.md`: add record-only branch target and compare
predicate vocabulary in `src/backend/mir/aarch64/codegen/records.hpp` /
`.cpp`, with focused backend tests proving true/false target ids, predicate
and type facts, compare operands, and prepared value ids survive without
rendered-name recovery.

## Watchouts

- Missing upstream facts: none blocking Step 2. The prepared carrier already
  has branch condition kind, target label ids, predicate opcode, compare type,
  lhs/rhs values, and materialized condition value.
- Deferred blocker for later conversion work: mapping arbitrary `bir::Value`
  operands to target operands may need lookup through prepared value locations
  or storage plans; do not invent string recovery in Step 2.
- Keep Step 2 record-only: no `cmp`, `cset`, `b.cond`, `cbz`, `cbnz`, `tbz`,
  `tbnz`, assembler text, object emission, or opcode selection.
- Keep branch/compare target records under `codegen/`; `module/` remains the
  prepared/BIR snapshot layer.

## Proof

No compile proof required or run for this inspection-only packet; no
`test_before.log` or `test_after.log` was created.

Recommended Step 2 proof command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```
