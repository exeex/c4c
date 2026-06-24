Status: Active
Source Idea Path: ideas/open/329_native_object_emission_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Compare Relevant Reference Design Pieces

# Current Packet

## Just Finished

Completed Step 4 research: compared the relevant reference RV64
assembler/object writer design pieces under
`ref/claudes-c-compiler/src/backend/riscv/assembler/` plus the shared reference
ELF writer records it composes with. Recorded which concepts should map into
c4c-owned structured APIs without copying code, changing implementation files,
tests, CLI/runtime wiring, source ideas, or child idea files.

Inspected commands/files:

- `git status --short`
- `rg --files ref/claudes-c-compiler/src/backend/riscv/assembler`
- `find ref/claudes-c-compiler/src/backend/riscv/assembler -maxdepth 3 -type f | sort`
- Targeted `rg` searches for:
  - reference assembler/object terms:
    `EncodeResult`, `Reloc`, `Relocation`, `pcrel`, `Pcrel`, `PCREL`,
    `relax`, `RVC`, `compressed`, `Section`, `Symbol`, `Elf`, `ELF`,
    `rela`, `fixup`, `Fixup`, `Label`, `Object`, `write`, `emit`
  - shared ELF/object terms:
    `struct ElfWriterBase`, `struct ObjReloc`, `struct ObjSection`,
    `struct ObjSymbol`, `write_elf`, `build_symbol_table`, `add_reloc`,
    `sections`, `symbols`
- Targeted reads:
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/README.md`
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/mod.rs`
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/mod.rs`
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/base.rs`
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/elf_writer.rs`
  - `ref/claudes-c-compiler/src/backend/riscv/assembler/compress.rs`
  - `ref/claudes-c-compiler/src/backend/elf/mod.rs`
  - `ref/claudes-c-compiler/src/backend/elf/writer_base.rs`
  - `ref/claudes-c-compiler/src/backend/elf/object_writer.rs`
  - `ref/claudes-c-compiler/src/backend/elf/symbol_table.rs`

Relevant reference concepts:

- The reference RV64 assembler is still text-first at its outer boundary:
  `assemble_with_args(asm_text, output_path, extra_args)` parses `.s` into
  `AsmStatement`s, then feeds an `ElfWriter`. This is useful reference material
  for a future `c4c-as` lane, but it must not become the compiler's primary
  direct `.o` route.
- The reusable design shape is not the parser entry point; it is the typed
  intermediate contract between encoder and object writer:
  `EncodeResult::{Word, Half, Words, WordWithReloc, WordsWithRelocs, Skip}` and
  `Relocation { reloc_type, symbol, addend }`.
- c4c should map that shape to a target-owned encoded fragment API rather than
  to parser operands. A C++ version should carry encoded bytes or words,
  instruction width, relocation/fixup attachments, optional labels, and
  addends. It should be produced from backend machine records directly.
- `RelocType` is RV64-specific and should stay target-owned, with numeric ELF
  relocation mapping in the target layer: `CallPlt`, `PcrelHi20`,
  `PcrelLo12I`, `PcrelLo12S`, `Hi20`, `Lo12I`, `Lo12S`, `Branch`, `Jal`,
  absolute 32/64, GOT high, TLS high/low/add, and ADD/SUB width pairs.
- Shared object records should not know names like `PcrelHi20`; they should
  carry a target relocation number plus symbol/addend at serialization time.
  Target code owns how machine fixups become those relocation numbers.
- RISC-V `pcrel_hi` / `pcrel_lo` pairing is a target semantic rule. The
  reference creates synthetic `.Lpcrel_hiN` labels at AUIPC/GOT/TLS high sites,
  emits the high relocation against the real symbol, then makes the matching
  low relocation reference the synthetic AUIPC-site label.
- The pairing rule means c4c's structured API needs stable label identities for
  instruction offsets, not just string labels from printed assembly. A useful
  C++ API would let the RV64 object builder create internal labels attached to
  encoded instruction offsets and expose them to the symbol table when
  referenced by relocations.
- The reference treats local branch/JAL fixups and external relocations
  differently. Same-section local branches can be patched after labels are
  bound; cross-section or undefined targets become relocations. c4c child ideas
  should make that policy explicit instead of relying on a textual assembler's
  label pass.

Shared versus RV64-specific split:

- Shared: object module, sections, raw data bytes, alignment, section order,
  section flags/types, symbols, symbol binding/type/visibility/size, undefined
  symbols from relocation references, relocation records, string tables,
  `.symtab`, `.strtab`, `.shstrtab`, `.rela.*` / `.rel.*` selection, ELF class,
  machine id, flags, and final ELF serialization.
- RV64-specific: instruction encoders and register ids, RV64 relocation enum,
  PC-relative high/low pairing, branch/JAL immediate patching, `CALL_PLT`
  expansion semantics, `R_RISCV_RELAX` and `R_RISCV_ALIGN` policy,
  `EF_RISCV_*` flags, RV32/RV64 class choice from target config, and optional
  RVC compression decisions.
- AArch64 should share the same object model and ELF serialization shape, but
  it needs its own relocation enum, ADRP/ADD/branch fixup rules, target flags,
  NOP/alignment behavior, and target encoder fragments. Do not leak RV64
  concepts like `pcrel_hi` labels into the shared API names.

ELF section/symbol/relocation writing:

- The reference shared `ObjSection` has `name`, `sh_type`, `sh_flags`, `data`,
  `sh_addralign`, `relocs`, and optional COMDAT group. That is a good c4c-owned
  object-section seed.
- `ObjReloc` has `offset`, numeric `reloc_type`, `symbol_name`, and `addend`.
  c4c should keep this target-neutral at the shared layer and let each target
  convert typed fixups into numeric ELF relocation ids before serialization.
- `ObjSymbol` has `name`, `value`, `size`, `binding`, `sym_type`,
  `visibility`, and `section_name`, including special undefined/common section
  names. c4c should preserve symbol facts from the compiler instead of
  recovering `.globl`, `.weak`, `.type`, and `.size` directives from text.
- The reference object writer serializes from structured sections and symbols:
  string tables, section symbols, locals before globals, relocation sections,
  symbol table, and section headers. That is the right dependency direction for
  c4c: `ObjectModule -> ELF writer -> bytes`, not `assembly text -> parser ->
  ELF writer`.
- Referenced local symbols are normally omitted, but RV64 must include
  referenced synthetic local labels for `pcrel_lo` relocations. This should be
  a target option on symbol-table construction, not a global default.

RVC and linker relaxation decisions:

- The reference contains a post-encoding RVC compressor that rewrites eligible
  32-bit words to 16-bit halfwords and remaps relocation, branch, label, and
  symbol offsets.
- That compression pass is disabled in the active reference flow because linker
  relaxation uses `R_RISCV_RELAX` hints and assembler-side compression can
  change layout in ways the linker does not expect.
- For c4c's first direct object children, the conservative mapping is to emit
  valid uncompressed RV64 instructions plus `R_RISCV_RELAX` / `R_RISCV_ALIGN`
  where required, and leave actual relaxation to the linker. RVC compression
  should be a later explicit target policy, not implicit in the first object
  writer.
- `.option norelax` / relax stack behavior is text-assembler policy. The direct
  compiler route should instead derive relaxation eligibility from target
  codegen decisions and compile options, while a future textual assembler lane
  can support `.option` directives.

Mapping to c4c-owned APIs:

- Child idea 1 should introduce a shared structured object model:
  `ObjectModule`, `ObjectSection`, `ObjectSymbol`, `ObjectRelocation`,
  `SectionId`/`SymbolId` or stable names, target ELF config, and byte append /
  alignment helpers. It should be independent of RV64 parsing.
- A target fragment API should represent encoded machine output as
  bytes/words/halfwords plus attached typed fixups. RV64 can start with
  `Rv64RelocKind` and later AArch64 can add `Aarch64RelocKind`; both lower to
  shared numeric `ObjectRelocation`s.
- Label binding should be structured: internal labels and function/data symbols
  should be attached to section offsets before serialization. That avoids
  using printed label names as the compiler's source of truth.
- Direct compiler `.o` emission should preserve existing backend facts:
  functions, data, sections, symbol visibility, type/size, label targets,
  relocation kind, addends, and target options. Text parsing would discard and
  then reconstruct those facts through whitespace-sensitive directives.
- A future textual assembler can still use parser records and the same target
  encoders/object writer, but it should be layered beside the compiler route:
  `c4c-as .s -> parser -> target fragments -> object model -> ELF`, while the
  compiler route remains `backend machine model -> target fragments -> object
  model -> ELF`.

## Suggested Next

Execute Step 5 by creating focused child ideas under `ideas/open/` for the
shared native object model/API, RV64 minimal relocatable object emission,
AArch64 minimal relocatable object emission, CLI/test integration for
`--codegen obj`, broader object-route scan readiness, and textual assembler
follow-up if still needed.

## Watchouts

- Do not copy the reference Rust code or import its text-first entry point as a
  compiler dependency. Use it only as design guidance for c4c-owned C++ records
  and tests.
- The shared API should avoid RV64 names. Keep target relocation enums and
  fixup resolution target-owned, while sections/symbols/relocations/ELF
  serialization stay shared.
- RV64 `pcrel_lo` relocation correctness depends on referencing the AUIPC-site
  synthetic label, not the final target symbol. This requirement should be
  explicit in the RV64 child idea.
- First object-route children should not require RVC compression. Prefer
  uncompressed correct code with linker relax hints; make compression a later
  target policy after the object model is stable.
- Parser records are appropriate for a future `c4c-as` path, but direct
  compiler `.o` emission must be fed from backend machine state, not from
  printed `.s` round-tripping.

## Proof

Research-only packet; no build or test validation required and no
`test_after.log` was written. Proof is the inspected file/command list and
reference design mapping recorded above in `todo.md`.
