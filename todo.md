Status: Active
Source Idea Path: ideas/open/329_native_object_emission_umbrella.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inspect Existing RV64 Assembler/Object Port

# Current Packet

## Just Finished

Completed Step 3 research: inspected the existing partial C++ RV64
assembler/object writer port under `src/backend/mir/riscv/assembler/` and
recorded reusable pieces, missing pieces, and integration risks for future
direct compiler `.o` emission without changing implementation files, tests,
CLI/runtime wiring, source ideas, or child idea files.

Inspected commands/files:

- `git status --short`
- `rg --files src/backend/mir/riscv/assembler`
- `find src/backend/mir/riscv/assembler -maxdepth 3 -type f | sort`
- Targeted `rg` searches for:
  - assembler records and public API terms:
    `assemble`, `Object`, `ELF`, `Reloc`, `Section`, `Symbol`,
    `Instruction`, `Encode`, `Fixup`, `Label`, `parse`, `write`, `emit`
  - integration terms:
    `assemble_target_lir_module`, `object_emitted`, `staged_text`,
    `write_elf_object`, `write_minimal_elf_object`
  - current RV64 text-emission terms:
    `append_simple_prepared_bir_function_asm`, `emit_prepared_module_text`,
    `.text`, `.globl`, `addi`, `ret`, `jal`
- Targeted reads:
  - `src/backend/mir/riscv/assembler/README.md`
  - `src/backend/mir/riscv/assembler/mod.hpp`
  - `src/backend/mir/riscv/assembler/mod.cpp`
  - `src/backend/mir/riscv/assembler/parser.hpp`
  - `src/backend/mir/riscv/assembler/parser.cpp`
  - `src/backend/mir/riscv/assembler/elf_writer.cpp`
  - `src/backend/mir/riscv/assembler/encoder/mod.cpp`
  - `src/backend/mir/riscv/assembler/encoder/base.cpp`
  - `src/backend/mir/riscv/assembler/encoder/compressed.cpp` summary hits
  - `src/backend/mir/riscv/assembler/encoder/atomics.cpp`,
    `float.cpp`, `pseudo.cpp`, `system.cpp`, `vector.cpp` summary hits
  - `src/backend/mir/riscv/codegen/emit.hpp`
  - `src/backend/mir/riscv/codegen/emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
  - `src/backend/backend.hpp`
  - `src/backend/backend.cpp`

Current public API shape:

- `src/backend/mir/riscv/assembler/mod.hpp` exposes a text-first
  `AssembleRequest { asm_text, output_path }` and
  `AssembleResult { staged_text, output_path, object_emitted, error }`, plus a
  compatibility overload returning only staged text. It does not expose a
  machine-record/object-model API.
- `src/backend/mir/riscv/assembler/mod.cpp` always preserves `asm_text` in
  `staged_text`; if `output_path` is empty it does no object work. With an
  output path, it parses text with `parse_asm(...)`, filters empty statements,
  then calls private `write_elf_object(...)`.
- The only surfaced object-success signal is the boolean `object_emitted`.
  Failure text explicitly says current RV64 built-in object emission supports
  only the minimal prepared-LIR return-add handoff and a bounded single-object
  `jal` helper relocation handoff.
- `src/backend/mir/riscv/codegen/emit.hpp` has
  `riscv::assemble_module(...)`, but `emit.cpp` delegates it back through the
  generic `backend::assemble_target_lir_module(...)`. Today that generic
  front door returns `object_emitted = false` for non-x86 targets with
  `backend bootstrap mode does not assemble objects yet`; it does not route
  RV64 compiler output into `riscv::assembler::assemble(...)`.

Encoded instruction and relocation records:

- Parser-facing records in `parser.hpp` are real and useful:
  `AsmStatement` distinguishes labels, directives, instructions, and empty
  lines; `Operand` captures registers, immediates, symbols, symbol offsets,
  memory operands, symbolic memory operands, labels, fence args, CSRs, and
  rounding modes; `Directive` captures section switches, symbol attributes,
  data directives, alignment, `.comm`, `.set`, `.option`-style directives,
  `.insn`, `.incbin`, and unknown directives.
- Encoder records in `encoder/base.cpp` are target-useful but currently
  private to one translation unit: `RelocType`, `Relocation`, `EncodeWord`,
  `EncodeHalf`, `EncodeWords`, `EncodeWordWithReloc`,
  `EncodeWordsWithRelocs`, and `EncodeSkip`. They encode the right shape for
  direct object emission: bytes/words plus typed relocations instead of text.
- The encoder type surface is not currently reusable from compiler code:
  `encoder/mod.cpp` is an empty namespace with comments listing the intended
  Rust-mirrored public surface, while `base.cpp` defines its records locally.
  There is no shared header for `EncodeResult`, relocation enums, register
  ids, format encoders, or `encode_instruction(...)`.
- `RelocType` already lists useful RV64 relocation concepts:
  `CallPlt`, `PcrelHi20`, `PcrelLo12I/S`, `Hi20`, `Lo12I/S`, `Branch`, `Jal`,
  `Abs64`, `Abs32`, `GotHi20`, TLS high/low/add forms, and add/sub width
  pairs. The active writer only materializes `R_RISCV_JAL`.

Section, symbol, and ELF handling:

- `elf_writer.cpp` contains local ELF64 serialization helpers and constants for
  RV64 relocatable objects: ELF header fields, `.text`, `.rela.text`,
  `.symtab`, `.strtab`, `.shstrtab`, section headers, symbol info, and
  little-endian integer append helpers.
- The active writer recognizes only two hard-coded text shapes:
  - `.text`, `.globl main`, `main:`, `addi a0, zero, <imm12>`, `ret`
  - `.text`, `.globl main`, `.globl helper`, `main:`, `jal helper`, `ret`,
    `helper:`, `ret`
- For the return-add shape, it emits `.text`, a local section symbol, and one
  global `STT_FUNC` symbol. For the `jal` helper shape, it emits `.text`,
  `.rela.text`, two global function symbols, and one `R_RISCV_JAL` relocation
  at offset 0 against the helper symbol.
- `ElfWriter` has placeholders for future branch relocation, deferred
  expressions, numeric labels, option stack/no-relax, alignment-with-reloc,
  and numeric-label resolution, but those paths are stubs or only helper logic
  today.
- There is no general section map, object-section abstraction, symbol table
  builder, directive executor, data-section writer, relocation accumulator, or
  finalized ELF writer base in the active C++ port. The README documents those
  as the intended design, not the current implementation.

Reusable pieces:

- Directly reusable now:
  - ELF64 little-endian serialization helpers and minimal section/symbol table
    layout patterns in `elf_writer.cpp`.
  - The parser's `AsmStatement`, `Operand`, `Directive`, `SectionInfo`,
    `DataValue`, and symbol attribute records, for a future textual assembler
    lane or compatibility path.
  - RV64 instruction bitfield helpers such as `encode_i(...)` and
    `encode_j(...)`, once promoted behind a real header/API instead of copied
    from local anonymous helpers.
  - The private encoder result/relocation vocabulary as a design seed for the
    structured object model.
- Reusable after redesign:
  - `RelocType`/`Relocation` should become a target-owned public record type
    that can be produced by machine instruction selection and consumed by a
    shared object writer.
  - The minimal `.text`/`.rela.text` writer can seed object-contract tests, but
    needs a generic `ObjectModule`/`ObjectSection`/`ObjectSymbol`/`ObjectReloc`
    layer before it can handle normal compiler output.
  - Parser records are useful for `c4c-as` or an external textual assembler
    follow-up, but should not be the primary compiler `.o` dependency.

Missing pieces:

- No compiler-facing direct object API that accepts prepared machine
  instructions, encoded bytes, labels, symbols, sections, and typed
  relocations.
- No public encoder header or dispatch that accepts current RV64 backend
  instruction records. Existing prepared RV64 emitters still append assembly
  text strings.
- No general label binding, local branch fixup, `pcrel_hi` / `pcrel_lo`
  pairing, GOT/TLS relocations, relax relocations, data relocations, `.bss` /
  `.data` / `.rodata`, `.size` / `.type` propagation, weak/local visibility,
  `.comm`, or section stack support in the active writer.
- No connection from `assemble_target_lir_module(...)` / CLI object mode to the
  RV64 assembler port. The current public backend assemble contract is still
  bootstrap staging with `object_emitted = false`.
- No direct reuse path for the linker-side ELF parser/linker structures from
  this writer; the writer builds raw byte vectors locally instead of sharing an
  object model.

Integration risks:

- The README and top-level RV64 docs overstate the current C++ assembler and
  linker completeness. Child ideas should treat source code behavior as
  authoritative and use docs only as design intent/reference.
- Depending on parsing compiler-emitted `.s` would couple direct `.o` emission
  to whitespace, directives, pseudo-instruction spelling, peephole output, and
  backend printer quirks. It would make object support fail whenever the asm
  printer changes, even if the machine semantics are valid.
- Text parsing would also duplicate semantic information the compiler already
  has before printing: symbol binding, section selection, label identity,
  relocation kind, addends, branch target identity, function sizes, and data
  layout. Direct object output should preserve those facts as structured
  records and then serialize them.
- Using `write_elf_object(parse_asm(text), path)` as the primary compiler
  route would extend the existing testcase-shaped minimal-slice pattern instead
  of creating the semantic object-emission capability the umbrella is meant to
  decompose.
- A future textual assembler can consume the parser and encoder records, but it
  should sit beside the compiler route: `backend machine model -> object writer
  -> .o` and `backend machine model -> asm writer -> .s`, with `c4c-as .s ->
  object writer -> .o` as a separate compatibility lane.

## Suggested Next

Execute Step 4 by comparing only the relevant reference design pieces under
`ref/claudes-c-compiler/src/backend/riscv/assembler/`: typed `EncodeResult`
records, target relocation enums, RISC-V `pcrel_hi` / `pcrel_lo` pairing, ELF
section/symbol/relocation writing, RVC/linker-relaxation decisions, and which
concepts should map to c4c-owned structured APIs instead of text parsing.

## Watchouts

- Treat `src/backend/mir/riscv/assembler/README.md` as aspirational; the active
  C++ code only proves two minimal object slices.
- The private encoder record vocabulary is promising, but it is not currently
  an API. Child ideas should include header/API ownership before relying on it.
- Do not route compiler `.o` output through `parse_asm(...)` as the primary
  design. Parser/assembler work belongs to a separate textual assembler lane.
- The RV64 prepared emitters currently produce text. A direct object child
  should introduce structured instruction/label/relocation publication from
  the backend machine model rather than adding more named text-shape recognizers
  to `elf_writer.cpp`.
- Existing `assemble_target_lir_module(...)` behavior is a compatibility
  bootstrap contract; changing the public result semantics needs explicit
  route/API tests, not an incidental writer-only patch.

## Proof

Research-only packet; no build or test validation required and no
`test_after.log` was written. Proof is the inspected file/command list and
assembler/object-port findings recorded above in `todo.md`.
