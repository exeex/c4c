# Pseudo Lowering Pass

Status: scaffold (unimplemented).

## Owns

Lowering Canonical BIR semantic nodes into the closed admitted pseudo
instruction set before allocation, using verified target-layout and typed
constraint facts where target shape affects the pseudo operation.

## Does not own

Register allocation, pseudo-home assignment, spill/reload choice, concrete
machine opcodes, or assembler parsing.

## Input

Verified Canonical BIR plus a profile-keyed target layout and verified
constraint facts.

## Output

An immutable BIR revision containing only admitted pseudo nodes and ordinary
SSA/CFG identities, ready for out-of-SSA processing and allocation.

## Verification and publication gate

The pass must publish transactionally and re-run the pseudo-stage verifier.
Publication rejects unlowered semantic nodes, target opcodes/register
spellings, malformed pseudo shapes, and stale derived facts.
