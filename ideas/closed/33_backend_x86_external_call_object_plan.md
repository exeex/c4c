# x86 External Call Object Emission Plan

## Status

Completed on 2026-03-29.

Staged on 2026-03-29 from the relocation-contract discovery notes captured in
`ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md`.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/23_backend_builtin_assembler_x86_plan.md`
- `ideas/closed/24_backend_builtin_linker_x86_plan.md`
- `ideas/closed/31_backend_x86_runtime_case_convergence_plan.md`

Supersedes the relocation-bearing continuation that was incorrectly framed
against `tests/c/internal/backend_case/call_helper.c` in
`ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md`.

## Goal

Implement the bounded built-in x86 assembler/object slice that emits one
relocation-bearing external call object for the existing `helper_ext` contract,
so the backend-owned assembler can hand the already-staged x86 linker the same
`R_X86_64_PLT32` object shape produced by the external assembler.

## Why This Is Separate

- `call_helper.c` currently lowers to a local in-object `call helper` shape and
  does not produce a `.rela.text` relocation.
- The real relocation-bearing contract already exists in the repo through the
  `helper_ext` external-call object fixture used by bounded x86 assembler and
  linker tests.
- The first built-in x86 linker slice already consumes this relocation shape,
  so the remaining gap is making the built-in assembler emit that caller object
  directly on the same bounded contract.

## Contract Snapshot

- The bounded caller text is `call helper_ext` followed by `ret`.
- The emitted `.text` payload is `e8 00 00 00 00 c3`.
- The object preserves one relocation at offset `1` with type
  `R_X86_64_PLT32` (`4`), symbol `helper_ext`, and addend `-4`.
- The caller object defines `main` in `.text` and leaves `helper_ext`
  undefined.

## Scope

- parse and encode the narrow x86 external-call asm subset needed for one
  `call rel32` relocation-bearing object
- emit the ELF64 relocatable object metadata needed for `.text`, `.rela.text`,
  `.symtab`, `.strtab`, and `.shstrtab` on that bounded path
- route the staged `helper_ext` object module through the built-in x86 assembler
  handoff
- validate the built-in emitted object against external assembler output and the
  already-closed linker slice

## Primary Targets

- `src/backend/x86/assembler/`
- `src/backend/x86/codegen/emit.hpp`
- `src/backend/x86/codegen/`
- `tests/backend/backend_lir_adapter_tests.cpp`

## Validation Target

- keep or tighten the encoder-level test for one relocation-bearing
  `call helper_ext` object
- keep or tighten the shared ELF parser coverage for the built-in emitted x86
  extern-call object
- keep or tighten the external `objdump` parity check for the bounded emitted
  object surface
- prove the bounded x86 linker slice still links and executes the emitted
  caller/helper pair
- keep full-suite regression monotonic after the slice lands

## Explicit Non-Goals

- widening beyond the first external `call rel32` relocation shape
- PLT/GOT expansion, dynamic linking, shared-library support, or generic x86
  relocation completeness
- broader runtime convergence work for unrelated x86 backend cases
- new linker mechanism work beyond the already-closed first x86 linker slice

## Suggested Execution Order

1. confirm the staged `helper_ext` object contract and name the exact tests that
   lock it in
2. tighten the smallest object-surface test that proves the built-in assembler
   emits the expected relocation-bearing caller object
3. implement only the parser, encoder, and ELF writer pieces needed for one
   external `call rel32` relocation
4. route the bounded helper-ext object path through the built-in assembler
   handoff
5. validate parser parity, external assembler parity, linker compatibility, and
   full regression health

## Good First Patch

Make the built-in x86 assembler emit the single `helper_ext`
relocation-bearing object contract already exercised by the bounded adapter
tests, then prove its object surface matches the external assembler baseline.

## Completion Notes

- The built-in x86 assembler path now parses and encodes the bounded
  `call helper_ext` plus `ret` slice and emits the staged `.text` bytes
  `e8 00 00 00 00 c3`.
- The emitted caller object preserves the `.rela.text` PLT32 relocation
  contract at offset `1` with type `4`, symbol `helper_ext`, and addend `-4`.
- The bounded helper-ext object path routes through the built-in assembler
  handoff and remains consumable by the first x86 linker slice.
- Validation on 2026-03-29:
  `build/backend_lir_adapter_tests` passed and
  `ctest --test-dir build -j8 --output-on-failure` passed with
  `2339/2339` tests green.

## Leftover Issues

- None recorded for this bounded slice.
