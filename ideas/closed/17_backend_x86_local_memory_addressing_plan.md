# x86 Local Memory Addressing Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`

Should follow:

- the initial bounded x86 asm bring-up from `ideas/closed/15_backend_x86_port_plan.md`

## Goal

Extend the x86 backend from narrow return/direct-call seams into explicit backend-owned lowering for local stack-slot addressing, starting with frontend-emitted local array shapes that require `alloca` plus `getelementptr`-based address formation.

## Why This Is Separate

- the next local-memory x86 cases are not another exact single-slot scalar rewrite
- they require stack-local base-address materialization, indexed addressing, and repeated indirect loads/stores
- that is a real emitter-contract expansion and should not be hidden inside the original x86 bring-up closeout

## Scope

### Initial target

- one bounded single-function local-array stack case through the x86 asm path
- integer-element local storage with straightforward indexed addressing
- an explicit backend-owned address-formation seam rather than ad hoc LIR collapse

### Follow-on targets

- local address-taken scalars
- repeated load/store traffic through derived local addresses
- small adjacent member-addressing cases only after the first local-array slice is stable

## Explicit Non-Goals

- broad pointer arithmetic lowering
- global-address materialization
- built-in assembler or linker work
- broad x86 ABI cleanup beyond what this first local-memory seam strictly requires

## Suggested Execution Order

1. capture the real x86 LIR shape for the smallest local-array runtime case
2. define the narrowest backend-owned stack-base plus indexed-address representation
3. add targeted adapter/emitter tests
4. promote one runtime case through `BACKEND_OUTPUT_KIND=asm`
5. split again if the implementation starts requiring broad aggregate or pointer lowering

## Validation

- backend adapter tests cover the exact local-array addressing seam
- one bounded x86 runtime local-array case runs with `BACKEND_OUTPUT_KIND=asm`
- the emitted assembly keeps the local-memory/addressing boundary explicit in backend code

## Good First Patch

Promote one bounded local-array runtime case through the x86 asm path with the smallest explicit stack-slot plus indexed-address seam.

## Completion

Completed on 2026-03-29.

- Added one focused x86 backend adapter/emitter validation for the bounded local `[2 x i32]` addressing slice.
- Implemented a minimal x86 local-array emitter path that recognizes the single-function alloca/GEP/store/load/add shape and emits backend-owned stack-base addressing.
- Promoted `tests/c/internal/backend_case/local_array.c` through `BACKEND_OUTPUT_KIND=asm`.

## Leftover Issues

- Remaining x86 asm runtime failures such as branch, helper-call, global-address, and broader local-slot rewrites are still open and should continue as separate follow-on slices under the backend roadmap umbrella.
- This idea intentionally stopped at the first explicit stack-local base plus indexed `i32` access seam and did not widen into general pointer arithmetic or global-address materialization.
