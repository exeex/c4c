# AArch64 First Static Link Orchestration Surface

## Source Role

`link.cpp` was the staged AArch64 linker orchestration surface for the first
static-link slice. It was translated from the old Rust linker path:

`ref/claudes-c-compiler/src/backend/arm/linker/link.rs`

The C++ file did not implement a complete AArch64 linker. It coordinated the
narrow first static executable route by loading input objects, summarizing the
link slice for tests, merging `.text`, assigning symbol addresses, applying
first-slice relocations, and emitting a minimal static executable image.

The public entry points were declared in `linker/mod.hpp`:

```cpp
std::optional<FirstStaticLinkSlice> inspect_first_static_link_slice(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

std::optional<FirstStaticExecutable> link_first_static_executable(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);
```

## Entry Points

### `inspect_first_static_link_slice`

This entry point built a descriptive summary of the staged link inputs without
emitting an executable. It was primarily a test-facing inspection path.

The function:

1. Cleared `error` when provided.
2. Rejected fewer than two input paths with
   `first static-link slice requires at least two object files`.
3. Loaded objects through `load_first_static_input_objects`.
4. Collected per-object defined and unresolved strong undefined symbols.
5. Built a symbol-provider map from global and weak definitions.
6. Recorded allocated input section names as merged output-section candidates.
7. Recorded relocation summaries and marked them resolved when their symbol was
   provided by another loaded object or when an unnamed local non-undefined
   symbol resolved within the same object.
8. Reported unresolved strong undefined symbols that had no provider.

The returned `FirstStaticLinkSlice` used the fixed case name
`aarch64-static-call26-two-object`, sorted and deduplicated symbol lists, and
sorted merged output-section names.

### `link_first_static_executable`

This entry point produced the first minimal static executable image. It was the
live semantic caller for the staged first-static helpers.

The function:

1. Cleared `error` when provided.
2. Loaded objects through `load_first_static_input_objects`.
3. Required every loaded object to contain a `.text` section.
4. Concatenated each input `.text` section into `merged_text` in loaded-object
   order.
5. Recorded each object's text offset within the merged text stream.
6. Collected global and weak defined symbol addresses as
   `symbol.value + object_text_offset`.
7. Collected unresolved strong undefined symbols and removed any that were
   satisfied by the final symbol-address map.
8. Rejected the link if any strong undefined symbol remained.
9. Converted symbol offsets to virtual addresses by adding
   `base_address + text_file_offset`.
10. Applied first static text relocations.
11. Required a global `main` symbol as the executable entry point.
12. Emitted the final image with `emit_first_static_executable_image`.

The returned `FirstStaticExecutable` preserved the emitted image, base address,
entry address, text file offset, text virtual address, text size, and final
symbol-address map.

## Local Helpers

The old file contained two local helpers.

### `sorted_symbols`

Sorted a string vector, removed duplicates with `std::unique`, and returned the
normalized vector. The helper was used for inspection output stability, not for
linker semantics.

### `find_text_section_index`

Scanned an `Elf64Object` section table for the first section named `.text` and
returned its index. Missing `.text` caused `link_first_static_executable` to
fail with:

`<path>: missing .text section`

The helper did not recognize alternate executable section names, split text
sections, COMDAT text, or section groups.

## Symbol Model

The staged linker treated symbols narrowly:

- empty names were ignored
- section symbols were ignored for exported definitions
- global and weak definitions were providers
- weak undefined symbols did not block linking
- strong undefined symbols had to resolve to a provider by name
- local unnamed relocation targets could resolve within their own object

The executable path used a single `unordered_map<string, uint64_t>` for final
symbol addresses. Duplicate definitions were not diagnosed; the first inserted
provider/address won because the old code used `emplace`.

## Section And Address Layout

`link_first_static_executable` only merged `.text` bytes:

- the base address was fixed at `0x400000`
- the text file offset was assumed to be `64 + 56`
- the first text byte therefore lived at virtual address `0x400000 + 120`
- each symbol's object-relative value was adjusted by its object's text offset
  and then by the final text virtual address
- no data, BSS, TLS, rodata, unwind, note, relocation, symbol, or section-header
  output was allocated by this layer

The fixed offset matched the minimal image layout in `emit_static.cpp`. Any
change to the emitted ELF header/program-header shape would have required a
matching change here.

## Relocation Flow

The orchestration layer delegated relocation semantics to
`apply_first_static_text_relocations` after finalizing symbol virtual
addresses. It passed:

- the loaded input objects
- final symbol addresses
- per-object text offsets
- the merged text virtual address
- the mutable merged text byte vector
- the optional error sink

This kept relocation encoding and range checks in the relocation surface while
keeping input ordering, symbol-address assignment, and final image emission in
`link.cpp`.

## Dependencies

The implementation depended on:

- `linker/mod.hpp` for public declarations and shared linker types
- `load_first_static_input_objects`
- `apply_first_static_text_relocations`
- `emit_first_static_executable_image`
- `linker_common::Elf64Object`
- `elf::SHF_ALLOC`, `elf::SHN_UNDEF`, and `elf::STT_SECTION`
- standard containers, `std::optional`, `std::sort`, `std::unique`,
  `std::unordered_map`, and `std::unordered_set`

Downstream tests and callers used this surface as the first-static linker
facade. Lower-level linker files supplied input parsing, relocation patching,
and ELF image writing.

## Error Behavior

The old file reported these failures directly:

- fewer than two input paths:
  `first static-link slice requires at least two object files`
- relocation symbol index outside the object's symbol table during inspection:
  `<source>: relocation symbol index out of bounds`
- missing `.text` during executable linking:
  `<path>: missing .text section`
- unresolved strong symbols during executable linking:
  `first static-link slice still has unresolved symbols: <symbol>`
- missing entry point:
  `first static-link slice requires a global main symbol`

Input parsing, archive selection, relocation application, and image emission
errors were delegated to their respective helper surfaces.

## Differences From A Complete Linker

This was not a general static linker driver. It omitted:

- command-line or linker-script driven input expansion
- library search paths and `-l` resolution
- repeated archive rescans and group semantics
- duplicate definition diagnostics
- weak/strong replacement policy beyond first-provider insertion
- section layout beyond concatenated `.text`
- data, BSS, TLS, rodata, unwind, note, GOT, PLT, dynamic, and relocation
  output allocation
- startup-file or runtime entry selection
- section header, symbol table, and debug-info preservation
- executable segment permission planning beyond the minimal static image writer

The reference Rust linker had broader input and output machinery. This C++
surface only preserved enough behavior for the staged first static AArch64
slice.

## Assumptions

- The first-static route receives explicit object/archive paths.
- At least two loaded objects are required for the supported test slice.
- Every executable input object has a single relevant `.text` section.
- Concatenating `.text` in loaded-object order is sufficient for the slice.
- Global `main` is the executable entry symbol.
- Relocations only need the final symbol-address map, per-object text offsets,
  and merged text virtual address.
- The fixed minimal static image text offset is known before image emission.
- Weak undefined symbols can remain unresolved without failing the first slice.

## Rebuild Risks

- The `.text`-only layout can silently drop required runtime state when the
  input program needs data, BSS, constants, TLS, unwind data, or metadata.
- Duplicate symbol handling used first-provider wins. A rebuild should define
  explicit strong/weak/common replacement and diagnostic rules.
- The inspection path and executable path shared input loading but not all
  validation. Rebuild work should keep test-facing summaries from drifting away
  from actual link semantics.
- The fixed `64 + 56` text offset was coupled to the minimal emitter. Any richer
  ELF image layout must derive addresses from a shared layout plan instead of
  duplicating constants.
- Requiring `main` as the entry point is insufficient for real static binaries
  that need a startup symbol, runtime prelude, or user-selected entry.
- Missing `.text` was fatal even if the object only provided data or symbols
  needed by another object. A complete linker should separate object admission
  from executable-section allocation.
- Archive and input semantics lived below this file but directly affected
  unresolved-symbol behavior. Restoring broader linker support requires keeping
  input loading, symbol resolution, relocation, and output layout in one
  consistent plan.
