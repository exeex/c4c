# AArch64 Shared Library Linker Emission Surface

## Source Role

`emit_shared.cpp` was a translated placeholder for the old AArch64 shared
library emission path. It preserved the structure of the reference Rust module
from:

`ref/claudes-c-compiler/src/backend/arm/linker/emit_shared.rs`

The intended entry point was:

```rust
pub(super) fn emit_shared_library(
    objects: &[ElfObject],
    globals: &mut HashMap<String, GlobalSymbol>,
    output_sections: &mut [OutputSection],
    section_map: &HashMap<(usize, usize), (usize, u64)>,
    needed_sonames: &[String],
    output_path: &str,
    soname: Option<String>,
) -> Result<(), String>
```

The module emitted an ELF64 shared object for AArch64 with `ET_DYN`, PIC
relocation handling, exported dynamic symbols, PLT/GOT state for external
calls and data references, `.dynamic`, `.gnu.hash`, `.rela.dyn`,
`.rela.plt`, optional init/fini arrays, optional TLS program headers, and
Unix executable file permissions.

## Inputs And Dependencies

- `objects` supplied parsed ELF objects, section data, symbols, and relocation
  records.
- `globals` supplied merged symbol state and was mutated with final addresses,
  PLT indexes, linker-defined symbols, and GNU-style start/stop symbols.
- `output_sections` supplied merged output section metadata and was mutated
  with final virtual addresses, file offsets, flags, and merged data buffers.
- `section_map` mapped each input object section to an output section index and
  offset within the merged section.
- `needed_sonames` populated `DT_NEEDED` entries and the dynamic string table.
- `soname` optionally populated `DT_SONAME`.
- The implementation depended on the old AArch64 `elf`, `types`, and `reloc`
  surfaces plus shared `linker_common` helpers such as `DynStrTab`,
  `OutputSection`, `gnu_hash`, and `resolve_start_stop_symbols`.

## Dynamic Symbol Selection

The emitter built `dyn_sym_names` from several sources:

- defined non-dynamic global symbols with non-local binding and real section
  definitions, sorted before insertion
- existing dynamic globals
- PLT symbols collected from `R_AARCH64_CALL26` and `R_AARCH64_JUMP26`
  relocations that targeted dynamic, undefined, or external function symbols
- undefined or dynamic data symbols referenced through ADRP/ADD/LDST
  relocation pairs that needed GOT indirection inside a shared library

Before writing `.dynsym`, the list was reordered so undefined/import symbols
came first and defined/exported symbols came after them. The `.gnu.hash`
`symoffset` pointed at the first defined symbol, so only export candidates were
hashed. This was intentional: imported symbols must remain before the hash
range so runtime lookup does not incorrectly treat them as definitions in the
shared object.

Each dynamic symbol name was added to `DynStrTab`. The `.dynsym` table used one
null symbol followed by 24-byte ELF64 symbol entries. Defined exports retained
their symbol info, final value, size, and a nonzero section index. Undefined or
dynamic entries preserved original binding when possible and used `STT_FUNC`
for PLT-backed names.

## GNU Hash

The `.gnu.hash` table used:

- `symoffset = 1 + undefined_symbol_count`
- one bloom word
- bloom shift `6`
- a bucket count derived from the number of defined symbols, using the next
  power of two with a minimum of one
- bucket-sorted defined symbols so each chain group matched its bucket
- chain values with the low bit set on the last symbol in each bucket

The hash table, dynamic symbol ordering, and `.dynsym` indexes were coupled.
Any rebuild should keep those transformations in one unit.

## Layout Algorithm

The shared object used `base_addr = 0`, then laid out data after the ELF header
and program headers:

- `.gnu.hash`
- `.dynsym`
- `.dynstr`
- executable sections and optional `.plt` in an executable load segment
- read-only alloc sections in a separate read-only load segment when any
  non-writable, non-executable alloc data remained
- `.init_array` and `.fini_array` at the front of the writable segment
- reserved `.rela.dyn` storage sized from ABS64 relocations, init/fini array
  entries, and GOT dynamic relocation candidates
- `.dynamic`
- optional `.got.plt`
- optional `.rela.plt`
- regular GOT entries for GOT relocations and shared-library data references
- remaining writable non-TLS sections
- TLS file-backed sections, TLS nobits sections, and non-TLS BSS
- `.shstrtab` and section headers for the emitted dynamic sections

Alignment was page-size for load segments, 16 bytes for PLT, 8 bytes for ELF
tables and pointer-oriented areas, and section-specific alignment for merged
output sections.

Before layout, read-only sections with `R_AARCH64_ABS64` relocations were
promoted to writable unless they were executable. Those absolute pointer slots
became runtime-written `R_AARCH64_RELATIVE` dynamic relocations.

## Program Headers And ELF Header

The old emitter wrote an ELF64 header with:

- `ET_DYN`
- `EM_AARCH64`
- entry address `0`
- program headers at offset 64
- section headers for dynamic metadata sections

Program headers were:

- `PT_PHDR`
- read-only `PT_LOAD` for ELF metadata and dynamic symbol/string/hash tables
- executable `PT_LOAD` for text and PLT
- optional read-only `PT_LOAD` for rodata
- writable `PT_LOAD` for relocation tables, dynamic data, GOT, writable
  sections, TLS, and BSS
- `PT_DYNAMIC`
- `PT_GNU_STACK`
- optional `PT_TLS`

The writable segment file size was captured before appending `.shstrtab` and
section headers, so section-header data did not become part of the loadable
runtime writable segment.

## PLT, GOT, And Dynamic Tags

PLT names were collected from call and jump relocations that needed dynamic
resolution. Each such global received a `plt_idx` matching its PLT entry.

The AArch64 PLT model used:

- a 32-byte PLT header that saved `x16`/`x30`, loaded resolver state through
  GOT.PLT, and branched through `x17`
- 16-byte per-symbol entries using `ADRP`, `LDR`, `ADD`, and `BR x17`
- `.got.plt[0] = _DYNAMIC`
- `.got.plt[1]` and `.got.plt[2]` reserved for the dynamic linker
- `.got.plt[3..]` initialized to PLT0 because eager binding was requested

Regular GOT entries were allocated for explicit GOT relocations and for
undefined or dynamic symbols referenced by ADRP/ADD/LDST materialization that
must be indirect in a shared object. Locally defined GOT entries were
initialized to final symbol values and also received `R_AARCH64_RELATIVE`
relocations. Undefined or dynamic GOT entries received `R_AARCH64_GLOB_DAT`.

The `.dynamic` section emitted `DT_NEEDED`, optional `DT_SONAME`, table
locations and sizes for `.dynstr`, `.dynsym`, `.rela.dyn`, `.gnu.hash`,
`DT_RELACOUNT`, optional PLT relocation tags, optional init/fini array tags,
`DT_FLAGS = DF_BIND_NOW`, `DT_FLAGS_1 = DF_1_NOW`, and a terminating
`DT_NULL`.

## Relocation Behavior

Dynamic relocation values preserved by this surface were:

| Relocation | Value | Use |
| --- | ---: | --- |
| `R_AARCH64_GLOB_DAT` | 1025 | dynamic or undefined regular GOT entries |
| `R_AARCH64_JUMP_SLOT` | 1026 | PLT GOT entries |
| `R_AARCH64_RELATIVE` | 1027 | locally resolved absolute pointer/GOT slots |

Input relocation application covered:

- absolute and PC-relative integer relocations: `ABS64`, `ABS32`, `PREL64`,
  `PREL32`, and `PREL16`
- page and low-12 materialization: `ADR_PREL_PG_HI21`,
  `ADD_ABS_LO12_NC`, and `LDST*_ABS_LO12_NC`
- branch relocations: `CALL26` and `JUMP26`
- GOT page/load relocations: `ADR_GOT_PAGE` and `LD64_GOT_LO12_NC`
- MOVW unsigned absolute groups
- `R_AARCH64_NONE`

For undefined or dynamic shared-library data references, the relocation pass
redirected ADRP to the GOT page and converted the paired ADD into an LDR from
the GOT slot. Explicit GOT relocations also targeted the GOT slot when one was
available.

Calls and jumps to PLT-backed names were redirected to the corresponding PLT
entry. Weak undefined calls with no target were patched to an AArch64 NOP.
Unsupported relocation types produced warnings rather than hard failures.

`R_AARCH64_ABS64` wrote the resolved value and emitted a
`R_AARCH64_RELATIVE` relocation when the resolved symbol address was nonzero.
The final `.rela.dyn` table was sorted with all RELATIVE entries first so
`DT_RELACOUNT` could describe the prefix.

## Symbol Resolution And Linker Symbols

After layout, `globals` was updated with final addresses:

- common symbols were placed relative to `.bss`
- normal defined symbols used `section_map`, output section address, and input
  offset
- absolute and undefined symbols were left out of section-relative adjustment

The emitter then provided standard linker symbols through
`LinkerSymbolAddresses`, including base, GOT, dynamic, BSS, text end, data
start, init/fini array ranges, and empty preinit/IPLT ranges. It also resolved
GNU-style `__start_<section>` and `__stop_<section>` symbols for unresolved
non-dynamic references.

## Section Headers

Unlike the old dynamic executable surface, this shared-object emitter wrote a
small section header table. It described:

- null section
- `.dynsym`
- `.dynstr`
- `.gnu.hash`
- `.dynamic`
- `.rela.dyn`
- `.shstrtab`

The section header string table was appended after loadable content, followed
by seven ELF64 section headers.

## Assumptions

- The old file was a descriptive translation surface, not live C++ backend
  implementation.
- Output section merging and global symbol collection had already completed
  before this emitter ran.
- Shared libraries were PIC-oriented and used `base_addr = 0`.
- Undefined and dynamic references in ADRP/ADD/LDST sequences needed GOT
  indirection because their final addresses were not known until load time.
- The dynamic linker was expected to perform eager binding because the emitter
  always wrote `DF_BIND_NOW` and `DF_1_NOW`.
- TLS placement required coordinated file-backed and nobits accounting before
  `PT_TLS` could be written.

## Rebuild Risks

- Dynamic symbol ordering, undefined-symbol partitioning, `.gnu.hash`
  `symoffset`, and dynsym indexes must stay synchronized.
- PLT, GOT.PLT, `.rela.plt`, and `DT_JMPREL` addresses are tightly coupled.
- Promoting read-only ABS64 relocation targets to writable affects segment
  permissions and runtime relocation correctness.
- GOT indirection for undefined/dynamic ADRP/ADD pairs changes instruction
  shape by converting ADD to LDR; rebuild work needs tests that inspect both
  code generation and runtime resolution.
- `DT_RELACOUNT` depends on sorting RELATIVE relocations to the front of
  `.rela.dyn`.
- TLS layout crosses section placement, `PT_TLS`, symbol values, and
  relocation behavior. Treat it as one coordinated feature during
  reconstruction.
- Reintroducing this surface as live code should come with focused tests for
  exported symbols, imports, PLT calls, GOT data references, ABS64 dynamic
  relocations, init/fini arrays, SONAME and NEEDED tags, TLS, weak undefined
  calls, and section header metadata.
