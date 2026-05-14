# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Mark Terminal Printer And Input Boundaries

## Just Finished

Completed Step 2 from `plan.md`: marked the audited ambiguous live AArch64
headers with explicit boundary wording and no behavior changes.

Updated surfaces:

- `src/backend/mir/aarch64/assembler/mod.hpp`: states that the staged assembler
  namespace and overloads are external assembler text compatibility/object
  emission staging, not the internal compile-through bridge from machine
  instruction nodes.
- `src/backend/mir/aarch64/assembler/parser.hpp`: marks `parse_asm` as an
  external assembler text parser and rejects feeding terminal
  `machine_printer.cpp` `--codegen asm` output back into backend semantics.
- `src/backend/mir/aarch64/assembler/types.hpp`: marks `Operand` and
  `AsmStatement` payloads as text parser compatibility records, not the future
  structured encoder/object contract.
- `src/backend/mir/aarch64/assembler/encoder/mod.hpp`: marks encoder helpers
  as staged parsed-external-assembler helpers and keeps future internal
  compile-through input on structured asm/encoding records.
- `src/backend/mir/aarch64/codegen/emit.hpp`: marks emitters as terminal
  `--codegen asm`-style text output, analogous to the LIR printer, while
  preserving external clang/as smoke validation as terminal-output validation.
- `src/backend/mir/aarch64/linker/mod.hpp`: marks linker inputs as object
  records/files downstream of object emission, not terminal `.s`, external
  `.s`, or external `.ll` input.

## Suggested Next

Execute Step 3 from `plan.md`: define the minimum future structured
asm/encoding record surface without implementing the full encoder, object
writer, or linker.

## Watchouts

- Keep Step 3 in contract/design wording unless the supervisor explicitly
  delegates a live code surface.
- Preserve the separation between terminal printer output, external assembler
  input, structured encoder/object records, and linker object inputs.
- Do not turn the structured record surface into one broad catch-all operand
  payload; keep register reference, use kind, provenance, allocation
  provenance, and clobber/effect metadata separated.
- Do not edit the source idea for routine execution details; record packet
  state here instead.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; cmake --build --preset default > test_after.log 2>&1'
```

Proof result: `ninja: no work to do.`
