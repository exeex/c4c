# AArch64 Scalar And Control-Flow Prepared Authority Cleanup

## Goal

Make AArch64 scalar, cast, and comparison lowering consume BIR opcode/type
facts plus prepared scalar producer, computed-value, placement, and
control-flow facts before choosing concrete AArch64 scalar, cast, compare, and
branch records.

## Why This Exists

The large-owner residue audit classified several scalar and comparison helper
groups as `consume-shared`: BIR scalar and cast semantics, prepared scalar
homes, same-block producers, computed values, immediate-binary classification,
branch-condition authority, short-circuit plans, compare joins, fused compare
operand producers, and branch labels. The target-local residue should be
AArch64 opcode selection, immediate admissibility, materialization, extension
instruction choice, condition-code selection, and record construction.

## Owned Files

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- Shared prepared authority call sites only where needed to consume existing
  facts:
  - `src/backend/prealloc/control_flow.hpp`
  - `src/backend/prealloc/prepared_lookups.*`
  - BIR opcode and type query surfaces

## In Scope

- Use `bir::BinaryOpcode`, `bir::CastOpcode`, `bir::TypeKind`, and BIR
  value/type queries as the semantic source of truth.
- Consume `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage
  plans, regalloc facts, decoded home storage, `PreparedSameBlockScalarProducer`,
  `PreparedComputedValue`, and prepared immediate-binary classification.
- Consume `PreparedControlFlowFunction`, `PreparedBranchCondition`,
  `PreparedShortCircuitBranchPlan`, `PreparedMaterializedCompareJoin*`,
  `PreparedFusedCompareOperandProducer`, and prepared branch labels.
- Keep AArch64 immediate admissibility, fallback materialization, scalar/cast
  record production, compare operand materialization, condition-code spelling,
  and final branch emission local.

## Out Of Scope

- A broad rewrite of ALU, cast, and comparison owners for line-count reduction.
- Moving AArch64 condition-code spelling or immediate encoding rules into BIR
  or prealloc.
- Folding repeated local register-view helpers; that belongs to the local
  helper fold-back follow-up.
- Changing BIR semantics to fit one target testcase.

## Proof Expectations

- Focused AArch64 scalar, cast, comparison, branch, short-circuit, and
  materialized-compare tests or CTest subset.
- Dumps, assertions, or test coverage proving the route consumes prepared
  scalar/control-flow facts before target instruction selection.
- Regression guard logs for acceptance-sized slices.

## Reviewer Reject Signals

- Local helpers still duplicate BIR opcode/type semantics, branch-condition
  selection, compare-join decisions, or operand provenance behind new names.
- A patch special-cases a named scalar, cast, or compare testcase instead of
  consuming prepared authority generally.
- The route moves AArch64 immediate admissibility, condition-code spelling, or
  record construction into shared code.
- Expectations are weakened or unsupported markers are added without explicit
  approval.
- The diff mixes unrelated memory, call, or printer cleanup into this scalar
  and control-flow authority route.
