# AArch64 Linker Types Surface

Extracted from the old translated mirror
`src/backend/mir/aarch64/linker/types.cpp`, which was a commented structural
copy of `ref/claudes-c-compiler/src/backend/arm/linker/types.rs`.

This artifact records the intended linker type surface for future AArch64
reconstruction. It is descriptive only; the removed `.cpp` did not provide live
C++ behavior.

## Purpose

The old surface defined the shared data model used by the AArch64 linker
phases:

- architecture linker constants
- the `GlobalSymbol` record used for static, common, undefined, and dynamic
  symbols
- construction hooks expected by the generic linker-common symbol interface
- AArch64-specific replacement policy for resolving dynamic symbols with later
  local definitions

## Constants

- `INTERP`: dynamic loader path bytes for AArch64:
  `/lib/ld-linux-aarch64.so.1\0`.
- `BASE_ADDR`: executable virtual base address, `0x400000`.
- `PAGE_SIZE`: linker alignment page size, `0x10000`. The old note says the
  AArch64 linker aligned to 64 KiB pages.

These constants were consumed by the old dynamic and static emitter surfaces
when laying out executable images, program headers, interpreter strings, and
page-aligned regions.

## Dependencies

The translated Rust mirror depended on:

- `backend::linker_common`
- `linker_common::GlobalSymbolOps`
- `linker_common::Elf64Symbol`
- `linker_common::DynSymbol`
- `super::elf::SHN_UNDEF`
- `super::elf::SHN_COMMON`

The future C++ rebuild should keep this boundary explicit: symbol lifecycle
state belongs in the linker type layer, while ELF constants and parsed symbol
records should come from the ELF/common layers.

## `GlobalSymbol`

The old `GlobalSymbol` carried these fields:

- `value`: resolved symbol value, object symbol value, or assigned BSS offset.
- `size`: symbol size from static or dynamic symbol metadata.
- `info`: ELF symbol info byte.
- `defined_in`: optional object index for symbols resolved from input objects.
- `section_idx`: ELF section index, including special undefined/common/BSS
  sentinel values.
- `from_lib`: optional SONAME for a symbol resolved from a shared library.
- `plt_idx`: optional PLT entry index for dynamic function symbols.
- `got_idx`: optional GOT entry index for dynamic symbols that require GOT
  storage.
- `is_dynamic`: whether the symbol came from a shared library instead of a
  static object.
- `copy_reloc`: whether a dynamic data symbol needs a copy relocation in the
  executable.
- `lib_sym_value`: original value from the source shared library, used by the
  PLT/GOT copy-relocation alias logic.

## `GlobalSymbolOps` Surface

The old implementation exposed simple accessors:

- `is_defined()`: true when `defined_in` is present.
- `is_dynamic()`: returns `is_dynamic`.
- `info()`: returns `info`.
- `section_idx()`: returns `section_idx`.
- `value()`: returns `value`.
- `size()`: returns `size`.

It also exposed construction and mutation helpers used by linker symbol
resolution.

### `new_defined(obj_idx, sym)`

Constructed a symbol defined by an input object:

- copied `value`, `size`, `info`, and `shndx` from `Elf64Symbol`
- set `defined_in` to the object index
- cleared `from_lib`, `plt_idx`, and `got_idx`
- set `is_dynamic` and `copy_reloc` false
- set `lib_sym_value` to zero

### `new_common(obj_idx, sym)`

Constructed an ELF common symbol:

- copied `value`, `size`, and `info`
- set `defined_in` to the object index
- forced `section_idx` to `SHN_COMMON`
- cleared dynamic-library, PLT, GOT, copy-relocation, and library-value state

### `new_undefined(sym)`

Constructed an unresolved symbol placeholder:

- set `value` and `size` to zero
- copied `info`
- left `defined_in` empty
- forced `section_idx` to `SHN_UNDEF`
- cleared dynamic-library, PLT, GOT, copy-relocation, and library-value state

### `set_common_bss(bss_offset)`

Converted a common symbol into an assigned BSS symbol:

- wrote `value = bss_offset`
- wrote `section_idx = 0xffff`

The `0xffff` section index was a private sentinel in the old surface. A future
implementation should replace or document this sentinel before relying on it
across linker phases.

### `new_dynamic(dsym, soname)`

Constructed a symbol resolved from a shared library:

- set `value` to zero in the output image
- copied `size` and `info` from `DynSymbol`
- left `defined_in` empty
- set `from_lib` to the resolving SONAME
- cleared `plt_idx` and `got_idx`
- forced `section_idx` to `SHN_UNDEF`
- set `is_dynamic` true
- set `copy_reloc` false
- stored `dsym.value` in `lib_sym_value`

This state was later refined by the PLT/GOT and dynamic relocation passes.

## Replacement Policy

The old surface exposed `arm_should_replace_extra(existing)`, which returned
true when `existing.is_dynamic` was true.

The intent was that a later local/static definition may replace a symbol that
was provisionally resolved from a shared library. Future reconstruction should
decide whether this policy belongs in the AArch64-specific type layer, generic
linker-common symbol resolution, or an explicit symbol precedence routine.

## Cross-Surface Users

This type layer is referenced conceptually by the extracted linker artifacts:

- `linker/input.md` for object/archive symbol discovery
- `linker/link.md` for first static symbol resolution and text layout
- `linker/plt_got.md` for PLT/GOT indexes, copy relocation marking, and
  `lib_sym_value` alias checks
- `linker/reloc.md` for relocation target lookup through resolved
  `GlobalSymbol` state
- `linker/emit_dynamic.md` and `linker/emit_shared.md` for interpreter,
  dynamic symbol, relocation, PLT/GOT, and dynamic section emission
- `linker/emit_static.md` for base-address and first static executable layout

## Assumptions And Risks

- The old `.cpp` was not executable C++; it only preserved a Rust source mirror
  in comments.
- `GlobalSymbol` mixes static object resolution, dynamic-library resolution,
  PLT/GOT allocation, copy-relocation state, and BSS assignment. A rebuilt
  linker may want a stricter staged representation, but it must preserve the
  observable data each downstream phase expected.
- `section_idx = 0xffff` for assigned common/BSS symbols is an implicit
  sentinel and should not be reintroduced without a named constant or a better
  section model.
- `value` has phase-dependent meaning: source object value, output virtual
  address offset, assigned BSS offset, or zero for unresolved dynamic symbols.
  Rebuild code should avoid comparing values without knowing which phase wrote
  them.
- `lib_sym_value` matters for dynamic symbol alias/copy-relocation behavior;
  dropping it would lose information captured from shared libraries.
- The replacement policy for dynamic symbols is linker semantics, not just a
  data helper. It should be tested with local-definition-over-shared-symbol
  cases when the dynamic linker path is rebuilt.
