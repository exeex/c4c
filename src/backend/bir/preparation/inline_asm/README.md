# Inline Assembly Plan

Status: scaffold. Consumes the opaque canonical inline-asm operation and target
context, then resolves constraints, alternatives, clobbers, tied operands,
early-clobber rules, memory effects, and register-class requirements.

Legacy coverage: `prealloc/inline_asm.*`, stack-layout inline asm, regalloc
interaction, and every target emitter's inline-asm path.
