Status: Active
Source Idea Path: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory BIR Control Facts

# Current Packet

## Just Finished

Step 1 - Inventory BIR Control Facts is complete. I used `c4c-clang-tool`
and `c4c-clang-tool-ccdb` first to identify the relevant BIR carriers and
producer functions, then traced targeted source ranges in the Step 1 files.

### BIR Control Fact Inventory

| Fact | Classification | BIR producer/carrier evidence | Notes |
| --- | --- | --- | --- |
| Function/block CFG identity | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `bir::Function::blocks`, `bir::Block::label`, `bir::Block::label_id`; `src/backend/bir/lir_to_bir/module.cpp`: `lower_block` creates one BIR block per LIR block, and `intern_known_block_labels` interns block labels and fills `BlockLabelId` references. | BIR owns target-neutral block identity after finalization. Raw labels remain compatibility/display spellings; `BlockLabelId` is authoritative when present. |
| Return/branch/conditional branch terminators | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `ReturnTerminator`, `BranchTerminator`, `CondBranchTerminator`, `Terminator`, `TerminatorKind`; `src/backend/bir/lir_to_bir/module.cpp`: `lower_block_terminator` lowers `LirRet`, `LirBr`, and `LirCondBr`. | BIR explicitly carries return value/lanes, unconditional target, conditional I1 condition, and true/false targets. |
| Switch lowering shape | `bir-control-semantic-fact` | `src/backend/bir/lir_to_bir/module.cpp`: `lower_block_terminator` lowers `LirSwitch` into generated compare blocks using `bir::BinaryInst{Eq}` plus `bir::CondBranchTerminator`; `src/backend/bir/bir.hpp`: `BinaryInst`, `CondBranchTerminator`. | BIR does not retain a separate switch terminator; target-neutral semantics are normalized into compare-plus-branch blocks before prealloc. |
| Phi semantics and incoming edge values | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `PhiIncoming`, `PhiInst`; `src/backend/bir/lir_to_bir/cfg.cpp`: `collect_phi_lowering_plans`; `src/backend/bir/lir_to_bir/module.cpp`: `lower_block_phi_insts`, `initialize_aggregate_phi_state`, `apply_pending_aggregate_phi_copies`. | Scalar PHIs become `PhiInst` with incoming labels/values; aggregate PHIs become local-slot copy plans during lowering rather than persisted `PhiInst` values. |
| Phi incoming label identity | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `PhiIncoming::label_id`; `src/backend/bir/lir_to_bir/module.cpp`: `intern_known_block_labels` fills each phi incoming `label_id`. | Final BIR can reference incoming predecessor blocks by `BlockLabelId`; prealloc should not need to rediscover predecessor spelling identity when ids are valid. |
| Instruction value identity and constants | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `Value`, `Value::Kind`, `TypeKind`, `pointer_symbol_link_name_id`; `src/backend/bir/lir_to_bir/scalar.cpp`: `lower_value`, `make_integer_immediate`, float literal parsing, symbol-pointer lowering. | BIR owns typed named/immediate values and link-visible pointer identity; route-local SSA names remain display/final spellings rather than cross-object semantic ids. |
| Scalar binary operation and compare facts | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `BinaryOpcode`, `BinaryInst`, `is_compare_opcode`, `binary_operand_type`; `src/backend/bir/lir_to_bir/scalar.cpp`: `lower_scalar_binary_opcode`, `lower_cmp_predicate`, `lower_scalar_compare_inst`, `lower_scalar_family_inst`. | Compare semantics are stored as `BinaryInst` with compare opcodes (`Eq`, `Ne`, ordered/signed/unsigned relations), I1 result, operand type, and typed operands. |
| Select facts | `bir-control-semantic-fact` | `src/backend/bir/bir.hpp`: `SelectInst`, `select_compare_type`; `src/backend/bir/lir_to_bir/module.cpp`: `try_lower_canonical_select_function`; `src/backend/bir/lir_to_bir/scalar.cpp`: `lower_canonical_select_entry_inst`, `resolve_select_chain_inst`; `src/backend/bir/lir_to_bir/cfg.cpp`: `follow_canonical_select_chain`. | Canonical branch/phi select functions are folded into a BIR `SelectInst` carrying predicate, compare type, lhs/rhs, true value, and false value. |
| Select-chain recognition side facts | `bir-missing-target-neutral-fact` | `src/backend/bir/lir_to_bir/lowering.hpp`: `BranchChain`, `CompareExpr`, `CompareMap`; `src/backend/bir/lir_to_bir/cfg.cpp`: `follow_canonical_select_chain`; `src/backend/bir/lir_to_bir/module.cpp`: `lower_select_chain_value`, `try_lower_canonical_select_function`. | Recognition uses lowering-only side tables and raw LIR labels; after lowering, BIR retains the resulting `SelectInst` but not an explicit "this came from branch/phi select chain" tag. Later steps should check whether prealloc re-derives this shape or only consumes the explicit `SelectInst`. |
| Compare expression side facts | `bir-missing-target-neutral-fact` | `src/backend/bir/lir_to_bir/lowering.hpp`: `CompareExpr`, `CompareMap`; `src/backend/bir/lir_to_bir/scalar.cpp`: `lower_scalar_compare_inst`; `src/backend/bir/lir_to_bir/module.cpp`: `try_lower_canonical_select_function`, `canonicalize_compare_return_alias`. | BIR persists compare instructions and select predicates, but the temporary compare-expression map is not a public BIR contract. Later steps should verify whether prealloc needs a prepared/publication contract for compare joins instead of re-inspecting instruction shapes. |
| Aggregate PHI copy authority | `bir-missing-target-neutral-fact` | `src/backend/bir/lir_to_bir/cfg.cpp`: aggregate `PhiLoweringPlan`; `src/backend/bir/lir_to_bir/module.cpp`: aggregate phi slots and pending copy application. | Aggregate PHI movement is handled during BIR lowering into local copies, not represented as `PhiInst` semantics. Later prealloc audit should confirm no separate out-of-SSA authority is reconstructing aggregate PHI control facts from BIR shapes. |

## Suggested Next

Begin Step 2 by inventorying prealloc transfer and publication plans in
`src/backend/prealloc/out_of_ssa.cpp`, `src/backend/prealloc/control_flow.hpp`,
`src/backend/prealloc/publication_plans.*`, and
`src/backend/prealloc/formal_publications.*`, then compare each prealloc fact
against the BIR authority table above.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Produce follow-up ideas only for traced authority overlaps or contract-test gaps.
- Treat `BlockLabelId`, `PhiIncoming::label_id`, terminator label ids, typed
  `bir::Value`, `BinaryInst`, `PhiInst`, and `SelectInst` as existing BIR
  authority before calling any prealloc reconstruction a gap.
- The provisional `bir-missing-target-neutral-fact` items are not follow-up
  ideas yet; Step 2/3 must first show that prealloc actually re-derives those
  facts or that a lookup/printer contract is missing.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 1 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains:
`analysis-only Step 1 proof: no implementation changes under src/backend/bir or src/backend/prealloc`.
