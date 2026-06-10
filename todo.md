# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared comparison producer oracle

## Just Finished

Step 1 inspected the prepared comparison producer oracle.

Prepared helper surfaces:

- `prepare::find_prepared_fused_compare_operand_producer` returns
  `PreparedFusedCompareOperandProducer`: producer `kind`, `instruction`,
  `instruction_index`, prepared `value_name`, typed producer pointers
  (`cast`, `load_local`, `load_global`, `binary`, `select`), and optional
  folded `integer_constant`.
- `prepare::find_prepared_fused_compare_operand_producer_facts` returns
  optional lhs/rhs producer facts for `PreparedBranchCondition::lhs` and `rhs`
  when the branch condition is `FusedCompare`.
- `prepare::find_prepared_materialized_condition_producer` returns
  `PreparedMaterializedConditionProducer`: comparison-producing `BinaryInst`,
  producer `instruction_index`, and prepared `condition_value_name`.
- AArch64 wrappers in `src/backend/mir/aarch64/codegen/comparison.cpp` route
  these helpers through `PreparedFunctionLookups` when available, otherwise
  rebuild edge-publication source-producer lookups.

BIR-owned semantic facts:

- condition value identity by BIR value/name and type;
- comparison-producing `BinaryInst` identity and same-block instruction index;
- lhs/rhs operand producer kind and instruction index when same-block;
- integer constants, including immediate operands and same-block folded
  integer constants;
- enough producer/value identity to fail closed on missing, ambiguous,
  different-block, or after-branch producers.

Target/prepared-owned policy that must stay outside BIR:

- `PreparedBranchCondition::can_fuse_with_branch`, branch true/false label
  routing, short-circuit/materialized-compare join target rewriting, and
  target branch strategy;
- AArch64 condition suffix selection, operand register view selection,
  immediate-encoding legality, scratch preference, hazard/emitted-register
  tracking, final instruction records, and diagnostics;
- prepared numeric value ids and value-home/register authority.

Current coverage exists for prepared branch records and AArch64 lowering:
materialized bool records, fused compare records with named lhs/rhs, unsupported
predicate/non-fusable candidate distinction, missing condition value home,
selected compare operand materialization, and fused compare branch lowering.

Missing narrow coverage before consumer migration:

- BIR-level query fixtures for immediate operands, folded integer constants,
  same-block load/cast/binary/select producers, materialized condition binary
  producers, duplicate/ambiguous producers, missing value names, after-index
  producers, and non-fused/materialized-bool unavailable paths;
- prepared/BIR equivalence coverage for lhs-only, rhs-only, both operands,
  binary compare condition values, and non-fusable negative cases;
- production-lowered comparison/branch coverage proving normal BIR can provide
  the same facts before any AArch64 consumer reads BIR provenance.

## Suggested Next

Execute Step 2 from `plan.md`: add a BIR comparison/condition producer query
surface with fixture tests only. Keep AArch64 compare/branch behavior
unchanged and do not switch consumers.

## Watchouts

- `integer_constant` is a semantic producer fact, but AArch64 immediate
  encodability remains target policy.
- `can_fuse_with_branch` is prepared/AArch64 legality, not BIR provenance.
- Keep fused-compare legality, condition-code selection, branch emission
  strategy, final instruction records/errors, hazard handling, emitted-register
  state, and AArch64 compare/branch instruction selection outside BIR.
- Do not switch comparison or branch consumers before prepared-oracle
  equivalence is proven for the specific provenance read.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Inspection-only Step 1. No build or test proof required.
