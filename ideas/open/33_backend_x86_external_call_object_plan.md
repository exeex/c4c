# x86 External Call Object Relocation Plan

## Status

Open.

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
