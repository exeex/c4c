# x86 External Call Object Relocation Plan

## Status

Completed on 2026-03-29 and ready to archive.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Related blocked idea:

- `ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md`

## Goal

Add one bounded x86 backend-owned asm path that emits an actual external-call
relocation-bearing object, then route the built-in assembler validation through
that contract instead of the relocation-free local `call_helper.c` fixture.

## Why This Exists

- On 2026-03-29, execution of idea 32 confirmed that
  `tests/c/internal/backend_case/call_helper.c` currently emits a local
  `helper` definition plus `call helper`, and external assembly of that text
  produces no `.rela.text` relocation.
- The staged x86 linker slice already expects the separate external-call object
  shape: one `R_X86_64_PLT32` relocation at `.text` offset `1` with addend
  `-4`, targeting `helper_ext`.
- Preserving that relocation-bearing contract now requires a dedicated backend
  fixture or asm path that truly calls an undefined external helper symbol.

## Scope

- identify or add one bounded x86 backend fixture that lowers to `call
  helper_ext` (or equivalent undefined helper symbol)
- keep the generated object contract to one `.text` relocation entry and one
  imported symbol
- validate the built-in x86 assembler object against the staged linker
  expectations for the existing first static-link slice

## Non-Goals

- broader x86 relocation-family coverage
- new linker features beyond the already-closed first x86 PLT32 slice
- retargeting unrelated local helper-call runtime fixtures unless explicitly
  requested

## Suggested First Slice

1. Add or identify a backend-owned x86 fixture that emits a direct call to an
   undefined helper symbol.
2. Tighten a focused object test so the built-in assembler must emit:
   `R_X86_64_PLT32`, offset `1`, addend `-4`, symbol `helper_ext`.
3. Only after that contract exists, extend the x86 built-in assembler parser,
   encoder, and ELF writer for the single external-call relocation shape.

## Completion Notes

- The bounded x86 backend-owned extern-call fixture now lowers directly to
  `call helper_ext` followed by `ret`, which matches the staged linker
  contract instead of the earlier relocation-free local-call and
  argument-passing seams.
- Focused validation now covers the full narrow slice:
  x86 codegen renders the extern-call path as assembly, the built-in assembler
  records one `.text` `R_X86_64_PLT32` relocation at offset `1` with addend
  `-4`, and the emitted object parses cleanly through the shared linker object
  reader.
- External validation now compares the built-in x86 object surface against a
  `clang --target=x86_64-unknown-linux-gnu -c` baseline for the same bounded
  asm and confirms matching disassembly plus relocation-bearing object shape.
- Full-suite regression remained monotonic for closure:
  `test_fail_before.log` and `test_fail_after.log` both reported
  `2339 passed / 0 failed / 2339 total`, with no newly failing tests.

## Leftover Issues

- Broader x86 relocation families, multi-relocation object shapes, GOT/PLT
  expansion, and unrelated runtime-fixture retargeting remain intentionally out
  of scope for this closed first extern-call object slice.
