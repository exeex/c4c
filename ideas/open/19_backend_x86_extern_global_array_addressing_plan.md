# x86 Extern Global Array Addressing Plan

## Relationship To Roadmap

Follows:

- `ideas/open/18_backend_x86_global_addressing_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded x86 global-addressing seam after scalar globals: one extern global array element load through the asm path using explicit RIP-relative base-address formation plus one indexed element access.

## Why This Is Separate

- this seam is adjacent to broader global-addressing work but still narrow enough to test directly
- it exercises array decay and indexed element access without yet bundling string-pool or pointer-difference behavior
- keeping it separate preserves the narrow-slice rule for x86 follow-on work

## Candidate Case

- synthetic backend extern-global-array load fixture
- one runtime extern-global-array element case if needed to prove the seam end to end

## Validation Target

- tighten backend adapter tests so the extern-global-array slice stops accepting fallback
- promote the matching runtime case through `BACKEND_OUTPUT_KIND=asm` once the backend-owned path is explicit

## Non-Goals

- string-pool addressing
- pointer-difference work
- pointer round-tripping
- built-in assembler or linker expansion

## Execution Notes

- prefer one base-address materialization plus one scaled index/load seam
- compare the runtime-emitted LIR against the synthetic builder before changing the emitter
- split again if the implementation starts requiring generic global-pointer arithmetic
