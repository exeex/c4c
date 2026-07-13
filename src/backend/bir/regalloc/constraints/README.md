# Register Constraints

Status: scaffold (unimplemented).

## Owns

Typed interpretation of admitted allocation constraints. The initial RV64
table contains `r`, `=r`, `VR`, `VRM2`, `VRM4`, and `VRM8`, including reviewed
read/write, numeric ties, early-clobbers, and clobbers. Constraints attach to
ordinary ordered operands/results of `InlineAsm`; they do not create a
separate value system. `VRM1`, alternatives, named/fixed-register operands,
and unreviewed AArch64/x86 spellings fail closed.

## Does not own

Parsing inline assembly instruction text, interpreting hard-coded register
names inside that text, SSA construction, allocation policy, or concrete target
register selection.

## Input

Raw source constraint descriptions, ordinary BIR operand/result ordinals, and
the verified target-layout contract.

## Output

Typed, verifier-checked category/class/group requirements, assignment-equality
ties, early-clobber exclusions, and resolved abstract clobber units keyed to
the complete BIR revision, target-layout version, and instruction/value
identities.

## Verification and publication gate

Publication must reject unknown or target-ineligible constraints, missing
operand/result bindings, illegal ties or role combinations, incompatible
classes/groups, missing capacity, unresolved clobbers, and stale identity or
target keys. A tie never merges SSA identities. Opaque asm text is never
inspected by this gate, and failure publishes no partial fact bundle.
