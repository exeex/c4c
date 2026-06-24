Status: Active
Source Idea Path: ideas/open/330_native_object_model_and_emission_api.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Minimal Relocatable ELF Serialization

# Current Packet

## Just Finished

Step 4 implementation completed: added minimal shared relocatable ELF
serialization for modules built through the native object model.

The shared writer now emits ELF64 little-endian relocatable objects from an
explicit target config, including:

- ELF header target machine and flags.
- initialized and NOBITS section headers from object model sections.
- `.rela.*` relocation sections grouped by target section.
- `.symtab` with local symbols before globals and stable model symbol ids.
- `.strtab` and `.shstrtab` string tables.

Focused coverage now validates ELF structure, section names and types, symbol
ordering, undefined extern symbols, relocation links, numeric relocation types,
and signed addends.

No instruction encoding, target-specific relocation enum, parser dependency,
`--codegen obj`, target object emission, runtime/CLI wiring, or c-testsuite
route work was added.

## Suggested Next

Execute the next bounded child-330 slice by adding any missing shared writer
contract polish the supervisor wants before RV64 target children consume the
object model, or move lifecycle to the first target-specific object emission
child if this shared API surface is sufficient.

## Watchouts

- Do not implement RV64 or AArch64 object emission in child 330.
- Do not add `--codegen obj` CLI behavior in child 330.
- Do not make the compiler object path depend on printed `.s` parsing.
- The shared writer intentionally supports only ELF64 little-endian today;
  unsupported config variants return `std::nullopt`.
- Shared relocation records still store numeric target-chosen ELF relocation
  values only. Target-specific relocation names and fixup semantics remain for
  later RV64/AArch64 children.
- Label-based relocation addends are serialized as the recorded signed addend
  plus the bound label offset.
- The writer does not synthesize target section symbols; it serializes symbols
  explicitly present in the object model.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^backend_object_model_records$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_object_model_records` passed, 1/1 tests green.
