# AArch64 First Static Executable Emission Surface

## Source Role

`emit_static.cpp` was the narrow AArch64 linker emission surface for the first
static executable slice. Unlike the larger old Rust static linker path, this C++
translation unit only wrapped already-merged `.text` bytes in a minimal ELF64
`ET_EXEC` image.

The public entry point was declared in `linker/mod.hpp`:

```cpp
std::optional<std::vector<std::uint8_t>> emit_first_static_executable_image(
    const std::vector<std::uint8_t>& merged_text,
    std::uint64_t base_address,
    std::uint64_t entry_address,
    std::uint64_t* text_file_offset = nullptr,
    std::uint64_t* text_virtual_address = nullptr,
    std::string* error = nullptr);
```

`link.cpp` called this helper after loading input objects, merging text,
resolving symbol addresses, applying first-slice text relocations, and choosing
`main` as the entry symbol.

## Inputs And Outputs

- `merged_text` was the complete executable payload. The emitter did not merge
  sections, apply relocations, parse objects, synthesize symbols, or allocate
  data/BSS storage.
- `base_address` selected the executable load base. A zero input was replaced
  with the default `0x400000`.
- `entry_address` had to point inside the emitted text virtual-address range.
- `text_file_offset`, when non-null, received the fixed text file offset.
- `text_virtual_address`, when non-null, received the load address of the first
  text byte.
- `error`, when non-null, was cleared on entry and populated only for validation
  failures.
- The returned byte vector was the complete ELF image on success; failure
  returned `std::nullopt`.

## ELF Image Shape

The emitted file used a deliberately tiny ELF64 layout:

- 64-byte ELF header at file offset `0`
- one 56-byte program header immediately after the ELF header
- merged text bytes at file offset `64 + 56`
- no section header table
- no interpreter, dynamic table, symbol table, string table, relocation table,
  GOT, PLT, TLS, BSS, or writable segment

The output constants were:

| Field | Value |
| --- | ---: |
| ELF class | `ELFCLASS64` |
| Data encoding | little-endian |
| ELF version | `1` |
| `e_type` | `ET_EXEC` (`2`) |
| `e_machine` | `EM_AARCH64` |
| `e_entry` | caller-provided `entry_address` |
| `e_phoff` | `64` |
| `e_shoff` | `0` |
| `e_ehsize` | `64` |
| `e_phentsize` | `56` |
| `e_phnum` | `1` |
| `e_shentsize`, `e_shnum`, `e_shstrndx` | `0` |

The sole program header was a read/execute `PT_LOAD` segment:

- `p_type = PT_LOAD`
- `p_flags = PF_R | PF_X`
- `p_offset = 0`
- `p_vaddr = p_paddr = base_address`
- `p_filesz = p_memsz = text_file_offset + merged_text.size()`
- `p_align = 0x1000`

This means the ELF header and program header were part of the executable load
segment, and `.text` began at `base_address + 120`.

## Byte Writing Helpers

The old file contained local little-endian helper routines:

- `write_u16`
- `write_u32`
- `write_u64`

They wrote directly into the already-sized output vector at fixed offsets. The
helpers assumed all offsets were valid because the vector size was derived
before any header writes.

## Validation Behavior

The emitter rejected two cases:

- empty `merged_text`, with error text
  `first static executable requires merged .text bytes`
- an entry address outside `[text_virtual_address, text_virtual_address +
  merged_text.size())`, with error text
  `entry point for first static executable must live in merged .text`

On success it copied `merged_text` into the image at the fixed text offset,
reported the text offset and virtual address through optional out-parameters,
and returned the image bytes.

## Dependencies

The implementation depended only on:

- `linker/mod.hpp` for the public declaration and standard containers
- `elf::EM_AARCH64` from the AArch64 ELF surface included through `mod.hpp`
- `std::copy` and fixed-width integer types

Its real upstream dependency was `link.cpp`, which established all semantic
linking facts before calling the image writer. Any rebuild should keep that
division explicit: this surface writes a minimal image; it does not own symbol
resolution or relocation semantics.

## Assumptions

- The first static-link slice only needed one executable load segment.
- The input program had no runtime data segment, BSS, TLS, dynamic linking
  metadata, section headers, or startup/runtime requirements outside the merged
  text.
- The caller had already relocated branch and address materialization sites
  against the final text virtual address.
- `main` or any other chosen entry symbol was already expressed as a final
  virtual address.
- Mapping the ELF header into the executable segment was acceptable for this
  staged artifact.

## Rebuild Risks

- The fixed `text_file_offset = 120` is coupled to the one-program-header
  layout. Adding program headers changes symbol-address calculations in the
  caller unless both sides move together.
- The entry-range check enforces text-only entry points. A future startup stub,
  dynamic trampoline, or runtime prelude would need a broader entry contract.
- The single read/execute segment cannot represent writable data, BSS, TLS, or
  RELRO. Reusing this shape for a fuller static linker would silently omit
  required runtime memory.
- There are no section headers, so post-link tools that expect named sections
  cannot inspect the output in the usual way.
- The little-endian header writers are intentionally local and mechanical; a
  rebuild should prefer a central ELF writer once multiple AArch64 linker
  surfaces need shared header/table emission.
