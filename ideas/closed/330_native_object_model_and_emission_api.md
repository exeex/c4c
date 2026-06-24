# Native Object Model And Emission API

## Goal

Introduce the shared native object-emission model and compiler-facing API that
RV64 and AArch64 can both use to produce relocatable ELF objects without
round-tripping through printed assembly text.

## Why This Exists

The native object route needs a structured contract between backend machine
emission and ELF serialization. That contract should preserve sections,
symbols, labels, alignment, raw bytes, typed target relocations, and target ELF
configuration while keeping RV64-specific and AArch64-specific fixup rules out
of the shared layer.

This child exists before target object emission so later RV64 and AArch64
children can build on one object model instead of growing separate ad hoc ELF
writers.

## In Scope

- Add shared object model records such as an object module, sections, symbols,
  relocations, stable section or symbol identifiers, and target ELF config.
- Add byte append, alignment, label binding, symbol publication, and relocation
  attachment helpers suitable for compiler-produced machine output.
- Define the boundary between target-owned typed fixups and target-neutral ELF
  relocation records.
- Add minimal ELF serialization support needed by the first RV64 and AArch64
  object children, including string tables, symbol tables, section headers, and
  relocation sections.
- Add focused unit tests for model construction and ELF writer invariants.

## Out Of Scope

- Encoding RV64 or AArch64 instructions.
- Implementing `--codegen obj` CLI behavior.
- Changing c-testsuite defaults or backend route selection.
- Parsing `.s` text or building a GNU-compatible assembler.
- Replacing existing asm writers or weakening asm-route coverage.

## Acceptance And Proof Expectations

- A shared object model can describe at least `.text`, `.data`, `.bss`,
  undefined symbols, local/global function symbols, section-local labels, and
  relocations with addends.
- The shared ELF writer emits structurally valid relocatable ELF for a minimal
  target configuration, proved with object-model unit tests and readelf-style
  structural checks where available.
- Target relocation semantics remain target-owned; the shared layer stores
  numeric relocation records only after target lowering chooses the target ELF
  relocation number.
- Existing asm-route tests continue to pass.

## Dependency Notes

- This is the first implementation child from
  `ideas/open/329_native_object_emission_umbrella.md`.
- RV64 and AArch64 object-emission children should depend on this API rather
  than adding private parallel object models.
- A later textual assembler child may reuse the same object model, but the API
  must be usable directly from backend machine records.

## Reviewer Reject Signals

- The compiler-facing object API requires printed `.s` text or parser records
  as the source of truth.
- Shared records include RV64-specific names such as `pcrel_hi`,
  `PcrelLo12I`, or RVC policy in target-neutral types.
- Relocation handling is implemented as scattered strings, unchecked magic
  numbers, or backend-local ELF byte patches instead of typed target fixups
  lowered into shared relocation records.
- The slice claims object-model progress while only renaming existing asm
  helpers or rewriting tests.
- Existing supported asm-route tests are removed, weakened, or marked
  unsupported to make the object model look complete.
- The new abstraction preserves the old failure mode: no structured labels,
  symbols, or relocations are available to a target object emitter.
- The implementation attempts to finish RV64, AArch64, CLI, or c-testsuite
  object routing inside this shared-model child.

## Completion Note

Closed on 2026-06-24 after adding the shared object model records,
compiler-facing construction helpers, and minimal relocatable ELF writer.

Implemented scope:

- `src/backend/mir/object/model.hpp` and `.cpp` provide target-neutral object
  sections, labels, symbols, relocations, and construction helpers.
- `src/backend/mir/object/elf_writer.hpp` and `.cpp` serialize ELF64
  little-endian relocatable objects with explicit target machine and flags,
  string tables, symbol tables, section headers, and `.rela.*` sections.
- `tests/backend/mir/backend_object_model_records_test.cpp` covers model
  construction, helper behavior, and ELF writer structure.

Remaining work is intentionally delegated to later child ideas: RV64 object
emission, AArch64 object emission, `--codegen obj` CLI/test integration,
object-route scan/default readiness, and textual assembler follow-up.

Close-time proof:

```bash
cmake --preset default -DENABLE_C4C_BACKEND=ON &&
cmake --build --preset default &&
ctest --test-dir build -R '^(backend_object_model_records|backend_aarch64_target_record_core_contract|backend_riscv_prepared_edge_publication)$' --output-on-failure
```

Regression guard compared matching `test_before.log` and `test_after.log`
from the Step 4 pre-writer checkpoint to `HEAD`. Strict mode reported 3/3
passing before and after with no new failures; close accepted the same logs
with `--allow-non-decreasing-passed` because the selected test count stayed
constant while existing tests were extended.
