# AArch64 PLT/GOT Construction Surface

## Source Role

`plt_got.cpp` was a translated placeholder for the old AArch64 linker PLT/GOT
classification pass. It preserved the structure of the reference Rust module
from:

`ref/claudes-c-compiler/src/backend/arm/linker/plt_got.rs`

The intended entry point was:

```rust
pub(super) fn create_plt_got(
    objects: &[ElfObject],
    globals: &mut HashMap<String, GlobalSymbol>,
) -> (Vec<String>, Vec<(String, bool)>)
```

The pass scanned input object relocations, decided which dynamic symbols needed
PLT stubs, which symbols needed GOT-only entries, which dynamic objects needed
copy relocations, and wrote PLT/GOT indexes back into the merged global-symbol
table.

## Entry Point

### `create_plt_got`

`create_plt_got` returned two ordered lists:

- `plt_names`: dynamic symbols that needed PLT entries
- `got_entries`: GOT slots encoded as `(symbol_name, is_plt_entry)`

The implementation walked every object, every section index, and every
relocation attached to that section. Relocations whose symbol index was outside
the object's symbol table were ignored, as were relocations against empty
symbol names.

For each named relocation target, the pass consulted `globals` and only treated
the symbol as dynamically resolved when the global record had `is_dynamic` set.
The low four bits of `GlobalSymbol::info` were used as the symbol type.

## Relocation Classification

Branch relocations to dynamic non-object symbols required PLT entries:

- `R_AARCH64_CALL26`
- `R_AARCH64_JUMP26`

Branch relocations to dynamic object symbols were classified for copy
relocation handling instead of PLT handling.

Address-materialization relocations to dynamic symbols were split by symbol
type:

- `R_AARCH64_ADR_PREL_PG_HI21`
- `R_AARCH64_ADD_ABS_LO12_NC`
- `R_AARCH64_LDST64_ABS_LO12_NC`
- `R_AARCH64_LDST32_ABS_LO12_NC`
- `R_AARCH64_LDST8_ABS_LO12_NC`
- `R_AARCH64_LDST16_ABS_LO12_NC`
- `R_AARCH64_LDST128_ABS_LO12_NC`

Dynamic object symbols in this group required copy relocations. Dynamic
non-object symbols were treated as function-address references and required
PLT entries.

`R_AARCH64_ABS64` followed the same dynamic-symbol split: object symbols
required copy relocations, while non-object symbols required PLT entries.

Explicit GOT relocations added a GOT-only symbol when that symbol had not
already been classified for PLT:

- `R_AARCH64_ADR_GOT_PAGE`
- `R_AARCH64_LD64_GOT_LO12_NC`

All other relocation kinds were ignored by this classification pass.

## Copy Relocation Handling

The pass collected dynamic object symbols requiring copy relocations and then
mutated their global records:

- `copy_reloc` was set on each directly referenced object symbol.
- `(from_lib, lib_sym_value)` identities were collected for object symbols that
  came from a shared library and had a nonzero library symbol value.
- Other dynamic object globals with the same `(from_lib, lib_sym_value)` were
  marked as copy-reloc aliases.

This alias handling coalesced duplicate global names that referred to the same
library object storage. Later emitters depended on the shared copy-relocation
identity to avoid allocating multiple independent copy locations for aliases.

## GOT Layout Contract

The returned GOT vector always began with three reserved entries:

1. `GOT[0]`: `.dynamic` pointer slot
2. `GOT[1]`: reserved runtime linker slot
3. `GOT[2]`: reserved runtime linker slot

PLT-backed symbols were appended next. For each PLT symbol, the pass assigned:

- `GlobalSymbol::plt_idx` to the symbol's index in `plt_names`
- `GlobalSymbol::got_idx` to the appended GOT slot index

GOT-only symbols were appended after all PLT-backed entries. For each GOT-only
symbol, the pass assigned `GlobalSymbol::got_idx` but left `plt_idx`
unchanged.

The boolean in each returned GOT tuple was `true` for PLT-backed entries and
`false` for reserved or GOT-only entries.

## Ordering And Deduplication

The pass preserved first-seen order from the input relocation scan:

- PLT symbols were appended only once.
- GOT-only symbols were appended only when absent from both the GOT-only list
  and the PLT list at the time of discovery.
- Copy-reloc symbols were appended only once before alias expansion.

The old code used linear `contains` checks instead of a set. That preserved
deterministic scan order but made the pass quadratic in the number of
classified symbols.

## Dependencies

The translated surface depended on:

- `elf` relocation constants such as `R_AARCH64_CALL26`,
  `R_AARCH64_ADR_GOT_PAGE`, and `R_AARCH64_LD64_GOT_LO12_NC`
- `elf::STT_OBJECT`
- `ElfObject` section, symbol, and relocation storage
- `types::GlobalSymbol`
- mutable global symbol fields: `is_dynamic`, `info`, `copy_reloc`,
  `from_lib`, `lib_sym_value`, `plt_idx`, and `got_idx`
- standard map and vector behavior from the reference Rust implementation

The dynamic executable and shared-object emitters consumed the returned
`plt_names` and `got_entries` to lay out PLT stubs, `.got.plt`, dynamic
relocation tables, and dynamic symbol ordering.

## Assumptions

- Relocation arrays were indexed by section index and had an entry for every
  scanned section.
- Dynamic-symbol classification was already present in `globals`.
- The low four bits of `GlobalSymbol::info` matched ELF symbol type encoding.
- Dynamic object references in executable output used copy relocations.
- Function address materialization through ADRP/ADD-style relocations should
  route through PLT entries.
- Reserved GOT slots 0 through 2 were required by the dynamic linker model.
- First-seen relocation order was an acceptable stable order for emitted
  PLT/GOT state.

## Differences From A Complete Linker Pass

This surface did not:

- validate malformed relocation section indexing
- diagnose relocation references to missing globals
- distinguish all AArch64 GOT, TLS, IFUNC, or pointer-auth relocation cases
- handle symbol visibility, preemption, protected symbols, or local-exec TLS
  policy
- model lazy versus eager binding choices
- account for symbol versioning
- separate executable and shared-object PLT/GOT policy
- optimize duplicate lookup behavior with sets while preserving output order

The pass was a classifier and global-symbol annotator. It did not emit bytes,
apply relocations, allocate copy-reloc storage, or write dynamic table entries.

## Rebuild Risks

- PLT/GOT ordering is coupled to dynamic symbol ordering, `.rela.plt`, GOT slot
  addresses, and emitted PLT branch targets. A rebuild must keep these
  consumers in agreement.
- Copy-reloc aliasing depends on shared-library identity, not just symbol name.
  Losing that coalescing can duplicate storage for aliases of the same object.
- Treating function address materialization as PLT-backed affects both direct
  calls and address-taken function pointers; rebuilding this incorrectly can
  produce addresses that bypass dynamic interposition.
- GOT-only discovery excluded names already classified for PLT at discovery
  time. A rebuild should define the desired precedence explicitly if symbols
  are encountered through multiple relocation families.
- The linear deduplication behavior preserved first-seen order. Replacing it
  with hash sets can accidentally make output order nondeterministic unless an
  ordered list remains authoritative.
- Reintroducing this as live code should be tested with dynamic calls, function
  pointer references, dynamic data references, copy relocations, copy-reloc
  aliases, and mixed PLT/GOT references to the same symbol.
