# AArch64 Static Text Relocation Surface

## Source Role

`reloc.cpp` was the staged AArch64 linker relocation application surface for
the first static-link slice. It was not a complete AArch64 ELF relocation
engine. Its live entry point was:

```cpp
bool apply_first_static_text_relocations(
    const std::vector<LoadedInputObject>& loaded_objects,
    const std::unordered_map<std::string, std::uint64_t>& symbol_addresses,
    const std::unordered_map<std::string, std::uint64_t>& text_offsets,
    std::uint64_t text_virtual_address,
    std::vector<std::uint8_t>* merged_text,
    std::string* error = nullptr);
```

The function walked already-loaded input objects, found `.text` relocations,
resolved each relocation by symbol name through the final static symbol-address
map, and patched the merged text buffer in place.

## Entry Point

### `apply_first_static_text_relocations`

The entry point expected:

- `loaded_objects`: input objects with parsed sections, symbols, and
  relocation arrays
- `symbol_addresses`: final virtual addresses for named symbols
- `text_offsets`: per-object offsets into the merged `.text` buffer
- `text_virtual_address`: virtual address of the start of merged `.text`
- `merged_text`: mutable output text image
- `error`: optional diagnostic sink

It cleared the diagnostic string when present, rejected a missing `merged_text`
buffer, skipped loaded objects with no entry in `text_offsets`, and scanned
only sections named `.text`.

For each relocation in a `.text` section, the function:

1. rejected relocation types outside the first-slice allowlist
2. checked that the relocation symbol index was in range
3. looked up the symbol by name in `symbol_addresses`
4. computed the patch offset as object text offset plus relocation offset
5. rejected patches past the merged `.text` buffer
6. delegated instruction or data patching to `apply_relocation`

Failures returned `false` and, when possible, recorded an object-path-qualified
diagnostic.

## Relocation Families

The old surface carried AArch64 ELF relocation constants locally instead of
depending on a shared generated table. The allowlist in the entry point
accepted this first-slice set:

- `R_AARCH64_TSTBR14` (`279`)
- `R_AARCH64_CONDBR19`
- `R_AARCH64_JUMP26`
- `R_AARCH64_CALL26`
- `R_AARCH64_ADR_PREL_PG_HI21`
- `R_AARCH64_ADR_PREL_LO21`
- `R_AARCH64_ADD_ABS_LO12_NC`
- `R_AARCH64_LDST8_ABS_LO12_NC`
- `R_AARCH64_LDST16_ABS_LO12_NC`
- `R_AARCH64_LDST32_ABS_LO12_NC`
- `R_AARCH64_LDST64_ABS_LO12_NC`
- `R_AARCH64_LDST128_ABS_LO12_NC`

The implementation also had patch handlers for absolute and PC-relative data
relocations:

- `R_AARCH64_NONE`
- `R_AARCH64_ABS64`
- `R_AARCH64_ABS32`
- `R_AARCH64_ABS16`
- `R_AARCH64_PREL64`
- `R_AARCH64_PREL32`
- `R_AARCH64_PREL16`

Those data relocations were implemented in the helper but were not part of the
first static `.text` entry-point allowlist except for `NONE` not being reached
through that allowlist.

`kBoundedFirstSliceCallReloc` was defined as `279`, the same numeric value as
`R_AARCH64_TSTBR14`. The name was a historical first-slice marker and should
not be treated as an independent ABI relocation kind in a rebuild.

## Patch Helpers

The helper layer was intentionally small and byte-vector oriented:

- `has_room_for` checked patch bounds against the mutable byte buffer.
- `read_u32`, `write_u32`, and `write_u64` encoded little-endian values.
- `in_range` checked signed immediate ranges for branch and ADR-style
  encodings.
- `encode_adrp` wrote split page immediates into ADRP instructions.
- `encode_adr` wrote split byte immediates into ADR instructions.
- `encode_add_imm12` wrote the low 12-bit immediate field for ADD.
- `encode_ldst_imm12` wrote scaled low-12 load/store immediates.

All instruction encoders preserved opcode and register fields with masks, then
inserted only the relocation immediate bits.

## Address Model

`apply_relocation` computed:

- `target = target_address + addend`
- `place = text_virtual_address + patch_offset`
- `delta = target - place`

Branch and PC-relative relocations used `delta`. Absolute and low-12
relocations used `target`. ADRP used the page delta between the target page and
the PC page, while ADR used the byte delta directly.

The entry point passed `text_virtual_address + object_text_offset` as the
helper's text base while also passing a merged-buffer patch offset. That meant
the helper's `place` calculation effectively included both the object text
offset in the base and the merged patch offset. A rebuild should re-check this
relationship before copying the old arithmetic; the intended model is
relocation place equals merged text virtual address plus merged patch offset.

## Encoding Rules

Implemented relocation behavior:

- `R_AARCH64_NONE` returned success without patching.
- `R_AARCH64_ABS64`, `ABS32`, and `ABS16` wrote the absolute target value.
- `R_AARCH64_PREL64`, `PREL32`, and `PREL16` wrote `target - place`.
- `R_AARCH64_CALL26` and `JUMP26` required 4-byte alignment, checked a signed
  26-bit word offset, and patched the low 26 instruction bits.
- `R_AARCH64_CONDBR19` required 4-byte alignment, checked a signed 19-bit word
  offset, and patched bits 5 through 23.
- `R_AARCH64_TSTBR14` required 4-byte alignment, checked a signed 14-bit word
  offset, and patched bits 5 through 18.
- `R_AARCH64_ADR_PREL_PG_HI21` patched ADRP from page-relative signed 21-bit
  displacement.
- `R_AARCH64_ADR_PREL_LO21` patched ADR from byte-relative signed 21-bit
  displacement.
- `R_AARCH64_ADD_ABS_LO12_NC` inserted `target & 0xfff`.
- `R_AARCH64_LDST*_ABS_LO12_NC` inserted `target & 0xfff` after scaling by
  the access size: 0, 1, 2, 3, or 4 bits for 8-, 16-, 32-, 64-, or 128-bit
  accesses.

The `_NC` low-12 relocations did not check carry or page pairing. They simply
patched the low immediate field.

## Diagnostics

The old code produced diagnostics for:

- missing merged text buffer
- unsupported relocation type in the first static-link slice
- relocation symbol index out of bounds
- unresolved relocation symbol
- relocation offset past merged `.text`
- relocation offset out of bounds for some helper paths
- unaligned call or jump branch targets
- out-of-range call, jump, conditional branch, test branch, ADRP, or ADR
  relocations

Some helper paths returned `false` without setting `error`, especially generic
bounds failures for 4- or 8-byte data writes and unsupported relocation types.
A rebuild should normalize those diagnostics.

## Dependencies

This surface depended on:

- `linker::LoadedInputObject`
- `linker_common::Elf64Object` section, symbol, and relocation storage through
  `LoadedInputObject::object`
- `LoadedInputObject::path` for diagnostics and `text_offsets` lookup
- section names, especially the literal `.text`
- symbol names as the sole key into final static address resolution
- relocation fields: `rela_type`, `sym_idx`, `offset`, and `addend`
- little-endian byte encoding in the merged output text vector

The main upstream caller was the first static linker orchestration documented
in `link.md`. That caller was responsible for loading objects, merging `.text`,
constructing `symbol_addresses`, constructing `text_offsets`, and selecting
the final `text_virtual_address`.

## Assumptions

- Relocation vectors were indexed by section index and existed for each
  section scanned.
- Only `.text` relocations were relevant to the first static-link slice.
- Symbol-address resolution by final symbol name was already complete.
- The merged output text buffer was already sized and laid out.
- Branch relocation places and targets were 4-byte aligned.
- Low-12 relocation users had matching ADRP or address-materialization
  sequences elsewhere in the instruction stream.
- The old static slice did not need dynamic, TLS, GOT, PLT, copy-relocation,
  IFUNC, pointer-auth, or section-symbol relocation policy.

## Differences From A Complete Relocation Engine

This surface did not:

- apply relocations outside `.text`
- support section-symbol, local-symbol, or anonymous relocation targets
- preserve symbol binding or visibility semantics during resolution
- handle GOT, PLT, dynamic, TLS, TLSDESC, IFUNC, or pointer-auth relocation
  forms
- generate veneers or thunks for out-of-range branches
- validate paired ADRP/ADD or ADRP/LDR relocation sequences
- check overflow for absolute 16- and 32-bit writes
- distinguish signed versus unsigned overflow for data relocations
- emit relocation records for dynamic linking
- normalize diagnostics across all failure paths

It was an in-place patcher for a narrow static executable image, not a full
linker relocation subsystem.

## Rebuild Risks

- The old `place` calculation mixed object-local and merged-text offsets in a
  way that should be audited before reuse.
- The historical `kBoundedFirstSliceCallReloc` alias for relocation `279` can
  hide the fact that `279` is `R_AARCH64_TSTBR14`, not a call relocation.
- Helper support and entry-point allowlisting differed; rebuilding from only
  one list can accidentally drop or expose relocation kinds.
- `_NC` low-12 relocations need pairing with page relocations. Treating them as
  independent absolute patches can pass small cases while breaking larger
  address layouts.
- A production AArch64 linker needs branch-range extension strategy rather
  than hard failure for every out-of-range branch.
- Symbol-name-only resolution is insufficient when local, section, weak,
  hidden, or duplicate symbol names are present.
