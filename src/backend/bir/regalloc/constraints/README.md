# Register Constraints

Status: scaffold (unimplemented).

## Owns

Typed interpretation of allocation constraints such as `r`, `=r`, `VR`, and
`VRM2`, including roles, ties, clobbers, class selection, and register-group
requirements. Constraints attach to the ordinary ordered operands/results of
`InlineAsm`; they do not create a separate value system.

## Does not own

Parsing inline assembly instruction text, interpreting hard-coded register
names inside that text, SSA construction, allocation policy, or concrete target
register selection.

## Input

Raw source constraint descriptions, ordinary BIR operand/result ordinals, and
the verified target-layout contract.

## Output

Typed, verifier-checked allocation facts keyed to the BIR revision and
instruction/value identities.

## Verification and publication gate

Publication must reject unknown or target-ineligible constraints, missing
operand/result bindings, illegal ties or role combinations, incompatible
classes/groups, and stale identity keys. Opaque asm text is never inspected by
this gate.
