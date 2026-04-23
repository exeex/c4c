# X86 Linker Contract

`src/backend/mir/x86/linker/` is an explicit deferred subsystem contract for
link-time ownership in the x86 backend route.

## Intended Ownership

- accept assembler outputs or future object/link requests
- own x86 target-local linker configuration and artifact handoff
- isolate link orchestration from module emission and assembler staging

## Must Not Own

- prepared-module emission
- x86 ABI or lowering policy
- assembler internal request shaping

## Current Status

The live linker seam is deferred. The important contract decision for this
packet is simply that linker ownership now has a named place in the x86 tree,
so later packets do not keep stuffing link concerns back into unrelated x86
seams.
