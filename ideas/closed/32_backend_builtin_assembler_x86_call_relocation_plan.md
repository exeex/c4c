# x86 Built-in Assembler Call Relocation Plan

## Status

Completed on 2026-03-29 as a contract-discovery note.

Closed after Step 1 disproved the assumed relocation-bearing contract for
`tests/c/internal/backend_case/call_helper.c` and the real bounded work moved
to `ideas/closed/33_backend_x86_external_call_object_plan.md`.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/23_backend_builtin_assembler_x86_plan.md`
- `ideas/closed/24_backend_builtin_linker_x86_plan.md`
- `ideas/closed/31_backend_x86_runtime_case_convergence_plan.md`

## Goal

Implement the next bounded built-in x86 assembler slice so the compiler can emit
a relocatable object for the already-closed helper-call runtime seam, with one
relocation-bearing external call on the backend-owned asm path.

## Execution Notes

- 2026-03-29: Step 1 verification showed that
  `tests/c/internal/backend_case/call_helper.c` currently lowers to:
  `.type helper, %function`, a local `helper:` definition, `.globl main`,
  `.type main, %function`, and `call helper`.
- External `clang --target=x86_64-unknown-linux-gnu -c` resolves that local
  in-object call directly and emits no `.rela.text` relocation for this
  fixture.
- The relocation-bearing x86 call contract already staged elsewhere in the repo
  still exists, but it belongs to the separate `helper_ext` external-call
  object shape rather than the current `call_helper.c` runtime fixture.
- This idea is now intentionally parked in favor of
  `ideas/open/33_backend_x86_external_call_object_plan.md`, which carries the
  real relocation-bearing external-call contract forward as a separate bounded
  initiative.

## Why This Is Separate

- the first built-in x86 assembler slice intentionally stopped before
  relocation-bearing instruction forms
- the runtime convergence plan already proved `call_helper.c` on the x86
  backend-owned asm-text path, so the next gap is object emission for that same
  bounded mechanism
- the first built-in linker slice already applies the relevant call relocation,
  so this child should focus on feeding that existing linker contract rather
  than widening linker scope

## Scope

- parse and encode the narrow x86 helper-call assembly subset needed for one
  external `call` relocation
- emit the matching ELF64 relocatable object metadata and relocation entry for
  that bounded call shape
- route one existing x86 backend helper-call fixture through the built-in
  assembler path instead of the external assembler for object generation
- validate the emitted object and final linked result against the already-staged
  helper-call reference path

## Primary Targets

- `src/backend/x86/assembler/`
- `src/backend/x86/codegen/emit.hpp`
- `src/backend/x86/codegen/`
- `tests/c/internal/backend_case/call_helper.c`
- x86 backend adapter / assembler tests that currently stop before
  relocation-bearing object output

## Validation Target

- add or tighten the narrowest assembler/object test that proves one external
  call relocation is emitted as expected
- prove the helper-call runtime case can complete the assembler stage through
  the built-in x86 assembler on the bounded path
- keep full-suite regression monotonic after the slice lands

## Explicit Non-Goals

- generic x86 relocation completeness
- RIP-relative data-address materialization beyond what the bounded helper-call
  path requires
- new built-in linker features, dynamic linking, shared-library support, TLS,
  or PLT/GOT expansion beyond the already-closed first linker slice
- widening into broader parameter-passing or branch families that are already
  covered by the closed runtime convergence plan

## Suggested Execution Order

1. inventory the exact x86 backend-owned asm emitted for the bounded
   `call_helper.c` path and confirm the relocation shape against external
   assembler output
2. tighten a focused assembler/object test so the built-in assembler must emit
   the expected relocation-bearing object for that helper-call slice
3. port only the parser, encoder, and ELF-writer pieces needed for one external
   call relocation
4. route the bounded helper-call object-emission path through the built-in x86
   assembler
5. validate the linked helper-call executable against the existing reference
   path and then run the full regression suite

## Good First Patch

Teach the built-in x86 assembler to emit the single relocation-bearing object
shape needed by `tests/c/internal/backend_case/call_helper.c`, then prove the
object metadata matches the external assembler for that bounded case.

## Parked Handoff

- Step 1 is complete only as a contract-discovery slice.
- Do not continue implementing relocation-bearing x86 assembler work from this
  idea against `call_helper.c`; that fixture currently exercises the local-call
  path and produces no `.rela.text` entry.
- Resume relocation-bearing work from
  `ideas/open/33_backend_x86_external_call_object_plan.md`.

## Completion Notes

- The discovery result for this idea is durable: `call_helper.c` is a local-call
  fixture and not the relocation-bearing x86 object contract.
- The actual bounded relocation-bearing work was split into the `helper_ext`
  child and is now closed in
  `ideas/closed/33_backend_x86_external_call_object_plan.md`.
- No implementation should resume from this note unless the runtime fixture
  itself changes shape in a future initiative.
