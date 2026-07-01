Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Emit Prepared Object Data

# Current Packet

## Just Finished

Step 4 of `plan.md` emitted prepared object data for records admitted by the
guarded selected object-data boundary, without changing expectations, scan
accounting, unsupported markers, or producer publication.

- Changed files:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.
- Emitted section/symbol shapes: coherent prepared records emit object symbols
  from prepared label text, extent, alignment, section kind, byte payload,
  zero-fill count, and `public_symbol` visibility. `.data` and `.rodata`
  append prepared initialized bytes; `.bss` reserves prepared zero-fill extents.
- Focused coverage now includes a prepared-record-authority case where RV64
  emits a local object symbol named from prepared label text into prepared
  `.rodata` with prepared alignment and bytes, while not defining storage from
  raw global spelling.
- Relocation shape remains unchanged: prepared object-data records with missing
  or ambiguous relocation authority still fail closed because the current
  prepared record only exposes relocation presence/conflict flags, not
  relocation-ready records. The pre-existing selected symbol-pointer fallback
  still emits a data `R_RISCV_64` relocation only for its narrow admitted shape.
- Representative `tests/c/external/gcc_torture/src/20000412-1.c` remains
  fail-closed with
  `unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656 emitted_byte_count=0 zero_fill_byte_count=0`;
  this is expected for Step 4 because the prepared facts still lack selected
  byte, zero-fill, or relocation authority.

## Suggested Next

Execute Step 5: add ordinary-C backend coverage that exercises selected
object-data emission through prepared facts without relying on
`src/20000412-1.c`, object label `2`, object size `1656`, or one initializer
layout.

## Watchouts

- Do not generalize from `src/20000412-1.c`, object label `2`, object size
  `1656`, raw global spelling, C source shape, or testcase identity.
- Do not treat `PreparedGlobalObjectData::section_kind`'s default `Data` value
  as section authority for an `unsupported_but_coherent` marker-only record.
- Current relocation support for selected symbol-pointer globals remains the
  pre-existing narrow fallback from raw BIR initializer shape; do not broaden
  relocation emission unless prepared relocation records exist.
- Thread-local storage, GOT-required globals, missing static-local object
  labels, parameter homes, unrelated global access widths, and F128 stay out
  of this RV64 consumer slice.
- Keep `unsupported_but_coherent` marker-only records fail-closed unless the
  prepared producer publishes explicit bytes, zero-fill, section, or relocation
  authority for the selected object-data record.

## Proof

- `scripts/plan_review_state.py set-step --step-id 4 --step-title 'Emit Prepared Object Data'`
  passed.
- `cmake --build build --target c4cll` passed.
- `cmake --build build --target backend_riscv_object_emission_test` passed.
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
  passed.
- Focused representative probe:
  `build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c -o build/agent_state/510_step4_prepared_object_data/20000412-1/20000412-1.o`
  failed closed with the expected marker-only selected object-data diagnostic.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
  passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed and
  wrote canonical proof log `test_after.log`.
