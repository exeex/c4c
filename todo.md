Status: Active
Source Idea Path: ideas/open/330_native_object_model_and_emission_api.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Shared Object Model Records

# Current Packet

## Just Finished

Step 2 implementation completed: added the first target-neutral native object
model records under `c4c::backend::mir::object`, wired the model source into
`c4c_backend`, and added focused `backend_object_model_records` coverage.

The new records can express:

- `.text`, `.data`, and `.bss` sections with bytes or reserved size,
  alignment, and basic section attributes.
- undefined symbols and defined local/global function or section symbols.
- section-local labels by section id and offset.
- numeric relocation records with target-chosen ELF relocation type values,
  symbol targets, optional label bases, and signed addends.

No ELF serialization, target-specific relocation enum, instruction encoding,
`--codegen obj`, parser dependency, or RV64/AArch64 object emission was added.

## Suggested Next

Execute the next Step 2/3 boundary slice: add only the minimal shared object
model helpers needed by the future writer, such as deterministic section and
symbol lookup/append helpers, without adding ELF bytes or target-specific
relocation policy yet.

## Watchouts

- Do not implement RV64 or AArch64 object emission in child 330.
- Do not add `--codegen obj` CLI behavior in child 330.
- Do not make the compiler object path depend on printed `.s` parsing.
- Keep target-specific relocation names and fixup semantics out of shared model
  helpers; shared relocation records currently store numeric target-chosen type
  values only.
- Defer string tables, symbol-table ordering, relocation section emission, ELF
  byte serialization, and target ELF config until the planned writer/API
  slices.
- The first proof initially exposed that backend library sources include from
  `src/backend`, so library-local source includes should use paths such as
  `mir/object/model.hpp`.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^backend_object_model_records$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_object_model_records` passed, 1/1 tests green.
