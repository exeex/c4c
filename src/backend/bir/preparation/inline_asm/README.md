# Inline Assembly Plan

Status: scaffold (unimplemented); architecture candidate pending independent
review.

Consumes the opaque canonical `InlineAsm` operation, its ordinary ordered
operands/results, and a verified target layout. It types admitted source
constraints into revision-bound category/class/group requirements, assignment
ties, early-clobber interference, effects, and resolved abstract clobber units.
It never mutates Canonical BIR or inspects asm template bytes.

RV64 admission is closed to `r`, `=r`, `VR`, `VRM2`, `VRM4`, and `VRM8`,
including reviewed read/write, numeric tie, early-clobber, and clobber forms.
`VRM1`, alternatives, named/fixed-register operands, and unreviewed AArch64 or
x86 spellings fail closed. A tie equates assignments without merging the
incoming use, produced result, destination, or tied-input identities.

Publication verifies exact revision/target keys, ordinal and role bindings,
type/class/group compatibility, pool capacity, tie legality, early-clobber
exclusions, and clobber resolution. Any error publishes no inline-asm facts.

Legacy coverage: `prealloc/inline_asm.*`, stack-layout inline asm, regalloc
interaction, and every target emitter's inline-asm path.
