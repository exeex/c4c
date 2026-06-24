Status: Active
Source Idea Path: ideas/open/330_native_object_model_and_emission_api.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Compiler-Facing Construction Helpers

# Current Packet

## Just Finished

Step 3 implementation completed: added compiler-facing construction helpers
for the shared native object model under `c4c::backend::mir::object`.

The helper surface now lets target-like clients:

- create or find sections by stable id or name.
- append initialized bytes, reserve uninitialized bytes, and apply alignment.
- bind labels at explicit offsets or the current section offset.
- publish defined symbols and undefined symbols through deterministic lookup.
- attach numeric symbol and label relocation records with signed addends.

No ELF serialization, target-specific relocation enum, instruction encoding,
`--codegen obj`, parser dependency, or RV64/AArch64 object emission was added.

## Suggested Next

Execute Step 4 as the next bounded shared-model slice: add the minimal
relocatable ELF writer/config records needed to serialize modules already built
through these helpers, without adding target instruction encoding or CLI route
wiring.

## Watchouts

- Do not implement RV64 or AArch64 object emission in child 330.
- Do not add `--codegen obj` CLI behavior in child 330.
- Do not make the compiler object path depend on printed `.s` parsing.
- Keep target-specific relocation names and fixup semantics out of shared model
  helpers; shared relocation records currently store numeric target-chosen type
  values only.
- Step 3 helper semantics currently update an existing named symbol when it is
  later defined after being declared undefined; later writer work should
  preserve stable symbol ids.
- `align_section` pads initialized sections only when `bytes.size()` matches
  `size_bytes`; reserved sections grow by size without gaining bytes.
- `clang-format` is not installed in this environment; C++ formatting was
  adjusted manually.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^backend_object_model_records$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_object_model_records` passed, 1/1 tests green.
