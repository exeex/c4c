# Pseudo Instruction Schema

Status: scaffold (unimplemented).

## Owns

The closed, target-aware but machine-independent pseudo instruction schema
accepted before BIR allocation. The admitted set will include ordinary
abstract operations, control-flow operations, `InlineAsm`, and explicit
`Spill` and `Reload` operations.

## Does not own

Concrete machine opcodes, concrete register spellings, assembler parsing,
allocation policy, or a second value/CFG graph. `InlineAsm` continues to use
ordinary BIR operands and results while its instruction text remains opaque.

## Input

Canonical BIR semantics plus verified target-layout and constraint facts used
by pseudo lowering.

## Output

A newly published immutable BIR revision/stage containing the admitted
pseudo-node form, ready for shared BIR liveness and allocation. Unchanged
entities preserve stable IDs, and the new revision maintains the input's
ownership-graph semantics.

## Verification and publication gate

Publication is transactional. The pseudo-stage verifier must reject nodes
outside the closed set, malformed operand/result shapes, invalid terminators,
target opcodes/register spellings, and stale layout or constraint facts before
the new revision becomes authoritative.
