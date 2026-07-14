# LIR Aggregate And Vector Value Identity Convergence

Status: Open
Type: bounded LIR aggregate/vector value authority repair
Predecessor: `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`

## Goal

Publish structured result/use authority for aggregate and vector LIR operations
whose values, indices, masks, or element carriers still rely on `%t` and raw
presentation strings.

## Why This Exists

Scalar call/binop/cmp/cast/select work has begun using `fresh_value` and
`LirOperand` authority, but aggregate and vector producers remain largely
text-based. These rows are separate from pointer/object work because they need
opcode-specific aggregate, element, index, and mask validation in addition to
ordinary value ownership.

## In Scope

- Convert representative `LirExtractValueOp`, `LirInsertValueOp`,
  `LirInsertElementOp`, `LirExtractElementOp`, and `LirShuffleVectorOp`
  producers to structured result/use authority.
- Add exact type/index/mask validation needed for those rows.
- Preserve compatibility rendering while preventing result/use recovery from
  display strings.
- Add focused coverage for aggregate extraction/insertion and vector
  insert/extract/shuffle chains, including malformed value/type/index/mask
  cases.

## Out Of Scope

- CFG/PHI predecessor identity, local/object pointer authority, memory/va
  pointer semantics, Raw-BIR receiver work, target lowering, MIR, or emission.
- Opaque inline-asm assembly/constraint text.
- Treating instruction order, `%t` spelling, rendered LLVM, or testcase names
  as identity.

## Acceptance Criteria

- Representative aggregate and vector rows publish structured result/use
  identity and exact row-specific typed facts.
- The verifier rejects invalid result IDs, unknown/cross-function uses, and
  type/index/mask conflicts.
- Misleading display text cannot repair or select aggregate/vector identity.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.
