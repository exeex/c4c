# x86 Global Addressing Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`

Should follow:

- the initial x86 asm bring-up from `ideas/closed/15_backend_x86_port_plan.md`

## Goal

Extend the x86 backend beyond exact scalar global loads into explicit backend-owned lowering for global and constant address formation, starting with the smallest cases that require array decay, RIP-relative indexed addressing, string-pool addressing, or pointer/address round-tripping.

## Why This Is Separate

- the next remaining x86 global cases are not another exact `load @symbol` seam
- they require backend-owned base-address formation and indexed or pointer-derived loads
- widening one active x86 plan to absorb all of that would blur the real bounded next slice

## Initial Targets

- one bounded extern-global-array element load through the x86 asm path
- one bounded string-pool decay/indexed-byte load through the x86 asm path
- one bounded pointer-difference or round-trip slice only after the first address-formation seam is explicit and test-backed

## Explicit Non-Goals

- broad local-memory lowering
- general aggregate or pointer arithmetic support
- built-in assembler or linker work
- broad relocation-model completeness

## Suggested Execution Order

1. capture the smallest remaining x86 global-addressing runtime case and matching synthetic backend fixture
2. define the narrowest backend-owned representation for RIP-relative base-address formation plus one indexed-load seam
3. add or tighten targeted adapter/emitter tests before implementation
4. promote the matching runtime case through `BACKEND_OUTPUT_KIND=asm`
5. split again if the implementation starts pulling in generic pointer arithmetic or mixed local/global lowering

## Candidate Cases

- synthetic backend extern-global-array fixture
- `tests/c/internal/backend_case/string_literal_char.c`
- `tests/c/internal/backend_case/global_char_pointer_diff.c`
- `tests/c/internal/backend_case/global_int_pointer_diff.c`
- `tests/c/internal/backend_case/global_int_pointer_roundtrip.c`

## Validation

- backend adapter tests cover the exact x86 global-addressing seam being implemented
- the promoted runtime case runs with `BACKEND_OUTPUT_KIND=asm`
- the x86 backend-owned path emits explicit global base-address formation instead of fallback

## Good First Patch

Promote the smallest explicit global-addressing slice onto the x86 asm path with a backend-owned RIP-relative base-address plus indexed-load seam.
