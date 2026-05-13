# AArch64 Linker ELF Surface

## Source Role

`elf.cpp` was a translated placeholder for the old AArch64 linker ELF module.
It did not contain live C++ implementation code. Instead, it preserved the
shape of the reference Rust module from:

`ref/claudes-c-compiler/src/backend/arm/linker/elf.rs`

The intended module role was to provide an AArch64-specific ELF64 facade over a
shared linker-common parser:

- re-export generic ELF constants, helpers, and dynamic-table tags used by the
  linker family
- define AArch64 relocation type constants
- alias shared ELF64 structures under the names expected by the AArch64 linker
- expose `parse_object(data, source_name)` as the AArch64 object parser entry
  point, delegating parsing to shared ELF64 logic with `EM_AARCH64`

## Shared ELF Re-Exports

The old source expected `use super::elf::*` consumers to find generic ELF64
definitions through this module. The re-exported shared surface covered:

- file identification and target constants:
  `ELF_MAGIC`, `ELFCLASS64`, `ELFDATA2LSB`, `ET_EXEC`, `ET_DYN`,
  `EM_AARCH64`
- section types and flags:
  `SHT_NOBITS`, `SHF_WRITE`, `SHF_ALLOC`, `SHF_EXECINSTR`, `SHF_TLS`
- symbol bindings, types, and special section indexes:
  `STB_GLOBAL`, `STB_WEAK`, `STT_OBJECT`, `STT_FUNC`, `STT_SECTION`,
  `STT_TLS`, `STT_GNU_IFUNC`, `SHN_UNDEF`, `SHN_ABS`, `SHN_COMMON`
- program header types and flags:
  `PT_LOAD`, `PT_TLS`, `PT_GNU_STACK`, `PT_GNU_EH_FRAME`, `PT_INTERP`,
  `PT_DYNAMIC`, `PT_PHDR`, `PF_X`, `PF_W`, `PF_R`
- endian readers and output writers:
  `read_u16`, `read_u32`, `w16`, `w32`, `w64`, `write_bytes`, `wphdr`
- archive and linker-script helpers:
  `is_thin_archive`, `parse_linker_script_entries`, `LinkerScriptEntry`
- standard linker-symbol helpers:
  `LinkerSymbolAddresses`, `get_standard_linker_symbols`
- dynamic table constants:
  `DT_NEEDED`, `DT_SONAME`, `DT_STRTAB`, `DT_SYMTAB`, `DT_STRSZ`,
  `DT_SYMENT`, `DT_DEBUG`, `DT_PLTGOT`, `DT_PLTRELSZ`, `DT_PLTREL`,
  `DT_JMPREL`, `DT_RELA`, `DT_RELASZ`, `DT_RELAENT`, `DT_GNU_HASH`,
  `DT_INIT_ARRAY`, `DT_INIT_ARRAYSZ`, `DT_FINI_ARRAY`, `DT_FINI_ARRAYSZ`,
  `DT_NULL`, `DT_RELACOUNT`, `DT_FLAGS`, `DF_BIND_NOW`, `DT_FLAGS_1`,
  `DF_1_NOW`

## AArch64 Relocation Constants

The AArch64-specific relocation table preserved by the old file was:

| Name | Value | Intended calculation |
| --- | ---: | --- |
| `R_AARCH64_NONE` | 0 | no relocation |
| `R_AARCH64_ABS64` | 257 | `S + A` |
| `R_AARCH64_ABS32` | 258 | `S + A`, truncated to 32 bits |
| `R_AARCH64_ABS16` | 259 | `S + A`, truncated to 16 bits |
| `R_AARCH64_PREL64` | 260 | `S + A - P` |
| `R_AARCH64_PREL32` | 261 | `S + A - P` |
| `R_AARCH64_PREL16` | 262 | `S + A - P` |
| `R_AARCH64_MOVW_UABS_G0` | 263 | unsigned MOVW absolute group 0 |
| `R_AARCH64_MOVW_UABS_G0_NC` | 264 | unsigned MOVW absolute group 0, no check |
| `R_AARCH64_MOVW_UABS_G1_NC` | 265 | unsigned MOVW absolute group 1, no check |
| `R_AARCH64_MOVW_UABS_G2_NC` | 266 | unsigned MOVW absolute group 2, no check |
| `R_AARCH64_MOVW_UABS_G3` | 267 | unsigned MOVW absolute group 3 |
| `R_AARCH64_ADR_PREL_LO21` | 274 | `S + A - P` |
| `R_AARCH64_ADR_PREL_PG_HI21` | 275 | `Page(S + A) - Page(P)` |
| `R_AARCH64_ADD_ABS_LO12_NC` | 277 | `(S + A) & 0xFFF` |
| `R_AARCH64_LDST8_ABS_LO12_NC` | 278 | load/store low 12 bits |
| `R_AARCH64_TSTBR14` | 279 | test-and-branch immediate |
| `R_AARCH64_CONDBR19` | 280 | conditional branch immediate |
| `R_AARCH64_JUMP26` | 282 | `S + A - P`, 26-bit `B` |
| `R_AARCH64_CALL26` | 283 | `S + A - P`, 26-bit `BL` |
| `R_AARCH64_LDST16_ABS_LO12_NC` | 284 | load/store low 12 bits |
| `R_AARCH64_LDST32_ABS_LO12_NC` | 285 | load/store low 12 bits |
| `R_AARCH64_LDST64_ABS_LO12_NC` | 286 | load/store low 12 bits |
| `R_AARCH64_LDST128_ABS_LO12_NC` | 299 | load/store low 12 bits |
| `R_AARCH64_ADR_GOT_PAGE` | 311 | GOT page address |
| `R_AARCH64_LD64_GOT_LO12_NC` | 312 | GOT load low 12 bits |

## Type Aliases

The old module expected AArch64 linker code to use shared ELF64 structures via
local aliases:

- `SectionHeader = linker_common::Elf64Section`
- `Symbol = linker_common::Elf64Symbol`
- `Rela = linker_common::Elf64Rela`
- `ElfObject = linker_common::Elf64Object`

These aliases were part of the compatibility surface for neighboring linker
files such as relocation and link orchestration code.

## Parser Entry Point

The sole function was the AArch64 object parser:

```rust
pub fn parse_object(data: &[u8], source_name: &str) -> Result<ElfObject, String> {
    linker_common::parse_elf64_object(data, source_name, EM_AARCH64)
}
```

The important behavior is the `EM_AARCH64` machine check. Actual ELF64 parsing
belongs to the shared `linker_common` layer rather than this target-specific
facade.

## Dependencies And Assumptions

- `backend::elf` owns generic ELF constants, dynamic-table tags, endian
  readers, output writers, archive detection, linker-script parsing, and
  standard linker-symbol helpers.
- `backend::linker_common` owns the `Elf64Section`, `Elf64Symbol`,
  `Elf64Rela`, and `Elf64Object` structures plus `parse_elf64_object`.
- Neighboring AArch64 linker modules were expected to import this facade
  rather than reaching into shared ELF modules directly.
- This extraction preserves the intended API shape only; it does not imply the
  current live C++ backend has an implemented AArch64 ELF parser.

## Rebuild Risks

- Rebuild work should avoid duplicating generic ELF64 parsing in the AArch64
  backend. Keep the target-specific module as a facade over shared parsing.
- Relocation constants must stay numerically exact. Several neighboring linker
  passes can silently produce invalid output if these values drift.
- Consumers that use `super::elf::*` may depend on broad re-export behavior;
  narrowing the facade too early can create hidden migration failures.
- `parse_object` must reject non-AArch64 objects through the machine type check
  rather than accepting any ELF64 input.
