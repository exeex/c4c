# AArch64 Linker Input Loading Surface

## Source Role

`input.cpp` was the staged AArch64 linker input-loading surface for the first
static-link slice. It was translated from the old Rust input module, but the
C++ surface was intentionally much narrower than the reference linker:

`ref/claudes-c-compiler/src/backend/arm/linker/input.rs`

The live entry point was declared in `linker/mod.hpp`:

```cpp
std::optional<std::vector<LoadedInputObject>> load_first_static_input_objects(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);
```

`link.cpp` called this helper before section merging, symbol-address
assignment, relocation application, and final static executable emission.

## Inputs And Outputs

- `object_paths` was the ordered list of file paths provided to the first
  static-link pipeline.
- `error`, when non-null, was cleared on entry and populated on the first
  read, parse, archive-selection, or validation failure.
- The return value was an ordered vector of `LoadedInputObject` records on
  success.
- Failure returned `std::nullopt` and left any partial input state local to the
  helper.

Each `LoadedInputObject` preserved:

- the source path visible to downstream diagnostics
- the parsed `linker_common::Elf64Object`

## Entry Point Behavior

`load_first_static_input_objects` enforced the first static-link slice shape:

1. Reject fewer than two input paths with
   `first static-link slice requires at least two object files`.
2. Read each file into a byte vector.
3. If the bytes described an archive, append selected archive members through
   the first-slice archive path.
4. Otherwise parse the file as an ELF64 relocatable object for `EM_AARCH64`.
5. Return the loaded objects in traversal order.

The helper did not resolve library names, search `-L` paths, parse linker
scripts, load thin archives, load shared libraries, register global symbol
replacement policy, or distinguish static versus dynamic link modes.

## Local Helpers

The old file contained three local helpers.

### `read_file_bytes`

Opened a path with `std::ifstream` in binary mode and copied the stream into a
`std::vector<std::uint8_t>`. Open failure reported:

`failed to open object file: <path>`

The helper did not separately report short reads or stream errors after open.

### `collect_unresolved_symbols`

Built a set of defined symbols from already-loaded objects, then collected
unresolved strong undefined symbols that were not satisfied by that set.

Defined-symbol collection skipped:

- empty symbol names
- undefined symbols
- section symbols
- non-global and non-weak definitions

Unresolved-symbol collection skipped:

- empty symbol names
- non-undefined symbols
- weak undefined symbols
- names already defined by the current loaded-object set
- duplicate unresolved names already reported

The resulting list preserved first-observed order across loaded objects.

### `append_archive_members_for_first_static_slice`

Parsed an archive through `linker_common::parse_elf64_archive` with
`EM_AARCH64`, inspected unresolved symbols from already-loaded objects, and
selected at most one member per unresolved symbol. A selected member was
appended as a `LoadedInputObject` when the archive member carried a parsed
object.

The helper rejected archives that did not increase the loaded-object count to
at least two objects with:

`<archive>: archive did not provide a member for the first static-link slice`

## Archive Selection Model

The archive logic was intentionally first-slice-only:

- it looked only at unresolved strong symbols from objects loaded before the
  archive
- it selected a member by symbol-table lookup for each currently unresolved
  name
- it prevented selecting the same member twice in one archive pass
- it did not rescan the archive after adding new members
- it did not resolve nested or circular archive dependencies
- it did not load archive members merely because they defined weak symbols

This was enough for the staged static-link tests that used a primary object and
one provider member, but it was not a complete Unix archive extraction
algorithm.

## Dependencies

The implementation depended on:

- `linker/mod.hpp` for `LoadedInputObject` and the public declaration
- shared ELF helpers visible through `mod.hpp`
- `elf::is_archive_file`
- `linker_common::parse_elf64_archive`
- `linker_common::parse_elf64_object`
- `elf::EM_AARCH64` and `elf::STT_SECTION`
- `std::ifstream`, stream iterators, `std::unordered_set`, and standard
  containers

Downstream surfaces depended on this helper to provide parsed inputs but owned
the semantic linker work: merged sections, symbol address assignment,
relocations, and final image emission were not performed here.

## Differences From The Reference Rust Input Module

The reference Rust linker input module handled the fuller AArch64 linker front
door:

- regular archives
- thin archives
- shared libraries for dynamic linking
- linker scripts with `GROUP` and `INPUT` directives
- library-name resolution through search paths
- global symbol registration and replacement policy
- static-mode skipping of shared libraries

The C++ first-slice surface deliberately omitted those behaviors. It loaded
explicit ELF64 object paths and a narrow archive case only.

## Assumptions

- The first static-link route receives explicit file paths, not unresolved
  `-l` names.
- Non-archive files are AArch64 ELF64 objects.
- Archive extraction only needs to satisfy symbols already unresolved before
  the archive is visited.
- Weak undefined symbols do not force archive extraction.
- Section symbols are not exported as provider definitions for unresolved
  symbol matching.
- Loading at least two objects is a valid proxy for the staged first static
  slice shape.

## Rebuild Risks

- A complete linker must implement repeated archive rescans or group semantics;
  the old one-pass member selection can miss circular or transitive archive
  dependencies.
- The first-slice validation couples input loading to a two-object test shape.
  A real static linker should separate file loading from policy about minimum
  object count.
- Linker script, thin archive, shared-library, and library-search handling were
  present in the reference Rust surface but absent here. Rebuild work should
  decide whether those belong in a restored input module or in a higher-level
  linker driver.
- Archive member paths used `member.object->source_name`, which may differ
  from the archive path. Diagnostics and duplicate-member handling should keep
  that source identity behavior deliberate.
- Read errors after a successful open were not distinguished from empty files;
  a robust rebuild should report I/O failure separately from ELF parse failure.
- Symbol-visibility and replacement policy were delegated away in this staged
  surface. Reintroducing dynamic or shared-link input loading will need the
  richer global-symbol registration model from the old reference linker.
