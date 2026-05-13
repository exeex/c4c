# AArch64 Dynamic Linker Emission Surface

## Source Role

`emit_dynamic.cpp` was a translated placeholder for the old AArch64 dynamic
executable emission path. It preserved the structure of the reference Rust
module from:

`ref/claudes-c-compiler/src/backend/arm/linker/emit_dynamic.rs`

The intended entry point was:

```rust
pub(super) fn emit_dynamic_executable(
    objects: &[ElfObject],
    globals: &mut HashMap<String, GlobalSymbol>,
    output_sections: &mut [OutputSection],
    section_map: &HashMap<(usize, usize), (usize, u64)>,
    plt_names: &[String],
    got_entries: &[(String, bool)],
    needed_sonames: &[String],
    output_path: &str,
    export_dynamic: bool,
) -> Result<(), String>
```

The module emitted a dynamically linked ELF64 executable for AArch64 when
shared-library symbols were present. The output included `.interp`,
`.gnu.hash`, `.dynsym`, `.dynstr`, `.rela.dyn`, `.rela.plt`, PLT/GOT state,
`.dynamic`, copy relocations, TLS program headers when needed, and executable
file permissions on Unix.

## Inputs And Dependencies

- `objects` supplied parsed ELF objects, section data, symbols, and relocation
  records.
- `globals` supplied merged global symbol state and was updated with final
  addresses, linker-defined symbols, copy-reloc storage, PLT indexes, and GOT
  indexes.
- `output_sections` supplied merged output section metadata and was mutated
  with file offsets, virtual addresses, and merged data buffers.
- `section_map` mapped each input object section to an output section index and
  offset within that output section.
- `plt_names` listed symbols requiring PLT entries.
- `got_entries` listed GOT-only and PLT-related GOT slots.
- `needed_sonames` populated `DT_NEEDED` and the dynamic string table.
- `export_dynamic` controlled whether non-dynamic global definitions were
  exported through `.dynsym`.
- The implementation depended on the old AArch64 `elf`, `types`, and `reloc`
  surfaces plus shared `linker_common` helpers such as `DynStrTab`,
  `OutputSection`, `gnu_hash`, and `resolve_start_stop_symbols`.

## Dynamic Symbol And String Tables

The emitter built `dyn_sym_names` in this order:

1. PLT symbols from `plt_names`.
2. dynamic non-copy-reloc GOT symbols from `got_entries`.
3. symbols requiring copy relocations.
4. optionally exported non-dynamic globals when `export_dynamic` was enabled.

Every dynamic symbol name was added to `DynStrTab`. The computed sizes were:

- `.dynsym`: one null symbol plus one 24-byte entry for each dynamic symbol
- `.dynstr`: byte length of the dynamic string table
- `.rela.plt`: one 24-byte relocation for each PLT symbol
- `.rela.dyn`: one 24-byte relocation for each dynamic GOT relocation plus
  each copy relocation

The `.gnu.hash` table used one bloom word, a bucket count derived from the
hashed symbol count, sorted hashed symbols by bucket, and set the low bit on
the last chain entry for each bucket.

## Layout Algorithm

The output layout began after the ELF header and program headers, then placed:

- `.interp`
- `.gnu.hash`
- `.dynsym`
- `.dynstr`
- `.rela.dyn`
- `.rela.plt`
- executable sections and `.plt` in the text load segment
- read-only alloc sections in a separate read-only load segment
- `.init_array`, `.fini_array`, `.dynamic`, `.got`, `.got.plt`,
  `.data.rel.ro`, remaining writable data, TLS file-backed sections, TLS
  nobits sections, and BSS/copy-reloc storage in the writable load segment

Alignment was mostly 8-byte for ELF tables, page-size for load segments,
16-byte for PLT, and section-specific alignment for merged output sections.
TLS state tracked `tls_addr`, file offset, file size, memory size, and maximum
alignment. Copy relocations allocated BSS storage and reused storage for
duplicate library symbol identities.

## Program Headers And ELF Header

The old emitter wrote an ELF64 executable header with `ET_EXEC`,
`EM_AARCH64`, entry address from `_start` when present, and no section header
table. Program headers were:

- `PT_PHDR`
- `PT_INTERP`
- read-only `PT_LOAD` for ELF metadata and dynamic relocation tables
- executable `PT_LOAD` for text and PLT
- read-only `PT_LOAD` for rodata, including an empty placeholder when needed
- writable `PT_LOAD` for dynamic data, GOT, writable sections, TLS, and BSS
- `PT_DYNAMIC`
- `PT_GNU_STACK`
- optional `PT_TLS`

## PLT, GOT, And Dynamic Tags

The PLT model used an AArch64 32-byte PLT header followed by 16-byte entries:

- PLT header saved `x16`/`x30`, loaded GOT resolver state through `ADRP`,
  `LDR`, `ADD`, and branched through `x17`.
- Each symbol PLT entry loaded its GOT slot and branched through `x17`.
- `.got.plt[0]` held the `.dynamic` address, `.got.plt[1]` and `[2]` were
  zero, and symbol entries initially pointed at PLT0.
- GOT-only entries held local defined addresses, TLS offsets, or copy-reloc
  addresses.

The `.dynamic` section emitted `DT_NEEDED` entries followed by fixed tags for
string/symbol tables, PLTGOT, PLT relocations, regular RELA relocations,
GNU hash, `DF_BIND_NOW`, and `DF_1_NOW`. Init/fini array tags were emitted
only when those sections existed, and the table ended with `DT_NULL`.

## Relocation Behavior

Dynamic relocations emitted by the module were:

| Relocation | Value | Use |
| --- | ---: | --- |
| `R_AARCH64_COPY` | 1024 | copy relocations into allocated BSS storage |
| `R_AARCH64_GLOB_DAT` | 1025 | dynamic non-PLT GOT entries |
| `R_AARCH64_JUMP_SLOT` | 1026 | PLT GOT entries |

Input relocation application covered:

- absolute and PC-relative integer relocations: `ABS64`, `ABS32`, `ABS16`,
  `PREL64`, `PREL32`, `PREL16`
- page and low-12 address materialization: `ADR_PREL_PG_HI21`,
  `ADR_PREL_LO21`, `ADD_ABS_LO12_NC`, and `LDST*_ABS_LO12_NC`
- branch relocations: `CALL26`, `JUMP26`, `CONDBR19`, `TSTBR14`
- MOVW unsigned absolute groups
- GOT page/load relocations through a dynamic `GotInfo`
- all remaining supported relocations through the shared AArch64 relocation
  handler, including TLS cases

Calls and jumps to PLT-backed symbols resolved to the corresponding PLT entry.
Weak undefined call targets were patched to NOP when their resolved address was
zero.

## Symbol Resolution

`resolve_sym_dynamic` resolved:

- section symbols through `section_map` and output-section addresses
- already defined globals through their final `GlobalSymbol::value`
- dynamic symbols through PLT entries when available
- copy-reloc symbols through their allocated copy address
- weak undefined symbols to zero
- absolute symbols directly from their stored value
- ordinary defined input symbols through output-section address plus mapped
  offset plus symbol value

The emitter also defined standard linker-provided symbols from
`LinkerSymbolAddresses` and GNU-style `__start_<section>` /
`__stop_<section>` symbols when unresolved non-dynamic references existed.

## Assumptions

- The old file was a descriptive translation surface, not live C++ backend
  implementation.
- The dynamic path assumed a fixed `BASE_ADDR`, `PAGE_SIZE`, and interpreter
  string from the AArch64 linker `types` surface.
- Output section merging had already produced coherent `OutputSection` inputs
  and `section_map` entries before this emission path ran.
- The layout model deliberately omitted section headers in the final
  executable.
- The dynamic linker path used eager binding through `DF_BIND_NOW` and
  `DF_1_NOW`.

## Rebuild Risks

- The dynamic symbol ordering and GNU hash `symoffset` are coupled; changing
  one without the other can make dynamic lookup fail at runtime.
- PLT/GOT layout must stay consistent with `R_AARCH64_JUMP_SLOT` and
  `DT_PLTGOT` addresses.
- Copy relocations mutate global symbol values and allocate BSS storage; rebuild
  work must preserve duplicate-library-symbol coalescing behavior.
- TLS layout crosses section placement, `PT_TLS`, GOT entries, and relocation
  handling. Treat it as one coordinated behavior during reconstruction.
- The relocation pass depends on accurate final section addresses and file
  offsets. Applying relocations before all layout mutation is complete can write
  correct values to the wrong file positions.
- Reintroducing this surface as live code should happen with focused linker
  tests for dynamic symbols, PLT calls, GOT data references, copy relocations,
  init/fini arrays, TLS, and weak undefined symbols.
